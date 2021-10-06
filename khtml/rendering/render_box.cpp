/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004 Apple Computer, Inc.
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
// -------------------------------------------------------------------------
//#define DEBUG_LAYOUT


#include <qpainter.h>

#include "rendering/render_box.h"
#include "rendering/render_replaced.h"
#include "rendering/render_canvas.h"
#include "rendering/render_table.h"
#include "render_flexbox.h"
#include "render_arena.h"

#include "misc/htmlhashes.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom_docimpl.h"
#include "html/html_elementimpl.h"

#include "render_line.h"

#include <khtmlview.h>
#include <kdebug.h>
#include <assert.h>


using namespace DOM;
using namespace khtml;

#define TABLECELLMARGIN -0x4000

RenderBox::RenderBox(DOM::NodeImpl* node)
    : RenderObject(node)
{
    m_minWidth = -1;
    m_maxWidth = -1;
    m_overrideSize = -1;
    m_width = m_height = 0;
    m_x = 0;
    m_y = 0;
    m_marginTop = 0;
    m_marginBottom = 0;
    m_marginLeft = 0;
    m_marginRight = 0;
    m_staticX = 0;
    m_staticY = 0;
    m_layer = 0;
    m_inlineBoxWrapper = 0;
}

void RenderBox::setStyle(RenderStyle *_style)
{
    RenderObject::setStyle(_style);

    // The root always paints its background/border.
    if (isRoot())
        setShouldPaintBackgroundOrBorder(true);

    setInline(_style->isDisplayInlineType());

    switch(_style->position())
    {
    case ABSOLUTE:
    case FIXED:
        setPositioned(true);
        break;
    default:
        setPositioned(false);

        if (_style->isFloating())
            setFloating(true);

        if (_style->position() == RELATIVE)
            setRelPositioned(true);
    }

    // FIXME: Note that we restrict overflow to blocks for now.  One day table bodies 
    // will need to support overflow.
    // We also handle <body> and <html>, whose overflow applies to the viewport.
    if (_style->overflow() != OVISIBLE && isBlockFlow() && !isRoot() && (!isBody() || !document()->isHTMLDocument()))
        setHasOverflowClip();

    if (requiresLayer()) {
        if (!m_layer) {
            m_layer = new (renderArena()) RenderLayer(this);
            m_layer->insertOnlyThisLayer();
            if (parent() && containingBlock())
                m_layer->updateLayerPositions();
        }
    }
    else if (m_layer && !isRoot() && !isCanvas()) {
        m_layer->removeOnlyThisLayer();
        m_layer = 0;
    }

    if (m_layer)
        m_layer->styleChanged();
    
    // Set the text color if we're the body.
    if (isBody())
        element()->getDocument()->setTextColor(_style->color());
    
    if (style()->outlineWidth() > 0 && style()->outlineSize() > maximalOutlineSize(PaintActionOutline))
        static_cast<RenderCanvas*>(document()->renderer())->setMaximalOutlineSize(style()->outlineSize());
}

RenderBox::~RenderBox()
{
    //kdDebug( 6040 ) << "Element destructor: this=" << nodeName().string() << endl;
}

void RenderBox::detach()
{
    RenderLayer* layer = m_layer;
    RenderArena* arena = renderArena();
    
    // This must be done before we detach the RenderObject.
    if (layer)
        layer->clearClipRect();
        
    if (m_inlineBoxWrapper) {
        if (!documentBeingDestroyed())
            m_inlineBoxWrapper->remove();
        m_inlineBoxWrapper->detach(arena);
        m_inlineBoxWrapper = 0;
    }

    RenderObject::detach();
    
    if (layer)
        layer->detach(arena);
}

int RenderBox::contentWidth() const
{
    int w = m_width - borderLeft() - borderRight();
    w -= paddingLeft() + paddingRight();

    if (includeScrollbarSize())
        w -= m_layer->verticalScrollbarWidth();
    
    //kdDebug( 6040 ) << "RenderBox::contentWidth(2) = " << w << endl;
    return w;
}

int RenderBox::contentHeight() const
{
    int h = m_height - borderTop() - borderBottom();
    h -= paddingTop() + paddingBottom();

    if (includeScrollbarSize())
        h -= m_layer->horizontalScrollbarHeight();

    return h;
}

int RenderBox::overrideWidth() const
{
    return m_overrideSize == -1 ? m_width : m_overrideSize;
}

int RenderBox::overrideHeight() const
{
    return m_overrideSize == -1 ? m_height : m_overrideSize;
}

void RenderBox::setPos( int xPos, int yPos )
{
    if (xPos == m_x && yPos == m_y)
        return; // Optimize for the case where we don't move at all.
    
    m_x = xPos; m_y = yPos;
}

int RenderBox::width() const
{
    return m_width;
}

int RenderBox::height() const
{
    return m_height;
}

// Hit Testing
bool RenderBox::nodeAtPoint(NodeInfo& info, int _x, int _y, int _tx, int _ty,
                            HitTestAction hitTestAction)
{
    // Check kids first.
    _tx += m_x;
    _ty += m_y;
    for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
        // FIXME: We have to skip over inline flows, since they can show up inside table rows at the moment (a demoted inline <form> for example).  If we ever implement a
        // table-specific hit-test method (which we should do for performance reasons anyway), then we can remove this check.
        if (!child->layer() && !child->isInlineFlow() && child->nodeAtPoint(info, _x, _y, _tx, _ty, hitTestAction)) {
            setInnerNode(info);
            return true;
        }
    }
    
    // Check our bounds next.  For this purpose always assume that we can only be hit in the
    // foreground phase (which is true for replaced elements like images).
    if (hitTestAction != HitTestForeground)
        return false;
    
    QRect boundsRect(_tx, _ty, m_width, m_height);
    if (boundsRect.contains(_x, _y)) {
        setInnerNode(info);
        return true;
    }
    return false;
}

// --------------------- painting stuff -------------------------------

void RenderBox::paint(PaintInfo& i, int _tx, int _ty)
{
    _tx += m_x;
    _ty += m_y;

    // default implementation. Just pass paint through to the children
    for (RenderObject* child = firstChild(); child; child = child->nextSibling())
        child->paint(i, _tx, _ty);
}

