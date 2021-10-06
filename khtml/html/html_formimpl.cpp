/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#undef FORMS_DEBUG
//#define FORMS_DEBUG

#include "html/html_formimpl.h"

#include "khtmlview.h"
#include "khtml_part.h"
#include "html/html_documentimpl.h"
#include "khtml_settings.h"
#include "misc/htmlhashes.h"

#include "css/cssstyleselector.h"
#include "css/cssproperties.h"
#include "css/csshelper.h"
#include "xml/dom_textimpl.h"
#include "xml/dom2_eventsimpl.h"
#include "khtml_ext.h"

#include "rendering/render_form.h"

#include <kcharsets.h>
#include <kglobal.h>
#include <kdebug.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <netaccess.h>
#include <kfileitem.h>
#include <qfile.h>
#include <qtextcodec.h>

// for keygen
#include <qstring.h>
#include <ksslkeygen.h>

#include <assert.h>

using namespace DOM;
using namespace khtml;

HTMLFormElementImpl::HTMLFormElementImpl(DocumentPtr *doc)
    : HTMLElementImpl(doc)
{
    m_post = false;
    m_multipart = false;
    m_autocomplete = true;
    m_insubmit = false;
    m_doingsubmit = false;
    m_inreset = false;
    m_enctype = "application/x-www-form-urlencoded";
    m_boundary = "----------0xKhTmLbOuNdArY";
    m_acceptcharset = "UNKNOWN";
    m_malformed = false;
}

HTMLFormElementImpl::~HTMLFormElementImpl()
{
    QPtrListIterator<HTMLGenericFormElementImpl> it(formElements);
    for (; it.current(); ++it)
        it.current()->m_form = 0;
}

NodeImpl::Id HTMLFormElementImpl::id() const
{
    return ID_FORM;
}

#if APPLE_CHANGES

bool HTMLFormElementImpl::formWouldHaveSecureSubmission(const DOMString &url)
{
    if (url.isNull()) {
        return false;
    }
    return getDocument()->completeURL(url.string()).startsWith("https:", false);
}

#endif

void HTMLFormElementImpl::attach()
{
    HTMLElementImpl::attach();

    if (getDocument()->isHTMLDocument()) {
	HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
	document->addNamedImageOrForm(oldNameAttr);
	document->addNamedImageOrForm(oldIdAttr);
    }

#if APPLE_CHANGES
    // note we don't deal with calling secureFormRemoved() on detach, because the timing
    // was such that it cleared our state too early
    if (formWouldHaveSecureSubmission(m_url))
        getDocument()->secureFormAdded();
#endif
}

void HTMLFormElementImpl::detach()
{
    if (getDocument()->isHTMLDocument()) {
	HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
	document->removeNamedImageOrForm(oldNameAttr);
	document->removeNamedImageOrForm(oldIdAttr);
    }

    HTMLElementImpl::detach();
}

long HTMLFormElementImpl::length() const
{
    int len = 0;
    QPtrListIterator<HTMLGenericFormElementImpl> it(formElements);
    for (; it.current(); ++it)
	if (it.current()->isEnumeratable())
	    ++len;

    return len;
}

#if APPLE_CHANGES

void HTMLFormElementImpl::submitClick()
{
    bool submitFound = false;
    QPtrListIterator<HTMLGenericFormElementImpl> it(formElements);
    for (; it.current(); ++it) {
        if (it.current()->id() == ID_INPUT) {
            HTMLInputElementImpl *element = static_cast<HTMLInputElementImpl *>(it.current());
            if (element->isSuccessfulSubmitButton() && element->renderer()) {
                submitFound = true;
                element->click();
                break;
            }
        }
    }
    if (!submitFound) // submit the form without a submit or image input
        prepareSubmit();
}

#endif // APPLE_CHANGES

static QCString encodeCString(const QCString& e)
{
    // http://www.w3.org/TR/html4/interact/forms.html#h-17.13.4.1
    // safe characters like NS handles them for compatibility
    static const char *safe = "-._*";
    int elen = e.length();
    QCString encoded(( elen+e.contains( '\n' ) )*3+1);
    int enclen = 0;

    //QCString orig(e.data(), e.size());

    for(int pos = 0; pos < elen; pos++) {
        unsigned char c = e[pos];

        if ( (( c >= 'A') && ( c <= 'Z')) ||
             (( c >= 'a') && ( c <= 'z')) ||
             (( c >= '0') && ( c <= '9')) ||
             (strchr(safe, c))
            )
            encoded[enclen++] = c;
        else if ( c == ' ' )
            encoded[enclen++] = '+';
        else if ( c == '\n' || ( c == '\r' && e[pos+1] != '\n' ) )
        {
            encoded[enclen++] = '%';
            encoded[enclen++] = '0';
            encoded[enclen++] = 'D';
            encoded[enclen++] = '%';
            encoded[enclen++] = '0';
            encoded[enclen++] = 'A';
        }
        else if ( c != '\r' )
        {
            encoded[enclen++] = '%';
            unsigned int h = c / 16;
            h += (h > 9) ? ('A' - 10) : '0';
            encoded[enclen++] = h;

            unsigned int l = c % 16;
            l += (l > 9) ? ('A' - 10) : '0';
            encoded[enclen++] = l;
        }
    }
    encoded[enclen++] = '\0';
    encoded.truncate(enclen);

    return encoded;
}

// Change plain CR and plain LF to CRLF pairs.
static QCString fixLineBreaks(const QCString &s)
{
    // Compute the length.
    unsigned newLen = 0;
    const char *p = s.data();
    while (char c = *p++) {
        if (c == '\r') {
            // Safe to look ahead because of trailing '\0'.
            if (*p != '\n') {
                // Turn CR into CRLF.
                newLen += 2;
            }
        } else if (c == '\n') {
            // Turn LF into CRLF.
            newLen += 2;
        } else {
            // Leave other characters alone.
            newLen += 1;
        }
    }
    if (newLen == s.length()) {
        return s;
    }
    
    // Make a copy of the string.
    p = s.data();
    QCString result(newLen + 1);
    char *q = result.data();
    while (char c = *p++) {
        if (c == '\r') {
            // Safe to look ahead because of trailing '\0'.
            if (*p != '\n') {
                // Turn CR into CRLF.
                *q++ = '\r';
                *q++ = '\n';
            }
        } else if (c == '\n') {
            // Turn LF into CRLF.
            *q++ = '\r';
            *q++ = '\n';
        } else {
            // Leave other characters alone.
            *q++ = c;
        }
    }
    return result;
}

inline static QCString fixUpfromUnicode(const QTextCodec* codec, const QString& s)
{
    QCString str = fixLineBreaks(codec->fromUnicode(s));
    str.truncate(str.length());
    return str;
}

#if !APPLE_CHANGES

void HTMLFormElementImpl::i18nData()
{
    QString foo1 = i18n( "You're about to send data to the Internet "
                         "via an unencrypted connection. It might be possible "
                         "for others to see this information.\n"
                         "Do you want to continue?");
    QString foo2 = i18n("KDE Web browser");
    QString foo3 = i18n("When you send a password unencrypted to the Internet, "
                        "it might be possible for others to capture it as plain text.\n"
                        "Do you want to continue?");
    QString foo5 = i18n("Your data submission is redirected to "
                        "an insecure site. The data is sent unencrypted.\n"
                        "Do you want to continue?");
    QString foo6 = i18n("The page contents expired. You can repost the form"
                        "data by using <a href=\"javascript:go(0);\">Reload</a>");
}

#endif

QByteArray HTMLFormElementImpl::formData(bool& ok)
{
#ifdef FORMS_DEBUG
    kdDebug( 6030 ) << "form: formData()" << endl;
#endif

    QByteArray form_data(0);
    QCString enc_string = ""; // used for non-multipart data

    // find out the QTextcodec to use
    QString str = m_acceptcharset.string();
    str.replace(',', ' ');
    QStringList charsets = QStringList::split(' ', str);
    QTextCodec* codec = 0;
    KHTMLPart *part = getDocument()->part();
    for ( QStringList::Iterator it = charsets.begin(); it != charsets.end(); ++it )
    {
        QString enc = (*it);
        if(enc.contains("UNKNOWN"))
        {
            // use standard document encoding
            enc = "ISO-8859-1";
            if (part)
                enc = part->encoding();
        }
        if((codec = KGlobal::charsets()->codecForName(enc.latin1())))
            break;
    }

    if(!codec)
        codec = QTextCodec::codecForLocale();

    QStringList fileUploads;

    for (QPtrListIterator<HTMLGenericFormElementImpl> it(formElements); it.current(); ++it) {
        HTMLGenericFormElementImpl* current = it.current();
        khtml::encodingList lst;

        if (!current->disabled() && current->encoding(codec, lst, m_multipart))
        {
            //kdDebug(6030) << "adding name " << current->name().string() << endl;
            khtml::encodingList::Iterator it;
            for( it = lst.begin(); it != lst.end(); ++it )
            {
                if (!m_multipart)
                {
                    // handle ISINDEX / <input name=isindex> special
                    // but only if its the first entry
                    if ( enc_string.isEmpty() && *it == "isindex" ) {
                        ++it;
                        enc_string += encodeCString( *it );
                    }
                    else {
                        if(!enc_string.isEmpty())
                            enc_string += '&';

                        enc_string += encodeCString(*it);
                        enc_string += "=";
                        ++it;
                        enc_string += encodeCString(*it);
                    }
                }
                else
                {
                    QCString hstr("--");
                    hstr += m_boundary.string().latin1();
                    hstr += "\r\n";
                    hstr += "Content-Disposition: form-data; name=\"";
                    hstr += (*it).data();
                    hstr += "\"";

                    // if the current type is FILE, then we also need to
                    // include the filename
                    if (current->nodeType() == Node::ELEMENT_NODE && current->id() == ID_INPUT &&
                        static_cast<HTMLInputElementImpl*>(current)->inputType() == HTMLInputElementImpl::FILE)
                    {
                        QString path = static_cast<HTMLInputElementImpl*>(current)->value().string();
                        if (path.length()) fileUploads << path;

                        // Turn non-ASCII characters into &-escaped form.
                        // This doesn't work perfectly, because it doesn't escape &, for example.
                        // But it seems to be what Gecko does.
                        QString onlyfilename;
                        for (uint i = path.findRev('/') + 1; i < path.length(); ++i) {
                            QChar c = path.at(i).unicode();
                            if (c.unicode() >= 0x20 && c.unicode() <= 0x7F) {
                                onlyfilename.append(&c, 1);
                            } else {
                                QString ampersandEscape;
                                ampersandEscape.sprintf("&#%hu;", c.unicode());
                                onlyfilename.append(ampersandEscape);
                            }
                        }

                        // FIXME: This won't work if the filename includes a " mark,
                        // or control characters like CR or LF.
                        hstr += ("; filename=\"" + onlyfilename + "\"").ascii();
                        if(!static_cast<HTMLInputElementImpl*>(current)->value().isEmpty())
                        {
#if APPLE_CHANGES
                            QString mimeType = part ? KWQ(part)->mimeTypeForFileName(onlyfilename) : QString();
#else
                            KMimeType::Ptr ptr = KMimeType::findByURL(KURL(path));
                            QString mimeType = ptr->name();
#endif
                            if (!mimeType.isEmpty()) {
                                hstr += "\r\nContent-Type: ";
                                hstr += mimeType.ascii();
                            }
                        }
                    }

                    hstr += "\r\n\r\n";
                    ++it;

                    // append body
                    unsigned int old_size = form_data.size();
                    form_data.resize( old_size + hstr.length() + (*it).size() + 1);
                    memcpy(form_data.data() + old_size, hstr.data(), hstr.length());
                    memcpy(form_data.data() + old_size + hstr.length(), (*it), (*it).size());
                    form_data[form_data.size()-2] = '\r';
                    form_data[form_data.size()-1] = '\n';
                }
            }
        }
    }

#if !APPLE_CHANGES
    if (fileUploads.count()) {
        int result = KMessageBox::warningContinueCancelList( 0,
                                                             i18n("You're about to transfer the following files from "
                                                                  "your local computer to the Internet.\n"
                                                                  "Do you really want to continue?"),
                                                             fileUploads);


        if (result == KMessageBox::Cancel) {
            ok = false;
            return QByteArray();
        }
    }
#endif

    if (m_multipart)
        enc_string = ("--" + m_boundary.string() + "--\r\n").ascii();

    int old_size = form_data.size();
    form_data.resize( form_data.size() + enc_string.length() );
    memcpy(form_data.data() + old_size, enc_string.data(), enc_string.length() );

    ok = true;
    return form_data;
}

