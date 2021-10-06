/*
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
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
#ifndef render_object_h
#define render_object_h

#include <qcolor.h>
#include <qrect.h>
#include <assert.h>

#include "misc/khtmllayout.h"
#include "misc/loader_client.h"
#include "misc/helper.h"
#include "rendering/render_style.h"
#include "khtml_events.h"
#include "xml/dom_nodeimpl.h"

// Uncomment to turn on incremental repainting.
#define INCREMENTAL_REPAINTING 1

class QPainter;
class QTextStream;
class CSSStyle;
class KHTMLView;
class RenderArena;

#ifndef NDEBUG
#define KHTMLAssert( x ) if( !(x) ) { \
    const RenderObject *o = this; while( o->parent() ) o = o->parent(); \
    o->printTree(); \
    qDebug(" this object = %p", this ); \
    assert( false ); \
}
#else
#define KHTMLAssert( x )
#endif

/*
 *	The painting of a layer occurs in three distinct phases.  Each phase involves
 *	a recursive descent into the layer's render objects. The first phase is the background phase.
 *	The backgrounds and borders of all blocks are painted.  Inlines are not painted at all.
 *	Floats must paint above block backgrounds but entirely below inline content that can overlap them.
 *	In the foreground phase, all inlines are fully painted.  Inline replaced elements will get all
 *	three phases invoked on them during this phase.
 */

typedef enum {
    PaintActionElementBackground = 0,
    PaintActionChildBackground,
    PaintActionChildBackgrounds,
    PaintActionFloat,
    PaintActionForeground,
    PaintActionOutline,
    PaintActionSelection,
    PaintActionCollapsedTableBorders
} PaintAction;

typedef enum {
    HitTestAll = 0,
    HitTestSelfOnly = 1,
    HitTestChildrenOnly = 2
} HitTestAction;

namespace DOM {
    class HTMLAreaElementImpl;
    class DOMString;
    class DocumentImpl;
    class ElementImpl;
    class EventImpl;
};

namespace khtml {
    class RenderFlow;
    class RenderBlock;
    class RenderStyle;
    class RenderTable;
    class CachedObject;
    class RenderCanvas;
    class RenderText;
    class RenderFrameSet;
    class RenderLayer;
    class InlineBox;
    class InlineFlowBox;
    class CollapsedBorderValue;

/**
 * Base Class for all rendering tree objects.
 */
class RenderObject : public CachedObjectClient
{
public:
    // Anonymous objects should pass the document as their node, and they will then automatically be
    // marked as anonymous in the constructor.
    RenderObject(DOM::NodeImpl* node);
    virtual ~RenderObject();

    RenderObject *parent() const { return m_parent; }

    RenderObject *previousSibling() const { return m_previous; }
    RenderObject *nextSibling() const { return m_next; }

    virtual RenderObject *firstChild() const { return 0; }
    virtual RenderObject *lastChild() const { return 0; }

    virtual RenderLayer* layer() const { return 0; }
    RenderLayer* enclosingLayer();
    void addLayers(RenderLayer* parentLayer, RenderObject* newObject);
    void removeLayers(RenderLayer* parentLayer);
    void moveLayers(RenderLayer* oldParent, RenderLayer* newParent);
    RenderLayer* findNextLayer(RenderLayer* parentLayer, RenderObject* startPoint,
                               bool checkParent=true);
    virtual void positionChildLayers() { }
    virtual bool requiresLayer();
    
    virtual QRect getOverflowClipRect(int tx, int ty) { return QRect(0,0,0,0); }
    virtual QRect getClipRect(int tx, int ty) { return QRect(0,0,0,0); }
    bool hasClip() { return isPositioned() &&  style()->hasClip(); }
    bool hasOverflowClip() { return style()->hidesOverflow(); }

    virtual int getBaselineOfFirstLineBox() { return -1; } // Tables and blocks implement this.
    virtual InlineFlowBox* getFirstLineBox() { return 0; } // Tables and blocks implement this.

