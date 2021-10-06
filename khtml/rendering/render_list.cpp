/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
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

#include "render_list.h"
#include "render_arena.h"
#include "rendering/render_canvas.h"

#include "xml/dom_docimpl.h"
#include "misc/htmltags.h"

#include <qpainter.h>

#include "misc/helper.h"

#include <kdebug.h>

//#define BOX_DEBUG

using DOM::DocumentImpl;
using namespace khtml;

const int cMarkerPadding = 7;

static QString toRoman( int number, bool upper )
{
    QString roman;
    QChar ldigits[] = { 'i', 'v', 'x', 'l', 'c', 'd', 'm' };
    QChar udigits[] = { 'I', 'V', 'X', 'L', 'C', 'D', 'M' };
    QChar *digits = upper ? udigits : ldigits;
    int i, d = 0;

    do
    {
        int num = number % 10;

        if ( num % 5 < 4 )
            for ( i = num % 5; i > 0; i-- )
                roman.insert( 0, digits[ d ] );

        if ( num >= 4 && num <= 8)
            roman.insert( 0, digits[ d+1 ] );

        if ( num == 9 )
            roman.insert( 0, digits[ d+2 ] );

        if ( num % 5 == 4 )
            roman.insert( 0, digits[ d ] );

        number /= 10;
        d += 2;
    }
    while ( number );

    return roman;
}

static QString toLetter( int number, int base ) {
    number--;
    QString letter = (QChar) (base + (number % 26));
    // Add a single quote at the end of the alphabet.
    for (int i = 0; i < (number / 26); i++) {
       letter += '\'';
    }
    return letter;
}

static QString toHebrew( int number ) {
    const QChar tenDigit[] = {1497, 1499, 1500, 1502, 1504, 1505, 1506, 1508, 1510};

    QString letter;
    if (number>999) {
  	letter = toHebrew(number/1000) + QString::fromLatin1("'");
   	number = number%1000;
    }

    int hunderts = (number/400);
    if (hunderts > 0) {
	for(int i=0; i<hunderts; i++) {
	    letter += QChar(1511 + 3);
	}
    }
    number = number % 400;
    if ((number / 100) != 0) {
        letter += QChar (1511 + (number / 100) -1);
    }
    number = number % 100;
    int tens = number/10;
    if (tens > 0 && !(number == 15 || number == 16)) {
	letter += tenDigit[tens-1];
    }
    if (number == 15 || number == 16) { // special because of religious
	letter += QChar(1487 + 9);       // reasons
    	letter += QChar(1487 + number - 9);
    } else {
        number = number % 10;
        if (number != 0) {
            letter += QChar (1487 + number);
        }
    }
    return letter;
}

// -------------------------------------------------------------------------

RenderListItem::RenderListItem(DOM::NodeImpl* node)
    : RenderBlock(node), _notInList(false)
{
    // init RenderObject attributes
    setInline(false);   // our object is not Inline

    predefVal = -1;
    m_marker = 0;
}

void RenderListItem::setStyle(RenderStyle *_style)
{
    RenderBlock::setStyle(_style);

    if (style()->listStyleType() != LNONE ||
        (style()->listStyleImage() && !style()->listStyleImage()->isErrorImage())) {
        RenderStyle *newStyle = new (renderArena()) RenderStyle();
        newStyle->ref();
        // The marker always inherits from the list item, regardless of where it might end
        // up (e.g., in some deeply nested line box).  See CSS3 spec.
        newStyle->inheritFrom(style()); 
        if (!m_marker) {
            m_marker = new (renderArena()) RenderListMarker(document());
            m_marker->setStyle(newStyle);
            m_marker->setListItem(this);
        }
        else
            m_marker->setStyle(newStyle);
        newStyle->deref(renderArena());
    } else if (m_marker) {
        m_marker->detach();
        m_marker = 0;
    }
}

RenderListItem::~RenderListItem()
{
}

