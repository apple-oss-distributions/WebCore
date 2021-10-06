/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999 Lars Knoll (knoll@kde.org)
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
 */

//#define TABLE_DEBUG
//#define TABLE_PRINT
//#define DEBUG_LAYOUT
//#define BOX_DEBUG
#include "rendering/render_table.h"
#include "rendering/table_layout.h"
#include "html/html_tableimpl.h"
#include "misc/htmltags.h"
#include "misc/htmlattrs.h"
#include "xml/dom_docimpl.h"

#include <kglobal.h>

#include <qapplication.h>
#include <qstyle.h>

#include <kdebug.h>
#include <assert.h>

#include "khtmlview.h"

using namespace khtml;
using namespace DOM;

RenderTable::RenderTable(DOM::NodeImpl* node)
    : RenderBlock(node)
{

    tCaption = 0;
    head = foot = firstBody = 0;
    tableLayout = 0;
    m_currentBorder = 0;
    
    rules = None;
    frame = Void;
    has_col_elems = false;
    hspacing = 0;
    vspacing = 0;
    padding = 0;
    needSectionRecalc = false;
    padding = 0;

    columnPos.resize( 2 );
    columnPos.fill( 0 );
    columns.resize( 1 );
    columns.fill( ColumnStruct() );

    columnPos[0] = 0;
}

RenderTable::~RenderTable()
{
    delete tableLayout;
}

void RenderTable::setStyle(RenderStyle *_style)
{
    ETableLayout oldTableLayout = style() ? style()->tableLayout() : TAUTO;
    if ( _style->display() == INLINE ) _style->setDisplay(INLINE_TABLE);
    if ( _style->display() != INLINE_TABLE ) _style->setDisplay(TABLE);
    RenderBlock::setStyle(_style);

    // init RenderObject attributes
    setInline(style()->display()==INLINE_TABLE && !isPositioned());
    setReplaced(style()->display()==INLINE_TABLE);

    // In the collapsed border model, there is no cell spacing.
    hspacing = collapseBorders() ? 0 : style()->horizontalBorderSpacing();
    vspacing = collapseBorders() ? 0 : style()->verticalBorderSpacing();
    columnPos[0] = hspacing;

    if ( !tableLayout || style()->tableLayout() != oldTableLayout ) {
	delete tableLayout;

        // According to the CSS2 spec, you only use fixed table layout if an
        // explicit width is specified on the table.  Auto width implies auto table layout.
	if (style()->tableLayout() == TFIXED && !style()->width().isVariable()) {
	    tableLayout = new FixedTableLayout(this);
#ifdef DEBUG_LAYOUT
	    kdDebug( 6040 ) << "using fixed table layout" << endl;
#endif
	} else
	    tableLayout = new AutoTableLayout(this);
    }
}

void RenderTable::addChild(RenderObject *child, RenderObject *beforeChild)
{
#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(Table)::addChild( " << child->renderName() << ", " <<
                       (beforeChild ? beforeChild->renderName() : "0") << " )" << endl;
#endif
    RenderObject *o = child;

    if (child->element() && child->element()->id() == ID_FORM) {
        RenderContainer::addChild(child,beforeChild);
        return;
    }

    switch(child->style()->display())
    {
    case TABLE_CAPTION:
        tCaption = static_cast<RenderBlock *>(child);
        break;
    case TABLE_COLUMN:
    case TABLE_COLUMN_GROUP:
	has_col_elems = true;
        break;
    case TABLE_HEADER_GROUP:
	if ( !head )
	    head = static_cast<RenderTableSection *>(child);
	else if ( !firstBody )
            firstBody = static_cast<RenderTableSection *>(child);
        break;
    case TABLE_FOOTER_GROUP:
	if ( !foot ) {
	    foot = static_cast<RenderTableSection *>(child);
	    break;
	}
	// fall through
    case TABLE_ROW_GROUP:
        if(!firstBody)
            firstBody = static_cast<RenderTableSection *>(child);
        break;
    default:
        if ( !beforeChild && lastChild() &&
	     lastChild()->isTableSection() && lastChild()->isAnonymous() ) {
            o = lastChild();
        } else {
	    RenderObject *lastBox = beforeChild;
	    while ( lastBox && lastBox->parent()->isAnonymous() &&
		    !lastBox->isTableSection() && lastBox->style()->display() != TABLE_CAPTION )
		lastBox = lastBox->parent();
	    if ( lastBox && lastBox->isAnonymous() ) {
		lastBox->addChild( child, beforeChild );
		return;
	    } else {
		if ( beforeChild && !beforeChild->isTableSection() )
		    beforeChild = 0;
  		//kdDebug( 6040 ) << this <<" creating anonymous table section beforeChild="<< beforeChild << endl;
		o = new (renderArena()) RenderTableSection(document() /* anonymous */);
		RenderStyle *newStyle = new RenderStyle();
		newStyle->inheritFrom(style());
                newStyle->setDisplay(TABLE_ROW_GROUP);
		o->setStyle(newStyle);
		addChild(o, beforeChild);
	    }
        }
        o->addChild(child);
        child->setNeedsLayoutAndMinMaxRecalc();
	return;
    }
    RenderContainer::addChild(child,beforeChild);
}



void RenderTable::calcWidth()
{
    if ( isPositioned() ) {
        calcAbsoluteHorizontal();
    }

    RenderBlock *cb = containingBlock();
    int availableWidth = cb->contentWidth();

    LengthType widthType = style()->width().type;
    if (widthType > Relative && style()->width().value > 0) {
	// Percent or fixed table
        m_width = style()->width().minWidth( availableWidth );
        if(m_minWidth > m_width) m_width = m_minWidth;
    }
    else {
        // An auto width table should shrink to fit within the line width if necessary in order to 
        // avoid overlapping floats.
        availableWidth = cb->lineWidth( m_y );
        
        // Subtract out any fixed margins from our available width for auto width tables.
        int marginTotal = 0;
        if (style()->marginLeft().type != Variable)
            marginTotal += style()->marginLeft().width(availableWidth);
        if (style()->marginRight().type != Variable)
            marginTotal += style()->marginRight().width(availableWidth);
            
        // Subtract out our margins to get the available content width.
        int availContentWidth = KMAX(0, availableWidth - marginTotal);
        
        // Ensure we aren't bigger than our max width or smaller than our min width.
        m_width = KMIN(short(availContentWidth),m_maxWidth);
    }
    
    m_width = KMAX(m_width, m_minWidth);

    // Finally, with our true width determined, compute our margins for real.
    m_marginRight = 0;
    m_marginLeft = 0;
    calcHorizontalMargins(style()->marginLeft(),style()->marginRight(),availableWidth);
}

void RenderTable::layout()
{
    KHTMLAssert( needsLayout() );
    KHTMLAssert( minMaxKnown() );
    KHTMLAssert( !needSectionRecalc );

    if (posChildNeedsLayout() && !normalChildNeedsLayout() && !selfNeedsLayout()) {
        // All we have to is lay out our positioned objects.
        layoutPositionedObjects(true);
        setNeedsLayout(false);
        return;
    }

#ifdef INCREMENTAL_REPAINTING
    QRect oldBounds, oldFullBounds;
    bool checkForRepaint = checkForRepaintDuringLayout();
    if (checkForRepaint)
        getAbsoluteRepaintRectIncludingFloats(oldBounds, oldFullBounds);
#endif
    
    //kdDebug( 6040 ) << renderName() << "(Table)"<< this << " ::layout0() width=" << width() << ", needsLayout=" << needsLayout() << endl;
    
    m_height = 0;
    initMaxMarginValues();
    
    //int oldWidth = m_width;
    calcWidth();

    // the optimisation below doesn't work since the internal table
    // layout could have changed.  we need to add a flag to the table
    // layout that tells us if something has changed in the min max
    // calculations to do it correctly.
//     if ( oldWidth != m_width || columns.size() + 1 != columnPos.size() )
    tableLayout->layout();

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(Table)::layout1() width=" << width() << ", marginLeft=" << marginLeft() << " marginRight=" << marginRight() << endl;
#endif

    setCellWidths();

    // layout child objects
    int calculatedHeight = 0;

    RenderObject *child = firstChild();
    while( child ) {
	if ( child->needsLayout() && !(child->element() && child->element()->id() == ID_FORM))
	    child->layout();
	if ( child->isTableSection() ) {
	    static_cast<RenderTableSection *>(child)->calcRowHeight();
	    calculatedHeight += static_cast<RenderTableSection *>(child)->layoutRows( 0 );
	}
	child = child->nextSibling();
    }

    // ### collapse caption margin
    if(tCaption && tCaption->style()->captionSide() != CAPBOTTOM) {
        tCaption->setPos(tCaption->marginLeft(), m_height);
        m_height += tCaption->height() + tCaption->marginTop() + tCaption->marginBottom();
    }

    m_height += borderTop();
    if (!collapseBorders())
        m_height += paddingTop();

    int oldHeight = m_height;
    calcHeight();
    int newHeight = m_height;
    m_height = oldHeight;

    // html tables with percent height are relative to view
    Length h = style()->height();
    int th=0;
    if (isPositioned())
        th = newHeight;
    else if (h.isFixed())
        th = h.value;
    else if (h.isPercent())
    {
        RenderObject* c = containingBlock();
        for ( ; 
	     !c->isCanvas() && !c->isBody() && !c->isTableCell() && !c->isPositioned() && !c->isFloating(); 
             c = c->containingBlock()) {
            Length ch = c->style()->height();
            if (ch.isFixed()) {
                th = h.width(ch.value);
                break;
            }
        }

        if (c->isTableCell()) {
            RenderTableCell* cell = static_cast<RenderTableCell*>(c);
            int cellHeight = cell->getCellPercentageHeight();
            if (cellHeight && cell->style()->height().isFixed())
                th = h.width(cellHeight);
        }
        else  {
            Length ch = c->style()->height();
            if (ch.isFixed())
                th = h.width(ch.value);
            else {
                // we need to substract out the margins of this block. -dwh
                th = h.width(viewRect().height() - c->marginBottom() - c->marginTop());
                // not really, but this way the view height change
                // gets propagated correctly
                setOverhangingContents();
            }
        }
    }

    // layout rows
    if ( th > calculatedHeight ) {
	// we have to redistribute that height to get the constraint correctly
	// just force the first body to the height needed
	// ### FIXME This should take height constraints on all table sections into account and distribute
	// accordingly. For now this should be good enough
        if (firstBody) {
            firstBody->calcRowHeight();
            firstBody->layoutRows( th - calculatedHeight );
        }
    }
    
    int bl = borderLeft();
    if (!collapseBorders())
        bl += paddingLeft();

    // position the table sections
    if ( head ) {
	head->setPos(bl, m_height);
	m_height += head->height();
    }
    RenderObject *body = firstBody;
    while ( body ) {
	if ( body != head && body != foot && body->isTableSection() ) {
	    body->setPos(bl, m_height);
	    m_height += body->height();
	}
	body = body->nextSibling();
    }
    if ( foot ) {
	foot->setPos(bl, m_height);
	m_height += foot->height();
    }


    m_height += borderBottom();
    if (!collapseBorders())
        m_height += paddingBottom();

    if(tCaption && tCaption->style()->captionSide()==CAPBOTTOM) {
        tCaption->setPos(tCaption->marginLeft(), m_height);
        m_height += tCaption->height() + tCaption->marginTop() + tCaption->marginBottom();
    }

    //kdDebug(0) << "table height: " << m_height << endl;

    // table can be containing block of positioned elements.
    // ### only pass true if width or height changed.
    layoutPositionedObjects( true );

#ifdef INCREMENTAL_REPAINTING
    // Repaint with our new bounds if they are different from our old bounds.
    if (checkForRepaint)
        repaintAfterLayoutIfNeeded(oldBounds, oldFullBounds);
#endif
    
    setNeedsLayout(false);
}