    // Obtains the nearest enclosing block (including this block) that contributes a first-line style to our inline
    // children.
    virtual RenderBlock* firstLineBlock() const;
    virtual void updateFirstLetter();
    
    // Called when an object that was floating or positioned becomes a normal flow object
    // again.  We have to make sure the render tree updates as needed to accommodate the new
    // normal flow object.
    void handleDynamicFloatPositionChange();

    // This function is a convenience helper for creating an anonymous block that inherits its
    // style from this RenderObject.
    RenderBlock* createAnonymousBlock();
    
    // Whether or not a positioned element requires normal flow x/y to be computed
    // to determine its position.
    bool hasStaticX() const;
    bool hasStaticY() const;
    virtual void setStaticX(short staticX) {};
    virtual void setStaticY(int staticY) {};
    
    // RenderObject tree manipulation
    //////////////////////////////////////////
    virtual bool canHaveChildren() const;
    virtual void addChild(RenderObject *newChild, RenderObject *beforeChild = 0);
    virtual void removeChild(RenderObject *oldChild);

    // raw tree manipulation
    virtual RenderObject* removeChildNode(RenderObject* child);
    virtual void appendChildNode(RenderObject* child);
    virtual void insertChildNode(RenderObject* child, RenderObject* before);
    //////////////////////////////////////////

private:
    //////////////////////////////////////////
    // Helper functions. Dangerous to use!
    void setPreviousSibling(RenderObject *previous) { m_previous = previous; }
    void setNextSibling(RenderObject *next) { m_next = next; }
    void setParent(RenderObject *parent) { m_parent = parent; }
    //////////////////////////////////////////

public:
    virtual const char *renderName() const { return "RenderObject"; }
#ifndef NDEBUG
    QString information() const;
    virtual void printTree(int indent=0) const;
    virtual void dump(QTextStream *stream, QString ind = "") const;
#endif

    static RenderObject *createObject(DOM::NodeImpl* node, RenderStyle* style);

    // Overloaded new operator.  Derived classes must override operator new
    // in order to allocate out of the RenderArena.
    void* operator new(size_t sz, RenderArena* renderArena) throw();    

    // Overridden to prevent the normal delete from being called.
    void operator delete(void* ptr, size_t sz);
        
private:
    // The normal operator new is disallowed on all render objects.
    void* operator new(size_t sz) throw();
    
public:
    RenderArena* renderArena() const;
    
    // some helper functions...
    virtual bool isRenderBlock() const { return false; }
    virtual bool isRenderInline() const { return false; }
    virtual bool isInlineFlow() const { return false; }
    virtual bool isBlockFlow() const { return false; }
    virtual bool isInlineBlockOrInlineTable() const { return false; }
    virtual bool childrenInline() const { return false; }
    virtual void setChildrenInline(bool b) { };

    virtual RenderFlow* continuation() const;
    virtual bool isInlineContinuation() const;
    
    virtual bool isListItem() const { return false; }
    virtual bool isListMarker() const { return false; }
    virtual bool isCanvas() const { return false; }
    bool isRoot() const;
    bool isBody() const;
    bool isHR() const;
    virtual bool isBR() const { return false; }
    virtual bool isTableCell() const { return false; }
    virtual bool isTableRow() const { return false; }
    virtual bool isTableSection() const { return false; }
    virtual bool isTableCol() const { return false; }
    virtual bool isTable() const { return false; }
    virtual bool isWidget() const { return false; }
    virtual bool isFormElement() const { return false; }
    virtual bool isImage() const { return false; }
    virtual bool isTextArea() const { return false; }
    virtual bool isFrameSet() const { return false; }
    virtual bool isApplet() const { return false; }

    bool isHTMLMarquee() const;
    
    bool isAnonymous() const { return m_isAnonymous; }
    void setIsAnonymous(bool b) { m_isAnonymous = b; }
    bool isAnonymousBlock() const { return m_isAnonymous && 
                                           style()->display() == BLOCK && 
                                           style()->styleType() == RenderStyle::NOPSEUDO &&
                                           !isListMarker(); }
    