void RenderBox::paintRootBoxDecorations(PaintInfo& i, int _tx, int _ty)
{
    //kdDebug( 6040 ) << renderName() << "::paintBoxDecorations()" << _tx << "/" << _ty << endl;
    const BackgroundLayer* bgLayer = style()->backgroundLayers();
    QColor bgColor = style()->backgroundColor();
    if (document()->isHTMLDocument() && !style()->hasBackground()) {
        // Locate the <body> element using the DOM.  This is easier than trying
        // to crawl around a render tree with potential :before/:after content and
        // anonymous blocks created by inline <body> tags etc.  We can locate the <body>
        // render object very easily via the DOM.
        HTMLElementImpl* body = document()->body();
        RenderObject* bodyObject = (body && body->id() == ID_BODY) ? body->renderer() : 0;
        if (bodyObject) {
            bgLayer = bodyObject->style()->backgroundLayers();
            bgColor = bodyObject->style()->backgroundColor();
        }
    }

    int w = width();
    int h = height();

    //    kdDebug(0) << "width = " << w <<endl;

    int rw, rh;
    if (canvas()->view()) {
        rw = canvas()->view()->contentsWidth();
        rh = canvas()->view()->contentsHeight();
    }
    else {
        rw = canvas()->width();
        rh = canvas()->height();
    }
    
    //    kdDebug(0) << "rw = " << rw <<endl;

    int bx = _tx - marginLeft();
    int by = _ty - marginTop();
    int bw = kMax(w + marginLeft() + marginRight() + borderLeft() + borderRight(), rw);
    int bh = kMax(h + marginTop() + marginBottom() + borderTop() + borderBottom(), rh);

    // CSS2 14.2:
    // " The background of the box generated by the root element covers the entire canvas."
    // hence, paint the background even in the margin areas (unlike for every other element!)
    // I just love these little inconsistencies .. :-( (Dirk)
    int my = kMax(by, i.r.y());

    paintBackgrounds(i.p, bgColor, bgLayer, my, i.r.height(), bx, by, bw, bh);

    if (style()->hasBorder() && style()->display() != INLINE)
        paintBorder( i.p, _tx, _ty, w, h, style() );
}

void RenderBox::paintBoxDecorations(PaintInfo& i, int _tx, int _ty)
{
    if (!shouldPaintWithinRoot(i))
        return;

    //kdDebug( 6040 ) << renderName() << "::paintDecorations()" << endl;
    if (isRoot())
        return paintRootBoxDecorations(i, _tx, _ty);
    
    int w = width();
    int h = height() + borderTopExtra() + borderBottomExtra();
    _ty -= borderTopExtra();

    int my = kMax(_ty, i.r.y());
    int mh;
    if (_ty < i.r.y())
        mh= kMax(0, h - (i.r.y() - _ty));
    else
        mh = kMin(i.r.height(), h);

    // The <body> only paints its background if the root element has defined a background
    // independent of the body.  Go through the DOM to get to the root element's render object,
    // since the root could be inline and wrapped in an anonymous block.
    if (!isBody() || !document()->isHTMLDocument() || document()->documentElement()->renderer()->style()->hasBackground())
        paintBackgrounds(i.p, style()->backgroundColor(), style()->backgroundLayers(), my, mh, _tx, _ty, w, h);
   
    if (style()->hasBorder())
        paintBorder(i.p, _tx, _ty, w, h, style());
}

void RenderBox::paintBackgrounds(QPainter *p, const QColor& c, const BackgroundLayer* bgLayer, int clipy, int cliph, int _tx, int _ty, int w, int height)
{
    if (!bgLayer) return;
    paintBackgrounds(p, c, bgLayer->next(), clipy, cliph, _tx, _ty, w, height);
    paintBackground(p, c, bgLayer, clipy, cliph, _tx, _ty, w, height);
}

void RenderBox::paintBackground(QPainter *p, const QColor& c, const BackgroundLayer* bgLayer, int clipy, int cliph, int _tx, int _ty, int w, int height)
{
    paintBackgroundExtended(p, c, bgLayer, clipy, cliph, _tx, _ty, w, height,
                            borderLeft(), borderRight());
}

void RenderBox::paintBackgroundExtended(QPainter *p, const QColor& c, const BackgroundLayer* bgLayer, int clipy, int cliph,
                                        int _tx, int _ty, int w, int h,
                                        int bleft, int bright)
{
    CachedImage* bg = bgLayer->backgroundImage();
    bool shouldPaintBackgroundImage = bg && bg->pixmap_size() == bg->valid_rect().size() && !bg->isTransparent() && !bg->isErrorImage();
    QColor bgColor = c;
    
    // When this style flag is set, change existing background colors and images to a solid white background.
    // If there's no bg color or image, leave it untouched to avoid affecting transparency.
    // We don't try to avoid loading the background images, because this style flag is only set
    // when printing, and at that point we've already loaded the background images anyway. (To avoid
    // loading the background images we'd have to do this check when applying styles rather than
    // while rendering.)
    if (style()->forceBackgroundsToWhite()) {
        // Note that we can't reuse this variable below because the bgColor might be changed
        bool shouldPaintBackgroundColor = !bgLayer->next() && bgColor.isValid() && qAlpha(bgColor.rgb()) > 0;
        if (shouldPaintBackgroundImage || shouldPaintBackgroundColor) {
            bgColor = Qt::white;
            shouldPaintBackgroundImage = false;
        }
    }

    // Only fill with a base color (e.g., white) if we're the root document, since iframes/frames with
    // no background in the child document should show the parent's background.
    if (!bgLayer->next() && isRoot() && !(bgColor.isValid() && qAlpha(bgColor.rgb()) > 0) && canvas()->view()) {
        bool isTransparent;
        DOM::NodeImpl* elt = document()->ownerElement();
        if (elt) {
            if (elt->id() == ID_FRAME)
                isTransparent = false;
            else {
                // Locate the <body> element using the DOM.  This is easier than trying
                // to crawl around a render tree with potential :before/:after content and
                // anonymous blocks created by inline <body> tags etc.  We can locate the <body>
                // render object very easily via the DOM.
                HTMLElementImpl* body = document()->body();
                isTransparent = !body || body->id() != ID_FRAMESET; // Can't scroll a frameset document anyway.
            }
        } else
            isTransparent = canvas()->view()->isTransparent();
        
        if (isTransparent)
            canvas()->view()->useSlowRepaints(); // The parent must show behind the child.
        else
            bgColor = canvas()->view()->palette().active().color(QColorGroup::Base);
    }

    // Paint the color first underneath all images.
    if (!bgLayer->next() && bgColor.isValid() && qAlpha(bgColor.rgb()) > 0) {
        // If we have an alpha and we are painting the root element, go ahead and blend with our default
        // background color (typically white).
        if (qAlpha(bgColor.rgb()) < 0xFF && isRoot() && !canvas()->view()->isTransparent())
            p->fillRect(_tx, clipy, w, cliph, canvas()->view()->palette().active().color(QColorGroup::Base));
        p->fillRect(_tx, clipy, w, cliph, bgColor);
    }
    
    // no progressive loading of the background image
    if (shouldPaintBackgroundImage) {
        int sx = 0;
        int sy = 0;
        int cw,ch;
        int cx,cy;
        int vpab = bleft + bright;
        int hpab = borderTop() + borderBottom();
        
        // CSS2 chapter 14.2.1

        if (bgLayer->backgroundAttachment())
        {
            //scroll
            int pw = w - vpab;
            int ph = h - hpab;
            
            int pixw = bg->pixmap_size().width();
            int pixh = bg->pixmap_size().height();
            EBackgroundRepeat bgr = bgLayer->backgroundRepeat();
            if( (bgr == NO_REPEAT || bgr == REPEAT_Y) && w > pixw ) {
                cw = pixw;
                int xPosition = bgLayer->backgroundXPosition().minWidth(pw-pixw);
                if (xPosition >= 0)
                    cx = _tx + xPosition;
                else {
                    cx = _tx;
                    if (pixw == 0)
                        sx = 0;
                    else {
                        sx = -xPosition;
                        cw += xPosition;
                    }
                }
                cx += bleft;
            } else {
                cw = w;
                cx = _tx;
                if (pixw == 0)
                    sx = 0;
                else {
                    sx =  pixw - ((bgLayer->backgroundXPosition().minWidth(pw-pixw)) % pixw );
                    sx -= bleft % pixw;
                }
            }

            if( (bgr == NO_REPEAT || bgr == REPEAT_X) && h > pixh ) {
                ch = pixh;
                int yPosition = bgLayer->backgroundYPosition().minWidth(ph-pixh);
                if (yPosition >= 0)
                    cy = _ty + yPosition;
                else {
                    cy = _ty;
                    if (pixh == 0)
                        sy = 0;
                    else {
                        sy = -yPosition;
                        ch += yPosition;
                    }
                }
                
                cy += borderTop();
            } else {
                ch = h;
                cy = _ty;
                if(pixh == 0){
                    sy = 0;
                }else{
                    sy = pixh - ((bgLayer->backgroundYPosition().minWidth(ph-pixh)) % pixh );
                    sy -= borderTop() % pixh;
                }
            }
        }
        else
        {
            //fixed
            QRect vr = viewRect();
            int pw = vr.width();
            int ph = vr.height();

            int pixw = bg->pixmap_size().width();
            int pixh = bg->pixmap_size().height();
            EBackgroundRepeat bgr = bgLayer->backgroundRepeat();
            if( (bgr == NO_REPEAT || bgr == REPEAT_Y) && pw > pixw ) {
                cw = pixw;
                cx = vr.x() + bgLayer->backgroundXPosition().minWidth(pw-pixw);
            } else {
                cw = pw;
                cx = vr.x();
                if(pixw == 0){
                    sx = 0;
                }else{
                    sx =  pixw - ((bgLayer->backgroundXPosition().minWidth(pw-pixw)) % pixw );
                }
            }

            if( (bgr == NO_REPEAT || bgr == REPEAT_X) && ph > pixh ) {
                ch = pixh;
                cy = vr.y() + bgLayer->backgroundYPosition().minWidth(ph-pixh);
            } else {
                ch = ph;
                cy = vr.y();
                if(pixh == 0){
                    sy = 0;
                }else{
                    sy = pixh - ((bgLayer->backgroundYPosition().minWidth(ph-pixh)) % pixh );
                }
            }

            QRect fix(cx,cy,cw,ch);
            QRect ele(_tx,_ty,w,h);
            QRect b = fix.intersect(ele);
            sx+=b.x()-cx;
            sy+=b.y()-cy;
            cx=b.x();cy=b.y();cw=b.width();ch=b.height();
        }


//        kdDebug() << "cx="<<cx << " cy="<<cy<< " cw="<<cw << " ch="<<ch << " sx="<<sx << " sy="<<sy << endl;

        if (cw>0 && ch>0)
            p->drawTiledPixmap(cx, cy, cw, ch, bg->tiled_pixmap(c), sx, sy);
    }
}