void RenderTable::setCellWidths()
{
#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(Table, this=0x" << this << ")::setCellWidths()" << endl;
#endif

    RenderObject *child = firstChild();
    while( child ) {
	if ( child->isTableSection() )
	    static_cast<RenderTableSection *>(child)->setCellWidths();
	child = child->nextSibling();
    }
}

void RenderTable::paint( QPainter *p, int _x, int _y,
                                  int _w, int _h, int _tx, int _ty, PaintAction paintAction)
{
    if (needsLayout())
        return;
        
    _tx += xPos();
    _ty += yPos();

#ifdef TABLE_PRINT
    kdDebug( 6040 ) << "RenderTable::paint() w/h = (" << width() << "/" << height() << ")" << endl;
#endif
    if (!overhangingContents() && !isRelPositioned() && !isPositioned())
    {
        int os = 2*maximalOutlineSize(paintAction);
        if((_ty >= _y + _h + os) || (_ty + height() <= _y - os)) return;
        if((_tx >= _x + _w + os) || (_tx + width() <= _x - os)) return;
    }

#ifdef TABLE_PRINT
    kdDebug( 6040 ) << "RenderTable::paint(2) " << _tx << "/" << _ty << " (" << _y << "/" << _h << ")" << endl;
#endif

    if ((paintAction == PaintActionElementBackground || paintAction == PaintActionChildBackground)
        && shouldPaintBackgroundOrBorder() && style()->visibility() == VISIBLE)
        paintBoxDecorations(p, _x, _y, _w, _h, _tx, _ty);

    // We're done.  We don't bother painting any children.
    if (paintAction == PaintActionElementBackground)
        return;
    // We don't paint our own background, but we do let the kids paint their backgrounds.
    if (paintAction == PaintActionChildBackgrounds)
        paintAction = PaintActionChildBackground;
    
    RenderObject *child = firstChild();
    while( child ) {
	if ( child->isTableSection() || child == tCaption )
	    child->paint( p, _x, _y, _w, _h, _tx, _ty, paintAction );
	child = child->nextSibling();
    }

    if (collapseBorders() && paintAction == PaintActionChildBackground && style()->visibility() == VISIBLE) {
        // Collect all the unique border styles that we want to paint in a sorted list.  Once we
        // have all the styles sorted, we then do individual passes, painting each style of border
        // from lowest precedence to highest precedence.
        QValueList<CollapsedBorderValue> borderStyles;
        collectBorders(borderStyles);
        QValueListIterator<CollapsedBorderValue> it = borderStyles.begin();
        QValueListIterator<CollapsedBorderValue> end = borderStyles.end();
        for (; it != end; ++it) {
            m_currentBorder = &(*it);
            for (RenderObject* child = firstChild(); child; child = child->nextSibling())
                if (child->isTableSection())
                    child->paint(p, _x, _y, _w, _h, _tx, _ty, PaintActionCollapsedTableBorders);
            }
        }
    }
        
#ifdef BOX_DEBUG
    outlineBox(p, _tx, _ty, "blue");
#endif

void RenderTable::paintBoxDecorations(QPainter *p,int _x, int _y,
                                      int _w, int _h, int _tx, int _ty)
{
    int w = width();
    int h = height() + borderTopExtra() + borderBottomExtra();
    _ty -= borderTopExtra();
    
    int my = kMax(_ty,_y);
    int mh;
    if (_ty<_y)
        mh= kMax(0,h-(_y-_ty));
    else
        mh = kMin(_h,h);
    
    paintBackground(p, style()->backgroundColor(), style()->backgroundImage(), my, mh, _tx, _ty, w, h);
    
    if (style()->hasBorder() && !collapseBorders())
        paintBorder(p, _tx, _ty, w, h, style());
}

void RenderTable::calcMinMaxWidth()
{
    KHTMLAssert( !minMaxKnown() );

    if ( needSectionRecalc )
	recalcSections();

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(Table " << this << ")::calcMinMaxWidth()" <<  endl;
#endif

    tableLayout->calcMinMaxWidth();

    if (tCaption && tCaption->minWidth() > m_minWidth)
        m_minWidth = tCaption->minWidth();

    setMinMaxKnown();
#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << " END: (Table " << this << ")::calcMinMaxWidth() min = " << m_minWidth << " max = " << m_maxWidth <<  endl;
#endif
}

void RenderTable::close()
{
//    kdDebug( 6040 ) << "RenderTable::close()" << endl;
    setNeedsLayoutAndMinMaxRecalc();
}

int RenderTable::borderTopExtra()
{
    if (tCaption && tCaption->style()->captionSide()!=CAPBOTTOM)
        return -(tCaption->height() + tCaption->marginBottom() +  tCaption->marginTop());
    else
        return 0;

}

int RenderTable::borderBottomExtra()
{
    if (tCaption && tCaption->style()->captionSide()==CAPBOTTOM)
        return -(tCaption->height() + tCaption->marginBottom() +  tCaption->marginTop());
    else
        return 0;
}


void RenderTable::splitColumn( int pos, int firstSpan )
{
    // we need to add a new columnStruct
    int oldSize = columns.size();
    columns.resize( oldSize + 1 );
    int oldSpan = columns[pos].span;
//     qDebug("splitColumn( %d,%d ), oldSize=%d, oldSpan=%d", pos, firstSpan, oldSize, oldSpan );
    KHTMLAssert( oldSpan > firstSpan );
    columns[pos].span = firstSpan;
    memmove( columns.data()+pos+1, columns.data()+pos, (oldSize-pos)*sizeof(ColumnStruct) );
    columns[pos+1].span = oldSpan - firstSpan;

    // change width of all rows.
    RenderObject *child = firstChild();
    while ( child ) {
	if ( child->isTableSection() ) {
	    RenderTableSection *section = static_cast<RenderTableSection *>(child);
	    int size = section->grid.size();
	    int row = 0;
	    if ( section->cCol > pos )
		section->cCol++;
	    while ( row < size ) {
		section->grid[row].row->resize( oldSize+1 );
		RenderTableSection::Row &r = *section->grid[row].row;
		memmove( r.data()+pos+1, r.data()+pos, (oldSize-pos)*sizeof( RenderTableCell * ) );
// 		qDebug("moving from %d to %d, num=%d", pos, pos+1, (oldSize-pos-1) );
		r[pos+1] = r[pos] ? (RenderTableCell *)-1 : 0;
		row++;
	    }
	}
	child = child->nextSibling();
    }
    columnPos.resize( numEffCols()+1 );
    setNeedsLayoutAndMinMaxRecalc();
}

void RenderTable::appendColumn( int span )
{
    // easy case.
    int pos = columns.size();
//     qDebug("appendColumn( %d ), size=%d", span, pos );
    int newSize = pos + 1;
    columns.resize( newSize );
    columns[pos].span = span;
    //qDebug("appending column at %d, span %d", pos,  span );

    // change width of all rows.
    RenderObject *child = firstChild();
    while ( child ) {
	if ( child->isTableSection() ) {
	    RenderTableSection *section = static_cast<RenderTableSection *>(child);
	    int size = section->grid.size();
	    int row = 0;
	    while ( row < size ) {
		section->grid[row].row->resize( newSize );
		section->cellAt( row, pos ) = 0;
		row++;
	    }

	}
	child = child->nextSibling();
    }
    columnPos.resize( numEffCols()+1 );
    setNeedsLayoutAndMinMaxRecalc();
}