    bool isFloating() const { return m_floating; }
    bool isPositioned() const { return m_positioned; } // absolute or fixed positioning
    bool isRelPositioned() const { return m_relPositioned; } // relative positioning
    bool isText() const  { return m_isText; }
    bool isInline() const { return m_inline; }  // inline object
    bool isCompact() const { return style()->display() == COMPACT; } // compact object
    bool isRunIn() const { return style()->display() == RUN_IN; } // run-in object
    bool mouseInside() const;
    bool isReplaced() const { return m_replaced; } // a "replaced" element (see CSS)
    bool shouldPaintBackgroundOrBorder() const { return m_paintBackground; }
    bool needsLayout() const   { return m_needsLayout || m_normalChildNeedsLayout || m_posChildNeedsLayout; }
    bool selfNeedsLayout() const { return m_needsLayout; }
    bool posChildNeedsLayout() const { return m_posChildNeedsLayout; }
    bool normalChildNeedsLayout() const { return m_normalChildNeedsLayout; }
    bool minMaxKnown() const{ return m_minMaxKnown; }
    bool overhangingContents() const { return m_overhangingContents; }
    bool isSelectionBorder() const { return m_isSelectionBorder; }
    bool recalcMinMax() const { return m_recalcMinMax; }

    RenderStyle* getPseudoStyle(RenderStyle::PseudoId pseudo, RenderStyle* parentStyle = 0) const;
    
    RenderCanvas* canvas() const;

    // don't even think about making this method virtual!
    DOM::NodeImpl* element() const { return m_isAnonymous ? 0 : m_node; }
    DOM::DocumentImpl* document() const { return m_node->getDocument(); }
    void setNode(DOM::NodeImpl* node) { m_node = node; }
    DOM::NodeImpl* node() const { return m_node; }
    
   /**
     * returns the object containing this one. can be different from parent for
     * positioned elements
     */
    RenderObject *container() const;

    void setOverhangingContents(bool p=true);

    virtual void markAllDescendantsWithFloatsForLayout(RenderObject* floatToRemove = 0);
    void markContainingBlocksForLayout();
    void setNeedsLayout(bool b, bool markParents = true);
    void setChildNeedsLayout(bool b, bool markParents = true);
    void setMinMaxKnown(bool b=true) {
	m_minMaxKnown = b;
	if ( !b ) {
	    RenderObject *o = this;
	    RenderObject *root = this;
	    while( o ) { // ### && !o->m_recalcMinMax ) {
		o->m_recalcMinMax = true;
		root = o;
		o = o->m_parent;
	    }
	}
    }

    void setNeedsLayoutAndMinMaxRecalc() {
        setMinMaxKnown(false);
        setNeedsLayout(true);
    }
    
    void setPositioned(bool b=true)  { m_positioned = b;  }
    void setRelPositioned(bool b=true) { m_relPositioned = b; }
    void setFloating(bool b=true) { m_floating = b; }
    void setInline(bool b=true) { m_inline = b; }
    void setMouseInside(bool b=true) { m_mouseInside = b; }
    void setShouldPaintBackgroundOrBorder(bool b=true) { m_paintBackground = b; }
    void setRenderText() { m_isText = true; }
    void setReplaced(bool b=true) { m_replaced = b; }
    void setIsSelectionBorder(bool b=true) { m_isSelectionBorder = b; }

#ifdef INCREMENTAL_REPAINTING
    void scheduleRelayout();
#else
    void scheduleRelayout(RenderObject* clippedObj);
#endif
    
    virtual InlineBox* createInlineBox(bool makePlaceHolderBox, bool isRootLineBox);
    
    // for discussion of lineHeight see CSS2 spec
    virtual short lineHeight( bool firstLine, bool isRootLineBox=false ) const;
    // for the vertical-align property of inline elements
    // the difference between this objects baseline position and the lines baseline position.
    virtual short verticalPositionHint( bool firstLine ) const;
    // the offset of baseline from the top of the object.
    virtual short baselinePosition( bool firstLine, bool isRootLineBox=false ) const;