void RenderBox::outlineBox(QPainter *p, int _tx, int _ty, const char *color)
{
    p->setPen(QPen(QColor(color), 1, Qt::DotLine));
    p->setBrush( Qt::NoBrush );
    p->drawRect(_tx, _ty, m_width, m_height);
}

QRect RenderBox::getOverflowClipRect(int tx, int ty)
{
    // XXX When overflow-clip (CSS3) is implemented, we'll obtain the property
    // here.
    int bl=borderLeft(),bt=borderTop(),bb=borderBottom(),br=borderRight();
    int clipx = tx + bl;
    int clipy = ty + bt;
    int clipw = m_width - bl - br;
    int cliph = m_height - bt - bb + borderTopExtra() + borderBottomExtra();

    // Subtract out scrollbars if we have them.
    if (m_layer) {
        clipw -= m_layer->verticalScrollbarWidth();
        cliph -= m_layer->horizontalScrollbarHeight();
    }
    return QRect(clipx,clipy,clipw,cliph);
}

QRect RenderBox::getClipRect(int tx, int ty)
{
    int clipx = tx;
    int clipy = ty;
    int clipw = m_width;
    int cliph = m_height;

    if (!style()->clipLeft().isVariable())
    {
        int c=style()->clipLeft().width(m_width);
        clipx+=c;
        clipw-=c;
    }
        
    if (!style()->clipRight().isVariable())
    {
        int w = style()->clipRight().width(m_width);
        clipw -= m_width - w;
    }
    
    if (!style()->clipTop().isVariable())
    {
        int c=style()->clipTop().width(m_height);
        clipy+=c;
        cliph-=c;
    }
    if (!style()->clipBottom().isVariable())
    {
        int h = style()->clipBottom().width(m_height);
        cliph -= m_height - h;
    }
    //kdDebug( 6040 ) << "setting clip("<<clipx<<","<<clipy<<","<<clipw<<","<<cliph<<")"<<endl;

    QRect cr(clipx,clipy,clipw,cliph);
    return cr;
}

int RenderBox::containingBlockWidth() const
{
    RenderBlock* cb = containingBlock();
    if (!cb)
        return 0;
    if (usesLineWidth())
        return cb->lineWidth(m_y);
    else
        return cb->contentWidth();
}

bool RenderBox::absolutePosition(int &xPos, int &yPos, bool f)
{
    if (style()->position() == FIXED)
	f = true;
    RenderObject *o = container();
    if (o && o->absolutePosition(xPos, yPos, f)) {
        if (o->hasOverflowClip())
            o->layer()->subtractScrollOffset(xPos, yPos); 
            
        if (!isInline() || isReplaced())
            xPos += m_x, yPos += m_y;

        if (isRelPositioned())
            relativePositionOffset(xPos, yPos);

        return true;
    }
    else {
        xPos = yPos = 0;
        return false;
    }
}

void RenderBox::dirtyLineBoxes(bool fullLayout, bool)
{
    if (m_inlineBoxWrapper) {
        if (fullLayout) {
            m_inlineBoxWrapper->detach(renderArena());
            m_inlineBoxWrapper = 0;
        }
        else
            m_inlineBoxWrapper->dirtyLineBoxes();
    }
}