RenderTableCol *RenderTable::colElement( int col ) {
    if ( !has_col_elems )
	return 0;
    RenderObject *child = firstChild();
    int cCol = 0;
    while ( child ) {
	if ( child->isTableCol() ) {
	    RenderTableCol *colElem = static_cast<RenderTableCol *>(child);
	    int span = colElem->span();
	    if ( !colElem->firstChild() ) {
		cCol += span;
		if ( cCol > col )
		    return colElem;
	    }

	    RenderObject *next = child->firstChild();
	    if ( !next )
		next = child->nextSibling();
	    if ( !next && child->parent()->isTableCol() )
		next = child->parent()->nextSibling();
	    child = next;
	} else
	    break;
    }
    return 0;
}

void RenderTable::recalcSections()
{
    tCaption = 0;
    head = foot = firstBody = 0;
    has_col_elems = false;

    RenderObject *child = firstChild();
    // We need to get valid pointers to caption, head, foot and firstbody again
    while (child) {
	switch (child->style()->display()) {
	case TABLE_CAPTION:
	    if (!tCaption) {
		tCaption = static_cast<RenderBlock*>(child);
                tCaption->setNeedsLayout(true);
            }
	    break;
	case TABLE_COLUMN:
	case TABLE_COLUMN_GROUP:
	    has_col_elems = true;
	    break;
	case TABLE_HEADER_GROUP: {
	    RenderTableSection *section = static_cast<RenderTableSection *>(child);
	    if ( !head )
		head = section;
	    else if ( !firstBody )
		firstBody = section;
	    if ( section->needCellRecalc )
		section->recalcCells();
	    break;
	}
	case TABLE_FOOTER_GROUP: {
	    RenderTableSection *section = static_cast<RenderTableSection *>(child);
	    if ( !foot )
		foot = section;
	    else if ( !firstBody )
		firstBody = section;
	    if ( section->needCellRecalc )
		section->recalcCells();
	    break;
	}
	case TABLE_ROW_GROUP: {
	    RenderTableSection *section = static_cast<RenderTableSection *>(child);
	    if ( !firstBody )
		firstBody = section;
	    if ( section->needCellRecalc )
		section->recalcCells();
	}
	default:
	    break;
	}
	child = child->nextSibling();
    }
    needSectionRecalc = false;
    setNeedsLayout(true);
}

RenderObject* RenderTable::removeChildNode(RenderObject* child)
{
    setNeedSectionRecalc();
    return RenderContainer::removeChildNode( child );
}

int RenderTable::borderLeft() const
{
    if (collapseBorders()) {
        // FIXME: For strict mode, returning 0 is correct, since the table border half spills into the margin,
        // but I'm working to get this changed.  For now, follow the spec.
        return 0;
    }
    return RenderBlock::borderLeft();
}
    
int RenderTable::borderRight() const
{
    if (collapseBorders()) {
        // FIXME: For strict mode, returning 0 is correct, since the table border half spills into the margin,
        // but I'm working to get this changed.  For now, follow the spec.
        return 0;
    }
    return RenderBlock::borderRight();
}

int RenderTable::borderTop() const
{
    if (collapseBorders()) {
        // FIXME: For strict mode, returning 0 is correct, since the table border half spills into the margin,
        // but I'm working to get this changed.  For now, follow the spec.
        return 0;
    }
    return RenderBlock::borderTop();
}

int RenderTable::borderBottom() const
{
    if (collapseBorders()) {
        // FIXME: For strict mode, returning 0 is correct, since the table border half spills into the margin,
        // but I'm working to get this changed.  For now, follow the spec.
        return 0;
    }
    return RenderBlock::borderBottom();
}

RenderTableCell* RenderTable::cellAbove(const RenderTableCell* cell) const
{
    // Find the section and row to look in
    int r = cell->row();
    RenderTableSection* section = 0;
    int rAbove = -1;
    if (r > 0) {
        // cell is not in the first row, so use the above row in its own section
        section = cell->section();
        rAbove = r-1;
    } else {
        // cell is at top of a section, use last row in previous section
        for (RenderObject *prevSection = cell->section()->previousSibling();
             prevSection && rAbove < 0;
             prevSection = prevSection->previousSibling()) {
            if (prevSection->isTableSection()) {
                section = static_cast<RenderTableSection *>(prevSection);
                if (section->numRows() > 0)
                    rAbove = section->numRows()-1;
            }
        }
    }

    // Look up the cell in the section's grid, which requires effective col index
    if (section && rAbove >= 0) {
        int effCol = colToEffCol(cell->col());
        RenderTableCell* aboveCell;
        // If we hit a span back up to a real cell.
        do {
            aboveCell = section->cellAt(rAbove, effCol);
            effCol--;
        } while (aboveCell == (RenderTableCell *)-1 && effCol >=0);
        return (aboveCell == (RenderTableCell *)-1) ? 0 : aboveCell;
    } else {
        return 0;
    }
}

RenderTableCell* RenderTable::cellBelow(const RenderTableCell* cell) const
{
    // Find the section and row to look in
    int r = cell->row() + cell->rowSpan() - 1;
    RenderTableSection* section = 0;
    int rBelow = -1;
    if (r < cell->section()->numRows()-1) {
        // The cell is not in the last row, so use the next row in the section.
        section = cell->section();
        rBelow= r+1;
    } else {
        // The cell is at the bottom of a section. Use the first row in the next section.
        for (RenderObject* nextSection = cell->section()->nextSibling();
             nextSection && rBelow < 0;
             nextSection = nextSection->nextSibling()) 
        {
            if (nextSection->isTableSection()) {
                section = static_cast<RenderTableSection *>(nextSection);
                if (section->numRows() > 0)
                    rBelow = 0;
            }
        }
    }
    
    // Look up the cell in the section's grid, which requires effective col index
    if (section && rBelow >= 0) {
        int effCol = colToEffCol(cell->col());
        RenderTableCell* belowCell;
        // If we hit a colspan back up to a real cell.
        do {
            belowCell = section->cellAt(rBelow, effCol);
            effCol--;
        } while (belowCell == (RenderTableCell *)-1 && effCol >=0);
        return (belowCell == (RenderTableCell *)-1) ? 0 : belowCell;
    } else {
        return 0;
    }    
}

RenderTableCell* RenderTable::cellLeft(const RenderTableCell* cell) const
{
    RenderTableSection* section = cell->section();
    int effCol = colToEffCol(cell->col());
    if (effCol == 0)
        return 0;
    
    // If we hit a colspan back up to a real cell.
    RenderTableCell* prevCell;
    do {
        prevCell = section->cellAt(cell->row(), effCol-1);
        effCol--;
    } while (prevCell == (RenderTableCell *)-1 && effCol >=0);
    return (prevCell == (RenderTableCell *)-1) ? 0 : prevCell;
}

RenderTableCell* RenderTable::cellRight(const RenderTableCell* cell) const
{
    int effCol = colToEffCol(cell->col()+cell->colSpan());
    if (effCol >= numEffCols())
        return 0;
    RenderTableCell* result = cell->section()->cellAt(cell->row(), effCol);
    return (result == (RenderTableCell*)-1) ? 0 : result;
}

RenderBlock* RenderTable::firstLineBlock() const
{
    return 0;
}

void RenderTable::updateFirstLetter()
{}

#ifndef NDEBUG
void RenderTable::dump(QTextStream *stream, QString ind) const
{
    if (tCaption)
	*stream << " tCaption";
    if (head)
	*stream << " head";
    if (foot)
	*stream << " foot";

    *stream << endl << ind << "cspans:";
    for ( unsigned int i = 0; i < columns.size(); i++ )
	*stream << " " << columns[i].span;
    *stream << endl << ind;

    RenderBlock::dump(stream,ind);
}
#endif

// --------------------------------------------------------------------------

RenderTableSection::RenderTableSection(DOM::NodeImpl* node)
    : RenderBox(node)
{
    // init RenderObject attributes
    setInline(false);   // our object is not Inline
    cCol = 0;
    cRow = -1;
    needCellRecalc = false;
}

RenderTableSection::~RenderTableSection()
{
    clearGrid();
}

void RenderTableSection::detach()
{
    // recalc cell info because RenderTable has unguarded pointers
    // stored that point to this RenderTableSection.
    if (table())
        table()->setNeedSectionRecalc();

    RenderBox::detach();
}

void RenderTableSection::setStyle(RenderStyle* _style)
{
    // we don't allow changing this one
    if (style())
        _style->setDisplay(style()->display());
    else if (_style->display() != TABLE_FOOTER_GROUP && _style->display() != TABLE_HEADER_GROUP)
        _style->setDisplay(TABLE_ROW_GROUP);

    RenderBox::setStyle(_style);
}

void RenderTableSection::addChild(RenderObject *child, RenderObject *beforeChild)
{
#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(TableSection)::addChild( " << child->renderName()  << ", beforeChild=" <<
                       (beforeChild ? beforeChild->renderName() : "0") << " )" << endl;
#endif
    RenderObject *row = child;

    if (child->element() && child->element()->id() == ID_FORM) {
        RenderContainer::addChild(child,beforeChild);
        return;
    }
    
    if ( !child->isTableRow() ) {

        if( !beforeChild )
            beforeChild = lastChild();

        if( beforeChild && beforeChild->isAnonymous() )
            row = beforeChild;
        else {
	    RenderObject *lastBox = beforeChild;
	    while ( lastBox && lastBox->parent()->isAnonymous() && !lastBox->isTableRow() )
		lastBox = lastBox->parent();
	    if ( lastBox && lastBox->isAnonymous() ) {
		lastBox->addChild( child, beforeChild );
		return;
	    } else {
		//kdDebug( 6040 ) << "creating anonymous table row" << endl;
		row = new (renderArena()) RenderTableRow(document() /* anonymous table */);
		RenderStyle *newStyle = new RenderStyle();
		newStyle->inheritFrom(style());
		newStyle->setDisplay( TABLE_ROW );
		row->setStyle(newStyle);
		addChild(row, beforeChild);
	    }
        }
        row->addChild(child);
        child->setNeedsLayoutAndMinMaxRecalc();
        return;
    }

    if (beforeChild)
	setNeedCellRecalc();

    cRow++;
    cCol = 0;

    ensureRows( cRow+1 );

    if (!beforeChild) {
        grid[cRow].height = child->style()->height();
        if ( grid[cRow].height.type == Relative )
            grid[cRow].height = Length();
    }


    RenderContainer::addChild(child,beforeChild);
}