void HTMLFormElementImpl::setEnctype( const DOMString& type )
{
    if(type.string().find("multipart", 0, false) != -1 || type.string().find("form-data", 0, false) != -1)
    {
        m_enctype = "multipart/form-data";
        m_multipart = true;
        m_post = true;
    } else if (type.string().find("text", 0, false) != -1 || type.string().find("plain", 0, false) != -1)
    {
        m_enctype = "text/plain";
        m_multipart = false;
    }
    else
    {
        m_enctype = "application/x-www-form-urlencoded";
        m_multipart = false;
    }
}

void HTMLFormElementImpl::setBoundary( const DOMString& bound )
{
    m_boundary = bound;
}

bool HTMLFormElementImpl::prepareSubmit()
{
    KHTMLPart *part = getDocument()->part();
    if(m_insubmit || !part || part->onlyLocalReferences())
        return m_insubmit;

    m_insubmit = true;
    m_doingsubmit = false;

    if ( dispatchHTMLEvent(EventImpl::SUBMIT_EVENT,false,true) && !m_doingsubmit )
        m_doingsubmit = true;

    m_insubmit = false;

    if ( m_doingsubmit )
        submit(true);

    return m_doingsubmit;
}

void HTMLFormElementImpl::submit( bool activateSubmitButton )
{
    KHTMLView *view = getDocument()->view();
    KHTMLPart *part = getDocument()->part();
    if (!view || !part) {
        return;
    }

    if ( m_insubmit ) {
        m_doingsubmit = true;
        return;
    }

    m_insubmit = true;

#ifdef FORMS_DEBUG
    kdDebug( 6030 ) << "submitting!" << endl;
#endif

    HTMLGenericFormElementImpl* firstSuccessfulSubmitButton = 0;
    bool needButtonActivation = activateSubmitButton;	// do we need to activate a submit button?
    
#if APPLE_CHANGES
    KWQ(part)->clearRecordedFormValues();
#endif
    for (QPtrListIterator<HTMLGenericFormElementImpl> it(formElements); it.current(); ++it) {
        HTMLGenericFormElementImpl* current = it.current();
#if APPLE_CHANGES
        // Our app needs to get form values for password fields for doing password autocomplete,
        // so we are more lenient in pushing values, and let the app decide what to save when.
        if (current->id() == ID_INPUT) {
            HTMLInputElementImpl *input = static_cast<HTMLInputElementImpl*>(current);
            if (input->inputType() == HTMLInputElementImpl::TEXT
                || input->inputType() ==  HTMLInputElementImpl::PASSWORD)
            {
                KWQ(part)->recordFormValue(input->name().string(), input->value().string(), this);
            }
        }
#else
        if (current->id() == ID_INPUT &&
            static_cast<HTMLInputElementImpl*>(current)->inputType() == HTMLInputElementImpl::TEXT &&
            static_cast<HTMLInputElementImpl*>(current)->autoComplete() )
        {
            HTMLInputElementImpl *input = static_cast<HTMLInputElementImpl *>(current);
            view->addFormCompletionItem(input->name().string(), input->value().string());
        }
#endif

        if (needButtonActivation) {
            if (current->isActivatedSubmit()) {
                needButtonActivation = false;
            } else if (firstSuccessfulSubmitButton == 0 && current->isSuccessfulSubmitButton()) {
                firstSuccessfulSubmitButton = current;
            }
        }
    }

    if (needButtonActivation && firstSuccessfulSubmitButton) {
        firstSuccessfulSubmitButton->setActivatedSubmit(true);
    }

    bool ok;
    QByteArray form_data = formData(ok);
    if (ok) {
        if(m_post) {
            part->submitForm( "post", m_url.string(), form_data,
                                      m_target.string(),
                                      enctype().string(),
                                      boundary().string() );
        }
        else {
            part->submitForm( "get", m_url.string(), form_data,
                                      m_target.string() );
        }
    }

    if (needButtonActivation && firstSuccessfulSubmitButton) {
        firstSuccessfulSubmitButton->setActivatedSubmit(false);
    }
    
    m_doingsubmit = m_insubmit = false;
}

void HTMLFormElementImpl::reset(  )
{
    KHTMLPart *part = getDocument()->part();
    if(m_inreset || !part) return;

    m_inreset = true;

#ifdef FORMS_DEBUG
    kdDebug( 6030 ) << "reset pressed!" << endl;
#endif

    // ### DOM2 labels this event as not cancelable, however
    // common browsers( sick! ) allow it be cancelled.
    if ( !dispatchHTMLEvent(EventImpl::RESET_EVENT,true, true) ) {
        m_inreset = false;
        return;
    }

    for (QPtrListIterator<HTMLGenericFormElementImpl> it(formElements); it.current(); ++it)
        it.current()->reset();

    m_inreset = false;
}

void HTMLFormElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_ACTION:
#if APPLE_CHANGES
        {
        bool oldURLWasSecure = formWouldHaveSecureSubmission(m_url);
#endif
        m_url = khtml::parseURL(attr->value());
#if APPLE_CHANGES
        bool newURLIsSecure = formWouldHaveSecureSubmission(m_url);

        if (m_attached && (oldURLWasSecure != newURLIsSecure))
            if (newURLIsSecure)
                getDocument()->secureFormAdded();
            else
                getDocument()->secureFormRemoved();
        }