void RenderBox::position(InlineBox* box, int from, int len, bool reverse)
{
    if (isPositioned()) {
        // Cache the x position only if we were an INLINE type originally.
        bool wasInline = style()->isOriginalDisplayInlineType();
        if (wasInline && hasStaticX()) {
            // The value is cached in the xPos of the box.  We only need this value if
            // our object was inline originally, since otherwise it would have ended up underneath
            // the inlines.
            m_staticX = box->xPos();
        }
        else if (!wasInline && hasStaticY())
            // Our object was a block originally, so we make our normal flow position be
            // just below the line box (as though all the inlines that came before us got
            // wrapped in an anonymous block, which is what would have happened had we been
            // in flow).  This value was cached in the yPos() of the box.
            m_staticY = box->yPos();

        // Nuke the box.
        box->remove();
        box->detach(renderArena());
    }
    else if (isReplaced()) {
        m_x = box->xPos();
        m_y = box->yPos();
        m_inlineBoxWrapper = box;
    }
}

// For inline replaced elements, this function returns the inline box that owns us.  Enables
// the replaced RenderObject to quickly determine what line it is contained on and to easily
// iterate over structures on the line.
InlineBox* RenderBox::inlineBoxWrapper() const
{
    return m_inlineBoxWrapper;
}

void RenderBox::deleteLineBoxWrapper()
{
    if (m_inlineBoxWrapper) {
        if (!documentBeingDestroyed())
            m_inlineBoxWrapper->remove();
        m_inlineBoxWrapper->detach(renderArena());
        m_inlineBoxWrapper = 0;
    }
}

void RenderBox::setInlineBoxWrapper(InlineBox* b)
{
    m_inlineBoxWrapper = b;
}

QRect RenderBox::getAbsoluteRepaintRect()
{
    int ow = style() ? style()->outlineSize() : 0;
    QRect r(-ow, -ow, overflowWidth(false)+ow*2, overflowHeight(false)+ow*2);
    computeAbsoluteRepaintRect(r);
    return r;
}

void RenderBox::computeAbsoluteRepaintRect(QRect& r, bool f)
{
    int x = r.x() + m_x;
    int y = r.y() + m_y;
     
    // Apply the relative position offset when invalidating a rectangle.  The layer
    // is translated, but the render box isn't, so we need to do this to get the
    // right dirty rect.  Since this is called from RenderObject::setStyle, the relative position
    // flag on the RenderObject has been cleared, so use the one on the style().
    if (style()->position() == RELATIVE && m_layer)
        m_layer->relativePositionOffset(x,y);
    
    if (style()->position()==FIXED)
        f = true;

    RenderObject* o = container();
    if (o) {
        // <body> may not have overflow, since it might be applying its overflow value to the
        // scrollbars.
        if (o->hasOverflowClip()) {
            // o->height() is inaccurate if we're in the middle of a layout of |o|, so use the
            // layer's size instead.  Even if the layer's size is wrong, the layer itself will repaint
            // anyway if its size does change.
            QRect boxRect(0, 0, o->layer()->width(), o->layer()->height());
            o->layer()->subtractScrollOffset(x,y); // For overflow:auto/scroll/hidden.
            QRect repaintRect(x, y, r.width(), r.height());
            r = repaintRect.intersect(boxRect);
            if (r.isEmpty())
                return;
        }
        else {
            r.setX(x);
            r.setY(y);
        }
        o->computeAbsoluteRepaintRect(r, f);
    }
}

void RenderBox::repaintDuringLayoutIfMoved(int oldX, int oldY)
{
    int newX = m_x;
    int newY = m_y;
    if (oldX != newX || oldY != newY) {
        // The child moved.  Invalidate the object's old and new positions.  We have to do this
        // since the object may not have gotten a layout.
        m_x = oldX; m_y = oldY;
        repaint();
        repaintFloatingDescendants();
        m_x = newX; m_y = newY;
        repaint();
        repaintFloatingDescendants();
    }
}

void RenderBox::relativePositionOffset(int &tx, int &ty)
{
    if(!style()->left().isVariable())
        tx += style()->left().width(containingBlockWidth());
    else if(!style()->right().isVariable())
        tx -= style()->right().width(containingBlockWidth());
    if(!style()->top().isVariable())
    {
        if (!style()->top().isPercent()
                || containingBlock()->style()->height().isFixed())
            ty += style()->top().width(containingBlockHeight());
    }
    else if(!style()->bottom().isVariable())
    {
        if (!style()->bottom().isPercent()
                || containingBlock()->style()->height().isFixed())
            ty -= style()->bottom().width(containingBlockHeight());
    }
}

void RenderBox::calcWidth()
{
#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << "RenderBox("<<renderName()<<")::calcWidth()" << endl;
#endif
    if (isPositioned())
    {
        calcAbsoluteHorizontal();
    }
    else
    {
        // The parent box is flexing us, so it has increased or decreased our width.  Use the width
        // from the style context.
        if (m_overrideSize != -1 && parent()->isFlexibleBox() && parent()->style()->boxOrient() == HORIZONTAL
            && parent()->isFlexingChildren()) {
            m_width = m_overrideSize;
            return;
        }

        bool inVerticalBox = parent()->isFlexibleBox() && parent()->style()->boxOrient() == VERTICAL;
        bool stretching = parent()->style()->boxAlign() == BSTRETCH;
        bool treatAsReplaced = isReplaced() && !isInlineBlockOrInlineTable() &&
            (!inVerticalBox || !stretching);
        Length w;
        if (treatAsReplaced)
            w = Length( calcReplacedWidth(), Fixed );
        else
            w = style()->width();

        Length ml = style()->marginLeft();
        Length mr = style()->marginRight();

        RenderBlock *cb = containingBlock();
        int cw = containingBlockWidth();

        if (cw<0) cw = 0;

        m_marginLeft = 0;
        m_marginRight = 0;

        if (isInline() && !isInlineBlockOrInlineTable())
        {
            // just calculate margins
            m_marginLeft = ml.minWidth(cw);
            m_marginRight = mr.minWidth(cw);
            if (treatAsReplaced)
            {
                m_width = w.width(cw);
                m_width += paddingLeft() + paddingRight() + borderLeft() + borderRight();
                if(m_width < m_minWidth) m_width = m_minWidth;
            }

            return;
        }
        else {
            LengthType widthType, minWidthType, maxWidthType;
            if (treatAsReplaced) {
                m_width = w.width(cw);
                m_width += paddingLeft() + paddingRight() + borderLeft() + borderRight();
                widthType = w.type;
            } else {
                m_width = calcWidthUsing(Width, cw, widthType);
                int minW = calcWidthUsing(MinWidth, cw, minWidthType);
                int maxW = style()->maxWidth().value == UNDEFINED ?
                             m_width : calcWidthUsing(MaxWidth, cw, maxWidthType);
                
                if (m_width > maxW) {
                    m_width = maxW;
                    widthType = maxWidthType;
                }
                if (m_width < minW) {
                    m_width = minW;
                    widthType = minWidthType;
                }
            }
            
            if (widthType == Variable) {
    //          kdDebug( 6040 ) << "variable" << endl;
                m_marginLeft = ml.minWidth(cw);
                m_marginRight = mr.minWidth(cw);
            }
            else
            {
//          	kdDebug( 6040 ) << "non-variable " << w.type << ","<< w.value << endl;
                calcHorizontalMargins(ml,mr,cw);
            }
        }

        if (cw && cw != m_width + m_marginLeft + m_marginRight && !isFloating() && !isInline() &&
            !cb->isFlexibleBox())
        {
            if (cb->style()->direction()==LTR)
                m_marginRight = cw - m_width - m_marginLeft;
            else
                m_marginLeft = cw - m_width - m_marginRight;
        }
    }

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << "RenderBox::calcWidth(): m_width=" << m_width << " containingBlockWidth()=" << containingBlockWidth() << endl;
    kdDebug( 6040 ) << "m_marginLeft=" << m_marginLeft << " m_marginRight=" << m_marginRight << endl;
#endif
}