void RenderTableSection::ensureRows( int numRows )
{
    int nRows = grid.size();
    int nCols = table()->numEffCols();
    if ( numRows > nRows ) {
	grid.resize( numRows );
	for ( int r = nRows; r < numRows; r++ ) {
	    grid[r].row = new Row( nCols );
	    grid[r].row->fill( 0 );
	    grid[r].baseLine = 0;
	    grid[r].height = Length();
	}
    }

}

void RenderTableSection::addCell( RenderTableCell *cell )
{
    int rSpan = cell->rowSpan();
    int cSpan = cell->colSpan();
    QMemArray<RenderTable::ColumnStruct> &columns = table()->columns;
    int nCols = columns.size();

    // ### mozilla still seems to do the old HTML way, even for strict DTD
    // (see the annotation on table cell layouting in the CSS specs and the testcase below:
    // <TABLE border>
    // <TR><TD>1 <TD rowspan="2">2 <TD>3 <TD>4
    // <TR><TD colspan="2">5
    // </TABLE>
#if 0
    // find empty space for the cell
    bool found = false;
    while ( !found ) {
	found = true;
	while ( cCol < nCols && cellAt( cRow, cCol ) )
	    cCol++;
	int pos = cCol;
	int span = 0;
	while ( pos < nCols && span < cSpan ) {
	    if ( cellAt( cRow, pos ) ) {
		found = false;
		cCol = pos;
		break;
	    }
	    span += columns[pos].span;
	    pos++;
	}
    }
#else
    while ( cCol < nCols && cellAt( cRow, cCol ) )
	cCol++;
#endif

//       qDebug("adding cell at %d/%d span=(%d/%d)",  cRow, cCol, rSpan, cSpan );

    if ( rSpan == 1 ) {
	// we ignore height settings on rowspan cells
	Length height = cell->style()->height();
	if ( height.value > 0 || (height.type == Relative && height.value >= 0) ) {
	    Length cRowHeight = grid[cRow].height;
	    switch( height.type ) {
	    case Percent:
		if ( !(cRowHeight.type == Percent) ||
		     ( cRowHeight.type == Percent && cRowHeight.value < height.value ) )
		    grid[cRow].height = height;
		     break;
	    case Fixed:
		if ( cRowHeight.type < Percent ||
		     ( cRowHeight.type == Fixed && cRowHeight.value < height.value ) )
		    grid[cRow].height = height;
		break;
	    case Relative:
#if 0
		// we treat this as variable. This is correct according to HTML4, as it only specifies length for the height.
		if ( cRowHeight.type == Variable ||
		     ( cRowHeight.type == Relative && cRowHeight.value < height.value ) )
		     grid[cRow].height = height;
		     break;
#endif
	    default:
		break;
	    }
	}
    }

    // make sure we have enough rows
    ensureRows( cRow + rSpan );

    int col = cCol;
    // tell the cell where it is
    RenderTableCell *set = cell;
    while ( cSpan ) {
	int currentSpan;
	if ( cCol >= nCols ) {
	    table()->appendColumn( cSpan );
	    currentSpan = cSpan;
	} else {
	    if ( cSpan < columns[cCol].span )
		table()->splitColumn( cCol, cSpan );
	    currentSpan = columns[cCol].span;
	}
	int r = 0;
	while ( r < rSpan ) {
	    if ( !cellAt( cRow + r, cCol ) ) {
// 		qDebug("    adding cell at %d, %d",  cRow + r, cCol );
		cellAt( cRow + r, cCol ) = set;
	    }
	    r++;
	}
	cCol++;
	cSpan -= currentSpan;
	set = (RenderTableCell *)-1;
    }
    if ( cell ) {
	cell->setRow( cRow );
	cell->setCol( table()->effColToCol( col ) );
    }
}



void RenderTableSection::setCellWidths()
{
#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(Table, this=0x" << this << ")::setCellWidths()" << endl;
#endif
    QMemArray<int> &columnPos = table()->columnPos;

    int rows = grid.size();
    for ( int i = 0; i < rows; i++ ) {
	Row &row = *grid[i].row;
	int cols = row.size();
	for ( int j = 0; j < cols; j++ ) {
	    RenderTableCell *cell = row[j];
// 	    qDebug("cell[%d,%d] = %p", i, j, cell );
	    if ( !cell || cell == (RenderTableCell *)-1 )
		continue;
	    int endCol = j;
	    int cspan = cell->colSpan();
	    while ( cspan && endCol < cols ) {
		cspan -= table()->columns[endCol].span;
		endCol++;
	    }
	    int w = columnPos[endCol] - columnPos[j] - table()->hBorderSpacing();
#ifdef DEBUG_LAYOUT
	    kdDebug( 6040 ) << "setting width of cell " << cell << " " << cell->row() << "/" << cell->col() << " to " << w << " colspan=" << cell->colSpan() << " start=" << j << " end=" << endCol << endl;
#endif
	    int oldWidth = cell->width();
	    if ( w != oldWidth ) {
		cell->setNeedsLayout(true);
		cell->setWidth( w );
	    }
	}
    }
}


void RenderTableSection::calcRowHeight()
{
    int indx;
    RenderTableCell *cell;

    int totalRows = grid.size();
    int spacing = table()->vBorderSpacing();

    rowPos.resize( totalRows + 1 );
    rowPos[0] = spacing;

    for ( int r = 0; r < totalRows; r++ ) {
	rowPos[r+1] = 0;

	int baseline=0;
	int bdesc = 0;
// 	qDebug("height of row %d is %d/%d", r, grid[r].height.value, grid[r].height.type );
	int ch = grid[r].height.minWidth( 0 );
	int pos = rowPos[ r+1 ] + ch + spacing;

	if ( pos > rowPos[r+1] )
	    rowPos[r+1] = pos;

	Row *row = grid[r].row;
	int totalCols = row->size();
	int totalRows = grid.size();

	for ( int c = 0; c < totalCols; c++ ) {
	    cell = cellAt(r, c);
	    if ( !cell || cell == (RenderTableCell *)-1 )
		continue;
	    if ( r < totalRows - 1 && cellAt(r+1, c) == cell )
		continue;

	    if ( ( indx = r - cell->rowSpan() + 1 ) < 0 )
		indx = 0;

            if (cell->getCellPercentageHeight()) {
                cell->setCellPercentageHeight(0);
                cell->setChildNeedsLayout(true, false);
                cell->layoutIfNeeded();
            }
            
            // Explicit heights use the border box in quirks mode.  In strict mode do the right
            // thing and actually add in the border and padding.
	    ch = cell->style()->height().width(0) + 
                (cell->style()->htmlHacks() ? 0 : (cell->paddingTop() + cell->paddingBottom() +
                                                   cell->borderTop() + cell->borderBottom()));
	    if (cell->height() > ch)
		ch = cell->height();

            pos = rowPos[ indx ] + ch + spacing;

	    if ( pos > rowPos[r+1] )
		rowPos[r+1] = pos;

	    // find out the baseline
	    EVerticalAlign va = cell->style()->verticalAlign();
	    if (va == BASELINE || va == TEXT_BOTTOM || va == TEXT_TOP
		|| va == SUPER || va == SUB)
	    {
		int b=cell->baselinePosition();

		if (b>baseline)
		    baseline=b;

		int td = rowPos[ indx ] + ch - b;
		if (td>bdesc)
		    bdesc = td;
	    }
	}

	//do we have baseline aligned elements?
	if (baseline) {
	    // increase rowheight if baseline requires
	    int bRowPos = baseline + bdesc  + spacing ; // + 2*padding
	    if (rowPos[r+1]<bRowPos)
		rowPos[r+1]=bRowPos;

	    grid[r].baseLine = baseline;
	}

	if ( rowPos[r+1] < rowPos[r] )
	    rowPos[r+1] = rowPos[r];
//   	qDebug("rowpos(%d)=%d",  r, rowPos[r] );
    }
}