void RenderListItem::detach()
{    
    if (m_marker) {
        m_marker->detach();
        m_marker = 0;
    }
    RenderBlock::detach();
}

void RenderListItem::calcListValue()
{
    // only called from the marker so..
    KHTMLAssert(m_marker);

    if(predefVal != -1)
        m_marker->m_value = predefVal;
    else if(!previousSibling())
        m_marker->m_value = 1;
    else {
	RenderObject *o = previousSibling();
	while ( o && (!o->isListItem() || o->style()->listStyleType() == LNONE) )
	    o = o->previousSibling();
        if( o && o->isListItem() && o->style()->listStyleType() != LNONE ) {
            RenderListItem *item = static_cast<RenderListItem *>(o);
            m_marker->m_value = item->value() + 1;
        }
        else
            m_marker->m_value = 1;
    }
}

static RenderObject* getParentOfFirstLineBox(RenderObject* curr, RenderObject* marker)
{
    RenderObject* firstChild = curr->firstChild();
    if (!firstChild)
        return 0;
        
    for (RenderObject* currChild = firstChild;
         currChild; currChild = currChild->nextSibling()) {
        if (currChild == marker)
            continue;
            
        if (currChild->isInline())
            return curr;
        
        if (currChild->isFloating() || currChild->isPositioned())
            continue;
            
        if (currChild->isTable() || !currChild->isRenderBlock())
            break;
        
        if (currChild->style()->htmlHacks() && currChild->element() &&
            (currChild->element()->id() == ID_UL || currChild->element()->id() == ID_OL))
            break;
            
        RenderObject* lineBox = getParentOfFirstLineBox(currChild, marker);
        if (lineBox)
            return lineBox;
    }
    
    return 0;
}

void RenderListItem::updateMarkerLocation()
{
    // Sanity check the location of our marker.
    if (m_marker) {
        RenderObject* markerPar = m_marker->parent();
        RenderObject* lineBoxParent = getParentOfFirstLineBox(this, m_marker);
        if (!lineBoxParent) {
            // If the marker is currently contained inside an anonymous box,
            // then we are the only item in that anonymous box (since no line box
            // parent was found).  It's ok to just leave the marker where it is
            // in this case.
            if (markerPar && markerPar->isAnonymousBlock())
                lineBoxParent = markerPar;
            else
                lineBoxParent = this;
        }
        if (markerPar != lineBoxParent)
        {
            m_marker->remove();
            if (!lineBoxParent)
                lineBoxParent = this;
            lineBoxParent->addChild(m_marker, lineBoxParent->firstChild());
            if (!m_marker->minMaxKnown())
                m_marker->calcMinMaxWidth();
            recalcMinMaxWidths();
        }
    }
}

void RenderListItem::calcMinMaxWidth()
{
    // Make sure our marker is in the correct location.
    updateMarkerLocation();
    if (!minMaxKnown())
        RenderBlock::calcMinMaxWidth();
}

void RenderListItem::layout( )
{
    KHTMLAssert( needsLayout() );
    KHTMLAssert( minMaxKnown() );

    updateMarkerLocation();    
    RenderBlock::layout();
}

void RenderListItem::paint(PaintInfo& i, int _tx, int _ty)
{
    if (!m_height)
        return;

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << nodeName().string() << "(LI)::paint()" << endl;
#endif
    RenderBlock::paint(i, _tx, _ty);
}