int RenderBox::calcWidthUsing(WidthType widthType, int cw, LengthType& lengthType)
{
    int width = m_width;
    Length w;
    if (widthType == Width)
        w = style()->width();
    else if (widthType == MinWidth)
        w = style()->minWidth();
    else
        w = style()->maxWidth();
        
    lengthType = w.type;
    
    if (lengthType == Variable) {
        int marginLeft = style()->marginLeft().minWidth(cw);
        int marginRight = style()->marginRight().minWidth(cw);
        if (cw) width = cw - marginLeft - marginRight;
        
        if (sizesToMaxWidth()) {
            if (width < m_minWidth) 
                width = m_minWidth;
            if (width > m_maxWidth) 
                width = m_maxWidth;
        }
    }
    else
    {
        width = w.width(cw);
        width += paddingLeft() + paddingRight() + borderLeft() + borderRight();
    }
    
    return width;
}

void RenderBox::calcHorizontalMargins(const Length& ml, const Length& mr, int cw)
{
    if (isFloating() || isInline()) // Inline blocks/tables and floats don't have their margins increased.
    {
        m_marginLeft = ml.minWidth(cw);
        m_marginRight = mr.minWidth(cw);
    }
    else
    {
        if ( (ml.type == Variable && mr.type == Variable) ||
             (ml.type != Variable && mr.type != Variable &&
                containingBlock()->style()->textAlign() == KHTML_CENTER) )
        {
            m_marginLeft = (cw - m_width)/2;
            if (m_marginLeft<0) m_marginLeft=0;
            m_marginRight = cw - m_width - m_marginLeft;
        }
        else if (mr.type == Variable ||
                 (ml.type != Variable && containingBlock()->style()->direction() == RTL &&
                  containingBlock()->style()->textAlign() == KHTML_LEFT))
        {
            m_marginLeft = ml.width(cw);
            m_marginRight = cw - m_width - m_marginLeft;
        }
        else if (ml.type == Variable ||
                 (mr.type != Variable && containingBlock()->style()->direction() == LTR &&
                  containingBlock()->style()->textAlign() == KHTML_RIGHT))
        {
            m_marginRight = mr.width(cw);
            m_marginLeft = cw - m_width - m_marginRight;
        }
        else
        {
            m_marginLeft = ml.minWidth(cw);
            m_marginRight = mr.minWidth(cw);
        }
    }
}

void RenderBox::calcHeight()
{

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << "RenderBox::calcHeight()" << endl;
#endif

    // Cell height is managed by the table and inline non-replaced elements do not support a height property.
    if (isTableCell() || (isInline() && !isReplaced()))
        return;

    if (isPositioned())
        calcAbsoluteVertical();
    else
    {
        calcVerticalMargins();
        
        // For tables, calculate margins only
        if (isTable())
            return;
        
        Length h;
        bool inHorizontalBox = parent()->isFlexibleBox() && parent()->style()->boxOrient() == HORIZONTAL;
        bool stretching = parent()->style()->boxAlign() == BSTRETCH;
        bool treatAsReplaced = isReplaced() && !isInlineBlockOrInlineTable() && (!inHorizontalBox || !stretching);
        bool checkMinMaxHeight = false;
        
        // The parent box is flexing us, so it has increased or decreased our height.  We have to
        // grab our cached flexible height.
        if (m_overrideSize != -1 && parent()->isFlexibleBox() && parent()->style()->boxOrient() == VERTICAL
            && parent()->isFlexingChildren())
            h = Length(m_overrideSize - borderTop() - borderBottom() - paddingTop() - paddingBottom(), Fixed);
        else if (treatAsReplaced)
            h = Length(calcReplacedHeight(), Fixed);
        else {
            h = style()->height();
            checkMinMaxHeight = true;
        }
        
        // Block children of horizontal flexible boxes fill the height of the box.
        if (h.isVariable() && parent()->isFlexibleBox() && parent()->style()->boxOrient() == HORIZONTAL
            && parent()->isStretchingChildren()) {
            h = Length(parent()->contentHeight() - marginTop() - marginBottom() -
                       borderTop() - paddingTop() - borderBottom() - paddingBottom(), Fixed);
            checkMinMaxHeight = false;
        }

        int height;
        if (checkMinMaxHeight) {
            height = calcHeightUsing(style()->height());
            if (height == -1)
                height = m_height;
            int minH = calcHeightUsing(style()->minHeight()); // Leave as -1 if unset.
            int maxH = style()->maxHeight().value == UNDEFINED ? height : calcHeightUsing(style()->maxHeight());
            if (maxH == -1)
                maxH = height;
            height = kMin(maxH, height);
            height = kMax(minH, height);
        }
        else
            // The only times we don't check min/max height are when a fixed length has 
            // been given as an override.  Just use that.
            height = h.value + borderTop() + paddingTop() + borderBottom() + paddingBottom(); 
        m_height = height;
    }
    
    // Unfurling marquees override with the furled height.
    if (style()->overflow() == OMARQUEE && m_layer && m_layer->marquee() && 
        m_layer->marquee()->isUnfurlMarquee() && !m_layer->marquee()->isHorizontal()) {
        m_layer->marquee()->setEnd(m_height);
        m_height = kMin(m_height, m_layer->marquee()->unfurlPos());
    }
    
    // WinIE quirk: The <html> block always fills the entire canvas in quirks mode.  The <body> always fills the
    // <html> block in quirks mode.  Only apply this quirk if the block is normal flow and no height
    // is specified.
    if (style()->htmlHacks() && style()->height().isVariable() &&
        !isFloatingOrPositioned() && (isRoot() || isBody())) {
        int margins = collapsedMarginTop() + collapsedMarginBottom();
        int visHeight = canvas()->view()->visibleHeight();
        if (isRoot())
            m_height = kMax(m_height, visHeight - margins);
        else
            m_height = kMax(m_height, visHeight - 
                            (margins + parent()->marginTop() + parent()->marginBottom() + 
                             parent()->borderTop() + parent()->borderBottom() +
                             parent()->paddingTop() + parent()->paddingBottom()));
    }
}