int RenderTableSection::layoutRows( int toAdd )
{
    int rHeight;
    int rindx;
    int totalRows = grid.size();
    int hspacing = table()->hBorderSpacing();
    int vspacing = table()->vBorderSpacing();
    
    if (toAdd && totalRows && (rowPos[totalRows] || !nextSibling())) {

	int totalHeight = rowPos[totalRows] + toAdd;
// 	qDebug("layoutRows: totalHeight = %d",  totalHeight );

        int dh = toAdd;
	int totalPercent = 0;
	int numVariable = 0;
	for ( int r = 0; r < totalRows; r++ ) {
	    if ( grid[r].height.type == Variable )
		numVariable++;
	    else if ( grid[r].height.type == Percent )
		totalPercent += grid[r].height.value;
	}
	if ( totalPercent ) {
// 	    qDebug("distributing %d over percent rows totalPercent=%d", dh,  totalPercent );
	    // try to satisfy percent
	    int add = 0;
	    if ( totalPercent > 100 )
		totalPercent = 100;
	    int rh = rowPos[1]-rowPos[0];
	    for ( int r = 0; r < totalRows; r++ ) {
		if ( totalPercent > 0 && grid[r].height.type == Percent ) {
		    int toAdd = QMIN(dh, (totalHeight * grid[r].height.value / 100)-rh);
                    // If toAdd is negative, then we don't want to shrink the row (this bug
                    // affected Outlook Web Access).
                    toAdd = QMAX(0, toAdd);
		    add += toAdd;
		    dh -= toAdd;
		    totalPercent -= grid[r].height.value;
// 		    qDebug( "adding %d to row %d", toAdd, r );
		}
		if ( r < totalRows-1 )
		    rh = rowPos[r+2] - rowPos[r+1];
                rowPos[r+1] += add;
	    }
	}
	if ( numVariable ) {
	    // distribute over variable cols
// 	    qDebug("distributing %d over variable rows numVariable=%d", dh,  numVariable );
	    int add = 0;
	    for ( int r = 0; r < totalRows; r++ ) {
		if ( numVariable > 0 && grid[r].height.type == Variable ) {
		    int toAdd = dh/numVariable;
		    add += toAdd;
		    dh -= toAdd;
		}
                rowPos[r+1] += add;
	    }
	}
        if (dh>0) {
	    // if some left overs, distribute equally.
            int tot=rowPos[totalRows];
            int add=0;
            int prev=rowPos[0];
            for ( int r = 0; r < totalRows; r++ ) {
                //weight with the original height
                add+=dh*(rowPos[r+1]-prev)/tot;
                prev=rowPos[r+1];
                rowPos[r+1]+=add;
            }
        }
    }

    int leftOffset = hspacing;

    int nEffCols = table()->numEffCols();
    for ( int r = 0; r < totalRows; r++ )
    {
	Row *row = grid[r].row;
	int totalCols = row->size();
        for ( int c = 0; c < nEffCols; c++ )
        {
            RenderTableCell *cell = cellAt(r, c);
            if (!cell || cell == (RenderTableCell *)-1 )
                continue;
            if ( r < totalRows - 1 && cell == cellAt(r+1, c) )
		continue;

            if ( ( rindx = r-cell->rowSpan()+1 ) < 0 )
                rindx = 0;

            rHeight = rowPos[r+1] - rowPos[rindx] - vspacing;
            
            // Force percent height children to lay themselves out again.
            // This will cause, e.g., textareas to grow to
            // fill the area.  FIXME: <div>s and blocks still don't
            // work right.  We'll need to have an efficient way of
            // invalidating all percent height objects in a render subtree.
            // For now, we just handle immediate children. -dwh
            bool cellChildrenFlex = false;
            RenderObject* o = cell->firstChild();
            while (o) {
                if (o->style()->height().isPercent()) {
                    o->setNeedsLayout(true, false);
                    cell->setChildNeedsLayout(true, false);
                    cellChildrenFlex = true;
                }
                o = o->nextSibling();
            }
            if (cellChildrenFlex) {
                cell->setCellPercentageHeight(rHeight);
                cell->layoutIfNeeded();
           
                // Alignment within a cell is based off the calculated
                // height, which becomes irrelevant once the cell has
                // been resized based off its percentage. -dwh
                cell->setCellTopExtra(0);
                cell->setCellBottomExtra(0);
            }
            else {
#ifdef DEBUG_LAYOUT
            kdDebug( 6040 ) << "setting position " << r << "/" << c << ": "
			    << table()->columnPos[c] /*+ padding */ << "/" << rowPos[rindx] << " height=" << rHeight<< endl;
#endif

            EVerticalAlign va = cell->style()->verticalAlign();
            int te=0;
            switch (va)
            {
            case SUB:
            case SUPER:
            case TEXT_TOP:
            case TEXT_BOTTOM:
            case BASELINE:
		te = getBaseline(r) - cell->baselinePosition() ;
                break;
            case TOP:
                te = 0;
                break;
            case MIDDLE:
                te = (rHeight - cell->height())/2;
                break;
            case BOTTOM:
                te = rHeight - cell->height();
                break;
            default:
                break;
            }
#ifdef DEBUG_LAYOUT
	    //            kdDebug( 6040 ) << "CELL " << cell << " te=" << te << ", be=" << rHeight - cell->height() - te << ", rHeight=" << rHeight << ", valign=" << va << endl;
#endif
            cell->setCellTopExtra( te );
            cell->setCellBottomExtra( rHeight - cell->height() - te);
            }
            
#ifdef INCREMENTAL_REPAINTING
            int oldCellX = cell->xPos();
            int oldCellY = cell->yPos();
#endif
        
            if (style()->direction()==RTL) {
                cell->setPos(
		    table()->columnPos[(int)totalCols] -
		    table()->columnPos[table()->colToEffCol(cell->col()+cell->colSpan())] +
		    leftOffset,
                    rowPos[rindx] );
            } else {
                cell->setPos( table()->columnPos[c] + leftOffset, rowPos[rindx] );
	    }

#ifdef INCREMENTAL_REPAINTING
            // If the cell moved, we have to repaint it as well as any floating/positioned
            // descendants.  An exception is if we need a layout.  In this case, we know we're going to
            // repaint ourselves (and the cell) anyway.
            if (!table()->selfNeedsLayout() && checkForRepaintDuringLayout())
                cell->repaintDuringLayoutIfMoved(oldCellX, oldCellY);
#endif
        }
    }

    m_height = rowPos[totalRows];
    return m_height;
}


void RenderTableSection::paint( QPainter *p, int x, int y, int w, int h,
				int tx, int ty, PaintAction paintAction)
{
    unsigned int totalRows = grid.size();
    unsigned int totalCols = table()->columns.size();

    tx += m_x;
    ty += m_y;

    // check which rows and cols are visible and only paint these
    // ### fixme: could use a binary search here
    int os = 2*maximalOutlineSize(paintAction);
    unsigned int startrow = 0;
    unsigned int endrow = totalRows;
    for ( ; startrow < totalRows; startrow++ ) {
	if ( ty + rowPos[startrow+1] >= y - os)
	    break;
    }
    for ( ; endrow > 0; endrow-- ) {
	if ( ty + rowPos[endrow-1] <= y + h + os)
	    break;
    }
    unsigned int startcol = 0;
    unsigned int endcol = totalCols;
    if ( style()->direction() == LTR ) {
	for ( ; startcol < totalCols; startcol++ ) {
	    if ( tx + table()->columnPos[startcol+1] >= x - os)
		break;
	}
	for ( ; endcol > 0; endcol-- ) {
	    if ( tx + table()->columnPos[endcol-1] <= x + w + os)
		break;
	}
    }
    
    if ( startcol < endcol ) {
	// draw the cells
	for ( unsigned int r = startrow; r < endrow; r++ ) {
	    unsigned int c = startcol;
	    // since a cell can be -1 (indicating a colspan) we might have to search backwards to include it
	    while ( c && cellAt( r, c ) == (RenderTableCell *)-1 )
		c--;
	    for ( ; c < endcol; c++ ) {
		RenderTableCell *cell = cellAt(r, c);
		if (!cell || cell == (RenderTableCell *)-1 )
		    continue;
		if ( (r < endrow - 1) && (cellAt(r+1, c) == cell) )
		    continue;

#ifdef TABLE_PRINT
		kdDebug( 6040 ) << "painting cell " << r << "/" << c << endl;
#endif
		cell->paint( p, x, y, w, h, tx, ty, paintAction);
	    }
	}
    }
}

void RenderTableSection::recalcCells()
{
    cCol = 0;
    cRow = -1;
    clearGrid();
    grid.resize( 0 );

    RenderObject *row = firstChild();
    while ( row ) {
	cRow++;
	cCol = 0;
	ensureRows( cRow+1 );
	RenderObject *cell = row->firstChild();
	while ( cell ) {
	    if ( cell->isTableCell() )
		addCell( static_cast<RenderTableCell *>(cell) );
	    cell = cell->nextSibling();
	}
	row = row->nextSibling();
    }
    needCellRecalc = false;
    setNeedsLayout(true);
}

void RenderTableSection::clearGrid()
{
    int rows = grid.size();
    while ( rows-- ) {
	delete grid[rows].row;
    }
}

RenderObject* RenderTableSection::removeChildNode(RenderObject* child)
{
    setNeedCellRecalc();
    return RenderContainer::removeChildNode( child );
}

#ifndef NDEBUG
void RenderTableSection::dump(QTextStream *stream, QString ind) const
{
    *stream << endl << ind << "grid=(" << grid.size() << "," << table()->numEffCols() << ")" << endl << ind;
    for ( unsigned int r = 0; r < grid.size(); r++ ) {
	for ( int c = 0; c < table()->numEffCols(); c++ ) {
	    if ( cellAt( r,  c ) && cellAt( r, c ) != (RenderTableCell *)-1 )
		*stream << "(" << cellAt( r, c )->row() << "," << cellAt( r, c )->col() << ","
			<< cellAt(r, c)->rowSpan() << "," << cellAt(r, c)->colSpan() << ") ";
	    else
		*stream << cellAt( r, c ) << "null cell ";
	}
	*stream << endl << ind;
    }
    RenderContainer::dump(stream,ind);
}
#endif

// -------------------------------------------------------------------------

RenderTableRow::RenderTableRow(DOM::NodeImpl* node)
    : RenderContainer(node)
{
    // init RenderObject attributes
    setInline(false);   // our object is not Inline
}

void RenderTableRow::detach()
{
    RenderTableSection *s = section();
    if (s) {
        s->setNeedCellRecalc();
    }
    RenderContainer::detach();
}

void RenderTableRow::setStyle(RenderStyle* style)
{
    style->setDisplay(TABLE_ROW);
    RenderContainer::setStyle(style);
}