    /*
     * Paint the object and its children, clipped by (x|y|w|h).
     * (tx|ty) is the calculated position of the parent
     */
    virtual void paint(QPainter *p, int x, int y, int w, int h, int tx, int ty, 
                       PaintAction paintAction);

    virtual void paintObject( QPainter */*p*/, int /*x*/, int /*y*/,
                              int /*w*/, int /*h*/, int /*tx*/, int /*ty*/,
                              PaintAction paintAction /*paintAction*/) {}
    void paintBorder(QPainter *p, int _tx, int _ty, int w, int h, const RenderStyle* style, bool begin=true, bool end=true);
    void paintOutline(QPainter *p, int _tx, int _ty, int w, int h, const RenderStyle* style);

    // RenderBox implements this.
    virtual void paintBoxDecorations(QPainter *p,int _x, int _y,
                                     int _w, int _h, int _tx, int _ty) {};

    virtual void paintBackgroundExtended(QPainter *p, const QColor &c, CachedImage *bg, int clipy, int cliph,
                                         int _tx, int _ty, int w, int height,
                                         int bleft, int bright) {};

    /*
     * This function calculates the minimum & maximum width that the object
     * can be set to.
     *
     * when the Element calls setMinMaxKnown(true), calcMinMaxWidth() will
     * be no longer called.
     *
     * when a element has a fixed size, m_minWidth and m_maxWidth should be
     * set to the same value. This has the special meaning that m_width,
     * contains the actual value.
     *
     * assumes calcMinMaxWidth has already been called for all children.
     */
    virtual void calcMinMaxWidth() { }

    /*
     * Does the min max width recalculations after changes.
     */
    void recalcMinMaxWidths();

    /*
     * Calculates the actual width of the object (only for non inline
     * objects)
     */
    virtual void calcWidth() {}

    /*
     * This function should cause the Element to calculate its
     * width and height and the layout of its content
     *
     * when the Element calls setNeedsLayout(false), layout() is no
     * longer called during relayouts, as long as there is no
     * style sheet change. When that occurs, m_needsLayout will be
     * set to true and the Element receives layout() calls
     * again.
     */
    virtual void layout() = 0;

    /* This function performs a layout only if one is needed. */
    void layoutIfNeeded() { if (needsLayout()) layout(); }

    // used for element state updates that can not be fixed with a
    // repaint and do not need a relayout
    virtual void updateFromElement() {};

    // The corresponding closing element has been parsed. ### remove me
    virtual void close() { }

    virtual int availableHeight() const { return 0; }

    // Whether or not the element shrinks to its max width (rather than filling the width
    // of a containing block).  HTML4 buttons, legends, and floating/compact elements do this.
    bool sizesToMaxWidth() const;

#if APPLE_CHANGES
    // Called recursively to update the absolute positions of all widgets.
    virtual void updateWidgetPositions();
#endif

    // does a query on the rendertree and finds the innernode
    // and overURL for the given position
    // if readonly == false, it will recalc hover styles accordingly
    class NodeInfo
    {
        friend class RenderLayer;
        friend class RenderImage;
        friend class RenderText;
        friend class RenderInline;
        friend class RenderObject;
        friend class RenderFrameSet;
        friend class DOM::HTMLAreaElementImpl;
    public:
        NodeInfo(bool readonly, bool active)
            : m_innerNode(0), m_innerNonSharedNode(0), m_innerURLElement(0), m_readonly(readonly), m_active(active)
            { }

        DOM::NodeImpl* innerNode() const { return m_innerNode; }
        DOM::NodeImpl* innerNonSharedNode() const { return m_innerNonSharedNode; }
        DOM::NodeImpl* URLElement() const { return m_innerURLElement; }
        bool readonly() const { return m_readonly; }
        bool active() const { return m_active; }

