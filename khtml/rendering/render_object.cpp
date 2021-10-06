/**
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
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

#include "rendering/render_object.h"
#include "rendering/render_table.h"
#include "rendering/render_text.h"
#include "rendering/render_line.h"
#include "rendering/render_list.h"
#include "rendering/render_canvas.h"
#include "xml/dom_elementimpl.h"
#include "xml/dom2_eventsimpl.h"
#include "xml/dom_docimpl.h"
#include "xml/dom_position.h"
#include "css/cssstyleselector.h"
#include "misc/htmlhashes.h"
#include <kdebug.h>
#include <qpainter.h>
#include "khtmlview.h"
#include "khtml_part.h"
#include "render_arena.h"
#include "render_inline.h"
#include "render_block.h"
#include "render_flexbox.h"

#if APPLE_CHANGES
// For accessibility
#include "KWQAccObjectCache.h" 
#endif

#include <assert.h>
using namespace DOM;
using namespace khtml;

#ifndef NDEBUG
static void *baseOfRenderObjectBeingDeleted;
#endif

void* RenderObject::operator new(size_t sz, RenderArena* renderArena) throw()
{
    return renderArena->allocate(sz);
}

void RenderObject::operator delete(void* ptr, size_t sz)
{
    assert(baseOfRenderObjectBeingDeleted == ptr);
    
    // Stash size where detach can find it.
    *(size_t *)ptr = sz;
}

RenderObject *RenderObject::createObject(DOM::NodeImpl* node,  RenderStyle* style)
{
    RenderObject *o = 0;
    RenderArena* arena = node->getDocument()->renderArena();
    switch(style->display())
    {
    case NONE:
        break;
    case INLINE:
        o = new (arena) RenderInline(node);
        break;
    case BLOCK:
        o = new (arena) RenderBlock(node);
        break;
    case INLINE_BLOCK:
        o = new (arena) RenderBlock(node);
        break;
    case LIST_ITEM:
        o = new (arena) RenderListItem(node);
        break;
    case RUN_IN:
    case COMPACT:
        o = new (arena) RenderBlock(node);
        break;
    case TABLE:
    case INLINE_TABLE:
        //kdDebug( 6040 ) << "creating RenderTable" << endl;
        o = new (arena) RenderTable(node);
        break;
    case TABLE_ROW_GROUP:
    case TABLE_HEADER_GROUP:
    case TABLE_FOOTER_GROUP:
        o = new (arena) RenderTableSection(node);
        break;
    case TABLE_ROW:
        o = new (arena) RenderTableRow(node);
        break;
    case TABLE_COLUMN_GROUP:
    case TABLE_COLUMN:
        o = new (arena) RenderTableCol(node);
        break;
    case TABLE_CELL:
        o = new (arena) RenderTableCell(node);
        break;
    case TABLE_CAPTION:
        o = new (arena) RenderBlock(node);
        break;
    case BOX:
    case INLINE_BOX:
        o = new (arena) RenderFlexibleBox(node);
        break;
    }
    return o;
}

RenderObject::RenderObject(DOM::NodeImpl* node)
    : CachedObjectClient(),
m_style( 0 ),
m_node( node ),
m_parent( 0 ),
m_previous( 0 ),
m_next( 0 ),
m_verticalPosition( PositionUndefined ),
m_needsLayout( false ),
m_normalChildNeedsLayout( false ),
m_posChildNeedsLayout( false ),
m_minMaxKnown( false ),
m_floating( false ),

m_positioned( false ),
m_relPositioned( false ),
m_paintBackground( false ),

m_isAnonymous( node == node->getDocument() ),
m_recalcMinMax( false ),
m_isText( false ),
m_inline( true ),

m_replaced( false ),
m_isDragging( false ),
m_hasOverflowClip(false)
{
}

RenderObject::~RenderObject()
{
}

bool RenderObject::hasAncestor(const RenderObject *obj) const
{
    for (const RenderObject *r = this; r; r = r->m_parent)
        if (r == obj)
            return true;
    return false;
}

bool RenderObject::isRoot() const
{
    return element() && element()->renderer() == this &&
           element()->getDocument()->documentElement() == element();
}

bool RenderObject::isBody() const
{
    return element() && element()->renderer() == this && element()->id() == ID_BODY;
}

bool RenderObject::isHR() const
{
    return element() && element()->id() == ID_HR;
}

bool RenderObject::isHTMLMarquee() const
{
    return element() && element()->renderer() == this && element()->id() == ID_MARQUEE;
}

bool RenderObject::canHaveChildren() const
{
    return false;
}

RenderFlow* RenderObject::continuation() const
{
    return 0;
}

bool RenderObject::isInlineContinuation() const
{
    return false;
}

void RenderObject::addChild(RenderObject* , RenderObject *)
{
    KHTMLAssert(0);
}

RenderObject* RenderObject::removeChildNode(RenderObject* )
{
    KHTMLAssert(0);
    return 0;
}

void RenderObject::removeChild(RenderObject* )
{
    KHTMLAssert(0);
}

void RenderObject::appendChildNode(RenderObject*)
{
    KHTMLAssert(0);
}

void RenderObject::insertChildNode(RenderObject*, RenderObject*)
{
    KHTMLAssert(0);
}

RenderObject *RenderObject::nextRenderer() const
{
    if (firstChild())
        return firstChild();
    else if (nextSibling())
        return nextSibling();
    else {
        const RenderObject *r = this;
        while (r && !r->nextSibling())
            r = r->parent();
        if (r)
            return r->nextSibling();
    }
    return 0;
}

RenderObject *RenderObject::previousRenderer() const
{
    if (previousSibling()) {
        RenderObject *r = previousSibling();
        while (r->lastChild())
            r = r->lastChild();
        return r;
    }
    else if (parent()) {
        return parent();
    }
    else {
        return 0;
    }
}

bool RenderObject::isEditable() const
{
    RenderText *textRenderer = 0;
    if (isText()) {
        textRenderer = static_cast<RenderText *>(const_cast<RenderObject *>(this));
    }

    return style()->visibility() == VISIBLE && 
        element() && element()->isContentEditable() &&
        ((isBlockFlow() && !firstChild()) || 
        isReplaced() || 
        isBR() || 
        (textRenderer && textRenderer->firstTextBox()));
}

RenderObject *RenderObject::nextEditable() const
{
    RenderObject *r = const_cast<RenderObject *>(this);
    RenderObject *n = firstChild();
    if (n) {
        while (n) { 
            r = n; 
            n = n->firstChild(); 
        }
        if (r->isEditable())
            return r;
        else 
            return r->nextEditable();
    }
    n = r->nextSibling();
    if (n) {
        r = n;
        while (n) { 
            r = n; 
            n = n->firstChild(); 
        }
        if (r->isEditable())
            return r;
        else 
            return r->nextEditable();
    }
    n = r->parent();
    while (n) {
        r = n;
        n = r->nextSibling();
        if (n) {
            r = n;
            n = r->firstChild();
            while (n) { 
                r = n; 
                n = n->firstChild(); 
            }
            if (r->isEditable())
                return r;
            else 
                return r->nextEditable();
        }
        n = r->parent();
    }
    return 0;
}

RenderObject *RenderObject::previousEditable() const
{
    RenderObject *r = const_cast<RenderObject *>(this);
    RenderObject *n = firstChild();
    if (n) {
        while (n) { 
            r = n; 
            n = n->lastChild(); 
        }
        if (r->isEditable())
            return r;
        else 
            return r->previousEditable();
    }
    n = r->previousSibling();
    if (n) {
        r = n;
        while (n) { 
            r = n; 
            n = n->lastChild(); 
        }
        if (r->isEditable())
            return r;
        else 
            return r->previousEditable();
    }    
    n = r->parent();
    while (n) {
        r = n;
        n = r->previousSibling();
        if (n) {
            r = n;
            n = r->lastChild();
            while (n) { 
                r = n; 
                n = n->lastChild(); 
            }
            if (r->isEditable())
                return r;
            else 
                return r->previousEditable();
        }
        n = r->parent();
    }
    return 0;
} 

RenderObject *RenderObject::firstLeafChild() const
{
    RenderObject *r = firstChild();
    while (r) {
        RenderObject *n = 0;
        n = r->firstChild();
        if (!n)
            break;
        r = n;
    }
    return r;
}

RenderObject *RenderObject::lastLeafChild() const
{
    RenderObject *r = lastChild();
    while (r) {
        RenderObject *n = 0;
        n = r->lastChild();
        if (!n)
            break;
        r = n;
    }
    return r;
}

static void addLayers(RenderObject* obj, RenderLayer* parentLayer, RenderObject*& newObject,
                      RenderLayer*& beforeChild)
{
    if (obj->layer()) {
        if (!beforeChild && newObject) {
            // We need to figure out the layer that follows newObject.  We only do
            // this the first time we find a child layer, and then we update the
            // pointer values for newObject and beforeChild used by everyone else.
            beforeChild = newObject->parent()->findNextLayer(parentLayer, newObject);
            newObject = 0;
        }
        parentLayer->addChild(obj->layer(), beforeChild);
        return;
    }

    for (RenderObject* curr = obj->firstChild(); curr; curr = curr->nextSibling())
        addLayers(curr, parentLayer, newObject, beforeChild);
}

void RenderObject::addLayers(RenderLayer* parentLayer, RenderObject* newObject)
{
    if (!parentLayer)
        return;
    
    RenderObject* object = newObject;
    RenderLayer* beforeChild = 0;
    ::addLayers(this, parentLayer, object, beforeChild);
}

void RenderObject::removeLayers(RenderLayer* parentLayer)
{
    if (!parentLayer)
        return;
    
    if (layer()) {
        parentLayer->removeChild(layer());
        return;
    }

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->removeLayers(parentLayer);
}

void RenderObject::moveLayers(RenderLayer* oldParent, RenderLayer* newParent)
{
    if (!newParent)
        return;
    
    if (layer()) {
        if (oldParent)
            oldParent->removeChild(layer());
        newParent->addChild(layer());
        return;
    }

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->moveLayers(oldParent, newParent);
}

RenderLayer* RenderObject::findNextLayer(RenderLayer* parentLayer, RenderObject* startPoint,
                                         bool checkParent)
{
    // Error check the parent layer passed in.  If it's null, we can't find anything.
    if (!parentLayer)
        return 0;
        
    // Step 1: Descend into our siblings trying to find the next layer.  If we do find
    // a layer, and if its parent layer matches our desired parent layer, then we have
    // a match.
    for (RenderObject* curr = startPoint ? startPoint->nextSibling() : firstChild();
         curr; curr = curr->nextSibling()) {
        RenderLayer* nextLayer = curr->findNextLayer(parentLayer, 0, false);
        if (nextLayer) {
            if (nextLayer->parent() == parentLayer)
                return nextLayer;
            return 0;
        }
    }
    
    // Step 2: If our layer is the desired parent layer, then we're finished.  We didn't
    // find anything.
    RenderLayer* ourLayer = layer();
    if (parentLayer == ourLayer)
        return 0;
    
    // Step 3: If we have a layer, then return that layer.  It will be checked against
    // the desired parent layer in the for loop above.
    if (ourLayer)
        return ourLayer;
    
    // Step 4: If |checkParent| is set, climb up to our parent and check its siblings that
    // follow us to see if we can locate a layer.
    if (checkParent && parent())
        return parent()->findNextLayer(parentLayer, this, true);
    
    return 0;
}
    
RenderLayer* RenderObject::enclosingLayer()
{
    RenderObject* curr = this;
    while (curr) {
        RenderLayer *layer = curr->layer();
        if (layer)
            return layer;
        curr = curr->parent();
    }
    return 0;
}

bool RenderObject::requiresLayer()
{
    return isRoot() || isPositioned() || isRelPositioned() || style()->opacity() < 1.0f ||
           hasOverflowClip();
}

RenderBlock* RenderObject::firstLineBlock() const
{
    return 0;
}

void RenderObject::updateFirstLetter()
{}

int RenderObject::offsetLeft() const
{
    int x = xPos();
    if (!isPositioned()) {
        if (isRelPositioned()) {
            int y = 0;
            ((RenderBox*)this)->relativePositionOffset(x, y);
        }
        
        RenderObject* offsetPar = offsetParent();
        RenderObject* curr = parent();
        while (curr && curr != offsetPar) {
            x += curr->xPos();
            curr = curr->parent();
        }
    }
    return x;
}

int RenderObject::offsetTop() const
{
    int y = yPos();
    if (!isPositioned()) {
        if (isRelPositioned()) {
            int x = 0;
            ((RenderBox*)this)->relativePositionOffset(x, y);
        }
        RenderObject* offsetPar = offsetParent();
        RenderObject* curr = parent();
        while (curr && curr != offsetPar) {
            y += curr->yPos();
            curr = curr->parent();
        }
    }
    return y;
}
    
RenderObject* RenderObject::offsetParent() const
{
    bool skipTables = isPositioned() || isRelPositioned();
    RenderObject* curr = parent();
    while (curr && !curr->isPositioned() && !curr->isRelPositioned() &&
           !curr->isBody()) {
        if (!skipTables && (curr->isTableCell() || curr->isTable()))
            break;
        curr = curr->parent();
    }
    return curr;
}

// More IE extensions.  clientWidth and clientHeight represent the interior of an object
// excluding border and scrollbar.
int
RenderObject::clientWidth() const
{
    return width() - borderLeft() - borderRight() -
        (includeScrollbarSize() ? layer()->verticalScrollbarWidth() : 0);
}

int
RenderObject::clientHeight() const
{
    return height() - borderTop() - borderBottom() -
      (includeScrollbarSize() ? layer()->horizontalScrollbarHeight() : 0);
}

// scrollWidth/scrollHeight will be the same as clientWidth/clientHeight unless the
// object has overflow:hidden/scroll/auto specified and also has overflow.
int
RenderObject::scrollWidth() const
{
    return hasOverflowClip() ? layer()->scrollWidth() : overflowWidth();
}

int
RenderObject::scrollHeight() const
{
    return hasOverflowClip() ? layer()->scrollHeight() : overflowHeight();
}

bool RenderObject::scroll(KWQScrollDirection direction, KWQScrollGranularity granularity, float multiplier)
{
    RenderLayer *l = layer();
    if (l != 0 && l->scroll(direction, granularity, multiplier)) {
        return true;
    }
    RenderBlock *b = containingBlock();
    if (b != 0 && !b->isCanvas()) {
        return b->scroll(direction, granularity, multiplier);
    }
    return false;
}

bool
RenderObject::hasStaticX() const
{
    return (style()->left().isVariable() && style()->right().isVariable()) ||
            style()->left().isStatic() ||
            style()->right().isStatic();
}

bool
RenderObject::hasStaticY() const
{
    return (style()->top().isVariable() && style()->bottom().isVariable()) || style()->top().isStatic();
}

void RenderObject::markAllDescendantsWithFloatsForLayout(RenderObject*)
{
}

void RenderObject::setNeedsLayout(bool b, bool markParents) 
{
    bool alreadyNeededLayout = m_needsLayout;
    m_needsLayout = b;
    if (b) {
        if (!alreadyNeededLayout && markParents)
            markContainingBlocksForLayout();
    }
    else {
        m_posChildNeedsLayout = false;
        m_normalChildNeedsLayout = false;
    }
}

void RenderObject::setChildNeedsLayout(bool b, bool markParents)
{
    bool alreadyNeededLayout = m_normalChildNeedsLayout;
    m_normalChildNeedsLayout = b;
    if (b) {
        if (!alreadyNeededLayout && markParents)
            markContainingBlocksForLayout();
    }
    else {
        m_posChildNeedsLayout = false;
        m_normalChildNeedsLayout = false;
    }
}

void RenderObject::markContainingBlocksForLayout()
{
    RenderObject *o = container();
    RenderObject *last = this;

    while (o) {
        if (!last->isText() && (last->style()->position() == FIXED || last->style()->position() == ABSOLUTE)) {
            if (o->m_posChildNeedsLayout)
                return;
            o->m_posChildNeedsLayout = true;
        }
        else {
            if (o->m_normalChildNeedsLayout)
                return;
            o->m_normalChildNeedsLayout = true;
        }

        last = o;
        o = o->container();
    }

    last->scheduleRelayout();
}

RenderBlock* RenderObject::containingBlock() const
{
    if(isTableCell())
        return static_cast<const RenderTableCell *>(this)->table();
    if (isCanvas())
        return (RenderBlock*)this;

    RenderObject *o = parent();
    if (!isText() && m_style->position() == FIXED) {
        while ( o && !o->isCanvas() )
            o = o->parent();
    }
    else if (!isText() && m_style->position() == ABSOLUTE) {
        while (o && (o->style()->position() == STATIC || (o->isInline() && !o->isReplaced()))
               && !o->isRoot() && !o->isCanvas()) {
            // For relpositioned inlines, we return the nearest enclosing block.  We don't try
            // to return the inline itself.  This allows us to avoid having a positioned objects
            // list in all RenderInlines and lets us return a strongly-typed RenderBlock* result
            // from this method.  The container() method can actually be used to obtain the
            // inline directly.
            if (o->style()->position() == RELATIVE && o->isInline() && !o->isReplaced())
                return o->containingBlock();
            o = o->parent();
        }
    } else {
        while (o && ((o->isInline() && !o->isReplaced()) || o->isTableRow() || o->isTableSection()
                     || o->isTableCol() || o->isFrameSet()))
            o = o->parent();
    }

    if (!o || !o->isRenderBlock())
        return 0; // Probably doesn't happen any more, but leave just in case. -dwh
    
    return static_cast<RenderBlock*>(o);
}

int RenderObject::containingBlockWidth() const
{
    // ###
    return containingBlock()->contentWidth();
}

int RenderObject::containingBlockHeight() const
{
    // ###
    return containingBlock()->contentHeight();
}

bool RenderObject::sizesToMaxWidth() const
{
    // Marquees in WinIE are like a mixture of blocks and inline-blocks.  They size as though they're blocks,
    // but they allow text to sit on the same line as the marquee.
    if (isFloating() || (isCompact() && isInline()) || 
        (isInlineBlockOrInlineTable() && !isHTMLMarquee()) ||
        (element() && (element()->id() == ID_BUTTON || element()->id() == ID_LEGEND)))
        return true;
    
    // Children of a horizontal marquee do not fill the container by default.
    // FIXME: Need to deal with MAUTO value properly.  It could be vertical.
    if (parent()->style()->overflow() == OMARQUEE) {
        EMarqueeDirection dir = parent()->style()->marqueeDirection();
        if (dir == MAUTO || dir == MFORWARD || dir == MBACKWARD || dir == MLEFT || dir == MRIGHT)
            return true;
    }
    
    // Flexible horizontal boxes lay out children at their maxwidths.  Also vertical boxes
    // that don't stretch their kids lay out their children at their maxwidths.
    if (parent()->isFlexibleBox() &&
        (parent()->style()->boxOrient() == HORIZONTAL || parent()->style()->boxAlign() != BSTRETCH))
        return true;

    return false;
}

void RenderObject::drawBorder(QPainter *p, int x1, int y1, int x2, int y2,
                              BorderSide s, QColor c, const QColor& textcolor, EBorderStyle style,
                              int adjbw1, int adjbw2, bool invalidisInvert)
{
    int width = (s==BSTop||s==BSBottom?y2-y1:x2-x1);

    if(style == DOUBLE && width < 3)
        style = SOLID;

    if(!c.isValid()) {
        if(invalidisInvert)
        {
            p->setRasterOp(Qt::XorROP);
            c = Qt::white;
        }
        else {
            if(style == INSET || style == OUTSET || style == RIDGE || style ==
	    GROOVE)
                c.setRgb(238, 238, 238);
            else
                c = textcolor;
        }
    }

    switch(style)
    {
    case BNONE:
    case BHIDDEN:
        // should not happen
        if(invalidisInvert && p->rasterOp() == Qt::XorROP)
            p->setRasterOp(Qt::CopyROP);

        return;
    case DOTTED:
        p->setPen(QPen(c, width == 1 ? 0 : width, Qt::DotLine));
        /* nobreak; */
    case DASHED:
        if(style == DASHED)
            p->setPen(QPen(c, width == 1 ? 0 : width, Qt::DashLine));

        if (width > 0)
            switch(s)
            {
            case BSBottom:
            case BSTop:
                p->drawLine(x1, (y1+y2)/2, x2, (y1+y2)/2);
                break;
            case BSRight:
            case BSLeft:
                p->drawLine((x1+x2)/2, y1, (x1+x2)/2, y2);
                break;
            }
                
        break;

    case DOUBLE:
    {
        int third = (width+1)/3;

	if (adjbw1 == 0 && adjbw2 == 0)
	{
	    p->setPen(Qt::NoPen);
	    p->setBrush(c);
	    switch(s)
	    {
	    case BSTop:
	    case BSBottom:
	        p->drawRect(x1, y1      , x2-x1, third);
		p->drawRect(x1, y2-third, x2-x1, third);
		break;
	    case BSLeft:
	        p->drawRect(x1      , y1+1, third, y2-y1-1);
		p->drawRect(x2-third, y1+1, third, y2-y1-1);
		break;
	    case BSRight:
	        p->drawRect(x1      , y1+1, third, y2-y1-1);
		p->drawRect(x2-third, y1+1, third, y2-y1-1);
		break;
	    }
	}
	else
	{
	    int adjbw1bigthird;
	    if (adjbw1>0) adjbw1bigthird = adjbw1+1;
	    else adjbw1bigthird = adjbw1 - 1;
	    adjbw1bigthird /= 3;

	    int adjbw2bigthird;
	    if (adjbw2>0) adjbw2bigthird = adjbw2 + 1;
	    else adjbw2bigthird = adjbw2 - 1;
	    adjbw2bigthird /= 3;

	  switch(s)
	    {
	    case BSTop:
	      drawBorder(p, x1+QMAX((-adjbw1*2+1)/3,0), y1        , x2-QMAX((-adjbw2*2+1)/3,0), y1 + third, s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
	      drawBorder(p, x1+QMAX(( adjbw1*2+1)/3,0), y2 - third, x2-QMAX(( adjbw2*2+1)/3,0), y2        , s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
	      break;
	    case BSLeft:
	      drawBorder(p, x1        , y1+QMAX((-adjbw1*2+1)/3,0), x1+third, y2-QMAX((-adjbw2*2+1)/3,0), s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
	      drawBorder(p, x2 - third, y1+QMAX(( adjbw1*2+1)/3,0), x2      , y2-QMAX(( adjbw2*2+1)/3,0), s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
	      break;
	    case BSBottom:
	      drawBorder(p, x1+QMAX(( adjbw1*2+1)/3,0), y1      , x2-QMAX(( adjbw2*2+1)/3,0), y1+third, s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
	      drawBorder(p, x1+QMAX((-adjbw1*2+1)/3,0), y2-third, x2-QMAX((-adjbw2*2+1)/3,0), y2      , s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
	      break;
	    case BSRight:
            drawBorder(p, x1      , y1+QMAX(( adjbw1*2+1)/3,0), x1+third, y2-QMAX(( adjbw2*2+1)/3,0), s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
	    drawBorder(p, x2-third, y1+QMAX((-adjbw1*2+1)/3,0), x2      , y2-QMAX((-adjbw2*2+1)/3,0), s, c, textcolor, SOLID, adjbw1bigthird, adjbw2bigthird);
	      break;
	    default:
	      break;
	    }
	}
        break;
    }
    case RIDGE:
    case GROOVE:
    {
        EBorderStyle s1;
        EBorderStyle s2;
        if (style==GROOVE)
        {
            s1 = INSET;
            s2 = OUTSET;
        }
        else
        {
            s1 = OUTSET;
            s2 = INSET;
        }

	int adjbw1bighalf;
	int adjbw2bighalf;
	if (adjbw1>0) adjbw1bighalf=adjbw1+1;
	else adjbw1bighalf=adjbw1-1;
	adjbw1bighalf/=2;

	if (adjbw2>0) adjbw2bighalf=adjbw2+1;
	else adjbw2bighalf=adjbw2-1;
	adjbw2bighalf/=2;

        switch (s)
	{
	case BSTop:
	    drawBorder(p, x1+QMAX(-adjbw1  ,0)/2,  y1        , x2-QMAX(-adjbw2,0)/2, (y1+y2+1)/2, s, c, textcolor, s1, adjbw1bighalf, adjbw2bighalf);
	    drawBorder(p, x1+QMAX( adjbw1+1,0)/2, (y1+y2+1)/2, x2-QMAX( adjbw2+1,0)/2,  y2        , s, c, textcolor, s2, adjbw1/2, adjbw2/2);
	    break;
	case BSLeft:
            drawBorder(p,  x1        , y1+QMAX(-adjbw1  ,0)/2, (x1+x2+1)/2, y2-QMAX(-adjbw2,0)/2, s, c, textcolor, s1, adjbw1bighalf, adjbw2bighalf);
	    drawBorder(p, (x1+x2+1)/2, y1+QMAX( adjbw1+1,0)/2,  x2        , y2-QMAX( adjbw2+1,0)/2, s, c, textcolor, s2, adjbw1/2, adjbw2/2);
	    break;
	case BSBottom:
	    drawBorder(p, x1+QMAX( adjbw1  ,0)/2,  y1        , x2-QMAX( adjbw2,0)/2, (y1+y2+1)/2, s, c, textcolor, s2,  adjbw1bighalf, adjbw2bighalf);
	    drawBorder(p, x1+QMAX(-adjbw1+1,0)/2, (y1+y2+1)/2, x2-QMAX(-adjbw2+1,0)/2,  y2        , s, c, textcolor, s1, adjbw1/2, adjbw2/2);
	    break;
	case BSRight:
            drawBorder(p,  x1        , y1+QMAX( adjbw1  ,0)/2, (x1+x2+1)/2, y2-QMAX( adjbw2,0)/2, s, c, textcolor, s2, adjbw1bighalf, adjbw2bighalf);
	    drawBorder(p, (x1+x2+1)/2, y1+QMAX(-adjbw1+1,0)/2,  x2        , y2-QMAX(-adjbw2+1,0)/2, s, c, textcolor, s1, adjbw1/2, adjbw2/2);
	    break;
	}
        break;
    }
    case INSET:
        if(s == BSTop || s == BSLeft)
            c = c.dark();

        /* nobreak; */
    case OUTSET:
        if(style == OUTSET && (s == BSBottom || s == BSRight))
            c = c.dark();
        /* nobreak; */
    case SOLID:
        QPointArray quad(4);
        p->setPen(Qt::NoPen);
        p->setBrush(c);
	Q_ASSERT(x2>=x1);
	Q_ASSERT(y2>=y1);
	if (adjbw1==0 && adjbw2 == 0)
	  {
            p->drawRect(x1,y1,x2-x1,y2-y1);
	    return;
	  }
	switch(s) {
        case BSTop:
            quad.setPoints(4,
			   x1+QMAX(-adjbw1,0), y1,
                           x1+QMAX( adjbw1,0), y2,
                           x2-QMAX( adjbw2,0), y2,
                           x2-QMAX(-adjbw2,0), y1);
            break;
        case BSBottom:
            quad.setPoints(4,
			   x1+QMAX( adjbw1,0), y1,
                           x1+QMAX(-adjbw1,0), y2,
                           x2-QMAX(-adjbw2,0), y2,
                           x2-QMAX( adjbw2,0), y1);
            break;
        case BSLeft:
	  quad.setPoints(4,
			 x1, y1+QMAX(-adjbw1,0),
               		 x1, y2-QMAX(-adjbw2,0),
			 x2, y2-QMAX( adjbw2,0),
			 x2, y1+QMAX( adjbw1,0));
            break;
        case BSRight:
	  quad.setPoints(4,
			 x1, y1+QMAX( adjbw1,0),
               		 x1, y2-QMAX( adjbw2,0),
			 x2, y2-QMAX(-adjbw2,0),
			 x2, y1+QMAX(-adjbw1,0));
            break;
        }
	p->drawConvexPolygon(quad);
        break;
    }

    if(invalidisInvert && p->rasterOp() == Qt::XorROP)
        p->setRasterOp(Qt::CopyROP);
}

void RenderObject::paintBorder(QPainter *p, int _tx, int _ty, int w, int h, const RenderStyle* style, bool begin, bool end)
{
    const QColor& tc = style->borderTopColor();
    const QColor& bc = style->borderBottomColor();
    const QColor& lc = style->borderLeftColor();
    const QColor& rc = style->borderRightColor();

    bool tt = style->borderTopIsTransparent();
    bool bt = style->borderBottomIsTransparent();
    bool rt = style->borderRightIsTransparent();
    bool lt = style->borderLeftIsTransparent();
    
    EBorderStyle ts = style->borderTopStyle();
    EBorderStyle bs = style->borderBottomStyle();
    EBorderStyle ls = style->borderLeftStyle();
    EBorderStyle rs = style->borderRightStyle();

    bool render_t = ts > BHIDDEN && !tt;
    bool render_l = ls > BHIDDEN && begin && !lt;
    bool render_r = rs > BHIDDEN && end && !rt;
    bool render_b = bs > BHIDDEN && !bt;

    if(render_t) {
        bool ignore_left =
            (tc == lc) && (tt == lt) &&
            (ts >= OUTSET) &&
            (ls == DOTTED || ls == DASHED || ls == SOLID || ls == OUTSET);

        bool ignore_right =
            (tc == rc) && (tt == rt) &&
            (ts >= OUTSET) &&
            (rs == DOTTED || rs == DASHED || rs == SOLID || rs == INSET);
        
        drawBorder(p, _tx, _ty, _tx + w, _ty +  style->borderTopWidth(), BSTop, tc, style->color(), ts,
                   ignore_left?0:style->borderLeftWidth(),
                   ignore_right?0:style->borderRightWidth());
    }

    if(render_b) {
        bool ignore_left =
        (bc == lc) && (bt == lt) &&
        (bs >= OUTSET) &&
        (ls == DOTTED || ls == DASHED || ls == SOLID || ls == OUTSET);

        bool ignore_right =
            (bc == rc) && (bt == rt) &&
            (bs >= OUTSET) &&
            (rs == DOTTED || rs == DASHED || rs == SOLID || rs == INSET);
        
        drawBorder(p, _tx, _ty + h - style->borderBottomWidth(), _tx + w, _ty + h, BSBottom, bc, style->color(), bs,
                   ignore_left?0:style->borderLeftWidth(),
                   ignore_right?0:style->borderRightWidth());
    }
    
    if(render_l)
    {
	bool ignore_top =
	  (tc == lc) && (tt == lt) &&
	  (ls >= OUTSET) &&
	  (ts == DOTTED || ts == DASHED || ts == SOLID || ts == OUTSET);

	bool ignore_bottom =
	  (bc == lc) && (bt == lt) &&
	  (ls >= OUTSET) &&
	  (bs == DOTTED || bs == DASHED || bs == SOLID || bs == INSET);

        drawBorder(p, _tx, _ty, _tx + style->borderLeftWidth(), _ty + h, BSLeft, lc, style->color(), ls,
		   ignore_top?0:style->borderTopWidth(),
		   ignore_bottom?0:style->borderBottomWidth());
    }

    if(render_r)
    {
	bool ignore_top =
	  (tc == rc) && (tt == rt) &&
	  (rs >= DOTTED || rs == INSET) &&
	  (ts == DOTTED || ts == DASHED || ts == SOLID || ts == OUTSET);

	bool ignore_bottom =
	  (bc == rc) && (bt == rt) &&
	  (rs >= DOTTED || rs == INSET) &&
	  (bs == DOTTED || bs == DASHED || bs == SOLID || bs == INSET);

        drawBorder(p, _tx + w - style->borderRightWidth(), _ty, _tx + w, _ty + h, BSRight, rc, style->color(), rs,
		   ignore_top?0:style->borderTopWidth(),
		   ignore_bottom?0:style->borderBottomWidth());
    }
}

void RenderObject::absoluteRects(QValueList<QRect>& rects, int _tx, int _ty)
{
    // For blocks inside inlines, we go ahead and include margins so that we run right up to the
    // inline boxes above and below us (thus getting merged with them to form a single irregular
    // shape).
    if (continuation()) {
        rects.append(QRect(_tx, _ty - collapsedMarginTop(), 
                           width(), height()+collapsedMarginTop()+collapsedMarginBottom()));
        continuation()->absoluteRects(rects, 
                                      _tx - xPos() + continuation()->containingBlock()->xPos(),
                                      _ty - yPos() + continuation()->containingBlock()->yPos());
    }
    else
        rects.append(QRect(_tx, _ty, width(), height()));
}

QRect RenderObject::absoluteBoundingBoxRect()
{
    int x, y;
    absolutePosition(x, y);
    QValueList<QRect> rects;
    absoluteRects(rects, x, y);
    
    QValueList<QRect>::ConstIterator it = rects.begin();
    QRect result = *it;
    while (++it != rects.end()) {
        result = result.unite(*it);
    }
    return result;
}

void RenderObject::addAbsoluteRectForLayer(QRect& result)
{
    if (layer()) {
        result = result.unite(absoluteBoundingBoxRect());
    }
    for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
        current->addAbsoluteRectForLayer(result);
    }
}

QRect RenderObject::paintingRootRect(QRect& topLevelRect)
{
    QRect result = absoluteBoundingBoxRect();
    topLevelRect = result;
    for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
        current->addAbsoluteRectForLayer(result);
    }
    return result;
}

#if APPLE_CHANGES
void RenderObject::addFocusRingRects(QPainter *p, int _tx, int _ty)
{
    // For blocks inside inlines, we go ahead and include margins so that we run right up to the
    // inline boxes above and below us (thus getting merged with them to form a single irregular
    // shape).
    if (continuation()) {
        p->addFocusRingRect(_tx, _ty - collapsedMarginTop(), width(), height()+collapsedMarginTop()+collapsedMarginBottom());
        continuation()->addFocusRingRects(p, 
                                          _tx - xPos() + continuation()->containingBlock()->xPos(),
                                          _ty - yPos() + continuation()->containingBlock()->yPos());
    }
    else
        p->addFocusRingRect(_tx, _ty, width(), height());
}
#endif

void RenderObject::paintOutline(QPainter *p, int _tx, int _ty, int w, int h, const RenderStyle* style)
{
    int ow = style->outlineWidth();
    if(!ow) return;

    EBorderStyle os = style->outlineStyle();
    if (os <= BHIDDEN)
        return;
    
    QColor oc = style->outlineColor();
    if (!oc.isValid())
        oc = style->color();
    
    int offset = style->outlineOffset();
    
#ifdef APPLE_CHANGES
    if (style->outlineStyleIsAuto()) {
        p->initFocusRing(ow, offset, oc);
        addFocusRingRects(p, _tx, _ty);
        p->drawFocusRing();
        p->clearFocusRing();
        return;
    }
#endif

    _tx -= offset;
    _ty -= offset;
    w += 2*offset;
    h += 2*offset;
    
    drawBorder(p, _tx-ow, _ty-ow, _tx, _ty+h+ow, BSLeft,
	       QColor(oc), style->color(),
               os, ow, ow, true);

    drawBorder(p, _tx-ow, _ty-ow, _tx+w+ow, _ty, BSTop,
	       QColor(oc), style->color(),
	       os, ow, ow, true);

    drawBorder(p, _tx+w, _ty-ow, _tx+w+ow, _ty+h+ow, BSRight,
	       QColor(oc), style->color(),
	       os, ow, ow, true);

    drawBorder(p, _tx-ow, _ty+h, _tx+w+ow, _ty+h+ow, BSBottom,
	       QColor(oc), style->color(),
	       os, ow, ow, true);

}

void RenderObject::paint(PaintInfo& i, int tx, int ty)
{
}

void RenderObject::repaint(bool immediate)
{
    // Can't use canvas(), since we might be unrooted.
    RenderObject* o = this;
    while ( o->parent() ) o = o->parent();
    if (!o->isCanvas())
        return;
    RenderCanvas* c = static_cast<RenderCanvas*>(o);
    if (c->printingMode())
        return; // Don't repaint if we're printing.
    c->repaintViewRectangle(getAbsoluteRepaintRect(), immediate);    
}

void RenderObject::repaintRectangle(const QRect& r, bool immediate)
{
    // Can't use canvas(), since we might be unrooted.
    RenderObject* o = this;
    while ( o->parent() ) o = o->parent();
    if (!o->isCanvas())
        return;
    RenderCanvas* c = static_cast<RenderCanvas*>(o);
    if (c->printingMode())
        return; // Don't repaint if we're printing.
    QRect absRect(r);
    computeAbsoluteRepaintRect(absRect);
    c->repaintViewRectangle(absRect, immediate);
}

bool RenderObject::repaintAfterLayoutIfNeeded(const QRect& oldBounds, const QRect& oldFullBounds)
{
    QRect newBounds, newFullBounds;
    getAbsoluteRepaintRectIncludingFloats(newBounds, newFullBounds);
    if (newBounds != oldBounds || selfNeedsLayout()) {
        RenderCanvas* c = canvas();
        if (c->printingMode())
            return false; // Don't repaint if we're printing.
        c->repaintViewRectangle(oldFullBounds);
        if (newBounds != oldBounds)
            c->repaintViewRectangle(newFullBounds);
        return true;
    }
    return false;
}

void RenderObject::repaintDuringLayoutIfMoved(int x, int y)
{
}

void RenderObject::repaintFloatingDescendants()
{
}

bool RenderObject::checkForRepaintDuringLayout() const
{
    return !document()->view()->needsFullRepaint() && !layer();
}

void RenderObject::repaintObjectsBeforeLayout()
{
    if (!needsLayout() || isText())
        return;
    
    bool blockWithInlineChildren = (isRenderBlock() && !isTable() && normalChildNeedsLayout() && childrenInline());
    if (selfNeedsLayout()) {
        repaint();
        if (blockWithInlineChildren)
            return;
    }

    for (RenderObject* current = firstChild(); current; current = current->nextSibling()) {
        if (!current->isPositioned()) // RenderBlock subclass method handles walking the positioned objects.
            current->repaintObjectsBeforeLayout();
    }
}

QRect RenderObject::getAbsoluteRepaintRectWithOutline(int ow)
{
    QRect r(getAbsoluteRepaintRect());
    r.setRect(r.x()-ow, r.y()-ow, r.width()+ow*2, r.height()+ow*2);

    if (continuation() && !isInline())
        r.setRect(r.x(), r.y()-collapsedMarginTop(), r.width(), r.height()+collapsedMarginTop()+collapsedMarginBottom());
    
    if (isInlineFlow()) {
        for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
            if (!curr->isText()) {
                QRect childRect = curr->getAbsoluteRepaintRectWithOutline(ow);
                r = r.unite(childRect);
            }
        }
    }

    return r;
}

QRect RenderObject::getAbsoluteRepaintRect()
{
    if (parent())
        return parent()->getAbsoluteRepaintRect();
    return QRect();
}

void RenderObject::getAbsoluteRepaintRectIncludingFloats(QRect& bounds, QRect& fullBounds)
{
    bounds = fullBounds = getAbsoluteRepaintRect();
}

void RenderObject::computeAbsoluteRepaintRect(QRect& r, bool f)
{
    if (parent())
        return parent()->computeAbsoluteRepaintRect(r, f);
}

void RenderObject::dirtyLinesFromChangedChild(RenderObject* child, bool adding)
{
}

#ifndef NDEBUG

QString RenderObject::information() const
{
    QString str;
    QTextStream ts( &str, IO_WriteOnly );
    ts << renderName()
	<< "(" << (style() ? style()->refCount() : 0) << ")"
       << ": " << (void*)this << "  ";
    if (isInline()) ts << "il ";
    if (childrenInline()) ts << "ci ";
    if (isFloating()) ts << "fl ";
    if (isAnonymous()) ts << "an ";
    if (isRelPositioned()) ts << "rp ";
    if (isPositioned()) ts << "ps ";
    if (needsLayout()) ts << "nl ";
    if (m_recalcMinMax) ts << "rmm ";
    if (style() && style()->zIndex()) ts << "zI: " << style()->zIndex();
    if (element() && element()->active()) ts << "act ";
    if (element() && element()->hasAnchor()) ts << "anchor ";
    if (element() && element()->focused()) ts << "focus ";
    if (element()) ts << " <" <<  getTagName(element()->id()).string() << ">";
    ts << " (" << xPos() << "," << yPos() << "," << width() << "," << height() << ")"
	<< (isTableCell() ?
	    ( QString::fromLatin1(" [r=") +
	      QString::number( static_cast<const RenderTableCell *>(this)->row() ) +
	      QString::fromLatin1(" c=") +
	      QString::number( static_cast<const RenderTableCell *>(this)->col() ) +
	      QString::fromLatin1(" rs=") +
	      QString::number( static_cast<const RenderTableCell *>(this)->rowSpan() ) +
	      QString::fromLatin1(" cs=") +
	      QString::number( static_cast<const RenderTableCell *>(this)->colSpan() ) +
	      QString::fromLatin1("]") ) : QString::null );
	return str;
}

void RenderObject::printTree(int indent) const
{
    QString ind;
    ind.fill(' ', indent);

    kdDebug() << ind << information() << endl;

    RenderObject *child = firstChild();
    while( child != 0 )
    {
        child->printTree(indent+2);
        child = child->nextSibling();
    }
}

void RenderObject::dump(QTextStream *stream, QString ind) const
{
    if (isAnonymous()) { *stream << " anonymous"; }
    if (isFloating()) { *stream << " floating"; }
    if (isPositioned()) { *stream << " positioned"; }
    if (isRelPositioned()) { *stream << " relPositioned"; }
    if (isText()) { *stream << " text"; }
    if (isInline()) { *stream << " inline"; }
    if (isReplaced()) { *stream << " replaced"; }
    if (shouldPaintBackgroundOrBorder()) { *stream << " paintBackground"; }
    if (needsLayout()) { *stream << " needsLayout"; }
    if (minMaxKnown()) { *stream << " minMaxKnown"; }
    *stream << endl;

    RenderObject *child = firstChild();
    while( child != 0 )
    {
	*stream << ind << child->renderName() << ": ";
	child->dump(stream,ind+"  ");
	child = child->nextSibling();
    }
}
#endif

static NodeImpl *selectStartNode(const RenderObject *object)
{
    DOM::NodeImpl *node = 0;
    bool forcedOn = false;

    for (const RenderObject *curr = object; curr; curr = curr->parent()) {
        if (curr->style()->userSelect() == SELECT_TEXT)
	    forcedOn = true;
        if (!forcedOn && curr->style()->userSelect() == SELECT_NONE)
            return 0;

	if (!node)
	    node = curr->element();
    }

    // somewhere up the render tree there must be an element!
    assert(node);

    return node;
}

bool RenderObject::canSelect() const
{
    return selectStartNode(this) != 0;
}

bool RenderObject::shouldSelect() const
{
    NodeImpl *node = selectStartNode(this);
    if (!node)
        return false;
    return node->dispatchHTMLEvent(DOM::EventImpl::SELECTSTART_EVENT, true, true);
}

QColor RenderObject::selectionColor(QPainter *p) const
{
    QColor color;
    if (style()->userSelect() != SELECT_NONE) {
        RenderStyle* pseudoStyle = getPseudoStyle(RenderStyle::SELECTION);
        if (pseudoStyle && pseudoStyle->backgroundColor().isValid())
            color = pseudoStyle->backgroundColor();
        else
            color = p->selectedTextBackgroundColor();
    }

    return color;
}

DOM::NodeImpl* RenderObject::draggableNode(bool dhtmlOK, bool uaOK, int x, int y, bool& dhtmlWillDrag) const
{
    if (!dhtmlOK && !uaOK)
        return 0;

    const RenderObject* curr = this;
    while (curr) {
        DOM::NodeImpl *elt = curr->element();
        if (elt && elt->nodeType() == Node::TEXT_NODE) {
            // Since there's no way for the author to address the -khtml-user-drag style for a text node,
            // we use our own judgement.
            if (uaOK && canvas()->view()->part()->shouldDragAutoNode(curr->node(), x, y)) {
                dhtmlWillDrag = false;
                return curr->node();
            } else if (curr->shouldSelect()) {
                // In this case we have a click in the unselected portion of text.  If this text is
                // selectable, we want to start the selection process instead of looking for a parent
                // to try to drag.
                return 0;
            }
        } else {
            EUserDrag dragMode = curr->style()->userDrag();
            if (dhtmlOK && dragMode == DRAG_ELEMENT) {
                dhtmlWillDrag = true;
                return curr->node();
            } else if (uaOK && dragMode == DRAG_AUTO
                       && canvas()->view()->part()->shouldDragAutoNode(curr->node(), x, y))
            {
                dhtmlWillDrag = false;
                return curr->node();
            }
        }
        curr = curr->parent();
    }
    return 0;
}

void RenderObject::selectionStartEnd(int& spos, int& epos)
{
    canvas()->selectionStartEnd(spos, epos);
}

RenderBlock* RenderObject::createAnonymousBlock()
{
    RenderStyle *newStyle = new (renderArena()) RenderStyle();
    newStyle->inheritFrom(m_style);
    newStyle->setDisplay(BLOCK);

    RenderBlock *newBox = new (renderArena()) RenderBlock(document() /* anonymous box */);
    newBox->setStyle(newStyle);
    return newBox;
}

void RenderObject::handleDynamicFloatPositionChange()
{
    // We have gone from not affecting the inline status of the parent flow to suddenly
    // having an impact.  See if there is a mismatch between the parent flow's
    // childrenInline() state and our state.
    setInline(style()->isDisplayInlineType());
    if (isInline() != parent()->childrenInline()) {
        if (!isInline()) {
            if (parent()->isRenderInline()) {
                // We have to split the parent flow.
                RenderInline* parentInline = static_cast<RenderInline*>(parent());
                RenderBlock* newBox = parentInline->createAnonymousBlock();
                
                RenderFlow* oldContinuation = parent()->continuation();
                parentInline->setContinuation(newBox);

                RenderObject* beforeChild = nextSibling();
                parent()->removeChildNode(this);
                parentInline->splitFlow(beforeChild, newBox, this, oldContinuation);
            }
            else if (parent()->isRenderBlock())
                static_cast<RenderBlock*>(parent())->makeChildrenNonInline();
        }
        else {
            // An anonymous block must be made to wrap this inline.
            RenderBlock* box = createAnonymousBlock();
            parent()->insertChildNode(box, this);
            box->appendChildNode(parent()->removeChildNode(this));
        }
    }
}

void RenderObject::setStyle(RenderStyle *style)
{
    if (m_style == style)
        return;

    bool affectsParentBlock = false;
    RenderStyle::Diff d = RenderStyle::Equal;
    if (m_style) {
        // If our z-index changes value or our visibility changes,
        // we need to dirty our stacking context's z-order list.
        if (style) {
#if APPLE_CHANGES
            if (m_style->visibility() != style->visibility() ||
                m_style->zIndex() != style->zIndex() ||
                m_style->hasAutoZIndex() != style->hasAutoZIndex())
                document()->setDashboardRegionsDirty(true);
#endif

            if ((m_style->hasAutoZIndex() != style->hasAutoZIndex() ||
                 m_style->zIndex() != style->zIndex() ||
                 m_style->visibility() != style->visibility()) && layer()) {
                layer()->stackingContext()->dirtyZOrderLists();
                if (m_style->hasAutoZIndex() != style->hasAutoZIndex() ||
                    m_style->visibility() != style->visibility())
                    layer()->dirtyZOrderLists();
            }
        }

        d = m_style->diff(style);

        // If we have no layer(), just treat a RepaintLayer hint as a normal Repaint.
        if (d == RenderStyle::RepaintLayer && !layer())
            d = RenderStyle::Repaint;
        
        // The background of the root element or the body element could propagate up to
        // the canvas.  Just dirty the entire canvas when our style changes substantially.
        if (d >= RenderStyle::Repaint && element() &&
            (element()->id() == ID_HTML || element()->id() == ID_BODY))
            canvas()->repaint();
        else if (m_parent && !isText()) {
            // Do a repaint with the old style first, e.g., for example if we go from
            // having an outline to not having an outline.
            if (d == RenderStyle::RepaintLayer) {
                layer()->repaintIncludingDescendants();
                if (!(m_style->clip() == style->clip()))
                    layer()->clearClipRects();
            }
            else if (d == RenderStyle::Repaint)
                repaint();
        }

        // When a layout hint happens, we go ahead and do a repaint of the layer, since the layer could
        // end up being destroyed.
        if (d == RenderStyle::Layout && layer() &&
            (m_style->position() != style->position() ||
             m_style->zIndex() != style->zIndex() ||
             m_style->hasAutoZIndex() != style->hasAutoZIndex() ||
             !(m_style->clip() == style->clip()) ||
             m_style->hasClip() != style->hasClip() ||
             m_style->opacity() != style->opacity()))
            layer()->repaintIncludingDescendants();

        // When a layout hint happens and an object's position style changes, we have to do a layout
        // to dirty the render tree using the old position value now.
        if (d == RenderStyle::Layout && m_parent && m_style->position() != style->position())
            setNeedsLayoutAndMinMaxRecalc();
        
        if (isFloating() && (m_style->floating() != style->floating()))
            // For changes in float styles, we need to conceivably remove ourselves
            // from the floating objects list.
            removeFromObjectLists();
        else if (isPositioned() && (style->position() != ABSOLUTE && style->position() != FIXED))
            // For changes in positioning styles, we need to conceivably remove ourselves
            // from the positioned objects list.
            removeFromObjectLists();
        
        affectsParentBlock = m_style && isFloatingOrPositioned() &&
            (!style->isFloating() && style->position() != ABSOLUTE && style->position() != FIXED)
            && parent() && (parent()->isBlockFlow() || parent()->isInlineFlow());
        
        // reset style flags
        m_floating = false;
        m_positioned = false;
        m_relPositioned = false;
        m_paintBackground = false;
        m_hasOverflowClip = false;
    }

    RenderStyle *oldStyle = m_style;
    m_style = style;

    updateBackgroundImages(oldStyle);
    
    if (m_style)
        m_style->ref();
    
    if (oldStyle)
        oldStyle->deref(renderArena());

    setShouldPaintBackgroundOrBorder(m_style->hasBorder() || m_style->hasBackground());

    if (affectsParentBlock)
        handleDynamicFloatPositionChange();
    
    // No need to ever schedule repaints from a style change of a text run, since
    // we already did this for the parent of the text run.
    if (d == RenderStyle::Layout && m_parent)
        setNeedsLayoutAndMinMaxRecalc();
    else if (m_parent && !isText() && (d == RenderStyle::RepaintLayer || d == RenderStyle::Repaint))
        // Do a repaint with the new style now, e.g., for example if we go from
        // not having an outline to having an outline.
        repaint();
}

void RenderObject::setStyleInternal(RenderStyle* st)
{
    if (m_style == st)
        return;
    if (m_style)
        m_style->deref(renderArena());
    m_style = st;
    if (m_style)
        m_style->ref();
}

void RenderObject::updateBackgroundImages(RenderStyle* oldStyle)
{
    // FIXME: This will be slow when a large number of images is used.  Fix by using a dict.
    const BackgroundLayer* oldLayers = oldStyle ? oldStyle->backgroundLayers() : 0;
    const BackgroundLayer* newLayers = m_style ? m_style->backgroundLayers() : 0;
    for (const BackgroundLayer* currOld = oldLayers; currOld; currOld = currOld->next()) {
        if (currOld->backgroundImage() && (!newLayers || !newLayers->containsImage(currOld->backgroundImage())))
            currOld->backgroundImage()->deref(this);
    }
    for (const BackgroundLayer* currNew = newLayers; currNew; currNew = currNew->next()) {
        if (currNew->backgroundImage() && (!oldLayers || !oldLayers->containsImage(currNew->backgroundImage())))
            currNew->backgroundImage()->ref(this);
    }
}

QRect RenderObject::viewRect() const
{
    return canvas()->viewRect();
}

bool RenderObject::absolutePosition(int &xPos, int &yPos, bool f)
{
    RenderObject* o = parent();
    if (o) {
        o->absolutePosition(xPos, yPos, f);
        if (o->hasOverflowClip())
            o->layer()->subtractScrollOffset(xPos, yPos); 
        return true;
    }
    else
    {
        xPos = yPos = 0;
        return false;
    }
}

QRect RenderObject::caretRect(int offset, EAffinity affinity, int *extraWidthToEndOfLine)
{
   if (extraWidthToEndOfLine)
       *extraWidthToEndOfLine = 0;

    return QRect();
}

int RenderObject::paddingTop() const
{
    int w = 0;
    Length padding = m_style->paddingTop();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.minWidth(w);
    if ( isTableCell() && padding.isVariable() )
	w = static_cast<const RenderTableCell *>(this)->table()->cellPadding();
    return w;
}

int RenderObject::paddingBottom() const
{
    int w = 0;
    Length padding = style()->paddingBottom();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.minWidth(w);
    if ( isTableCell() && padding.isVariable() )
	w = static_cast<const RenderTableCell *>(this)->table()->cellPadding();
    return w;
}

int RenderObject::paddingLeft() const
{
    int w = 0;
    Length padding = style()->paddingLeft();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.minWidth(w);
    if ( isTableCell() && padding.isVariable() )
	w = static_cast<const RenderTableCell *>(this)->table()->cellPadding();
    return w;
}

int RenderObject::paddingRight() const
{
    int w = 0;
    Length padding = style()->paddingRight();
    if (padding.isPercent())
        w = containingBlock()->contentWidth();
    w = padding.minWidth(w);
    if ( isTableCell() && padding.isVariable() )
	w = static_cast<const RenderTableCell *>(this)->table()->cellPadding();
    return w;
}

int RenderObject::tabWidth() const
{
    if (style()->whiteSpace() != PRE)
        return 0;
        
    return containingBlock()->tabWidth(true);
}

RenderCanvas* RenderObject::canvas() const
{
    return static_cast<RenderCanvas*>(document()->renderer());
}

RenderObject *RenderObject::container() const
{
    // This method is extremely similar to containingBlock(), but with a few notable
    // exceptions.
    // (1) It can be used on orphaned subtrees, i.e., it can be called safely even when
    // the object is not part of the primary document subtree yet.
    // (2) For normal flow elements, it just returns the parent.
    // (3) For absolute positioned elements, it will return a relative positioned inline.
    // containingBlock() simply skips relpositioned inlines and lets an enclosing block handle
    // the layout of the positioned object.  This does mean that calcAbsoluteHorizontal and
    // calcAbsoluteVertical have to use container().
    EPosition pos = m_style->position();
    RenderObject *o = 0;
    if (!isText() && pos == FIXED) {
        // container() can be called on an object that is not in the
        // tree yet.  We don't call canvas() since it will assert if it
        // can't get back to the canvas.  Instead we just walk as high up
        // as we can.  If we're in the tree, we'll get the root.  If we
        // aren't we'll get the root of our little subtree (most likely
        // we'll just return 0).
        o = parent();
        while (o && o->parent()) o = o->parent();
    }
    else if (!isText() && pos == ABSOLUTE) {
        // Same goes here.  We technically just want our containing block, but
        // we may not have one if we're part of an uninstalled subtree.  We'll
        // climb as high as we can though.
        o = parent();
        while (o && o->style()->position() == STATIC && !o->isRoot() && !o->isCanvas())
            o = o->parent();
    }
    else
	o = parent();
    return o;
}

bool RenderObject::isSelectionBorder() const
{
    SelectionState st = selectionState();
    return st == SelectionStart || st == SelectionEnd || st == SelectionBoth;
}

#if 0
static void checkFloats(RenderObject* o, RenderObject* f)
{
    if (o->isRenderBlock()) {
        RenderBlock* b = static_cast<RenderBlock*>(o);
        if (b->containsFloat(f))
            assert(false);
    }
    
    for (RenderObject* c = o->firstChild(); c; c = c->nextSibling())
        checkFloats(c, f);
}
#endif

void RenderObject::removeFromObjectLists()
{
    if (isFloating()) {
        RenderBlock* outermostBlock = containingBlock();
        for (RenderBlock* p = outermostBlock; p && !p->isCanvas(); p = p->containingBlock()) {
            if (p->containsFloat(this))
                outermostBlock = p;
        }
        
        if (outermostBlock)
            outermostBlock->markAllDescendantsWithFloatsForLayout(this);
#if 0
        // Debugging code for float checking.
        checkFloats(canvas(), this);
#endif
    }

    if (isPositioned()) {
        RenderObject *p;
        for (p = parent(); p; p = p->parent()) {
            if (p->isRenderBlock())
                static_cast<RenderBlock*>(p)->removePositionedObject(this);
        }
    }
}

RenderArena* RenderObject::renderArena() const
{
    DOM::DocumentImpl* doc = document();
    return doc ? doc->renderArena() : 0;
}

void RenderObject::remove()
{
#if APPLE_CHANGES
    // Delete our accessibility object if we have one.
    document()->getAccObjectCache()->detach(this);
#endif

    removeFromObjectLists();

    if (parent())
        //have parent, take care of the tree integrity
        parent()->removeChild(this);
        
    deleteLineBoxWrapper();
}

void RenderObject::detach()
{
    remove();
    
    // by default no refcounting
    arenaDelete(document()->renderArena(), this);
}

void RenderObject::arenaDelete(RenderArena *arena, void *base)
{
    if (m_style->backgroundImage())
        m_style->backgroundImage()->deref(this);
    if (m_style)
        m_style->deref(arena);
    
#ifndef NDEBUG
    void *savedBase = baseOfRenderObjectBeingDeleted;
    baseOfRenderObjectBeingDeleted = base;
#endif
    delete this;
#ifndef NDEBUG
    baseOfRenderObjectBeingDeleted = savedBase;
#endif
    
    // Recover the size left there for us by operator delete and free the memory.
    arena->free(*(size_t *)base, base);
}

VisiblePosition RenderObject::positionForCoordinates(int x, int y)
{
    return VisiblePosition(element(), caretMinOffset(), DOWNSTREAM);
}

bool RenderObject::isDragging() const
{ 
    return m_isDragging; 
}

void RenderObject::updateDragState(bool dragOn)
{
    bool valueChanged = (dragOn != m_isDragging);
    m_isDragging = dragOn;
    if (valueChanged && style()->affectedByDragRules())
        element()->setChanged();
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->updateDragState(dragOn);
    if (continuation())
        continuation()->updateDragState(dragOn);
}

bool RenderObject::hitTest(NodeInfo& info, int x, int y, int tx, int ty, HitTestFilter hitTestFilter)
{
    bool inside = false;
    if (hitTestFilter != HitTestSelf) {
        // First test the foreground layer (lines and inlines).
        inside = nodeAtPoint(info, x, y, tx, ty, HitTestForeground);
        
        // Test floats next.
        if (!inside)
            inside = nodeAtPoint(info, x, y, tx, ty, HitTestFloat);

        // Finally test to see if the mouse is in the background (within a child block's background).
        if (!inside)
            inside = nodeAtPoint(info, x, y, tx, ty, HitTestChildBlockBackgrounds);
    }
    
    // See if the mouse is inside us but not any of our descendants
    if (hitTestFilter != HitTestDescendants && !inside)
        inside = nodeAtPoint(info, x, y, tx, ty, HitTestBlockBackground);
        
    return inside;
}

void RenderObject::setInnerNode(NodeInfo& info)
{
    if (!info.innerNode() && !isInline() && continuation()) {
        // We are in the margins of block elements that are part of a continuation.  In
        // this case we're actually still inside the enclosing inline element that was
        // split.  Go ahead and set our inner node accordingly.
        info.setInnerNode(continuation()->element());
        if (!info.innerNonSharedNode())
            info.setInnerNonSharedNode(continuation()->element());
    }

    if (!info.innerNode() && element())
        info.setInnerNode(element());
            
    if(!info.innerNonSharedNode() && element())
        info.setInnerNonSharedNode(element());
}

bool RenderObject::nodeAtPoint(NodeInfo& info, int _x, int _y, int _tx, int _ty,
                               HitTestAction hitTestAction)
{
    return false;
}

short RenderObject::verticalPositionHint( bool firstLine ) const
{
    short vpos = m_verticalPosition;
    if ( m_verticalPosition == PositionUndefined || firstLine ) {
	vpos = getVerticalPosition( firstLine );
	if ( !firstLine )
	    m_verticalPosition = vpos;
    }
    return vpos;

}

short RenderObject::getVerticalPosition( bool firstLine ) const
{
    if (!isInline())
        return 0;

    // This method determines the vertical position for inline elements.
    int vpos = 0;
    EVerticalAlign va = style()->verticalAlign();
    if ( va == TOP ) {
        vpos = PositionTop;
    } else if ( va == BOTTOM ) {
        vpos = PositionBottom;
    } else if ( va == LENGTH ) {
        vpos = -style()->verticalAlignLength().width( lineHeight( firstLine ) );
    } else  {
        bool checkParent = parent()->isInline() && !parent()->isInlineBlockOrInlineTable() && parent()->style()->verticalAlign() != TOP && parent()->style()->verticalAlign() != BOTTOM;
        vpos = checkParent ? parent()->verticalPositionHint( firstLine ) : 0;
        // don't allow elements nested inside text-top to have a different valignment.
        if ( va == BASELINE )
            return vpos;

        const QFont &f = parent()->font( firstLine );
        int fontsize = f.pixelSize();
    
        if (va == SUB)
            vpos += fontsize/5 + 1;
        else if (va == SUPER)
            vpos -= fontsize/3 + 1;
        else if (va == TEXT_TOP)
            vpos += baselinePosition( firstLine ) - QFontMetrics(f).ascent();
        else if (va == MIDDLE)
            vpos += - (int)(QFontMetrics(f).xHeight()/2) - lineHeight( firstLine )/2 + baselinePosition( firstLine );
        else if (va == TEXT_BOTTOM) {
            vpos += QFontMetrics(f).descent();
            if (!isReplaced())
                vpos -= fontMetrics(firstLine).descent();
        } else if ( va == BASELINE_MIDDLE )
            vpos += - lineHeight( firstLine )/2 + baselinePosition( firstLine );
    }
    
    return vpos;
}

short RenderObject::lineHeight( bool firstLine, bool ) const
{
    RenderStyle* s = style(firstLine);
    
    Length lh = s->lineHeight();

    // its "unset", choose nice default
    if (lh.value < 0)
        return s->fontMetrics().lineSpacing();

    if (lh.isPercent())
        return lh.minWidth(s->font().pixelSize());

    // its fixed
    return lh.value;
}

short RenderObject::baselinePosition( bool firstLine, bool isRootLineBox ) const
{
    const QFontMetrics &fm = fontMetrics( firstLine );
    return fm.ascent() + ( lineHeight( firstLine, isRootLineBox ) - fm.height() ) / 2;
}

void RenderObject::invalidateVerticalPositions()
{
    m_verticalPosition = PositionUndefined;
    RenderObject *child = firstChild();
    while( child ) {
	child->invalidateVerticalPositions();
	child = child->nextSibling();
    }
}

void RenderObject::recalcMinMaxWidths()
{
    KHTMLAssert( m_recalcMinMax );

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << " recalcMinMaxWidths() this=" << this <<endl;
#endif

    if (m_recalcMinMax)
        updateFirstLetter();
    
    RenderObject *child = firstChild();
    while( child ) {
	int cmin = 0;
	int cmax = 0;
	bool test = false;
	if ( ( m_minMaxKnown && child->m_recalcMinMax ) || !child->m_minMaxKnown ) {
	    cmin = child->minWidth();
	    cmax = child->maxWidth();
	    test = true;
	}
	if ( child->m_recalcMinMax )
	    child->recalcMinMaxWidths();
	if ( !child->m_minMaxKnown )
	    child->calcMinMaxWidth();
	if ( m_minMaxKnown && test && (cmin != child->minWidth() || cmax != child->maxWidth()) )
	    m_minMaxKnown = false;
	child = child->nextSibling();
    }

    // we need to recalculate, if the contains inline children, as the change could have
    // happened somewhere deep inside the child tree. Also do this for blocks or tables that
    // are inline (i.e., inline-block and inline-table).
    if ((!isInline() || isInlineBlockOrInlineTable()) && childrenInline())
	m_minMaxKnown = false;

    if ( !m_minMaxKnown )
	calcMinMaxWidth();
    m_recalcMinMax = false;
}

void RenderObject::scheduleRelayout()
{
    if (!isCanvas()) return;
    KHTMLView *view = static_cast<RenderCanvas *>(this)->view();
    if (view)
        view->scheduleRelayout();
}


void RenderObject::removeLeftoverAnonymousBoxes()
{
}

InlineBox* RenderObject::createInlineBox(bool, bool isRootLineBox, bool)
{
    KHTMLAssert(!isRootLineBox);
    return new (renderArena()) InlineBox(this);
}

void RenderObject::dirtyLineBoxes(bool, bool)
{
}

InlineBox* RenderObject::inlineBoxWrapper() const
{
    return 0;
}

void RenderObject::setInlineBoxWrapper(InlineBox* b)
{
}

void RenderObject::deleteLineBoxWrapper()
{
}

RenderStyle* RenderObject::style(bool firstLine) const {
    RenderStyle *s = m_style;
    if (firstLine) {
        const RenderObject* obj = isText() ? parent() : this;
        if (obj->isBlockFlow()) {
            RenderBlock* firstLineBlock = obj->firstLineBlock();
            if (firstLineBlock)
                s = firstLineBlock->getPseudoStyle(RenderStyle::FIRST_LINE, style());
        }
        else if (!obj->isAnonymous() && obj->isInlineFlow()) {
            RenderStyle* parentStyle = obj->parent()->style(true);
            if (parentStyle != obj->parent()->style()) {
                // A first-line style is in effect. We need to cache a first-line style
                // for ourselves.
                style()->setHasPseudoStyle(RenderStyle::FIRST_LINE_INHERITED);
                s = obj->getPseudoStyle(RenderStyle::FIRST_LINE_INHERITED, parentStyle);
            }
        }
    }
    return s;
}

RenderStyle* RenderObject::getPseudoStyle(RenderStyle::PseudoId pseudo, RenderStyle* parentStyle) const
{
    if (!style()->hasPseudoStyle(pseudo))
        return 0;
    
    if (!parentStyle)
        parentStyle = style();

    RenderStyle* result = style()->getPseudoStyle(pseudo);
    if (result) return result;
    
    DOM::NodeImpl* node = element();
    if (isText())
        node = element()->parentNode();
    if (!node) return 0;
    
    if (pseudo == RenderStyle::FIRST_LINE_INHERITED)
        result = document()->styleSelector()->styleForElement(static_cast<DOM::ElementImpl*>(node), 
                                                              parentStyle, false);
    else
        result = document()->styleSelector()->pseudoStyleForElement(pseudo, static_cast<DOM::ElementImpl*>(node), 
                                                                    parentStyle);
    if (result)
        style()->addPseudoStyle(result);
    return result;
}

void RenderObject::getTextDecorationColors(int decorations, QColor& underline, QColor& overline,
                                           QColor& linethrough, bool quirksMode)
{
    RenderObject* curr = this;
    do {
        int currDecs = curr->style()->textDecoration();
        if (currDecs) {
            if (currDecs & UNDERLINE) {
                decorations &= ~UNDERLINE;
                underline = curr->style()->color();
            }
            if (currDecs & OVERLINE) {
                decorations &= ~OVERLINE;
                overline = curr->style()->color();
            }
            if (currDecs & LINE_THROUGH) {
                decorations &= ~LINE_THROUGH;
                linethrough = curr->style()->color();
            }
        }
        curr = curr->parent();
        if (curr && curr->isRenderBlock() && curr->continuation())
            curr = curr->continuation();
    } while (curr && decorations && (!quirksMode || !curr->element() ||
                                     (curr->element()->id() != ID_A && curr->element()->id() != ID_FONT)));

    // If we bailed out, use the element we bailed out at (typically a <font> or <a> element).
    if (decorations && curr) {
        if (decorations & UNDERLINE)
            underline = curr->style()->color();
        if (decorations & OVERLINE)
            overline = curr->style()->color();
        if (decorations & LINE_THROUGH)
            linethrough = curr->style()->color();
    }        
}

#if APPLE_CHANGES
void RenderObject::updateWidgetPositions()
{
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->updateWidgetPositions();
}

QValueList<DashboardRegionValue> RenderObject::computeDashboardRegions()
{
    QValueList<DashboardRegionValue> regions;
    collectDashboardRegions(regions);
    return regions;
}

void RenderObject::addDashboardRegions (QValueList<DashboardRegionValue>& regions)
{
    // Convert the style regions to absolute coordinates.
    if (style()->visibility() != VISIBLE) 
        return;

    QValueList<StyleDashboardRegion> styleRegions = style()->dashboardRegions();
    if (styleRegions.count() > 0) {
        uint i, count = styleRegions.count();
        for (i = 0; i < count; i++){
            StyleDashboardRegion styleRegion = styleRegions[i];
            
            int w = width();
            int h = height();
            
            DashboardRegionValue region;
            region.label = styleRegion.label;
            region.bounds = QRect (
                styleRegion.offset.left.value,
                styleRegion.offset.top.value,
                w - styleRegion.offset.left.value - styleRegion.offset.right.value,
                h - styleRegion.offset.top.value - styleRegion.offset.bottom.value);
            region.type = styleRegion.type;

            region.clip = region.bounds;
            computeAbsoluteRepaintRect(region.clip);
            if (region.clip.height() < 0) {
                region.clip.setHeight(0);
                region.clip.setWidth(0);
            }

            int x, y;
            absolutePosition (x, y);
            region.bounds.setX (x + styleRegion.offset.left.value);
            region.bounds.setY (y + styleRegion.offset.top.value);
            
            regions.append (region);
        }
    }
}

void RenderObject::collectDashboardRegions (QValueList<DashboardRegionValue>& regions)
{
    // RenderTexts don't have their own style, they just use their parent's style,
    // so we don't want to include them.
    if (isText())
        return;
        
    addDashboardRegions (regions);
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        curr->collectDashboardRegions(regions);
    }
}

#endif

void RenderObject::collectBorders(QValueList<CollapsedBorderValue>& borderStyles)
{
    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling())
        curr->collectBorders(borderStyles);
}