void RenderTableRow::addChild(RenderObject *child, RenderObject *beforeChild)
{
#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(TableRow)::addChild( " << child->renderName() << " )"  << ", " <<
                       (beforeChild ? beforeChild->renderName() : "0") << " )" << endl;
#endif
    if (child->element() && child->element()->id() == ID_FORM) {
        RenderContainer::addChild(child,beforeChild);
        return;
    }

    RenderTableCell *cell;

    if ( !child->isTableCell() ) {
	RenderObject *last = beforeChild;
        if ( !last )
            last = lastChild();
        RenderTableCell *cell = 0;
        if( last && last->isAnonymous() && last->isTableCell() )
            cell = static_cast<RenderTableCell *>(last);
        else {
	    cell = new (renderArena()) RenderTableCell(document() /* anonymous object */);
	    RenderStyle *newStyle = new RenderStyle();
	    newStyle->inheritFrom(style());
	    newStyle->setDisplay( TABLE_CELL );
	    cell->setStyle(newStyle);
	    addChild(cell, beforeChild);
        }
        cell->addChild(child);
        child->setNeedsLayoutAndMinMaxRecalc();
        return;
    } else
        cell = static_cast<RenderTableCell *>(child);

    static_cast<RenderTableSection *>(parent())->addCell( cell );

    RenderContainer::addChild(cell,beforeChild);

    if ( ( beforeChild || nextSibling()) && section() )
	section()->setNeedCellRecalc();
}

RenderObject* RenderTableRow::removeChildNode(RenderObject* child)
{
// RenderTableCell detach should do it
//     if ( section() )
// 	section()->setNeedCellRecalc();
    return RenderContainer::removeChildNode( child );
}

#ifndef NDEBUG
void RenderTableRow::dump(QTextStream *stream, QString ind) const
{
    RenderContainer::dump(stream,ind);
}
#endif

void RenderTableRow::layout()
{
    KHTMLAssert( needsLayout() );
    KHTMLAssert( minMaxKnown() );

    RenderObject *child = firstChild();
    while( child ) {
        if (child->isTableCell()) {
            RenderTableCell *cell = static_cast<RenderTableCell *>(child);
            if (child->needsLayout()) {
                cell->calcVerticalMargins();
                cell->layout();
                cell->setCellTopExtra(0);
                cell->setCellBottomExtra(0);
            }
        }
	child = child->nextSibling();
    }
    setNeedsLayout(false);
}

QRect RenderTableRow::getAbsoluteRepaintRect()
{
    // For now, just repaint the whole table.
    // FIXME: Find a better way to do this.
    RenderTable* parentTable = table();
    if (parentTable)
        return parentTable->getAbsoluteRepaintRect();
    else
        return QRect();
}

// -------------------------------------------------------------------------

RenderTableCell::RenderTableCell(DOM::NodeImpl* _node)
  : RenderBlock(_node)
{
  _col = -1;
  _row = -1;
  updateFromElement();
  setShouldPaintBackgroundOrBorder(true);
  _topExtra = 0;
  _bottomExtra = 0;
  m_percentageHeight = 0;
}

void RenderTableCell::detach()
{
    if (parent() && section())
        section()->setNeedCellRecalc();

    RenderBlock::detach();
}

void RenderTableCell::updateFromElement()
{
  DOM::NodeImpl *node = element();
  if ( node && (node->id() == ID_TD || node->id() == ID_TH) ) {
      DOM::HTMLTableCellElementImpl *tc = static_cast<DOM::HTMLTableCellElementImpl *>(node);
      cSpan = tc->colSpan();
      rSpan = tc->rowSpan();
  } else {
      cSpan = rSpan = 1;
  }
}

int RenderTableCell::getCellPercentageHeight() const
{
    return m_percentageHeight;
}

void RenderTableCell::setCellPercentageHeight(int h)
{
    m_percentageHeight = h;
}
    
void RenderTableCell::calcMinMaxWidth()
{
    RenderBlock::calcMinMaxWidth();
    if (element() && style()->whiteSpace() == NORMAL) {
        // See if nowrap was set.
        DOMString nowrap = static_cast<ElementImpl*>(element())->getAttribute(ATTR_NOWRAP);
        if (!nowrap.isNull() && style()->width().isFixed())
            // Nowrap is set, but we didn't actually use it because of the
            // fixed width set on the cell.  Even so, it is a WinIE/Moz trait
            // to make the minwidth of the cell into the fixed width.  They do this
            // even in strict mode, so do not make this a quirk.  Affected the top
            // of hiptop.com.
            if (m_minWidth < style()->width().value)
                m_minWidth = style()->width().value;
    }
}

void RenderTableCell::calcWidth()
{
}

void RenderTableCell::setWidth( int width )
{
    assert(width <= SHRT_MAX);
    if ( width != m_width ) {
	m_width = width;
	m_widthChanged = true;
    }
}

void RenderTableCell::layout()
{
    layoutBlock(m_widthChanged);
    m_widthChanged = false;
}

void RenderTableCell::close()
{
    RenderBlock::close();

#ifdef DEBUG_LAYOUT
    kdDebug( 6040 ) << renderName() << "(RenderTableCell)::close() total height =" << m_height << endl;
#endif
}


void RenderTableCell::computeAbsoluteRepaintRect(QRect& r, bool f)
{
    r.setY(r.y() + _topExtra);
    RenderBlock::computeAbsoluteRepaintRect(r, f);
}

bool RenderTableCell::absolutePosition(int &xPos, int &yPos, bool f)
{
    bool ret = RenderBlock::absolutePosition(xPos, yPos, f);
    if (ret)
      yPos += _topExtra;
    return ret;
}

short RenderTableCell::baselinePosition( bool ) const
{
    RenderObject *o = firstChild();
    int offset = paddingTop() + borderTop();
    if ( !o ) return offset;
    while ( o->firstChild() ) {
	if ( !o->isInline() )
	    offset += o->paddingTop() + o->borderTop();
	o = o->firstChild();
    }
    offset += o->baselinePosition( true );
    return offset;
}


void RenderTableCell::setStyle( RenderStyle *style )
{
    style->setDisplay(TABLE_CELL);

    if (style->whiteSpace() == KHTML_NOWRAP) {
        // Figure out if we are really nowrapping or if we should just
        // use normal instead.  If the width of the cell is fixed, then
        // we don't actually use NOWRAP. 
        if (style->width().isFixed())
            style->setWhiteSpace(NORMAL);
        else
            style->setWhiteSpace(NOWRAP);
    }

    RenderBlock::setStyle( style );
    setShouldPaintBackgroundOrBorder(true);
}

// The following rules apply for resolving conflicts and figuring out which border
// to use.
// (1) Borders with the 'border-style' of 'hidden' take precedence over all other conflicting 
// borders. Any border with this value suppresses all borders at this location.
// (2) Borders with a style of 'none' have the lowest priority. Only if the border properties of all 
// the elements meeting at this edge are 'none' will the border be omitted (but note that 'none' is 
// the default value for the border style.)
// (3) If none of the styles are 'hidden' and at least one of them is not 'none', then narrow borders 
// are discarded in favor of wider ones. If several have the same 'border-width' then styles are preferred 
// in this order: 'double', 'solid', 'dashed', 'dotted', 'ridge', 'outset', 'groove', and the lowest: 'inset'.
// (4) If border styles differ only in color, then a style set on a cell wins over one on a row, 
// which wins over a row group, column, column group and, lastly, table. It is undefined which color 
// is used when two elements of the same type disagree.
static CollapsedBorderValue compareBorders(const CollapsedBorderValue& border1, 
                                           const CollapsedBorderValue& border2)
{
    // Sanity check the values passed in.  If either is null, return the other.
    if (!border2.exists()) return border1;
    if (!border1.exists()) return border2;
    
    // Rule #1 above.
    if (border1.style() == BHIDDEN || border2.style() == BHIDDEN)
        return CollapsedBorderValue(); // No border should exist at this location.
    
    // Rule #2 above.  A style of 'none' has lowest priority and always loses to any other border.
    if (border2.style() == BNONE) return border1;
    if (border1.style() == BNONE) return border2;
    
    // The first part of rule #3 above. Wider borders win.
    if (border1.width() != border2.width())
        return border1.width() > border2.width() ? border1 : border2;
    
    // The borders have equal width.  Sort by border style.
    if (border1.style() != border2.style())
        return border1.style() > border2.style() ? border1 : border2;
    
    // The border have the same width and style.  Rely on precedence (cell over row over row group, etc.)
    return border1.precedence >= border2.precedence ? border1 : border2;
}

CollapsedBorderValue RenderTableCell::collapsedLeftBorder() const
{
    // For border left, we need to check, in order of precedence:
    // (1) Our left border.
    CollapsedBorderValue result(&style()->borderLeft(), BCELL);
    
    // (2) The previous cell's right border.
    RenderTableCell* prevCell = table()->cellLeft(this);
    if (prevCell) {
        result = compareBorders(result, CollapsedBorderValue(&prevCell->style()->borderRight(), BCELL));
        if (!result.exists()) return result;
    }
    else if (col() == 0) {
        // (3) Our row's left border.
        result = compareBorders(result, CollapsedBorderValue(&parent()->style()->borderLeft(), BROW));
        if (!result.exists()) return result;
        
        // (4) Our row group's left border.
        result = compareBorders(result, CollapsedBorderValue(&section()->style()->borderLeft(), BROWGROUP));
        if (!result.exists()) return result;
    }
    
    // (5) Our column's left border.
    RenderTableCol* colElt = table()->colElement(col());
    if (colElt) {
        result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderLeft(), BCOL));
        if (!result.exists()) return result;
    }
    
    // (6) The previous column's right border.
    if (col() > 0) {
        colElt = table()->colElement(col()-1);
        if (colElt) {
            result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderRight(), BCOL));
            if (!result.exists()) return result;
        }
    }
    
    if (col() == 0) {
        // (7) The table's left border.
        result = compareBorders(result, CollapsedBorderValue(&table()->style()->borderLeft(), BTABLE));
        if (!result.exists()) return result;
    }
    
    return result;
}