int RenderBox::calcHeightUsing(const Length& h)
{
    int height = -1;
    if (!h.isVariable()) {
        if (h.isFixed())
            height = h.value;
        else if (h.isPercent())
            height = calcPercentageHeight(h);
        if (height != -1) {
            height += borderTop() + paddingTop() + borderBottom() + paddingBottom();
            return height;
        }
    }
    return height;
}

int RenderBox::calcPercentageHeight(const Length& height)
{
    int result = -1;
    bool includeBorderPadding = isTable();
    RenderBlock* cb = containingBlock();
    if (style()->htmlHacks()) {
        // In quirks mode, blocks with auto height are skipped, and we keep looking for an enclosing
        // block that may have a specified height and then use it.  In strict mode, this violates the
        // specification, which states that percentage heights just revert to auto if the containing
        // block has an auto height.
        for ( ; !cb->isCanvas() && !cb->isBody() && !cb->isTableCell() && !cb->isPositioned() &&
                cb->style()->height().isVariable(); cb = cb->containingBlock());
    }

    // Table cells violate what the CSS spec says to do with heights.  Basically we
    // don't care if the cell specified a height or not.  We just always make ourselves
    // be a percentage of the cell's current content height.
    if (cb->isTableCell()) {
        result = cb->overrideSize();
        if (result == -1) {
            // Normally we would let the cell size intrinsically, but scrolling overflow has to be
            // treated differently, since WinIE lets scrolled overflow regions shrink as needed.
            // While we can't get all cases right, we can at least detect when the cell has a specified
            // height or when the table has a specified height.  In these cases we want to initially have
            // no size and allow the flexing of the table or the cell to its specified height to cause us
            // to grow to fill the space.  This could end up being wrong in some cases, but it is
            // preferable to the alternative (sizing intrinsically and making the row end up too big).
            RenderTableCell* cell = static_cast<RenderTableCell*>(cb);
            if (scrollsOverflow() && 
                (!cell->style()->height().isVariable() || !cell->table()->style()->height().isVariable()))
                return 0;
            return -1;
        }
        includeBorderPadding = true;
    }

    // Otherwise we only use our percentage height if our containing block had a specified
    // height.
    else if (cb->style()->height().isFixed())
        result = cb->style()->height().value;
    else if (cb->style()->height().isPercent())
        // We need to recur and compute the percentage height for our containing block.
        result = cb->calcPercentageHeight(cb->style()->height());
    else if (cb->isCanvas() || (cb->isBody() && style()->htmlHacks())) {
        // Don't allow this to affect the block' m_height member variable, since this
        // can get called while the block is still laying out its kids.
        int oldHeight = cb->height();
        cb->calcHeight();
        result = cb->contentHeight();
        cb->setHeight(oldHeight);
    }
    if (result != -1) {
        result = height.width(result);
        if (includeBorderPadding) {
            // It is necessary to use the border-box to match WinIE's broken
            // box model.  This is essential for sizing inside
            // table cells using percentage heights.
            result -= (borderTop() + paddingTop() + borderBottom() + paddingBottom());
            result = kMax(0, result);
        }
    }
    return result;
}

int RenderBox::calcReplacedWidth() const
{
    int width = calcReplacedWidthUsing(Width);
    int minW = calcReplacedWidthUsing(MinWidth);
    int maxW = style()->maxWidth().value == UNDEFINED ? width : calcReplacedWidthUsing(MaxWidth);

    if (width > maxW)
        width = maxW;
    
    if (width < minW)
        width = minW;

    return width;
}

int RenderBox::calcReplacedWidthUsing(WidthType widthType) const
{
    Length w;
    if (widthType == Width)
        w = style()->width();
    else if (widthType == MinWidth)
        w = style()->minWidth();
    else
        w = style()->maxWidth();
    
    switch (w.type) {
    case Fixed:
        return w.value;
    case Percent:
    {
        const int cw = containingBlockWidth();
        if (cw > 0) {
            int result = w.minWidth(cw);
            return result;
        }
    }
    // fall through
    default:
        return intrinsicWidth();
    }
}

int RenderBox::calcReplacedHeight() const
{
    int height = calcReplacedHeightUsing(Height);
    int minH = calcReplacedHeightUsing(MinHeight);
    int maxH = style()->maxHeight().value == UNDEFINED ? height : calcReplacedHeightUsing(MaxHeight);

    if (height > maxH)
        height = maxH;

    if (height < minH)
        height = minH;

    return height;
}

int RenderBox::calcReplacedHeightUsing(HeightType heightType) const
{
    Length h;
    if (heightType == Height)
        h = style()->height();
    else if (heightType == MinHeight)
        h = style()->minHeight();
    else
        h = style()->maxHeight();
    switch( h.type ) {
    case Percent:
        return availableHeightUsing(h);
    case Fixed:
        return h.value;
    default:
        return intrinsicHeight();
    };
}

int RenderBox::availableHeight() const
{
    return availableHeightUsing(style()->height());
}

int RenderBox::availableHeightUsing(const Length& h) const
{
    if (h.isFixed())
        return h.value;

    if (isCanvas())
        return static_cast<const RenderCanvas*>(this)->viewportHeight();

    // We need to stop here, since we don't want to increase the height of the table
    // artificially.  We're going to rely on this cell getting expanded to some new
    // height, and then when we lay out again we'll use the calculation below.
    if (isTableCell() && (h.isVariable() || h.isPercent())) {
        return overrideSize() - (borderLeft()+borderRight()+paddingLeft()+paddingRight());
    }
    
    if (h.isPercent())
       return h.width(containingBlock()->availableHeight());
       
    return containingBlock()->availableHeight();
}

void RenderBox::calcVerticalMargins()
{
    if( isTableCell() ) {
	// table margins are basically infinite
	m_marginTop = TABLECELLMARGIN;
	m_marginBottom = TABLECELLMARGIN;
	return;
    }

    Length tm = style()->marginTop();
    Length bm = style()->marginBottom();

    // margins are calculated with respect to the _width_ of
    // the containing block (8.3)
    int cw = containingBlock()->contentWidth();

    m_marginTop = tm.minWidth(cw);
    m_marginBottom = bm.minWidth(cw);
}

void RenderBox::setStaticX(int staticX)
{
    m_staticX = staticX;
}

void RenderBox::setStaticY(int staticY)
{
    m_staticY = staticY;
}