QRect RenderListItem::getAbsoluteRepaintRect()
{
    QRect result = RenderBlock::getAbsoluteRepaintRect();
    if (m_marker && !m_marker->isInside()) {
        // This can be a sloppy and imprecise offset as long as it's always too big.
        int pixHeight = style()->htmlFont().getFontDef().computedPixelSize();
        int offset = pixHeight*2/3;
        bool haveImage = m_marker->listImage() && !m_marker->listImage()->isErrorImage();
        if (haveImage)
            offset = m_marker->listImage()->pixmap().width();
        int bulletWidth = offset/2;
        if (offset%2)
            bulletWidth++;
        int xoff = 0;
        if (style()->direction() == LTR)
            xoff = -cMarkerPadding - offset;
        else
            xoff = cMarkerPadding + (haveImage ? 0 : (offset - bulletWidth));

        if (xoff < 0) {
            result.setX(result.x() + xoff);
            result.setWidth(result.width() - xoff);
        }
        else
            result.setWidth(result.width() + xoff);
    }
    return result;
}

// -----------------------------------------------------------

RenderListMarker::RenderListMarker(DocumentImpl* document)
    : RenderBox(document), m_listImage(0), m_value(-1)
{
    // init RenderObject attributes
    setInline(true);   // our object is Inline
    setReplaced(true); // pretend to be replaced
    // val = -1;
    // m_listImage = 0;
}

RenderListMarker::~RenderListMarker()
{
    if(m_listImage)
        m_listImage->deref(this);
}

void RenderListMarker::setStyle(RenderStyle *s)
{
    if ( s && style() && s->listStylePosition() != style()->listStylePosition() )
        setNeedsLayoutAndMinMaxRecalc();
    
    RenderBox::setStyle(s);

    if ( m_listImage != style()->listStyleImage() ) {
	if(m_listImage)  m_listImage->deref(this);
	m_listImage = style()->listStyleImage();
	if(m_listImage) m_listImage->ref(this);
    }
}

InlineBox* RenderListMarker::createInlineBox(bool, bool isRootLineBox, bool)
{
    KHTMLAssert(!isRootLineBox);
    ListMarkerBox* box = new (renderArena()) ListMarkerBox(this);
    m_inlineBoxWrapper = box;
    return box;
}