CollapsedBorderValue RenderTableCell::collapsedRightBorder() const
{
    RenderTable* tableElt = table();
    bool inLastColumn = false;
    int effCol = tableElt->colToEffCol(col()+colSpan()-1);
    if (effCol == tableElt->numEffCols()-1)
        inLastColumn = true;
    
    // For border right, we need to check, in order of precedence:
    // (1) Our right border.
    CollapsedBorderValue result = CollapsedBorderValue(&style()->borderRight(), BCELL);
    
    // (2) The next cell's left border.
    if (!inLastColumn) {
        RenderTableCell* nextCell = tableElt->cellRight(this);
        if (nextCell) {
            result = compareBorders(result, CollapsedBorderValue(&nextCell->style()->borderLeft(), BCELL));
            if (!result.exists()) return result;
        }
    }
    else {
        // (3) Our row's right border.
        result = compareBorders(result, CollapsedBorderValue(&parent()->style()->borderRight(), BROW));
        if (!result.exists()) return result;
        
        // (4) Our row group's right border.
        result = compareBorders(result, CollapsedBorderValue(&section()->style()->borderRight(), BROWGROUP));
        if (!result.exists()) return result;
    }
    
    // (5) Our column's right border.
    RenderTableCol* colElt = table()->colElement(col()+colSpan()-1);
    if (colElt) {
        result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderRight(), BCOL));
        if (!result.exists()) return result;
    }
    
    // (6) The next column's left border.
    if (!inLastColumn) {
        colElt = tableElt->colElement(col()+colSpan());
        if (colElt) {
            result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderLeft(), BCOL));
            if (!result.exists()) return result;
        }
    }
    else {
        // (7) The table's right border.
        result = compareBorders(result, CollapsedBorderValue(&tableElt->style()->borderRight(), BTABLE));
        if (!result.exists()) return result;
    }
    
    return result;
}

CollapsedBorderValue RenderTableCell::collapsedTopBorder() const
{
    // For border top, we need to check, in order of precedence:
    // (1) Our top border.
    CollapsedBorderValue result = CollapsedBorderValue(&style()->borderTop(), BCELL);
    
    RenderTableCell* prevCell = table()->cellAbove(this);
    if (prevCell) {
        // (2) A previous cell's bottom border.
        result = compareBorders(result, CollapsedBorderValue(&prevCell->style()->borderBottom(), BCELL));
        if (!result.exists()) return result;
    }
    
    // (3) Our row's top border.
    result = compareBorders(result, CollapsedBorderValue(&parent()->style()->borderTop(), BROW));
    if (!result.exists()) return result;
    
    // (4) The previous row's bottom border.
    if (prevCell) {
        RenderObject* prevRow = 0;
        if (prevCell->section() == section())
            prevRow = parent()->previousSibling();
        else
            prevRow = prevCell->section()->lastChild();
    
        if (prevRow) {
            result = compareBorders(result, CollapsedBorderValue(&prevRow->style()->borderBottom(), BROW));
            if (!result.exists()) return result;
        }
    }
    
    // Now check row groups.
    RenderObject* currSection = parent()->parent();
    if (row() == 0) {
        // (5) Our row group's top border.
        result = compareBorders(result, CollapsedBorderValue(&currSection->style()->borderTop(), BROWGROUP));
        if (!result.exists()) return result;
        
        // (6) Previous row group's bottom border.
        for (currSection = currSection->previousSibling(); currSection;
             currSection = currSection->previousSibling()) {
            if (currSection->isTableSection()) {
                RenderTableSection* section = static_cast<RenderTableSection*>(currSection);
                result = compareBorders(result, CollapsedBorderValue(&section->style()->borderBottom(), BROWGROUP));
                if (!result.exists()) return result;
            }
        }
    }
    
    if (!currSection) {
        // (8) Our column's top border.
        RenderTableCol* colElt = table()->colElement(col());
        if (colElt) {
            result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderTop(), BCOL));
            if (!result.exists()) return result;
        }
        
        // (9) The table's top border.
        result = compareBorders(result, CollapsedBorderValue(&table()->style()->borderTop(), BTABLE));
        if (!result.exists()) return result;
    }
    
    return result;
}

CollapsedBorderValue RenderTableCell::collapsedBottomBorder() const
{
    // For border top, we need to check, in order of precedence:
    // (1) Our bottom border.
    CollapsedBorderValue result = CollapsedBorderValue(&style()->borderBottom(), BCELL);
    
    RenderTableCell* nextCell = table()->cellBelow(this);
    if (nextCell) {
        // (2) A following cell's top border.
        result = compareBorders(result, CollapsedBorderValue(&nextCell->style()->borderTop(), BCELL));
        if (!result.exists()) return result;
    }
    
    // (3) Our row's bottom border. (FIXME: Deal with rowspan!)
    result = compareBorders(result, CollapsedBorderValue(&parent()->style()->borderBottom(), BROW));
    if (!result.exists()) return result;
    
    // (4) The next row's top border.
    if (nextCell) {
        result = compareBorders(result, CollapsedBorderValue(&nextCell->parent()->style()->borderTop(), BROW));
        if (!result.exists()) return result;
    }
    
    // Now check row groups.
    RenderObject* currSection = parent()->parent();
    if (row()+rowSpan() >= static_cast<RenderTableSection*>(currSection)->numRows()) {
        // (5) Our row group's bottom border.
        result = compareBorders(result, CollapsedBorderValue(&currSection->style()->borderBottom(), BROWGROUP));
        if (!result.exists()) return result;
        
        // (6) Following row group's top border.
        for (currSection = currSection->nextSibling(); currSection;
             currSection = currSection->nextSibling()) {
            if (currSection->isTableSection()) {
                RenderTableSection* section = static_cast<RenderTableSection*>(currSection);
                result = compareBorders(result, CollapsedBorderValue(&section->style()->borderTop(), BROWGROUP));
                if (!result.exists()) return result;
            }
        }
    }
    
    if (!currSection) {
        // (8) Our column's bottom border.
        RenderTableCol* colElt = table()->colElement(col());
        if (colElt) {
            result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderBottom(), BCOL));
            if (!result.exists()) return result;
        }
        
        // (9) The table's bottom border.
        result = compareBorders(result, CollapsedBorderValue(&table()->style()->borderBottom(), BTABLE));
        if (!result.exists()) return result;
    }
    
    return result;    
}

int RenderTableCell::borderLeft() const
{
    if (table()->collapseBorders()) {
        CollapsedBorderValue border = collapsedLeftBorder();
        if (border.exists())
            return int(border.width()/2.0+0.5); // Give the extra pixel to top and left.
        return 0;
    }
    return RenderBlock::borderLeft();
}
    
int RenderTableCell::borderRight() const
{
    if (table()->collapseBorders()) {
        CollapsedBorderValue border = collapsedRightBorder();
        if (border.exists())
            return border.width()/2;
        return 0;
    }
    return RenderBlock::borderRight();
}

int RenderTableCell::borderTop() const
{
    if (table()->collapseBorders()) {
        CollapsedBorderValue border = collapsedTopBorder();
        if (border.exists())
            return int(border.width()/2.0+0.5); // Give the extra pixel to top and left.
        return 0;
    }
    return RenderBlock::borderTop();
}

int RenderTableCell::borderBottom() const
{
    if (table()->collapseBorders()) {
        CollapsedBorderValue border = collapsedBottomBorder();
        if (border.exists())
            return border.width()/2;
        return 0;
    }
    return RenderBlock::borderBottom();
}

#ifdef BOX_DEBUG
#include <qpainter.h>

static void outlineBox(QPainter *p, int _tx, int _ty, int w, int h)
{
    p->setPen(QPen(QColor("yellow"), 3, Qt::DotLine));
    p->setBrush( Qt::NoBrush );
    p->drawRect(_tx, _ty, w, h );
}
#endif

void RenderTableCell::paint(QPainter *p, int _x, int _y,
                                       int _w, int _h, int _tx, int _ty, PaintAction paintAction)
{

#ifdef TABLE_PRINT
    kdDebug( 6040 ) << renderName() << "(RenderTableCell)::paint() w/h = (" << width() << "/" << height() << ")" << " _y/_h=" << _y << "/" << _h << endl;
#endif

    if (needsLayout()) return;

    _tx += m_x;
    _ty += m_y + _topExtra;

    // check if we need to do anything at all...
    int os = 2*maximalOutlineSize(paintAction);
    if(!overhangingContents() && ((_ty-_topExtra >= _y + _h + os)
        || (_ty + m_height + _bottomExtra <= _y - os))) return;

    paintObject(p, _x, _y, _w, _h, _tx, _ty, paintAction);

#ifdef BOX_DEBUG
    ::outlineBox( p, _tx, _ty - _topExtra, width(), height() + borderTopExtra() + borderBottomExtra());
#endif
}

static EBorderStyle collapsedBorderStyle(EBorderStyle style)
{
    if (style == OUTSET)
        style = GROOVE;
    else if (style == INSET)
        style = RIDGE;
    return style;
}

struct CollapsedBorder {
    CollapsedBorder(){}
    
    CollapsedBorderValue border;
    RenderObject::BorderSide side;
    bool shouldPaint;
    int x1;
    int y1;
    int x2;
    int y2;
    EBorderStyle style;
};

class CollapsedBorders
{
public:
    CollapsedBorders(int i) :count(0) {}
    