    private:
        void setInnerNode(DOM::NodeImpl* n) { m_innerNode = n; }
        void setInnerNonSharedNode(DOM::NodeImpl* n) { m_innerNonSharedNode = n; }
        void setURLElement(DOM::NodeImpl* n) { m_innerURLElement = n; }

        DOM::NodeImpl* m_innerNode;
        DOM::NodeImpl* m_innerNonSharedNode;
        DOM::NodeImpl* m_innerURLElement;
        bool m_readonly;
        bool m_active;
    };

    FindSelectionResult checkSelectionPoint(int x, int y, int tx, int ty, DOM::NodeImpl*&, int& offset);
    virtual FindSelectionResult checkSelectionPointIgnoringContinuations(int x, int y, int tx, int ty, DOM::NodeImpl*&, int& offset);
    virtual bool nodeAtPoint(NodeInfo& info, int x, int y, int tx, int ty,
                             HitTestAction hitTestAction = HitTestAll, bool inside=false);
    
    // set the style of the object.
    virtual void setStyle(RenderStyle *style);

    // returns the containing block level element for this element.
    RenderBlock *containingBlock() const;

    // return just the width of the containing block
    virtual short containingBlockWidth() const;
    // return just the height of the containing block
    virtual int containingBlockHeight() const;

    // size of the content area (box size minus padding/border)
    virtual short contentWidth() const { return 0; }
    virtual int contentHeight() const { return 0; }

    // intrinsic extend of replaced elements. undefined otherwise
    virtual short intrinsicWidth() const { return 0; }
    virtual int intrinsicHeight() const { return 0; }

    // relative to parent node
    virtual void setPos( int /*xPos*/, int /*yPos*/ ) { }
    virtual void setWidth( int /*width*/ ) { }
    virtual void setHeight( int /*height*/ ) { }

    virtual int xPos() const { return 0; }
    virtual int yPos() const { return 0; }

    // calculate client position of box
    virtual bool absolutePosition(int &/*xPos*/, int &/*yPos*/, bool fixed = false);

    // width and height are without margins but include paddings and borders
    virtual short width() const { return 0; }
    virtual int height() const { return 0; }

    // The height of a block when you include normal flow overflow spillage out of the bottom
    // of the block (e.g., a <div style="height:25px"> that has a 100px tall image inside
    // it would have an overflow height of borderTop() + paddingTop() + 100px.
    virtual int overflowHeight(bool includeInterior=true) const { return height(); }
    virtual int overflowWidth(bool includeInterior=true) const { return width(); }
    virtual void setOverflowHeight(int) {}
    virtual void setOverflowWidth(int) {}

    // IE extensions. Used to calculate offsetWidth/Height.  Overridden by inlines (render_flow) 
    // to return the remaining width on a given line (and the height of a single line). -dwh
    virtual short offsetWidth() const { return width(); }
    virtual int offsetHeight() const { return height(); }
    
    // IE exxtensions.  Also supported by Gecko.  We override in render flow to get the
    // left and top correct. -dwh
    virtual int offsetLeft() const;
    virtual int offsetTop() const;
    virtual RenderObject* offsetParent() const;

    // More IE extensions.  clientWidth and clientHeight represent the interior of an object
    // excluding border and scrollbar.
    short clientWidth() const;
    int clientHeight() const;

    // scrollWidth/scrollHeight will be the same as clientWidth/clientHeight unless the
    // object has overflow:hidden/scroll/auto specified and also has overflow.
    short scrollWidth() const;
    int scrollHeight() const;