#endif
        break;
    case ATTR_TARGET:
        m_target = attr->value();
        break;
    case ATTR_METHOD:
        if ( strcasecmp( attr->value(), "post" ) == 0 )
            m_post = true;
        else if ( strcasecmp( attr->value(), "get" ) == 0 )
            m_post = false;
        break;
    case ATTR_ENCTYPE:
        setEnctype( attr->value() );
        break;
    case ATTR_ACCEPT_CHARSET:
        // space separated list of charsets the server
        // accepts - see rfc2045
        m_acceptcharset = attr->value();
        break;
    case ATTR_ACCEPT:
        // ignore this one for the moment...
        break;
    case ATTR_AUTOCOMPLETE:
        m_autocomplete = strcasecmp( attr->value(), "off" );
        break;
    case ATTR_ONSUBMIT:
        setHTMLEventListener(EventImpl::SUBMIT_EVENT,
	    getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONRESET:
        setHTMLEventListener(EventImpl::RESET_EVENT,
	    getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_NAME:
	{
	    QString newNameAttr = attr->value().string();
	    if (attached() && getDocument()->isHTMLDocument()) {
		HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
		document->removeNamedImageOrForm(oldNameAttr);
		document->addNamedImageOrForm(newNameAttr);
	    }
	    oldNameAttr = newNameAttr;
	}
	break;
    case ATTR_ID:
	{
	    QString newIdAttr = attr->value().string();
	    if (attached() && getDocument()->isHTMLDocument()) {
		HTMLDocumentImpl *document = static_cast<HTMLDocumentImpl *>(getDocument());
		document->removeNamedImageOrForm(oldIdAttr);
		document->addNamedImageOrForm(newIdAttr);
	    }
	    oldIdAttr = newIdAttr;
	}
	// fall through
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLFormElementImpl::radioClicked( HTMLGenericFormElementImpl *caller )
{
    for (QPtrListIterator<HTMLGenericFormElementImpl> it(formElements); it.current(); ++it) {
        HTMLGenericFormElementImpl *current = it.current();
        if (current->id() == ID_INPUT &&
            static_cast<HTMLInputElementImpl*>(current)->inputType() == HTMLInputElementImpl::RADIO &&
            current != caller && current->form() == caller->form() && current->name() == caller->name()) {
            static_cast<HTMLInputElementImpl*>(current)->setChecked(false);
        }
    }
}

void HTMLFormElementImpl::registerFormElement(HTMLGenericFormElementImpl *e)
{
    formElements.append(e);
}

void HTMLFormElementImpl::removeFormElement(HTMLGenericFormElementImpl *e)
{
    formElements.remove(e);
}

// -------------------------------------------------------------------------

HTMLGenericFormElementImpl::HTMLGenericFormElementImpl(DocumentPtr *doc, HTMLFormElementImpl *f)
    : HTMLElementImpl(doc)
{
    m_disabled = m_readOnly = false;
    m_name = 0;

    if (f)
	m_form = f;
    else
	m_form = getForm();
    if (m_form)
        m_form->registerFormElement(this);
}

HTMLGenericFormElementImpl::~HTMLGenericFormElementImpl()
{
    if (m_form)
        m_form->removeFormElement(this);
}

void HTMLGenericFormElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_NAME:
        break;
    case ATTR_DISABLED:
        setDisabled( attr->val() != 0 );
        break;
    case ATTR_READONLY:
    {
        bool m_oldreadOnly = m_readOnly;
        m_readOnly = attr->val() != 0;
        if (m_oldreadOnly != m_readOnly) setChanged();
        break;
    }
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

void HTMLGenericFormElementImpl::attach()
{
    assert(!attached());

    // FIXME: This handles the case of a new form element being created by
    // JavaScript and inserted inside a form. What it does not handle is
    // a form element being moved from inside a form to outside, or from one
    // inside one form to another. The reason this other case is hard to fix
    // is that during parsing, we may have been passed a form that we are not
    // inside, DOM-tree-wise. If so, it's hard for us to know when we should
    // be removed from that form's element list.
    if (!m_form) {
	m_form = getForm();
	if (m_form)
	    m_form->registerFormElement(this);
    }

    HTMLElementImpl::attach();

    // The call to updateFromElement() needs to go after the call through
    // to the base class's attach() because that can sometimes do a close
    // on the renderer.
    if (m_render) {
        m_render->updateFromElement();
    
        // Delayed attachment in order to prevent FOUC can result in an object being
        // programmatically focused before it has a render object.  If we have been focused
        // (i.e., if we are the focusNode) then go ahead and focus our corresponding native widget.
        // (Attach/detach can also happen as a result of display type changes, e.g., making a widget
        // block instead of inline, and focus should be restored in that case as well.)
        if (getDocument()->focusNode() == this && m_render->isWidget() && 
            static_cast<RenderWidget*>(renderer())->widget())
            static_cast<RenderWidget*>(renderer())->widget()->setFocus();
    }
}

HTMLFormElementImpl *HTMLGenericFormElementImpl::getForm() const
{
    NodeImpl *p = parentNode();
    while(p)
    {
        if( p->id() == ID_FORM )
            return static_cast<HTMLFormElementImpl *>(p);
        p = p->parentNode();
    }
#ifdef FORMS_DEBUG
    kdDebug( 6030 ) << "couldn't find form!" << endl;
#endif
    return 0;
}

DOMString HTMLGenericFormElementImpl::name() const
{
    if (m_name) return m_name;

// ###
//     DOMString n = getDocument()->htmlMode() != DocumentImpl::XHtml ?
//                   getAttribute(ATTR_NAME) : getAttribute(ATTR_ID);
    DOMString n = getAttribute(ATTR_NAME);
    if (n.isNull())
        return new DOMStringImpl("");

    return n;
}

void HTMLGenericFormElementImpl::setName(const DOMString& name)
{
    if (m_name) m_name->deref();
    m_name = name.implementation();
    if (m_name) m_name->ref();
}

void HTMLGenericFormElementImpl::onSelect()
{
    // ### make this work with new form events architecture
    dispatchHTMLEvent(EventImpl::SELECT_EVENT,true,false);
}

void HTMLGenericFormElementImpl::onChange()
{
    // ### make this work with new form events architecture
    dispatchHTMLEvent(EventImpl::CHANGE_EVENT,true,false);
}

bool HTMLGenericFormElementImpl::disabled() const
{
    return m_disabled;
}

void HTMLGenericFormElementImpl::setDisabled( bool _disabled )
{
    if ( m_disabled != _disabled ) {
        m_disabled = _disabled;
        setChanged();
    }
}

void HTMLGenericFormElementImpl::recalcStyle( StyleChange ch )
{
    //bool changed = changed();
    HTMLElementImpl::recalcStyle( ch );

    if (m_render /*&& changed*/)
        m_render->updateFromElement();
}

bool HTMLGenericFormElementImpl::isFocusable() const
{
    if (!m_render || (m_render->style() && m_render->style()->visibility() != VISIBLE))
        return false;
    return true;
}

bool HTMLGenericFormElementImpl::isKeyboardFocusable() const
{
    if (isFocusable()) {
        if (m_render->isWidget()) {
            return static_cast<RenderWidget*>(m_render)->widget() &&
            ((static_cast<RenderWidget*>(m_render)->widget()->focusPolicy() == QWidget::TabFocus) ||
             (static_cast<RenderWidget*>(m_render)->widget()->focusPolicy() == QWidget::StrongFocus));
        }
	if (getDocument()->part())
	    return getDocument()->part()->tabsToAllControls();
    }
    return false;
}

bool HTMLGenericFormElementImpl::isMouseFocusable() const
{
    if (isFocusable()) {
        if (m_render->isWidget()) {
            return static_cast<RenderWidget*>(m_render)->widget() &&
            ((static_cast<RenderWidget*>(m_render)->widget()->focusPolicy() == QWidget::ClickFocus) ||
             (static_cast<RenderWidget*>(m_render)->widget()->focusPolicy() == QWidget::StrongFocus));
        }
#if APPLE_CHANGES
        // For <input type=image> and <button>, we will assume no mouse focusability.  This is
        // consistent with OS X behavior for buttons.
        return false;
#else
        return true;
#endif
    }
    return false;
}

void HTMLGenericFormElementImpl::defaultEventHandler(EventImpl *evt)
{
    if (evt->target()==this)
    {
        // Report focus in/out changes to the browser extension (editable widgets only)
        KHTMLPart *part = getDocument()->part();
        if (evt->id()==EventImpl::DOMFOCUSIN_EVENT && isEditable() && part && m_render && m_render->isWidget()) {
            KHTMLPartBrowserExtension *ext = static_cast<KHTMLPartBrowserExtension *>(part->browserExtension());
            QWidget *widget = static_cast<RenderWidget*>(m_render)->widget();
            if (ext)
                ext->editableWidgetFocused(widget);
        }
        if (evt->id()==EventImpl::MOUSEDOWN_EVENT || evt->id()==EventImpl::KEYDOWN_EVENT)
        {
            setActive();
        }
        else if (evt->id() == EventImpl::MOUSEUP_EVENT || evt->id()==EventImpl::KEYUP_EVENT)
        {
	    if (m_active)
	    {
		setActive(false);
		setFocus();
	    }
	    else {
                setActive(false);
            }
        }

#if APPLE_CHANGES
	// We don't want this default key event handling, we'll count on
	// Cocoa event dispatch if the event doesn't get blocked.
#else
	if (evt->id()==EventImpl::KEYDOWN_EVENT ||
	    evt->id()==EventImpl::KEYUP_EVENT)
	{
	    KeyboardEventImpl * k = static_cast<KeyboardEventImpl *>(evt);
	    if (k->keyVal() == QChar('\n').unicode() && m_render && m_render->isWidget() && k->qKeyEvent)
		QApplication::sendEvent(static_cast<RenderWidget *>(m_render)->widget(), k->qKeyEvent);
	}
#endif

	if (evt->id()==EventImpl::DOMFOCUSOUT_EVENT && isEditable() && part && m_render && m_render->isWidget()) {
	    KHTMLPartBrowserExtension *ext = static_cast<KHTMLPartBrowserExtension *>(part->browserExtension());
	    QWidget *widget = static_cast<RenderWidget*>(m_render)->widget();
	    if (ext)
		ext->editableWidgetBlurred(widget);

	    // ### Don't count popup as a valid reason for losing the focus (example: opening the options of a select
	    // combobox shouldn't emit onblur)
	}
    }
    HTMLElementImpl::defaultEventHandler(evt);
}

bool HTMLGenericFormElementImpl::isEditable()
{
    return false;
}

// Special chars used to encode form state strings.
// We pick chars that are unlikely to be used in an HTML attr, so we rarely have to really encode.
const char stateSeparator = '&';
const char stateEscape = '<';
static const char stateSeparatorMarker[] = "<A";
static const char stateEscapeMarker[] = "<<";

// Encode an element name so we can put it in a state string without colliding
// with our separator char.
static QString encodedElementName(QString str)
{
    int sepLoc = str.find(stateSeparator);
    int escLoc = str.find(stateSeparator);
    if (sepLoc >= 0 || escLoc >= 0) {
        QString newStr = str;
        //   replace "<" with "<<"
        while (escLoc >= 0) {
            newStr.replace(escLoc, 1, stateEscapeMarker);
            escLoc = str.find(stateSeparator, escLoc+1);
        }
        //   replace "&" with "<A"
        while (sepLoc >= 0) {
            newStr.replace(sepLoc, 1, stateSeparatorMarker);
            sepLoc = str.find(stateSeparator, sepLoc+1);
        }
        return newStr;
    } else {
        return str;
    }
}

QString HTMLGenericFormElementImpl::state( )
{
    // Build a string that contains ElementName&ElementType&
    return encodedElementName(name().string()) + stateSeparator + type().string() + stateSeparator;
}

QString HTMLGenericFormElementImpl::findMatchingState(QStringList &states)
{
    QString encName = encodedElementName(name().string());
    QString typeStr = type().string();
    for (QStringList::Iterator it = states.begin(); it != states.end(); ++it) {
        QString state = *it;
        int sep1 = state.find(stateSeparator);
        int sep2 = state.find(stateSeparator, sep1+1);
        assert(sep1 >= 0);
        assert(sep2 >= 0);

        QString nameAndType = state.left(sep2);
        if (encName.length() + typeStr.length() + 1 == (uint)sep2
            && nameAndType.startsWith(encName)
            && nameAndType.endsWith(typeStr))
        {
            states.remove(it);
            return state.mid(sep2+1);
        }
    }
    return QString::null;
}

// -------------------------------------------------------------------------

HTMLButtonElementImpl::HTMLButtonElementImpl(DocumentPtr *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
    m_type = SUBMIT;
    m_dirty = true;
    m_activeSubmit = false;
}

HTMLButtonElementImpl::~HTMLButtonElementImpl()
{
}

NodeImpl::Id HTMLButtonElementImpl::id() const
{
    return ID_BUTTON;
}

DOMString HTMLButtonElementImpl::type() const
{
    return getAttribute(ATTR_TYPE);
}

void HTMLButtonElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_TYPE:
        if ( strcasecmp( attr->value(), "submit" ) == 0 )
            m_type = SUBMIT;
        else if ( strcasecmp( attr->value(), "reset" ) == 0 )
            m_type = RESET;
        else if ( strcasecmp( attr->value(), "button" ) == 0 )
            m_type = BUTTON;
        break;
    case ATTR_VALUE:
        m_value = attr->value();
        m_currValue = m_value.string();
        break;
    case ATTR_ACCESSKEY:
        break;
    case ATTR_ONFOCUS:
        setHTMLEventListener(EventImpl::FOCUS_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONBLUR:
        setHTMLEventListener(EventImpl::BLUR_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    default:
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

void HTMLButtonElementImpl::defaultEventHandler(EventImpl *evt)
{
    if (m_type != BUTTON && (evt->id() == EventImpl::DOMACTIVATE_EVENT)) {

        if(m_form && m_type == SUBMIT) {
            m_activeSubmit = true;
            m_form->prepareSubmit();
            m_activeSubmit = false; // in case we were canceled
        }
        if(m_form && m_type == RESET) m_form->reset();
    }
    HTMLGenericFormElementImpl::defaultEventHandler(evt);
}

bool HTMLButtonElementImpl::isSuccessfulSubmitButton() const
{
    // HTML spec says that buttons must have names
    // to be considered successful. However, other browsers
    // do not impose this constraint. Therefore, we behave
    // differently and can use different buttons than the 
    // author intended. 
    // Remove the name constraint for now.
    // Was: m_type == SUBMIT && !m_disabled && !name().isEmpty()
    return m_type == SUBMIT && !m_disabled;
}

bool HTMLButtonElementImpl::isActivatedSubmit() const
{
    return m_activeSubmit;
}

void HTMLButtonElementImpl::setActivatedSubmit(bool flag)
{
    m_activeSubmit = flag;
}

bool HTMLButtonElementImpl::encoding(const QTextCodec* codec, khtml::encodingList& encoding, bool /*multipart*/)
{
    if (m_type != SUBMIT || name().isEmpty() || !m_activeSubmit)
        return false;

    encoding += fixUpfromUnicode(codec, name().string());
    QString enc_str = m_currValue.isNull() ? QString("") : m_currValue;
    encoding += fixUpfromUnicode(codec, enc_str);

    return true;
}

void HTMLButtonElementImpl::click()
{
#if APPLE_CHANGES
    QWidget *widget;
    if (renderer() && (widget = static_cast<RenderWidget *>(renderer())->widget())) {
        // using this method gives us nice Cocoa user interface feedback
        static_cast<QButton *>(widget)->click();
    }
    else
#endif
        HTMLGenericFormElementImpl::click();
}

void HTMLButtonElementImpl::accessKeyAction()
{   
    click();
}

// -------------------------------------------------------------------------

HTMLFieldSetElementImpl::HTMLFieldSetElementImpl(DocumentPtr *doc, HTMLFormElementImpl *f)
   : HTMLGenericFormElementImpl(doc, f)
{
}

HTMLFieldSetElementImpl::~HTMLFieldSetElementImpl()
{
}

bool HTMLFieldSetElementImpl::isFocusable() const
{
    return false;
}

NodeImpl::Id HTMLFieldSetElementImpl::id() const
{
    return ID_FIELDSET;
}

DOMString HTMLFieldSetElementImpl::type() const
{
    return "fieldset";
}

RenderObject* HTMLFieldSetElementImpl::createRenderer(RenderArena* arena, RenderStyle* style)
{
    return new (arena) RenderFieldset(this);
}

// -------------------------------------------------------------------------

HTMLInputElementImpl::HTMLInputElementImpl(DocumentPtr *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
    m_type = TEXT;
    m_maxLen = -1;
    m_size = 20;
    m_checked = false;
    m_defaultChecked = false;
    m_useDefaultChecked = true;
    
    m_haveType = false;
    m_activeSubmit = false;
    m_autocomplete = true;
    m_inited = false;

    xPos = 0;
    yPos = 0;

    if ( m_form )
        m_autocomplete = f->autoComplete();
}

HTMLInputElementImpl::~HTMLInputElementImpl()
{
    if (getDocument()) getDocument()->deregisterMaintainsState(this);
}

NodeImpl::Id HTMLInputElementImpl::id() const
{
    return ID_INPUT;
}

void HTMLInputElementImpl::setType(const DOMString& t)
{
    typeEnum newType;
    
    if ( strcasecmp( t, "password" ) == 0 )
        newType = PASSWORD;
    else if ( strcasecmp( t, "checkbox" ) == 0 )
        newType = CHECKBOX;
    else if ( strcasecmp( t, "radio" ) == 0 )
        newType = RADIO;
    else if ( strcasecmp( t, "submit" ) == 0 )
        newType = SUBMIT;
    else if ( strcasecmp( t, "reset" ) == 0 )
        newType = RESET;
    else if ( strcasecmp( t, "file" ) == 0 )
        newType = FILE;
    else if ( strcasecmp( t, "hidden" ) == 0 )
        newType = HIDDEN;
    else if ( strcasecmp( t, "image" ) == 0 )
        newType = IMAGE;
    else if ( strcasecmp( t, "button" ) == 0 )
        newType = BUTTON;
    else if ( strcasecmp( t, "khtml_isindex" ) == 0 )
        newType = ISINDEX;
    else
        newType = TEXT;

    // ### IMPORTANT: Don't allow the type to be changed to FILE after the first
    // type change, otherwise a JavaScript programmer would be able to set a text
    // field's value to something like /etc/passwd and then change it to a file field.
    if (m_type != newType) {
        if (newType == FILE && m_haveType) {
            // Set the attribute back to the old value.
            // Useful in case we were called from inside parseAttribute.
            setAttribute(ATTR_TYPE, type());
        } else {
            m_type = newType;
        }
    }
    m_haveType = true;
}

DOMString HTMLInputElementImpl::type() const
{
    // needs to be lowercase according to DOM spec
    switch (m_type) {
    case TEXT: return "text";
    case PASSWORD: return "password";
    case CHECKBOX: return "checkbox";
    case RADIO: return "radio";
    case SUBMIT: return "submit";
    case RESET: return "reset";
    case FILE: return "file";
    case HIDDEN: return "hidden";
    case IMAGE: return "image";
    case BUTTON: return "button";
    default: return "";
    }
}

QString HTMLInputElementImpl::state( )
{
    assert(m_type != PASSWORD);		// should never save/restore password fields

    QString state = HTMLGenericFormElementImpl::state();
    switch (m_type) {
    case CHECKBOX:
    case RADIO:
        return state + (checked() ? "on" : "off");
    default:
        return state + value().string()+'.'; // Make sure the string is not empty!
    }
}

void HTMLInputElementImpl::restoreState(QStringList &states)
{
    assert(m_type != PASSWORD);		// should never save/restore password fields
    
    QString state = HTMLGenericFormElementImpl::findMatchingState(states);
    if (state.isNull()) return;

    switch (m_type) {
    case CHECKBOX:
    case RADIO:
        setChecked((state == "on"));
        break;
    default:
        setValue(DOMString(state.left(state.length()-1)));
        break;
    }
}

void HTMLInputElementImpl::select(  )
{
    if(!m_render) return;

    if (m_type == TEXT || m_type == PASSWORD)
        static_cast<RenderLineEdit*>(m_render)->select();
    else if (m_type == FILE)
        static_cast<RenderFileButton*>(m_render)->select();
}

void HTMLInputElementImpl::click()
{
    switch (inputType()) {
        case HIDDEN:
            // a no-op for this type
            break;
        case CHECKBOX:
        case RADIO:
        case SUBMIT:
        case RESET:
        case BUTTON: 
#if APPLE_CHANGES
        {
            QWidget *widget;
            if (renderer() && (widget = static_cast<RenderWidget *>(renderer())->widget())) {
                // using this method gives us nice Cocoa user interface feedback
                static_cast<QButton *>(widget)->click();
                break;
            }
        }
#endif
            HTMLGenericFormElementImpl::click();
            break;
        case FILE:
#if APPLE_CHANGES
            if (renderer()) {
                static_cast<RenderFileButton *>(renderer())->click();
                break;
            }
#endif
            HTMLGenericFormElementImpl::click();
            break;
        case IMAGE:
        case ISINDEX:
        case PASSWORD:
        case TEXT:
            HTMLGenericFormElementImpl::click();
            break;
    }
}

void HTMLInputElementImpl::accessKeyAction()
{
    switch (inputType()) {
        case HIDDEN:
            // a no-op for this type
            break;
        case TEXT:
        case PASSWORD:
        case ISINDEX:
            focus();
            break;
        case CHECKBOX:
        case RADIO:
        case SUBMIT:
        case RESET:
        case IMAGE:
        case BUTTON:
        case FILE:
            // focus and click
            focus();
            click();
            break;
    }
}

void HTMLInputElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_AUTOCOMPLETE:
        m_autocomplete = strcasecmp( attr->value(), "off" );
        break;
    case ATTR_TYPE:
        setType(attr->value());
        break;
    case ATTR_VALUE:
        if (m_value.isNull()) // We only need to setChanged if the form is looking
            setChanged();     // at the default value right now.
        break;
    case ATTR_CHECKED:
        m_defaultChecked = attr->val() != 0;
        if (m_useDefaultChecked) {
            setChecked(m_defaultChecked);
            m_useDefaultChecked = true;
        }
        break;
    case ATTR_MAXLENGTH:
        m_maxLen = attr->val() ? attr->val()->toInt() : -1;
        setChanged();
        break;
    case ATTR_SIZE:
        m_size = attr->val() ? attr->val()->toInt() : 20;
        break;
    case ATTR_ALT:
    case ATTR_SRC:
        if (m_render && m_type == IMAGE) m_render->updateFromElement();
        break;
    case ATTR_USEMAP:
    case ATTR_ACCESSKEY:
        // ### ignore for the moment
        break;
    case ATTR_VSPACE:
        addCSSLength(CSS_PROP_MARGIN_TOP, attr->value());
        addCSSLength(CSS_PROP_MARGIN_BOTTOM, attr->value());
        break;
    case ATTR_HSPACE:
        addCSSLength(CSS_PROP_MARGIN_LEFT, attr->value());
        addCSSLength(CSS_PROP_MARGIN_RIGHT, attr->value());
        break;        
    case ATTR_ALIGN:
        addHTMLAlignment( attr->value() );
        break;
    case ATTR_WIDTH:
        // ignore this attribute,  do _not_ add
        // a CSS_PROP_WIDTH here (we will honor this attribute for input type=image
        // in the attach call)!
        // webdesigner are stupid - and IE/NS behave the same ( Dirk )
        break;
    case ATTR_HEIGHT:
        addCSSLength(CSS_PROP_HEIGHT, attr->value() );
        break;
    case ATTR_ONFOCUS:
        setHTMLEventListener(EventImpl::FOCUS_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONBLUR:
        setHTMLEventListener(EventImpl::BLUR_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONSELECT:
        setHTMLEventListener(EventImpl::SELECT_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONCHANGE:
        setHTMLEventListener(EventImpl::CHANGE_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    default:
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

bool HTMLInputElementImpl::rendererIsNeeded(RenderStyle *style)
{
    switch(m_type)
    {
    case TEXT:
    case PASSWORD:
    case ISINDEX:
    case CHECKBOX:
    case RADIO:
    case SUBMIT:
    case IMAGE:
    case RESET:
    case FILE:
    case BUTTON:   return HTMLGenericFormElementImpl::rendererIsNeeded(style);
    case HIDDEN:   return false;
    }
    assert(false);
    return false;
}

RenderObject *HTMLInputElementImpl::createRenderer(RenderArena *arena, RenderStyle *style)
{
    switch(m_type)
    {
    case TEXT:
    case PASSWORD:
    case ISINDEX:  return new (arena) RenderLineEdit(this);
    case CHECKBOX: return new (arena) RenderCheckBox(this);
    case RADIO:    return new (arena) RenderRadioButton(this);
    case SUBMIT:   return new (arena) RenderSubmitButton(this);
    case IMAGE:    return new (arena) RenderImageButton(this);
    case RESET:    return new (arena) RenderResetButton(this);
    case FILE:     return new (arena) RenderFileButton(this);
    case BUTTON:   return new (arena) RenderPushButton(this);
    case HIDDEN:   break;
    }
    assert(false);
    return 0;
}

void HTMLInputElementImpl::attach()
{
    if (!m_inited) {
        if (!m_haveType)
            setType(getAttribute(ATTR_TYPE));

        // FIXME: This needs to be dynamic, doesn't it, since someone could set this
        // after attachment?
        DOMString val = getAttribute(ATTR_VALUE);
        if ((uint) m_type <= ISINDEX && !val.isEmpty()) {
            // remove newline stuff..
            QString nvalue;
            for (unsigned int i = 0; i < val.length(); ++i)
                if (val[i] >= ' ')
                    nvalue += val[i];

            if (val.length() != nvalue.length())
                setAttribute(ATTR_VALUE, nvalue);
        }

        m_defaultChecked = (getAttribute(ATTR_CHECKED) != 0);
        
        m_inited = true;
    }

    switch( m_type ) {
    case HIDDEN:
    case IMAGE:
        if (!getAttribute(ATTR_WIDTH).isEmpty())
            addCSSLength(CSS_PROP_WIDTH, getAttribute(ATTR_WIDTH));
        break;
    default:
        break;
    }

    HTMLGenericFormElementImpl::attach();

#if APPLE_CHANGES
    // note we don't deal with calling passwordFieldRemoved() on detach, because the timing
    // was such that it cleared our state too early
    if (m_type == PASSWORD)
        getDocument()->passwordFieldAdded();
#endif
}

DOMString HTMLInputElementImpl::altText() const
{
    // http://www.w3.org/TR/1998/REC-html40-19980424/appendix/notes.html#altgen
    // also heavily discussed by Hixie on bugzilla
    // note this is intentionally different to HTMLImageElementImpl::altText()
    DOMString alt = getAttribute( ATTR_ALT );
    // fall back to title attribute
    if ( alt.isNull() )
        alt = getAttribute( ATTR_TITLE );
    if ( alt.isNull() )
        alt = getAttribute( ATTR_VALUE );
    if ( alt.isEmpty() )
#if APPLE_CHANGES
        alt = inputElementAltText();
#else
        alt = i18n( "Submit" );
#endif

    return alt;
}

bool HTMLInputElementImpl::isSuccessfulSubmitButton() const
{
    // HTML spec says that buttons must have names
    // to be considered successful. However, other browsers
    // do not impose this constraint. Therefore, we behave
    // differently and can use different buttons than the 
    // author intended. 
    // Was: (m_type == SUBMIT && !name().isEmpty())
    return !m_disabled && (m_type == IMAGE || m_type == SUBMIT);
}

bool HTMLInputElementImpl::isActivatedSubmit() const
{
    return m_activeSubmit;
}

void HTMLInputElementImpl::setActivatedSubmit(bool flag)
{
    m_activeSubmit = flag;
}

bool HTMLInputElementImpl::encoding(const QTextCodec* codec, khtml::encodingList& encoding, bool multipart)
{
    QString nme = name().string();

    // image generates its own name's
    if (nme.isEmpty() && m_type != IMAGE) return false;

    // IMAGE needs special handling later
    if(m_type != IMAGE) encoding += fixUpfromUnicode(codec, nme);

    switch (m_type) {
        case HIDDEN:
        case TEXT:
        case PASSWORD:
            // always successful
            encoding += fixUpfromUnicode(codec, value().string());
            return true;
        case CHECKBOX:

            if( checked() ) {
                encoding += fixUpfromUnicode(codec, value().string());
                return true;
            }
            break;

        case RADIO:

            if( checked() ) {
                encoding += fixUpfromUnicode(codec, value().string());
                return true;
            }
            break;

        case BUTTON:
        case RESET:
            // those buttons are never successful
            return false;

        case IMAGE:

            if(m_activeSubmit)
            {
                QString astr(nme.isEmpty() ? QString::fromLatin1("x") : nme + ".x");

                encoding += fixUpfromUnicode(codec, astr);
                astr.setNum(clickX());
                encoding += fixUpfromUnicode(codec, astr);
                astr = nme.isEmpty() ? QString::fromLatin1("y") : nme + ".y";
                encoding += fixUpfromUnicode(codec, astr);
                astr.setNum(clickY());
                encoding += fixUpfromUnicode(codec, astr);

                return true;
            }
            break;

        case SUBMIT:

            if (m_activeSubmit)
            {
                QString enc_str = value().string();
                if (enc_str.isEmpty())
                    enc_str = static_cast<RenderSubmitButton*>(m_render)->defaultLabel();
                if (!enc_str.isEmpty()) {
                    encoding += fixUpfromUnicode(codec, enc_str);
                    return true;
                }
            }
            break;

        case FILE:
        {
            // can't submit file on GET
            // don't submit if display: none or display: hidden
            if(!multipart || !renderer() || renderer()->style()->visibility() != khtml::VISIBLE)
                return false;

            QString local;
            QCString dummy("");

            // if no filename at all is entered, return successful, however empty
            // null would be more logical but netscape posts an empty file. argh.
            if(value().isEmpty()) {
                encoding += dummy;
                return true;
            }

            KURL fileurl("file:///");
            fileurl.setPath(value().string());
            KIO::UDSEntry filestat;

            if (!KIO::NetAccess::stat(fileurl, filestat)) {
#if APPLE_CHANGES
                // FIXME: Figure out how to report this error.
#else
                KMessageBox::sorry(0L, i18n("Error fetching file for submission:\n%1").arg(KIO::NetAccess::lastErrorString()));
#endif
                return false;
            }

            KFileItem fileitem(filestat, fileurl, true, false);
            if(fileitem.isDir()) {
                encoding += dummy;
                return false;
            }

            if ( KIO::NetAccess::download(fileurl, local) )
            {
                QFile file(local);
                if (file.open(IO_ReadOnly))
                {
                    QCString filearray(file.size()+1);
                    int readbytes = file.readBlock( filearray.data(), file.size());
                    if ( readbytes >= 0 )
                        filearray[readbytes] = '\0';
                    file.close();

                    encoding += filearray;
                    KIO::NetAccess::removeTempFile( local );

                    return true;
                }
                return false;
            }
            else {
#if APPLE_CHANGES
                // FIXME: Figure out how to report this error.
#else
                KMessageBox::sorry(0L, i18n("Error fetching file for submission:\n%1").arg(KIO::NetAccess::lastErrorString()));
#endif
                return false;
            }
            break;
        }
        case ISINDEX:
            encoding += fixUpfromUnicode(codec, value().string());
            return true;
    }
    return false;
}

void HTMLInputElementImpl::reset()
{
    setValue(DOMString());
    setChecked(m_defaultChecked);
    m_useDefaultChecked = true;
}

void HTMLInputElementImpl::setChecked(bool _checked)
{
    if (checked() == _checked) return;

    if (m_form && m_type == RADIO && _checked && !name().isEmpty())
        m_form->radioClicked(this);

    m_useDefaultChecked = false;
    m_checked = _checked;
    setChanged();
}


DOMString HTMLInputElementImpl::value() const
{
    if (m_type == CHECKBOX || m_type == RADIO) {
        DOMString val = getAttribute(ATTR_VALUE);

        // If no attribute exists, then just use "on" or "" based off the checked() state
        // of the control.
        if (val.isNull()) {
            if (checked())
                return DOMString("on");
            else
                return DOMString("");
        }
        return val;
    }

    // It's important *not* to fall back to the value attribute for file inputs,
    // because that would allow a malicious web page to upload files by setting the
    // value attribute in markup.
    if (m_value.isNull() && m_type != FILE)
        return getAttribute(ATTR_VALUE);
    return m_value;
}


void HTMLInputElementImpl::setValue(DOMString val)
{
    if (m_type == FILE) return;

    m_value = val;
    setChanged();
}

void HTMLInputElementImpl::blur()
{
    if(getDocument()->focusNode() == this)
	getDocument()->setFocusNode(0);
}

void HTMLInputElementImpl::focus()
{
    getDocument()->setFocusNode(this);
}

void HTMLInputElementImpl::defaultEventHandler(EventImpl *evt)
{
    if (evt->isMouseEvent() &&
        ( evt->id() == EventImpl::KHTML_CLICK_EVENT || evt->id() == EventImpl::KHTML_DBLCLICK_EVENT ) &&
        m_type == IMAGE
        && m_render) {
        // record the mouse position for when we get the DOMActivate event
        MouseEventImpl *me = static_cast<MouseEventImpl*>(evt);
        int offsetX, offsetY;
        m_render->absolutePosition(offsetX,offsetY);
        xPos = me->clientX()-offsetX;
        yPos = me->clientY()-offsetY;

	me->setDefaultHandled();
    }

    // DOMActivate events cause the input to be "activated" - in the case of image and submit inputs, this means
    // actually submitting the form. For reset inputs, the form is reset. These events are sent when the user clicks
    // on the element, or presses enter while it is the active element. Javacsript code wishing to activate the element
    // must dispatch a DOMActivate event - a click event will not do the job.
    if ((evt->id() == EventImpl::DOMACTIVATE_EVENT) &&
        (m_type == IMAGE || m_type == SUBMIT || m_type == RESET)){

        if (!m_form || !m_render)
            return;

        if (m_type == RESET) {
            m_form->reset();
        }
        else {
            m_activeSubmit = true;
            if (!m_form->prepareSubmit()) {
                xPos = 0;
                yPos = 0;
            }
            m_activeSubmit = false;
        }
    }

#if APPLE_CHANGES
    // Use key press event here since sending simulated mouse events
    // on key down blocks the proper sending of the key press event.
    if (evt->id() == EventImpl::KEYPRESS_EVENT) {
    
        if (!m_form || !m_render || !evt->isKeyboardEvent())
            return;
        
        DOMString key = static_cast<KeyboardEventImpl *>(evt)->keyIdentifier();
        
        switch (m_type) {
            case IMAGE:
            case RESET:
            case SUBMIT:
                // simulate mouse click for spacebar and enter
                if (key == "U+000020" || key == "Enter") {
                    m_form->submitClick();
                    evt->setDefaultHandled();
                }
                break;
            case CHECKBOX:
            case RADIO:
                // for enter, find the first successful image or submit element 
                // send it a simulated mouse click
                if (key == "Enter") {
                    m_form->submitClick();
                    evt->setDefaultHandled();
                }
                break;
            case TEXT:
            case PASSWORD: {
                // For enter, find the first successful image or submit element 
                // send it a simulated mouse click.
                if (key == "Enter") {
                    m_form->submitClick();
                    evt->setDefaultHandled();
                }
                break;
            }
            default:
                // not handled for the other widgets
                break;
        }
    }
#endif
    HTMLGenericFormElementImpl::defaultEventHandler(evt);
}

bool HTMLInputElementImpl::isEditable()
{
    return ((m_type == TEXT) || (m_type == PASSWORD) || (m_type == ISINDEX) || (m_type == FILE));
}

// -------------------------------------------------------------------------

HTMLLabelElementImpl::HTMLLabelElementImpl(DocumentPtr *doc)
    : HTMLElementImpl(doc)
{
}

HTMLLabelElementImpl::~HTMLLabelElementImpl()
{
}

bool HTMLLabelElementImpl::isFocusable() const
{
    return false;
}

NodeImpl::Id HTMLLabelElementImpl::id() const
{
    return ID_LABEL;
}

void HTMLLabelElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_ONFOCUS:
        setHTMLEventListener(EventImpl::FOCUS_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONBLUR:
        setHTMLEventListener(EventImpl::BLUR_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    default:
        HTMLElementImpl::parseAttribute(attr);
    }
}

ElementImpl *HTMLLabelElementImpl::formElement()
{
    DOMString formElementId = getAttribute(ATTR_FOR);
    if (formElementId.isNull() || formElementId.isEmpty())
        return 0;
    return getDocument()->getElementById(formElementId);
}

// -------------------------------------------------------------------------

HTMLLegendElementImpl::HTMLLegendElementImpl(DocumentPtr *doc, HTMLFormElementImpl *f)
: HTMLGenericFormElementImpl(doc, f)
{
}

HTMLLegendElementImpl::~HTMLLegendElementImpl()
{
}

bool HTMLLegendElementImpl::isFocusable() const
{
    return false;
}

NodeImpl::Id HTMLLegendElementImpl::id() const
{
    return ID_LEGEND;
}

RenderObject* HTMLLegendElementImpl::createRenderer(RenderArena* arena, RenderStyle* style)
{
    return new (arena) RenderLegend(this);
}

DOMString HTMLLegendElementImpl::type() const
{
    return "legend";
}

// -------------------------------------------------------------------------

HTMLSelectElementImpl::HTMLSelectElementImpl(DocumentPtr *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
    m_multiple = false;
    m_recalcListItems = false;
    // 0 means invalid (i.e. not set)
    m_size = 0;
    m_minwidth = 0;
}

HTMLSelectElementImpl::~HTMLSelectElementImpl()
{
    if (getDocument()) getDocument()->deregisterMaintainsState(this);
}

NodeImpl::Id HTMLSelectElementImpl::id() const
{
    return ID_SELECT;
}

void HTMLSelectElementImpl::recalcStyle( StyleChange ch )
{
    if (hasChangedChild() && m_render) {
        static_cast<khtml::RenderSelect*>(m_render)->setOptionsChanged(true);
    }

    HTMLGenericFormElementImpl::recalcStyle( ch );
}


DOMString HTMLSelectElementImpl::type() const
{
    return (m_multiple ? "select-multiple" : "select-one");
}

long HTMLSelectElementImpl::selectedIndex() const
{
    // return the number of the first option selected
    uint o = 0;
    QMemArray<HTMLGenericFormElementImpl*> items = listItems();
    for (unsigned int i = 0; i < items.size(); i++) {
        if (items[i]->id() == ID_OPTION) {
            if (static_cast<HTMLOptionElementImpl*>(items[i])->selected())
                return o;
            o++;
        }
    }
    Q_ASSERT(m_multiple);
    return -1;
}

void HTMLSelectElementImpl::setSelectedIndex( long  index )
{
    // deselect all other options and select only the new one
    QMemArray<HTMLGenericFormElementImpl*> items = listItems();
    int listIndex;
    for (listIndex = 0; listIndex < int(items.size()); listIndex++) {
        if (items[listIndex]->id() == ID_OPTION)
            static_cast<HTMLOptionElementImpl*>(items[listIndex])->setSelected(false);
    }
    listIndex = optionToListIndex(index);
    if (listIndex >= 0)
        static_cast<HTMLOptionElementImpl*>(items[listIndex])->setSelected(true);

    setChanged(true);
}

long HTMLSelectElementImpl::length() const
{
    int len = 0;
    uint i;
    QMemArray<HTMLGenericFormElementImpl*> items = listItems();
    for (i = 0; i < items.size(); i++) {
        if (items[i]->id() == ID_OPTION)
            len++;
    }
    return len;
}

void HTMLSelectElementImpl::add( const HTMLElement &element, const HTMLElement &before )
{
    if(element.isNull() || element.handle()->id() != ID_OPTION)
        return;

    int exceptioncode = 0;
    insertBefore(element.handle(), before.handle(), exceptioncode );
    if (!exceptioncode)
        setRecalcListItems();
}

void HTMLSelectElementImpl::remove( long index )
{
    int exceptioncode = 0;
    int listIndex = optionToListIndex(index);

    QMemArray<HTMLGenericFormElementImpl*> items = listItems();
    if(listIndex < 0 || index >= int(items.size()))
        return; // ### what should we do ? remove the last item?

    removeChild(items[listIndex], exceptioncode);
    if( !exceptioncode )
        setRecalcListItems();
}

void HTMLSelectElementImpl::blur()
{
    if(getDocument()->focusNode() == this)
	getDocument()->setFocusNode(0);
}

void HTMLSelectElementImpl::focus()
{
    getDocument()->setFocusNode(this);
}

DOMString HTMLSelectElementImpl::value( )
{
    uint i;
    QMemArray<HTMLGenericFormElementImpl*> items = listItems();
    for (i = 0; i < items.size(); i++) {
        if ( items[i]->id() == ID_OPTION
            && static_cast<HTMLOptionElementImpl*>(items[i])->selected())
            return static_cast<HTMLOptionElementImpl*>(items[i])->value();
    }
    return DOMString("");
}

void HTMLSelectElementImpl::setValue(DOMStringImpl* /*value*/)
{
    // ### find the option with value() matching the given parameter
    // and make it the current selection.
    kdWarning() << "Unimplemented HTMLSelectElementImpl::setValue called" << endl;
}

QString HTMLSelectElementImpl::state( )
{
#if !APPLE_CHANGES
    QString state;
#endif
    QMemArray<HTMLGenericFormElementImpl*> items = listItems();

    int l = items.count();

#if APPLE_CHANGES
    QChar stateChars[l];
    
    for(int i = 0; i < l; i++)
        if(items[i]->id() == ID_OPTION && static_cast<HTMLOptionElementImpl*>(items[i])->selected())
            stateChars[i] = 'X';
        else
            stateChars[i] = '.';
    QString state(stateChars, l);
#else /* APPLE_CHANGES not defined */
    state.fill('.', l);
    for(int i = 0; i < l; i++)
        if(items[i]->id() == ID_OPTION && static_cast<HTMLOptionElementImpl*>(items[i])->selected())
            state[i] = 'X';
#endif /* APPLE_CHANGES not defined */

    return HTMLGenericFormElementImpl::state() + state;
}

void HTMLSelectElementImpl::restoreState(QStringList &_states)
{
    QString _state = HTMLGenericFormElementImpl::findMatchingState(_states);
    if (_state.isNull()) return;

    recalcListItems();

    QString state = _state;
    if(!state.isEmpty() && !state.contains('X') && !m_multiple) {
        qWarning("should not happen in restoreState!");
#if APPLE_CHANGES
        // KWQString doesn't support this operation. Should never get here anyway.
        //state[0] = 'X';
#else
        state[0] = 'X';
#endif
    }

    QMemArray<HTMLGenericFormElementImpl*> items = listItems();

    int l = items.count();
    for(int i = 0; i < l; i++) {
        if(items[i]->id() == ID_OPTION) {
            HTMLOptionElementImpl* oe = static_cast<HTMLOptionElementImpl*>(items[i]);
            oe->setSelected(state[i] == 'X');
        }
    }
    setChanged(true);
}

NodeImpl *HTMLSelectElementImpl::insertBefore ( NodeImpl *newChild, NodeImpl *refChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::insertBefore(newChild,refChild, exceptioncode );
    if (!exceptioncode)
        setRecalcListItems();
    return result;
}

NodeImpl *HTMLSelectElementImpl::replaceChild ( NodeImpl *newChild, NodeImpl *oldChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::replaceChild(newChild,oldChild, exceptioncode);
    if( !exceptioncode )
        setRecalcListItems();
    return result;
}

NodeImpl *HTMLSelectElementImpl::removeChild ( NodeImpl *oldChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::removeChild(oldChild, exceptioncode);
    if( !exceptioncode )
        setRecalcListItems();
    return result;
}

NodeImpl *HTMLSelectElementImpl::appendChild ( NodeImpl *newChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::appendChild(newChild, exceptioncode);
    if( !exceptioncode )
        setRecalcListItems();
    setChanged(true);
    return result;
}

NodeImpl* HTMLSelectElementImpl::addChild(NodeImpl* newChild)
{
    setRecalcListItems();
    return HTMLGenericFormElementImpl::addChild(newChild);
}

void HTMLSelectElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_SIZE:
        m_size = QMAX( attr->val()->toInt(), 1 );
        break;
    case ATTR_WIDTH:
        m_minwidth = QMAX( attr->val()->toInt(), 0 );
        break;
    case ATTR_MULTIPLE:
        m_multiple = (attr->val() != 0);
        break;
    case ATTR_ACCESSKEY:
        // ### ignore for the moment
        break;
    case ATTR_ONFOCUS:
        setHTMLEventListener(EventImpl::FOCUS_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONBLUR:
        setHTMLEventListener(EventImpl::BLUR_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONCHANGE:
        setHTMLEventListener(EventImpl::CHANGE_EVENT,
            getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    default:
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

RenderObject *HTMLSelectElementImpl::createRenderer(RenderArena *arena, RenderStyle *style)
{
    return new (arena) RenderSelect(this);
}

bool HTMLSelectElementImpl::encoding(const QTextCodec* codec, khtml::encodingList& encoded_values, bool)
{
    bool successful = false;
    QCString enc_name = fixUpfromUnicode(codec, name().string());
    QMemArray<HTMLGenericFormElementImpl*> items = listItems();

    uint i;
    for (i = 0; i < items.size(); i++) {
        if (items[i]->id() == ID_OPTION) {
            HTMLOptionElementImpl *option = static_cast<HTMLOptionElementImpl*>(items[i]);
            if (option->selected()) {
                encoded_values += enc_name;
                encoded_values += fixUpfromUnicode(codec, option->value().string());
                successful = true;
            }
        }
    }

    // ### this case should not happen. make sure that we select the first option
    // in any case. otherwise we have no consistency with the DOM interface. FIXME!
    // we return the first one if it was a combobox select
    if (!successful && !m_multiple && m_size <= 1 && items.size() &&
        (items[0]->id() == ID_OPTION) ) {
        HTMLOptionElementImpl *option = static_cast<HTMLOptionElementImpl*>(items[0]);
        encoded_values += enc_name;
        if (option->value().isNull())
            encoded_values += fixUpfromUnicode(codec, option->text().string().stripWhiteSpace());
        else
            encoded_values += fixUpfromUnicode(codec, option->value().string());
        successful = true;
    }

    return successful;
}

int HTMLSelectElementImpl::optionToListIndex(int optionIndex) const
{
    QMemArray<HTMLGenericFormElementImpl*> items = listItems();
    if (optionIndex < 0 || optionIndex >= int(items.size()))
        return -1;

    int listIndex = 0;
    int optionIndex2 = 0;
    for (;
         optionIndex2 < int(items.size()) && optionIndex2 <= optionIndex;
         listIndex++) { // not a typo!
        if (items[listIndex]->id() == ID_OPTION)
            optionIndex2++;
    }
    listIndex--;
    return listIndex;
}

int HTMLSelectElementImpl::listToOptionIndex(int listIndex) const
{
    QMemArray<HTMLGenericFormElementImpl*> items = listItems();
    if (listIndex < 0 || listIndex >= int(items.size()) ||
        items[listIndex]->id() != ID_OPTION)
        return -1;

    int optionIndex = 0; // actual index of option not counting OPTGROUP entries that may be in list
    int i;
    for (i = 0; i < listIndex; i++)
        if (items[i]->id() == ID_OPTION)
            optionIndex++;
    return optionIndex;
}

void HTMLSelectElementImpl::recalcListItems()
{
    NodeImpl* current = firstChild();
    m_listItems.resize(0);
    HTMLOptionElementImpl* foundSelected = 0;
    while(current) {
        if (current->id() == ID_OPTGROUP && current->firstChild()) {
            // ### what if optgroup contains just comments? don't want one of no options in it...
            m_listItems.resize(m_listItems.size()+1);
            m_listItems[m_listItems.size()-1] = static_cast<HTMLGenericFormElementImpl*>(current);
            current = current->firstChild();
        }
        if (current->id() == ID_OPTION) {
            m_listItems.resize(m_listItems.size()+1);
            m_listItems[m_listItems.size()-1] = static_cast<HTMLGenericFormElementImpl*>(current);
            if (!foundSelected && !m_multiple && m_size <= 1) {
                foundSelected = static_cast<HTMLOptionElementImpl*>(current);
                foundSelected->m_selected = true;
            }
            else if (foundSelected && !m_multiple && static_cast<HTMLOptionElementImpl*>(current)->selected()) {
                foundSelected->m_selected = false;
                foundSelected = static_cast<HTMLOptionElementImpl*>(current);
            }
        }
        NodeImpl *parent = current->parentNode();
        current = current->nextSibling();
        if (!current) {
            if (parent != this)
                current = parent->nextSibling();
        }
    }
    m_recalcListItems = false;
}

void HTMLSelectElementImpl::childrenChanged()
{
    setRecalcListItems();

    HTMLGenericFormElementImpl::childrenChanged();
}

void HTMLSelectElementImpl::setRecalcListItems()
{
    m_recalcListItems = true;
    if (m_render)
        static_cast<khtml::RenderSelect*>(m_render)->setOptionsChanged(true);
    setChanged();
}

void HTMLSelectElementImpl::reset()
{
    QMemArray<HTMLGenericFormElementImpl*> items = listItems();
    uint i;
    for (i = 0; i < items.size(); i++) {
        if (items[i]->id() == ID_OPTION) {
            HTMLOptionElementImpl *option = static_cast<HTMLOptionElementImpl*>(items[i]);
            bool selected = (!option->getAttribute(ATTR_SELECTED).isNull());
            option->setSelected(selected);
        }
    }
    if ( m_render )
        static_cast<RenderSelect*>(m_render)->setSelectionChanged(true);
    setChanged( true );
}

void HTMLSelectElementImpl::notifyOptionSelected(HTMLOptionElementImpl *selectedOption, bool selected)
{
    if (selected && !m_multiple) {
        // deselect all other options
        QMemArray<HTMLGenericFormElementImpl*> items = listItems();
        uint i;
        for (i = 0; i < items.size(); i++) {
            if (items[i]->id() == ID_OPTION)
                static_cast<HTMLOptionElementImpl*>(items[i])->m_selected = (items[i] == selectedOption);
        }
    }
    if (m_render)
        static_cast<RenderSelect*>(m_render)->setSelectionChanged(true);

    setChanged(true);
}

#if APPLE_CHANGES

void HTMLSelectElementImpl::defaultEventHandler(EventImpl *evt)
{
    // Use key press event here since sending simulated mouse events
    // on key down blocks the proper sending of the key press event.
    if (evt->id() == EventImpl::KEYPRESS_EVENT) {
    
        if (!m_form || !m_render || !evt->isKeyboardEvent())
            return;
        
        DOMString key = static_cast<KeyboardEventImpl *>(evt)->keyIdentifier();
        
        if (key == "Enter") {
            m_form->submitClick();
            evt->setDefaultHandled();
        }
    }
    HTMLGenericFormElementImpl::defaultEventHandler(evt);
}

#endif // APPLE_CHANGES

void HTMLSelectElementImpl::accessKeyAction()
{
    focus();
}

// -------------------------------------------------------------------------

HTMLKeygenElementImpl::HTMLKeygenElementImpl(DocumentPtr* doc, HTMLFormElementImpl* f)
    : HTMLSelectElementImpl(doc, f)
{
    QStringList keys = KSSLKeyGen::supportedKeySizes();
    for (QStringList::Iterator i = keys.begin(); i != keys.end(); ++i) {
        HTMLOptionElementImpl* o = new HTMLOptionElementImpl(doc, form());
        addChild(o);
        o->addChild(new TextImpl(doc, DOMString(*i)));
    }
}

NodeImpl::Id HTMLKeygenElementImpl::id() const
{
    return ID_KEYGEN;
}

DOMString HTMLKeygenElementImpl::type() const
{
    return "keygen";
}

void HTMLKeygenElementImpl::parseAttribute(AttributeImpl* attr)
{
    switch(attr->id())
    {
    case ATTR_CHALLENGE:
        m_challenge = attr->val();
        break;
    case ATTR_KEYTYPE:
        m_keyType = attr->val();
        break;
    default:
        // skip HTMLSelectElementImpl parsing!
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

bool HTMLKeygenElementImpl::encoding(const QTextCodec* codec, khtml::encodingList& encoded_values, bool)
{
    bool successful = false;
    QCString enc_name = fixUpfromUnicode(codec, name().string());

#if APPLE_CHANGES
    // Only RSA is supported at this time.
    if (!m_keyType.isNull() && m_keyType.lower() != "rsa") {
        return false;
    }
    QString value = KSSLKeyGen::signedPublicKeyAndChallengeString((unsigned)selectedIndex(), m_challenge.string(), getDocument()->part()->baseURL());
    if (!value.isNull()) {
        encoded_values += enc_name;
        encoded_values += value.utf8();
        successful = true;
    }
#else
    encoded_values += enc_name;
    
    // pop up the fancy certificate creation dialog here
    KSSLKeyGen *kg = new KSSLKeyGen(static_cast<RenderWidget *>(m_render)->widget(), "Key Generator", true);

    kg->setKeySize(0);
    successful = (QDialog::Accepted == kg->exec());

    delete kg;

    encoded_values += "deadbeef";
#endif
    
    return successful;
}

// -------------------------------------------------------------------------

HTMLOptGroupElementImpl::HTMLOptGroupElementImpl(DocumentPtr *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
}

HTMLOptGroupElementImpl::~HTMLOptGroupElementImpl()
{
}

bool HTMLOptGroupElementImpl::isFocusable() const
{
    return false;
}

NodeImpl::Id HTMLOptGroupElementImpl::id() const
{
    return ID_OPTGROUP;
}

DOMString HTMLOptGroupElementImpl::type() const
{
    return "optgroup";
}

NodeImpl *HTMLOptGroupElementImpl::insertBefore ( NodeImpl *newChild, NodeImpl *refChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::insertBefore(newChild,refChild, exceptioncode);
    if ( !exceptioncode )
        recalcSelectOptions();
    return result;
}

NodeImpl *HTMLOptGroupElementImpl::replaceChild ( NodeImpl *newChild, NodeImpl *oldChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::replaceChild(newChild,oldChild, exceptioncode);
    if(!exceptioncode)
        recalcSelectOptions();
    return result;
}

NodeImpl *HTMLOptGroupElementImpl::removeChild ( NodeImpl *oldChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::removeChild(oldChild, exceptioncode);
    if( !exceptioncode )
        recalcSelectOptions();
    return result;
}

NodeImpl *HTMLOptGroupElementImpl::appendChild ( NodeImpl *newChild, int &exceptioncode )
{
    NodeImpl *result = HTMLGenericFormElementImpl::appendChild(newChild, exceptioncode);
    if( !exceptioncode )
        recalcSelectOptions();
    return result;
}

NodeImpl* HTMLOptGroupElementImpl::addChild(NodeImpl* newChild)
{
    recalcSelectOptions();

    return HTMLGenericFormElementImpl::addChild(newChild);
}

void HTMLOptGroupElementImpl::parseAttribute(AttributeImpl *attr)
{
    HTMLGenericFormElementImpl::parseAttribute(attr);
    recalcSelectOptions();
}

void HTMLOptGroupElementImpl::recalcSelectOptions()
{
    NodeImpl *select = parentNode();
    while (select && select->id() != ID_SELECT)
        select = select->parentNode();
    if (select)
        static_cast<HTMLSelectElementImpl*>(select)->setRecalcListItems();
}

// -------------------------------------------------------------------------

HTMLOptionElementImpl::HTMLOptionElementImpl(DocumentPtr *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
    m_selected = false;
}

bool HTMLOptionElementImpl::isFocusable() const
{
    return false;
}

NodeImpl::Id HTMLOptionElementImpl::id() const
{
    return ID_OPTION;
}

DOMString HTMLOptionElementImpl::type() const
{
    return "option";
}

DOMString HTMLOptionElementImpl::text() const
{
    DOMString label = getAttribute(ATTR_LABEL);
    if (label.isEmpty() && firstChild() && firstChild()->nodeType() == Node::TEXT_NODE) {
	if (firstChild()->nextSibling()) {
	    DOMString ret = "";
	    NodeImpl *n = firstChild();
	    for (; n; n = n->nextSibling()) {
		if (n->nodeType() == Node::TEXT_NODE ||
		    n->nodeType() == Node::CDATA_SECTION_NODE)
		    ret += n->nodeValue();
	    }
	    return ret;
	}
	else
	    return firstChild()->nodeValue();
    }
    else
        return label;
}

long HTMLOptionElementImpl::index() const
{
    // Let's do this dynamically. Might be a bit slow, but we're sure
    // we won't forget to update a member variable in some cases...
    QMemArray<HTMLGenericFormElementImpl*> items = getSelect()->listItems();
    int l = items.count();
    int optionIndex = 0;
    for(int i = 0; i < l; i++) {
        if(items[i]->id() == ID_OPTION)
        {
            if (static_cast<HTMLOptionElementImpl*>(items[i]) == this)
                return optionIndex;
            optionIndex++;
        }
    }
    kdWarning() << "HTMLOptionElementImpl::index(): option not found!" << endl;
    return 0;
}

void HTMLOptionElementImpl::setIndex( long  )
{
    kdWarning() << "Unimplemented HTMLOptionElementImpl::setIndex(long) called" << endl;
    // ###
}

void HTMLOptionElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_SELECTED:
        m_selected = (attr->val() != 0);
        break;
    case ATTR_VALUE:
        m_value = attr->value();
        break;
    default:
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

DOMString HTMLOptionElementImpl::value() const
{
    if ( !m_value.isNull() )
        return m_value;
    // Use the text if the value wasn't set.
    return text().string().stripWhiteSpace();
}

void HTMLOptionElementImpl::setValue(DOMStringImpl* value)
{
    setAttribute(ATTR_VALUE, value);
}

void HTMLOptionElementImpl::setSelected(bool _selected)
{
    if(m_selected == _selected)
        return;
    m_selected = _selected;
    HTMLSelectElementImpl *select = getSelect();
    if (select)
        select->notifyOptionSelected(this,_selected);
}

void HTMLOptionElementImpl::childrenChanged()
{
   HTMLSelectElementImpl *select = getSelect();
   if (select)
       select->childrenChanged();
}

HTMLSelectElementImpl *HTMLOptionElementImpl::getSelect() const
{
    NodeImpl *select = parentNode();
    while (select && select->id() != ID_SELECT)
        select = select->parentNode();
    return static_cast<HTMLSelectElementImpl*>(select);
}

// -------------------------------------------------------------------------

HTMLTextAreaElementImpl::HTMLTextAreaElementImpl(DocumentPtr *doc, HTMLFormElementImpl *f)
    : HTMLGenericFormElementImpl(doc, f)
{
    // DTD requires rows & cols be specified, but we will provide reasonable defaults
    m_rows = 2;
    m_cols = 20;
    m_wrap = ta_Virtual;
    m_dirtyvalue = true;
}

HTMLTextAreaElementImpl::~HTMLTextAreaElementImpl()
{
    if (getDocument()) getDocument()->deregisterMaintainsState(this);
}

NodeImpl::Id HTMLTextAreaElementImpl::id() const
{
    return ID_TEXTAREA;
}

DOMString HTMLTextAreaElementImpl::type() const
{
    return "textarea";
}

QString HTMLTextAreaElementImpl::state( )
{
    // Make sure the string is not empty!
    return HTMLGenericFormElementImpl::state() + value().string()+'.';
}

void HTMLTextAreaElementImpl::restoreState(QStringList &states)
{
    QString state = HTMLGenericFormElementImpl::findMatchingState(states);
    if (state.isNull()) return;
    setDefaultValue(state.left(state.length()-1));
    // the close() in the rendertree will take care of transferring defaultvalue to 'value'
}

void HTMLTextAreaElementImpl::select(  )
{
    if (m_render)
        static_cast<RenderTextArea*>(m_render)->select();
    onSelect();
}

void HTMLTextAreaElementImpl::parseAttribute(AttributeImpl *attr)
{
    switch(attr->id())
    {
    case ATTR_ROWS:
        m_rows = attr->val() ? attr->val()->toInt() : 3;
        break;
    case ATTR_COLS:
        m_cols = attr->val() ? attr->val()->toInt() : 60;
        break;
    case ATTR_WRAP:
        // virtual / physical is Netscape extension of HTML 3.0, now deprecated
        // soft/ hard / off is recommendation for HTML 4 extension by IE and NS 4
        if ( strcasecmp( attr->value(), "virtual" ) == 0  || strcasecmp( attr->value(), "soft") == 0)
            m_wrap = ta_Virtual;
        else if ( strcasecmp ( attr->value(), "physical" ) == 0 || strcasecmp( attr->value(), "hard") == 0)
            m_wrap = ta_Physical;
        else if(strcasecmp( attr->value(), "on" ) == 0)
            m_wrap = ta_Physical;
        else if(strcasecmp( attr->value(), "off") == 0)
            m_wrap = ta_NoWrap;
        break;
    case ATTR_ACCESSKEY:
        // ignore for the moment
        break;
    case ATTR_ONFOCUS:
        setHTMLEventListener(EventImpl::FOCUS_EVENT,
	    getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONBLUR:
        setHTMLEventListener(EventImpl::BLUR_EVENT,
	    getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONSELECT:
        setHTMLEventListener(EventImpl::SELECT_EVENT,
	    getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    case ATTR_ONCHANGE:
        setHTMLEventListener(EventImpl::CHANGE_EVENT,
	    getDocument()->createHTMLEventListener(attr->value().string()));
        break;
    default:
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

RenderObject *HTMLTextAreaElementImpl::createRenderer(RenderArena *arena, RenderStyle *style)
{
    return new (arena) RenderTextArea(this);
}

bool HTMLTextAreaElementImpl::encoding(const QTextCodec* codec, encodingList& encoding, bool)
{
    if (name().isEmpty()) return false;

    encoding += fixUpfromUnicode(codec, name().string());
    encoding += fixUpfromUnicode(codec, value().string());

    return true;
}

void HTMLTextAreaElementImpl::reset()
{
    setValue(defaultValue());
}

DOMString HTMLTextAreaElementImpl::value()
{
    if ( m_dirtyvalue) {
        if ( m_render )
            m_value = static_cast<RenderTextArea*>( m_render )->text();
        else
            m_value = defaultValue().string();
        m_dirtyvalue = false;
    }

    if ( m_value.isNull() ) return "";

    return m_value;
}

void HTMLTextAreaElementImpl::setValue(DOMString _value)
{
    m_value = _value.string();
    m_dirtyvalue = false;
    setChanged(true);
}


DOMString HTMLTextAreaElementImpl::defaultValue()
{
    DOMString val = "";
    // there may be comments - just grab the text nodes
    NodeImpl *n;
    for (n = firstChild(); n; n = n->nextSibling())
        if (n->isTextNode())
            val += static_cast<TextImpl*>(n)->data();
    if (val[0] == '\r' && val[1] == '\n') {
	val = val.copy();
	val.remove(0,2);
    }
    else if (val[0] == '\r' || val[0] == '\n') {
	val = val.copy();
	val.remove(0,1);
    }

    return val;
}

void HTMLTextAreaElementImpl::setDefaultValue(DOMString _defaultValue)
{
    // there may be comments - remove all the text nodes and replace them with one
    QPtrList<NodeImpl> toRemove;
    NodeImpl *n;
    for (n = firstChild(); n; n = n->nextSibling())
        if (n->isTextNode())
            toRemove.append(n);
    QPtrListIterator<NodeImpl> it(toRemove);
    int exceptioncode = 0;
    for (; it.current(); ++it) {
        removeChild(it.current(), exceptioncode);
    }
    insertBefore(getDocument()->createTextNode(_defaultValue),firstChild(), exceptioncode);
    setValue(_defaultValue);
}

void HTMLTextAreaElementImpl::blur()
{
    if(getDocument()->focusNode() == this)
	getDocument()->setFocusNode(0);
}

void HTMLTextAreaElementImpl::focus()
{
    getDocument()->setFocusNode(this);
}

bool HTMLTextAreaElementImpl::isEditable()
{
    return true;
}

void HTMLTextAreaElementImpl::accessKeyAction()
{
    focus();
}

// -------------------------------------------------------------------------

HTMLIsIndexElementImpl::HTMLIsIndexElementImpl(DocumentPtr *doc, HTMLFormElementImpl *f)
    : HTMLInputElementImpl(doc, f)
{
    m_type = TEXT;
    setName("isindex");
}

HTMLIsIndexElementImpl::~HTMLIsIndexElementImpl()
{
}

NodeImpl::Id HTMLIsIndexElementImpl::id() const
{
    return ID_ISINDEX;
}

void HTMLIsIndexElementImpl::parseAttribute(AttributeImpl* attr)
{
    switch(attr->id())
    {
    case ATTR_PROMPT:
	setValue(attr->value());
    default:
        // don't call HTMLInputElement::parseAttribute here, as it would
        // accept attributes this element does not support
        HTMLGenericFormElementImpl::parseAttribute(attr);
    }
}

// -------------------------------------------------------------------------