    void addBorder(const CollapsedBorderValue& b, RenderObject::BorderSide s, bool paint, 
                   int _x1, int _y1, int _x2, int _y2,
                   EBorderStyle _style)
    {
        if (b.exists() && paint) {
            borders[count].border = b;
            borders[count].side = s;
            borders[count].shouldPaint = paint;
            borders[count].x1 = _x1;
            borders[count].x2 = _x2;
            borders[count].y1 = _y1;
            borders[count].y2 = _y2;
            borders[count].style = _style;
            count++;
        }
    }

    CollapsedBorder* nextBorder() {
        for (int i = 0; i < count; i++) {
            if (borders[i].border.exists() && borders[i].shouldPaint) {
                borders[i].shouldPaint = false;
                return &borders[i];
            }
        }
        
        return 0;
    }
    
    CollapsedBorder borders[4];
    int count;
};

static void addBorderStyle(QValueList<CollapsedBorderValue>& borderStyles, CollapsedBorderValue borderValue)
{
    if (!borderValue.exists() || borderStyles.contains(borderValue))
        return;
    
    QValueListIterator<CollapsedBorderValue> it = borderStyles.begin();
    QValueListIterator<CollapsedBorderValue> end = borderStyles.end();
    for (; it != end; ++it) {
        CollapsedBorderValue result = compareBorders(*it, borderValue);
        if (result == *it) {
            borderStyles.insert(it, borderValue);
            return;
        }
    }

    borderStyles.append(borderValue);
}

void RenderTableCell::collectBorders(QValueList<CollapsedBorderValue>& borderStyles)
{
    addBorderStyle(borderStyles, collapsedLeftBorder());
    addBorderStyle(borderStyles, collapsedRightBorder());
    addBorderStyle(borderStyles, collapsedTopBorder());
    addBorderStyle(borderStyles, collapsedBottomBorder());
}

void RenderTableCell::paintCollapsedBorder(QPainter* p, int _tx, int _ty, int w, int h)
{
    if (!table()->currentBorderStyle())
        return;
    
    CollapsedBorderValue leftVal = collapsedLeftBorder();
    CollapsedBorderValue rightVal = collapsedRightBorder();
    CollapsedBorderValue topVal = collapsedTopBorder();
    CollapsedBorderValue bottomVal = collapsedBottomBorder();
     
    // Adjust our x/y/width/height so that we paint the collapsed borders at the correct location.
    int topWidth = topVal.width();
    int bottomWidth = bottomVal.width();
    int leftWidth = leftVal.width();
    int rightWidth = rightVal.width();
    
    _tx -= leftWidth/2;
    _ty -= topWidth/2;
    w += leftWidth/2 + int(rightWidth/2.0+0.5);
    h += topWidth/2 + int(bottomWidth/2.0+0.5);
    
    bool tt = topVal.isTransparent();
    bool bt = bottomVal.isTransparent();
    bool rt = rightVal.isTransparent();
    bool lt = leftVal.isTransparent();
    
    EBorderStyle ts = collapsedBorderStyle(topVal.style());
    EBorderStyle bs = collapsedBorderStyle(bottomVal.style());
    EBorderStyle ls = collapsedBorderStyle(leftVal.style());
    EBorderStyle rs = collapsedBorderStyle(rightVal.style());
    
    bool render_t = ts > BHIDDEN && !tt;
    bool render_l = ls > BHIDDEN && !lt;
    bool render_r = rs > BHIDDEN && !rt;
    bool render_b = bs > BHIDDEN && !bt;

    // We never paint diagonals at the joins.  We simply let the border with the highest
    // precedence paint on top of borders with lower precedence.  
    CollapsedBorders borders(4);
    borders.addBorder(topVal, BSTop, render_t, _tx, _ty, _tx + w, _ty + topWidth, ts);
    borders.addBorder(bottomVal, BSBottom, render_b, _tx, _ty + h - bottomWidth, _tx + w, _ty + h, bs);
    borders.addBorder(leftVal, BSLeft, render_l, _tx, _ty, _tx + leftWidth, _ty + h, ls);
    borders.addBorder(rightVal, BSRight, render_r, _tx + w - rightWidth, _ty, _tx + w, _ty + h, rs);
    
    for (CollapsedBorder* border = borders.nextBorder(); border; border = borders.nextBorder()) {
        if (border->border == *table()->currentBorderStyle())
            drawBorder(p, border->x1, border->y1, border->x2, border->y2, border->side, 
                       border->border.color(), style()->color(), border->style, 0, 0);
    }
}

QRect RenderTableCell::getAbsoluteRepaintRect()
{
    int ow = style() ? style()->outlineSize() : 0;
    QRect r(-ow, -ow - borderTopExtra(), 
            overflowWidth(false)+ow*2, overflowHeight(false)+borderTopExtra()+borderBottomExtra()+ow*2);
    computeAbsoluteRepaintRect(r);
    return r;
}

void RenderTableCell::paintObject(QPainter* p, int _x, int _y, int _w, int _h,
                                  int _tx, int _ty, PaintAction paintAction)
{
    if (paintAction == PaintActionCollapsedTableBorders && style()->visibility() == VISIBLE) {
        int w = width();
        int h = height() + borderTopExtra() + borderBottomExtra();
        _ty -= borderTopExtra();
        paintCollapsedBorder(p, _tx, _ty, w, h);
    }
    else
        RenderBlock::paintObject(p, _x, _y, _w, _h, _tx, _ty, paintAction);
}

void RenderTableCell::paintBoxDecorations(QPainter *p,int, int _y,
                                          int, int _h, int _tx, int _ty)
{
    RenderTable* tableElt = table();
    if (!tableElt->collapseBorders() && style()->emptyCells() == HIDE && !firstChild())
        return;
    
    int w = width();
    int h = height() + borderTopExtra() + borderBottomExtra();
    _ty -= borderTopExtra();

    QColor c = style()->backgroundColor();
    if ( !c.isValid() && parent() ) // take from row
        c = parent()->style()->backgroundColor();
    if ( !c.isValid() && parent() && parent()->parent() ) // take from rowgroup
        c = parent()->parent()->style()->backgroundColor();
    if ( !c.isValid() ) {
	// see if we have a col or colgroup for this
	RenderTableCol *col = table()->colElement( _col );
	if ( col ) {
	    c = col->style()->backgroundColor();
	    if ( !c.isValid() ) {
		// try column group
		RenderStyle *style = col->parent()->style();
		if ( style->display() == TABLE_COLUMN_GROUP )
		    c = style->backgroundColor();
	    }
	}
    }

    // ### get offsets right in case the bgimage is inherited.
    CachedImage *bg = style()->backgroundImage();
    if ( !bg && parent() )
        bg = parent()->style()->backgroundImage();
    if ( !bg && parent() && parent()->parent() )
        bg = parent()->parent()->style()->backgroundImage();
    if ( !bg ) {
	// see if we have a col or colgroup for this
	RenderTableCol *col = table()->colElement( _col );
	if ( col ) {
	    bg = col->style()->backgroundImage();
	    if ( !bg ) {
		// try column group
		RenderStyle *style = col->parent()->style();
		if ( style->display() == TABLE_COLUMN_GROUP )
		    bg = style->backgroundImage();
	    }
	}
    }

    int my = QMAX(_ty,_y);
    int end = QMIN( _y + _h,  _ty + h );
    int mh = end - my;

    if ( bg || c.isValid() )
	paintBackground(p, c, bg, my, mh, _tx, _ty, w, h);

    if (style()->hasBorder() && !tableElt->collapseBorders())
        paintBorder(p, _tx, _ty, w, h, style());
}


#ifndef NDEBUG
void RenderTableCell::dump(QTextStream *stream, QString ind) const
{
    *stream << " row=" << _row;
    *stream << " col=" << _col;
    *stream << " rSpan=" << rSpan;
    *stream << " cSpan=" << cSpan;
//    *stream << " nWrap=" << nWrap;

    RenderBlock::dump(stream,ind);
}
#endif

// -------------------------------------------------------------------------

RenderTableCol::RenderTableCol(DOM::NodeImpl* node)
    : RenderContainer(node)
{
    // init RenderObject attributes
    setInline(true);   // our object is not Inline

    _span = 1;
    updateFromElement();
}

void RenderTableCol::updateFromElement()
{
  DOM::NodeImpl *node = element();
  if ( node && (node->id() == ID_COL || node->id() == ID_COLGROUP) ) {
      DOM::HTMLTableColElementImpl *tc = static_cast<DOM::HTMLTableColElementImpl *>(node);
      _span = tc->span();
  } else
      _span = ! ( style() && style()->display() == TABLE_COLUMN_GROUP );
}

bool RenderTableCol::canHaveChildren() const
{
    // cols cannot have children.  This is actually necessary to fix a bug
    // with libraries.uc.edu, which makes a <p> be a table-column.
    return style()->display() == TABLE_COLUMN_GROUP;
}

void RenderTableCol::addChild(RenderObject *child, RenderObject *beforeChild)
{
#ifdef DEBUG_LAYOUT
    //kdDebug( 6040 ) << renderName() << "(Table)::addChild( " << child->renderName() << " )"  << ", " <<
    //                   (beforeChild ? beforeChild->renderName() : 0) << " )" << endl;
#endif

    KHTMLAssert(child->style()->display() == TABLE_COLUMN);

    // these have to come before the table definition!
    RenderContainer::addChild(child,beforeChild);
}

#ifndef NDEBUG
void RenderTableCol::dump(QTextStream *stream, QString ind) const
{
    *stream << " _span=" << _span;
    RenderContainer::dump(stream,ind);
}
#endif

#undef TABLE_DEBUG
#undef DEBUG_LAYOUT
#undef BOX_DEBUG