void RenderListMarker::paint(PaintInfo& i, int _tx, int _ty)
{
    if (i.phase != PaintActionForeground)
        return;
    
    if (style()->visibility() != VISIBLE)  return;

    _tx += m_x;
    _ty += m_y;

    if ((_ty > i.r.y() + i.r.height()) || (_ty + m_height < i.r.y()))
        return;

    if (shouldPaintBackgroundOrBorder()) 
        paintBoxDecorations(i, _tx, _ty);

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << nodeName().string() << "(ListMarker)::paintObject(" << _tx << ", " << _ty << ")" << endl;
#endif

    QPainter* p = i.p;
    p->setFont(style()->font());
    const QFontMetrics fm = p->fontMetrics();
    
    // The marker needs to adjust its tx, for the case where it's an outside marker.
    RenderObject* listItem = 0;
    int leftLineOffset = 0;
    int rightLineOffset = 0;
    if (!isInside()) {
        listItem = this;
        int yOffset = 0;
        int xOffset = 0;
        while (listItem && listItem != m_listItem) {
            yOffset += listItem->yPos();
            xOffset += listItem->xPos();
            listItem = listItem->parent();
        }
        
        // Now that we have our xoffset within the listbox, we need to adjust ourselves by the delta
        // between our current xoffset and our desired position (which is just outside the border box
        // of the list item).
        if (style()->direction() == LTR) {
            leftLineOffset = m_listItem->leftRelOffset(yOffset, m_listItem->leftOffset(yOffset));
            _tx -= (xOffset - leftLineOffset) + m_listItem->paddingLeft() + m_listItem->borderLeft();
        }
        else {
            rightLineOffset = m_listItem->rightRelOffset(yOffset, m_listItem->rightOffset(yOffset));
            _tx += (rightLineOffset-xOffset) + m_listItem->paddingRight() + m_listItem->borderRight();
        }
    }

    bool isPrinting = (p->device()->devType() == QInternal::Printer);
    if (isPrinting) {
        if (_ty < i.r.y())
            // This has been printed already we suppose.
            return;
        
        RenderCanvas* c = canvas();
        if (_ty + m_height + paddingBottom() + borderBottom() >= c->printRect().y() + c->printRect().height()) {
            if (_ty < c->truncatedAt())
                c->setBestTruncatedAt(_ty, this);
            // Let's print this on the next page.
            return; 
        }
    }
    
    int offset = fm.ascent()*2/3;
    bool haveImage = m_listImage && !m_listImage->isErrorImage();
    if (haveImage)
        offset = m_listImage->pixmap().width();
    
    int xoff = 0;
    int yoff = fm.ascent() - offset;

    int bulletWidth = offset/2;
    if (offset%2)
        bulletWidth++;
    if (!isInside()) {
        if (listItem->style()->direction() == LTR)
            xoff = -cMarkerPadding - offset;
        else
            xoff = cMarkerPadding + (haveImage ? 0 : (offset - bulletWidth));
    }
    else if (style()->direction() == RTL)
        xoff += haveImage ? cMarkerPadding : (m_width - bulletWidth);
    
    if (m_listImage && !m_listImage->isErrorImage()) {
        m_listPixmap = m_listImage->pixmap();
        p->drawPixmap(QPoint(_tx + xoff, _ty), m_listPixmap);
        return;
    }

#ifdef BOX_DEBUG
    p->setPen( Qt::red );
    p->drawRect( _tx + xoff, _ty + yoff, offset, offset );
#endif

    const QColor color( style()->color() );
    p->setPen( color );

    switch(style()->listStyleType()) {
    case DISC:
        p->setBrush(color);
        p->drawEllipse(_tx + xoff, _ty + (3 * yoff)/2, bulletWidth, bulletWidth);
        return;
    case CIRCLE:
        p->setBrush(Qt::NoBrush);
        p->drawEllipse(_tx + xoff, _ty + (3 * yoff)/2, bulletWidth, bulletWidth);
        return;
    case SQUARE:
        p->setBrush(color);
        p->drawRect(_tx + xoff, _ty + (3 * yoff)/2, bulletWidth, bulletWidth);
        return;
    case LNONE:
        return;
    default:
        if (!m_item.isEmpty()) {
#if APPLE_CHANGES
            // Text should be drawn on the baseline, so we add in the ascent of the font. 
            // For some inexplicable reason, this works in Konqueror.  I'm not sure why.
            // - dwh
       	    _ty += fm.ascent();
#else
       	    //_ty += fm.ascent() - fm.height()/2 + 1;
#endif

            if (isInside()) {
            	if( style()->direction() == LTR) {
                    p->drawText(_tx, _ty, 0, 0, 0, 0, Qt::AlignLeft|Qt::DontClip, m_item);
                    p->drawText(_tx + fm.width(m_item, 0, 0), _ty, 0, 0, 0, 0, Qt::AlignLeft|Qt::DontClip, 
                            QString::fromLatin1(". "));
                }
            	else {
                    const QString& punct(QString::fromLatin1(" ."));
                    p->drawText(_tx, _ty, 0, 0, 0, 0, Qt::AlignLeft|Qt::DontClip, punct);
            	    p->drawText(_tx + fm.width(punct, 0, 0), _ty, 0, 0, 0, 0, Qt::AlignLeft|Qt::DontClip, m_item);
                }
            } else {
                if (style()->direction() == LTR) {
                    const QString& punct(QString::fromLatin1(". "));
                    p->drawText(_tx-offset/2, _ty, 0, 0, 0, 0, Qt::AlignRight|Qt::DontClip, punct);
                    p->drawText(_tx-offset/2-fm.width(punct, 0, 0), _ty, 0, 0, 0, 0, Qt::AlignRight|Qt::DontClip, m_item);
                }
            	else {
                    const QString& punct(QString::fromLatin1(" ."));
            	    p->drawText(_tx+offset/2, _ty, 0, 0, 0, 0, Qt::AlignLeft|Qt::DontClip, punct);
                    p->drawText(_tx+offset/2+fm.width(punct, 0, 0), _ty, 0, 0, 0, 0, Qt::AlignLeft|Qt::DontClip, m_item);
                }
            }
        }
    }
}