    // The following seven functions are used to implement collapsing margins.
    // All objects know their maximal positive and negative margins.  The
    // formula for computing a collapsed margin is |maxPosMargin|-|maxNegmargin|.
    // For a non-collapsing, e.g., a leaf element, this formula will simply return
    // the margin of the element.  Blocks override the maxTopMargin and maxBottomMargin
    // methods.
    virtual bool isSelfCollapsingBlock() const { return false; }
    virtual short collapsedMarginTop() const 
        { return maxTopMargin(true)-maxTopMargin(false); }
    virtual short collapsedMarginBottom() const 
        { return maxBottomMargin(true)-maxBottomMargin(false); }
    virtual bool isTopMarginQuirk() const { return false; }
    virtual bool isBottomMarginQuirk() const { return false; }
    virtual short maxTopMargin(bool positive) const {
        if (positive)
            if (marginTop() > 0)
                return marginTop();
            else
                return 0;
        else
            if (marginTop() < 0)
                return 0 - marginTop();
            else
                return 0;
    }
    virtual short maxBottomMargin(bool positive) const {
        if (positive)
            if (marginBottom() > 0)
                return marginBottom();
            else
                return 0;
        else
            if (marginBottom() < 0)
                return 0 - marginBottom();
            else
                return 0;
    }

    virtual short marginTop() const { return 0; }
    virtual short marginBottom() const { return 0; }
    virtual short marginLeft() const { return 0; }
    virtual short marginRight() const { return 0; }

    // Virtual since table cells override 
    virtual int paddingTop() const;
    virtual int paddingBottom() const;
    virtual int paddingLeft() const;
    virtual int paddingRight() const;
    
    virtual int borderTop() const { return style()->borderTopWidth(); }
    virtual int borderBottom() const { return style()->borderBottomWidth(); }
    virtual int borderLeft() const { return style()->borderLeftWidth(); }
    virtual int borderRight() const { return style()->borderRightWidth(); }

    virtual void absoluteRects(QValueList<QRect>& rects, int _tx, int _ty);

#if APPLE_CHANGES
    virtual void addFocusRingRects(QPainter *painter, int _tx, int _ty);
#endif

    virtual short minWidth() const { return 0; }
    virtual short maxWidth() const { return 0; }

    RenderStyle* style() const { return m_style; }
    RenderStyle* style( bool firstLine ) const;

    void getTextDecorationColors(int decorations, QColor& underline, QColor& overline,
                                 QColor& linethrough, bool quirksMode=false);

    enum BorderSide {
        BSTop, BSBottom, BSLeft, BSRight
    };
    void drawBorder(QPainter *p, int x1, int y1, int x2, int y2, BorderSide s,
                    QColor c, const QColor& textcolor, EBorderStyle style,
                    int adjbw1, int adjbw2, bool invalidisInvert = false);

    virtual void setTable(RenderTable*) {};

    // Used by collapsed border tables.
    virtual void collectBorders(QValueList<CollapsedBorderValue>& borderStyles);

    // Repaint the entire object.  Called when, e.g., the color of a border changes, or when a border
    // style changes.
    void repaint(bool immediate = false);

    // Repaint a specific subrectangle within a given object.  The rect |r| is in the object's coordinate space.
    void repaintRectangle(const QRect& r, bool immediate = false);
    
#ifdef INCREMENTAL_REPAINTING
    // Repaint only if our old bounds and new bounds are different.
    virtual void repaintAfterLayoutIfNeeded(const QRect& oldBounds, const QRect& oldFullBounds);

    // Repaint only if the object moved.
    virtual void repaintDuringLayoutIfMoved(int oldX, int oldY);

    // Called to repaint a block's floats.
    virtual void repaintFloatingDescendants();

    // Called before layout to repaint all dirty children (with selfNeedsLayout() set).
    virtual void repaintObjectsBeforeLayout();

    bool checkForRepaintDuringLayout() const;
#endif

    // Returns the rect that should be repainted whenever this object changes.  The rect is in the view's
    // coordinate space.  This method deals with outlines and overflow.
    virtual QRect getAbsoluteRepaintRect();

    QRect getAbsoluteRepaintRectWithOutline(int ow);

#ifdef INCREMENTAL_REPAINTING
    virtual void getAbsoluteRepaintRectIncludingFloats(QRect& bounds, QRect& boundsWithChildren);
#endif

    // Given a rect in the object's coordinate space, this method converts the rectangle to the view's
    // coordinate space.
    virtual void computeAbsoluteRepaintRect(QRect& r, bool f=false);
    