void RenderBox::calcAbsoluteHorizontal()
{
    const int AUTO = -666666;
    int l, r, cw;

    int pab = borderLeft()+ borderRight()+ paddingLeft()+ paddingRight();

    l = r = AUTO;
 
    // We don't use containingBlock(), since we may be positioned by an enclosing relpositioned inline.
    RenderObject* cb = container();
    cw = containingBlockWidth() + cb->paddingLeft() + cb->paddingRight();

    if (!style()->left().isVariable())
        l = style()->left().width(cw);
    if (!style()->right().isVariable())
        r = style()->right().width(cw);

    int static_distance=0;
    if ((parent()->style()->direction()==LTR && (l == AUTO && r == AUTO))
            || style()->left().isStatic())
    {
        static_distance = m_staticX - cb->borderLeft(); // Should already have been set through layout of the parent().
        RenderObject* po = parent();
        for (; po && po != cb; po = po->parent())
            static_distance += po->xPos();

        if (l == AUTO || style()->left().isStatic())
            l = static_distance;
    }

    else if ((parent()->style()->direction()==RTL && (l==AUTO && r==AUTO ))
            || style()->right().isStatic())
    {
        RenderObject* po = parent();
        static_distance = m_staticX - cb->borderLeft(); // Should already have been set through layout of the parent().
        while (po && po!=containingBlock()) {
            static_distance+=po->xPos();
            po=po->parent();
        }

        if (r==AUTO || style()->right().isStatic())
            r = static_distance;
    }

    calcAbsoluteHorizontalValues(Width, cb, cw, pab, static_distance, l, r, m_width, m_marginLeft, m_marginRight, m_x);

    // Avoid doing any work in the common case (where the values of min-width and max-width are their defaults).
    int minW = m_width, minML, minMR, minX;
    calcAbsoluteHorizontalValues(MinWidth, cb, cw, pab, static_distance, l, r, minW, minML, minMR, minX);

    int maxW = m_width, maxML, maxMR, maxX;
    if (style()->maxWidth().value != UNDEFINED)
        calcAbsoluteHorizontalValues(MaxWidth, cb, cw, static_distance, pab, l, r, maxW, maxML, maxMR, maxX);

    if (m_width > maxW) {
        m_width = maxW;
        m_marginLeft = maxML;
        m_marginRight = maxMR;
        m_x = maxX;
    }
    
    if (m_width < minW) {
        m_width = minW;
        m_marginLeft = minML;
        m_marginRight = minMR;
        m_x = minX;
    }
}

void RenderBox::calcAbsoluteHorizontalValues(WidthType widthType, RenderObject* cb, int cw, int pab, int static_distance,
                                             int l, int r, int& w, int& ml, int& mr, int& x)
{
    const int AUTO = -666666;
    w = ml = mr = AUTO;

    if (!style()->marginLeft().isVariable())
        ml = style()->marginLeft().width(cw);
    if (!style()->marginRight().isVariable())
        mr = style()->marginRight().width(cw);

    Length width;
    if (widthType == Width)
        width = style()->width();
    else if (widthType == MinWidth)
        width = style()->minWidth();
    else
        width = style()->maxWidth();

    if (!width.isVariable())
        w = width.width(cw);
    else if (isReplaced())
        w = intrinsicWidth();

    if (l != AUTO && w != AUTO && r != AUTO) {
        // left, width, right all given, play with margins
        int ot = l + w + r + pab;

        if (ml==AUTO && mr==AUTO) {
            // both margins auto, solve for equality
            ml = (cw - ot)/2;
            mr = cw - ot - ml;
        }
        else if (ml==AUTO)
            // solve for left margin
            ml = cw - ot - mr;
        else if (mr==AUTO)
            // solve for right margin
            mr = cw - ot - ml;
        else {
            // overconstrained, solve according to dir
            if (style()->direction() == LTR)
                r = cw - ( l + w + ml + mr + pab);
            else
                l = cw - ( r + w + ml + mr + pab);
        }
    }
    else
    {
        // one or two of (left, width, right) missing, solve

        // auto margins are ignored
        if (ml==AUTO) ml = 0;
        if (mr==AUTO) mr = 0;

        //1. solve left & width.
        if (l == AUTO && w == AUTO && r != AUTO) {
            // From section 10.3.7 of the CSS2.1 specification.
            // "The shrink-to-fit width is: min(max(preferred minimum width, available width), preferred width)."
            w = kMin(kMax(m_minWidth - pab, cw - (r + ml + mr + pab)), m_maxWidth - pab);
            l = cw - (r + w + ml + mr + pab);
        }
        else

        //2. solve left & right. use static positioning.
        if (l == AUTO && w != AUTO && r == AUTO) {
            if (style()->direction()==RTL) {
                r = static_distance;
                l = cw - (r + w + ml + mr + pab);
            }
            else {
                l = static_distance;
                r = cw - (l + w + ml + mr + pab);
            }

        } //3. solve width & right.
        else if (l != AUTO && w == AUTO && r == AUTO) {
            // From section 10.3.7 of the CSS2.1 specification.
            // "The shrink-to-fit width is: min(max(preferred minimum width, available width), preferred width)."
            w = kMin(kMax(m_minWidth - pab, cw - (l + ml + mr + pab)), m_maxWidth - pab);
            r = cw - (l + w + ml + mr + pab);
        }
        else

        //4. solve left
        if (l==AUTO && w!=AUTO && r!=AUTO)
            l = cw - (r + w + ml + mr + pab);
        else
        //5. solve width
        if (l!=AUTO && w==AUTO && r!=AUTO)
            w = cw - (r + l + ml + mr + pab);
        else

        //6. solve right
        if (l!=AUTO && w!=AUTO && r==AUTO)
            r = cw - (l + w + ml + mr + pab);
    }

    w += pab;
    x = l + ml + cb->borderLeft();
}

void RenderBox::calcAbsoluteVertical()
{
    // css2 spec 10.6.4 & 10.6.5

    // based on
    // http://www.w3.org/Style/css2-updates/REC-CSS2-19980512-errata
    // (actually updated 2000-10-24)
    // that introduces static-position value for top, left & right

    const int AUTO = -666666;
    int t, b, ch;

    t = b = AUTO;

    int pab = borderTop()+borderBottom()+paddingTop()+paddingBottom();

    // We don't use containingBlock(), since we may be positioned by an enclosing relpositioned inline.
    RenderObject* cb = container();
    if (cb->isRoot()) // Even in strict mode (where we don't grow the root to fill the viewport) other browsers
                      // position as though the root fills the viewport.
        ch = cb->availableHeight();
    else
        ch = cb->height() - cb->borderTop() - cb->borderBottom();

    if (!style()->top().isVariable())
        t = style()->top().width(ch);
    if (!style()->bottom().isVariable())
        b = style()->bottom().width(ch);

    int h, mt, mb, y;
    calcAbsoluteVerticalValues(Height, cb, ch, pab, t, b, h, mt, mb, y);

    // Avoid doing any work in the common case (where the values of min-height and max-height are their defaults).
    int minH = h, minMT, minMB, minY;
    calcAbsoluteVerticalValues(MinHeight, cb, ch, pab, t, b, minH, minMT, minMB, minY);

    int maxH = h, maxMT, maxMB, maxY;
    if (style()->maxHeight().value != UNDEFINED)
        calcAbsoluteVerticalValues(MaxHeight, cb, ch, pab, t, b, maxH, maxMT, maxMB, maxY);

    if (h > maxH) {
        h = maxH;
        mt = maxMT;
        mb = maxMB;
        y = maxY;
    }
    
    if (h < minH) {
        h = minH;
        mt = minMT;
        mb = minMB;
        y = minY;
    }
    
    // If our natural height exceeds the new height once we've set it, then we need to make sure to update
    // overflow to track the spillout.
    if (m_height > h)
        setOverflowHeight(m_height);
        
    // Set our final values.
    m_height = h;
    m_marginTop = mt;
    m_marginBottom = mb;
    m_y = y;
}