void RenderListMarker::layout()
{
    KHTMLAssert( needsLayout() );
    // ### KHTMLAssert( minMaxKnown() );
    if ( !minMaxKnown() )
	calcMinMaxWidth();
    setNeedsLayout(false);
}

void RenderListMarker::setPixmap( const QPixmap &p, const QRect& r, CachedImage *o)
{
    if(o != m_listImage) {
        RenderBox::setPixmap(p, r, o);
        return;
    }

    if (m_width != m_listImage->pixmap_size().width() || m_height != m_listImage->pixmap_size().height())
        setNeedsLayoutAndMinMaxRecalc();
    else
        repaint();
}

void RenderListMarker::calcMinMaxWidth()
{
    KHTMLAssert( !minMaxKnown() );

    m_width = 0;

    if (m_listImage) {
        if (isInside())
            m_width = m_listImage->pixmap().width() + cMarkerPadding;
        m_height = m_listImage->pixmap().height();
        m_minWidth = m_maxWidth = m_width;
        setMinMaxKnown();
        return;
    }

    if (m_value < 0) // not yet calculated
        m_listItem->calcListValue();

    const QFontMetrics &fm = style()->fontMetrics();
    m_height = fm.ascent();

    switch(style()->listStyleType())
    {
    case DISC:
    case CIRCLE:
    case SQUARE:
        if (isInside())
            m_width = m_height;
    	goto end;
    case ARMENIAN:
    case GEORGIAN:
    case CJK_IDEOGRAPHIC:
    case HIRAGANA:
    case KATAKANA:
    case HIRAGANA_IROHA:
    case KATAKANA_IROHA:
    case DECIMAL_LEADING_ZERO:
        // ### unsupported, we use decimal instead
    case LDECIMAL:
        m_item.sprintf( "%ld", m_value );
        break;
    case LOWER_ROMAN:
        m_item = toRoman( m_value, false );
        break;
    case UPPER_ROMAN:
        m_item = toRoman( m_value, true );
        break;
    case LOWER_GREEK:
     {
    	int number = m_value - 1;
      	int l = (number % 24);

	if (l>16) {l++;} // Skip GREEK SMALL LETTER FINAL SIGMA

   	m_item = QChar(945 + l);
    	for (int i = 0; i < (number / 24); i++) {
       	    m_item += QString::fromLatin1("'");
    	}
	break;
     }
    case HEBREW:
     	m_item = toHebrew( m_value );
	break;
    case LOWER_ALPHA:
    case LOWER_LATIN:
        m_item = toLetter( m_value, 'a' );
        break;
    case UPPER_ALPHA:
    case UPPER_LATIN:
        m_item = toLetter( m_value, 'A' );
        break;
    case LNONE:
        break;
    }

    if (isInside()) {
        m_width = fm.width(m_item, 0, 0) + fm.width(QString::fromLatin1(". "), 0, 0);
    }

end:

    m_minWidth = m_width;
    m_maxWidth = m_width;

    setMinMaxKnown();
}

void RenderListMarker::calcWidth()
{
    RenderBox::calcWidth();
}

short RenderListMarker::lineHeight(bool, bool) const
{
    if (!m_listImage)
        return m_listItem->lineHeight(false, true);
    return height();
}

short RenderListMarker::baselinePosition(bool, bool) const
{
    if (!m_listImage) {
        const QFontMetrics &fm = style()->fontMetrics();
        return fm.ascent() + (lineHeight(false) - fm.height())/2;
    }
    return height();
}

bool RenderListMarker::isInside() const
{
    return m_listItem->notInList() || style()->listStylePosition() == INSIDE;
}

#undef BOX_DEBUG