    virtual unsigned int length() const { return 1; }

    bool isFloatingOrPositioned() const { return (isFloating() || isPositioned()); };
    virtual bool containsFloats() { return false; }
    virtual bool containsFloat(RenderObject* o) { return false; }
    virtual bool hasOverhangingFloats() { return false; }

    bool avoidsFloats() const;
    bool usesLineWidth() const;

    // positioning of inline children (bidi)
    virtual void position(InlineBox*, int, int, bool) {}

    // Applied as a "slop" to dirty rect checks during the outline painting phase's dirty-rect checks.
    int maximalOutlineSize(PaintAction p) const;

    enum SelectionState {
        SelectionNone,
        SelectionStart,
        SelectionInside,
        SelectionEnd,
        SelectionBoth
    };

    virtual SelectionState selectionState() const { return SelectionNone;}
    virtual void setSelectionState(SelectionState) {}

    virtual void cursorPos(int /*offset*/, int &/*_x*/, int &/*_y*/, int &/*height*/);

    virtual int lowestPosition(bool includeOverflowInterior=true, bool includeSelf=true) const { return 0; }
    virtual int rightmostPosition(bool includeOverflowInterior=true, bool includeSelf=true) const { return 0; }
    virtual int leftmostPosition(bool includeOverflowInterior=true, bool includeSelf=true) const { return 0; }
    
    virtual void calcVerticalMargins() {}
    void removeFromObjectLists();

    virtual void detach();

    const QFont &font(bool firstLine) const {
	return style( firstLine )->font();
    }

    const QFontMetrics &fontMetrics(bool firstLine) const {
	return style( firstLine )->fontMetrics();
    }

    // Virtual function helpers for CSS3 Flexible Box Layout
    virtual bool isFlexibleBox() const { return false; }
    virtual bool isFlexingChildren() const { return false; }
    virtual bool isStretchingChildren() const { return false; }

    // Convenience, to avoid repeating the code to dig down to get this.
    QChar backslashAsCurrencySymbol() const;

    virtual void setPixmap(const QPixmap&, const QRect&, CachedImage *);

protected:
    virtual void selectionStartEnd(int& spos, int& epos);

    virtual void printBoxDecorations(QPainter* /*p*/, int /*_x*/, int /*_y*/,
                                     int /*_w*/, int /*_h*/, int /*_tx*/, int /*_ty*/) {}

    virtual QRect viewRect() const;

    void remove();

    void invalidateVerticalPositions();
    short getVerticalPosition( bool firstLine ) const;

    virtual void removeLeftoverAnonymousBoxes();
    
    void arenaDelete(RenderArena *arena);

private:
    RenderStyle* m_style;

    DOM::NodeImpl* m_node;

    RenderObject *m_parent;
    RenderObject *m_previous;
    RenderObject *m_next;

    short m_verticalPosition;

    bool m_needsLayout               : 1;
    bool m_normalChildNeedsLayout    : 1;
    bool m_posChildNeedsLayout       : 1;
    bool m_minMaxKnown               : 1;
    bool m_floating                  : 1;

    bool m_positioned                : 1;
    bool m_overhangingContents       : 1;
    bool m_relPositioned             : 1;
    bool m_paintBackground           : 1; // if the box has something to paint in the
                                          // background painting phase (background, border, etc)

    bool m_isAnonymous               : 1;
    bool m_recalcMinMax 	     : 1;
    bool m_isText                    : 1;
    bool m_inline                    : 1;
    bool m_replaced                  : 1;
    bool m_mouseInside               : 1;
    bool m_isSelectionBorder         : 1;

    void arenaDelete(RenderArena *arena, void *objectBase);

    // note: do not add unnecessary bitflags, we have 32 bit already!
    friend class RenderListItem;
    friend class RenderContainer;
    friend class RenderCanvas;
};


enum VerticalPositionHint {
    PositionTop = -0x4000,
    PositionBottom = 0x4000,
    PositionUndefined = 0x3fff
};

}; //namespace
#endif