bool RenderObject::avoidsFloats() const
{
    return isReplaced() || isTable() || hasOverflowClip() || isHR() || isFlexibleBox(); 
}

bool RenderObject::usesLineWidth() const
{
    // 1. All auto-width objects that avoid floats should always use lineWidth
    // 2. For objects with a specified width, we match WinIE's behavior:
    // (a) tables use contentWidth
    // (b) <hr>s use lineWidth
    // (c) all other objects use lineWidth in quirks mode and contentWidth in strict mode.
    return (avoidsFloats() && (style()->width().isVariable() || isHR() || (style()->htmlHacks() && !isTable())));
}

QChar RenderObject::backslashAsCurrencySymbol() const
{
#if !APPLE_CHANGES
    return '\\';
#else
    NodeImpl *node = element();
    if (!node)
        return '\\';
    DocumentImpl *document = node->getDocument();
    if (!document)
        return '\\';
    Decoder *decoder = document->decoder();
    if (!decoder)
        return '\\';
    const QTextCodec *codec = decoder->codec();
    if (!codec)
        return '\\';
    return codec->backslashAsCurrencySymbol();
#endif
}

void RenderObject::setPixmap(const QPixmap&, const QRect&, CachedImage *image)
{
    // Repaint when the background image finishes loading.
    // This is needed for RenderBox objects, and also for table objects that hold
    // backgrounds that are then respected by the table cells (which are RenderBox
    // subclasses). It would be even better to find a more elegant way of doing this that
    // would avoid putting this function and the CachedObjectClient base class into RenderObject.

    if (image && image->pixmap_size() == image->valid_rect().size() && parent()) {
        if (canvas() && element() && (element()->id() == ID_HTML || element()->id() == ID_BODY))
            canvas()->repaint();    // repaint the entire canvas since the background gets propagated up
        else
            repaint();              // repaint object, which is a box or a container with boxes inside it
    }
}

int RenderObject::maximalOutlineSize(PaintAction p) const
{
    if (p != PaintActionOutline)
        return 0;
    return static_cast<RenderCanvas*>(document()->renderer())->maximalOutlineSize();
}

long RenderObject::caretMinOffset() const
{
    return 0;
}

long RenderObject::caretMaxOffset() const
{
    return 0;
}

unsigned long RenderObject::caretMaxRenderedOffset() const
{
    return 0;
}

long RenderObject::previousOffset (long current) const
{
    long previousOffset = current - 1;
    return previousOffset;
}

long RenderObject::nextOffset (long current) const
{
    long nextOffset = current + 1;
    return nextOffset;
}

InlineBox *RenderObject::inlineBox(long offset, EAffinity affinity)
{
    return inlineBoxWrapper();
}