void RenderBox::calcAbsoluteVerticalValues(HeightType heightType, RenderObject* cb, int ch, int pab, 
                                           int t, int b, int& h, int& mt, int& mb, int& y)
{
    const int AUTO = -666666;
    h = mt = mb = AUTO;

    if (!style()->marginTop().isVariable())
        mt = style()->marginTop().width(ch);
    if (!style()->marginBottom().isVariable())
        mb = style()->marginBottom().width(ch);

    Length height;
    if (heightType == Height)
        height = style()->height();
    else if (heightType == MinHeight)
        height = style()->minHeight();
    else
        height = style()->maxHeight();

    int ourHeight = m_height;

    if (isTable() && height.isVariable())
        // Height is never unsolved for tables. "auto" means shrink to fit.  Use our
        // height instead.
        h = ourHeight - pab;
    else if (!height.isVariable())
    {
        h = height.width(ch);
        if (ourHeight - pab > h)
            ourHeight = h + pab;
    }
    else if (isReplaced())
        h = intrinsicHeight();

    int static_top=0;
    if ((t == AUTO && b == AUTO) || style()->top().isStatic()) {
        // calc hypothetical location in the normal flow
        // used for 1) top=static-position
        //          2) top, bottom, height are all auto -> calc top -> 3.
        //          3) precalc for case 2 below
        static_top = m_staticY - cb->borderTop(); // Should already have been set through layout of the parent().
        RenderObject* po = parent();
        for (; po && po != cb; po = po->parent())
            static_top += po->yPos();

        if (h == AUTO || style()->top().isStatic())
            t = static_top;
    }

    if (t != AUTO && h != AUTO && b != AUTO) {
        // top, height, bottom all given, play with margins
        int ot = h + t + b + pab;

        if (mt == AUTO && mb == AUTO) {
            // both margins auto, solve for equality
            mt = (ch - ot)/2;
            mb = ch - ot - mt;
        }
        else if (mt==AUTO)
            // solve for top margin
            mt = ch - ot - mb;
        else if (mb==AUTO)
            // solve for bottom margin
            mb = ch - ot - mt;
        else
            // overconstrained, solve for bottom
            b = ch - (h + t + mt + mb + pab);
    }
    else {
        // one or two of (top, height, bottom) missing, solve

        // auto margins are ignored
        if (mt == AUTO)
            mt = 0;
        if (mb == AUTO)
            mb = 0;

        //1. solve top & height. use content height.
        if (t == AUTO && h == AUTO && b != AUTO) {
            h = ourHeight - pab;
            t = ch - (h + b + mt + mb + pab);
        }
        else if (t == AUTO && h != AUTO && b == AUTO) //2. solve top & bottom. use static positioning.
        {
            t = static_top;
            b = ch - (h + t + mt + mb + pab);
        }
        else if (t != AUTO && h == AUTO && b == AUTO) //3. solve height & bottom. use content height.
        {
            h = ourHeight - pab;
            b = ch - (h + t + mt + mb + pab);
        }
        else
        //4. solve top
        if (t == AUTO && h != AUTO && b != AUTO)
            t = ch - (h + b + mt + mb + pab);
        else

        //5. solve height
        if (t != AUTO && h == AUTO && b != AUTO)
            h = ch - (t + b + mt + mb + pab);
        else

        //6. solve bottom
        if (t != AUTO && h != AUTO && b == AUTO)
            b = ch - (h + t + mt + mb + pab);
    }

    if (ourHeight < h + pab) //content must still fit
        ourHeight = h + pab;

    if (hasOverflowClip() && ourHeight > h + pab)
        ourHeight = h + pab;
    
    // Do not allow the height to be negative.  This can happen when someone specifies both top and bottom
    // but the containing block height is less than top, e.g., top:20px, bottom:0, containing block height 16.
    ourHeight = kMax(0, ourHeight);
    
    h = ourHeight;
    y = t + mt + cb->borderTop();
}

QRect RenderBox::caretRect(int offset, EAffinity affinity, int *extraWidthToEndOfLine)
{
    // FIXME: Is it OK to check only first child instead of picking
    // right child based on offset? Is it OK to pass the same offset
    // along to the child instead of offset 0 or whatever?

    // propagate it downwards to its children, someone will feel responsible
    RenderObject *child = firstChild();
    if (child) {
        QRect result = child->caretRect(offset, affinity, extraWidthToEndOfLine);
        // FIXME: in-band signalling!
        if (result.isEmpty())
            return result;
    }
    
    int _x, _y, height;

    // if not, use the extents of this box 
    // offset 0 means left, offset 1 means right
    _x = xPos() + (offset == 0 ? 0 : m_width);
    InlineBox *box = inlineBoxWrapper();
    if (box) {
        height = box->root()->bottomOverflow() - box->root()->topOverflow();
        _y = box->root()->topOverflow();
    }
    else {
        _y = yPos();
        height = m_height;
    }
    // If height of box is smaller than font height, use the latter one,
    // otherwise the caret might become invisible.
    // 
    // Also, if the box is not a replaced element, always use the font height.
    // This prevents the "big caret" bug described in:
    // <rdar://problem/3777804> Deleting all content in a document can result in giant tall-as-window insertion point
    //
    // FIXME: ignoring :first-line, missing good reason to take care of
    int fontHeight = style()->fontMetrics().height();
    if (fontHeight > height || !isReplaced())
        height = fontHeight;
    
    int absx, absy;
    RenderObject *cb = containingBlock();
    if (cb && cb != this && cb->absolutePosition(absx,absy)) {
        _x += absx;
        _y += absy;
    } 
    else {
        // we don't know our absolute position, and there is no point returning
        // just a relative one
        return QRect();
    }

    if (extraWidthToEndOfLine)
        *extraWidthToEndOfLine = m_width - _x;

    return QRect(_x, _y, 1, height);
}

int RenderBox::lowestPosition(bool includeOverflowInterior, bool includeSelf) const
{
    return includeSelf ? m_height : 0;
}

int RenderBox::rightmostPosition(bool includeOverflowInterior, bool includeSelf) const
{
    return includeSelf ? m_width : 0;
}

int RenderBox::leftmostPosition(bool includeOverflowInterior, bool includeSelf) const
{
    return includeSelf ? 0 : m_width;
}

#undef DEBUG_LAYOUT
