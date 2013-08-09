/*
 * Copyright (C) 2008, 2009, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#include "config.h"
#include "Document.h"

#if PLATFORM(IOS)

#if ENABLE(TOUCH_EVENTS)

#include "Chrome.h"
#include "ChromeClient.h"
#include "FrameView.h"
#include "HTMLFrameOwnerElement.h"
#include "Page.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "Touch.h"
#include "TouchList.h"

namespace WebCore {

PassRefPtr<Touch> Document::createTouch(DOMWindow* view, EventTarget* target, long identifier, long pageX, long pageY, long screenX, long screenY, ExceptionCode&)
{
    return Touch::create(view, target, identifier, pageX, pageY, screenX, screenY);
}

PassRefPtr<TouchList> Document::createTouchList(ExceptionCode&)
{
    return TouchList::create();
}

void Document::setInTouchEventHandling(bool handling)
{
    Document* topDoc = topDocument();
    if (topDoc && this != topDoc) {
        topDoc->setInTouchEventHandling(handling);
        return;
    }
    
    if (m_handlingTouchEvent == handling)
        return;
        
    m_handlingTouchEvent = handling;

    // If we're done touch handling, see if the touch event regions need to be reset
    if (!m_handlingTouchEvent)
        setTouchEventListenersDirty(m_touchEventRegionsDirty);
}

void Document::addTouchEventListener(Node* listener)
{
    Document* topDoc = topDocument();
    if (topDoc && this != topDoc) {
        topDoc->addTouchEventListener(listener);
        return;
    }

    TouchListenerMap::AddResult result = m_touchEventListenersCounts.add(listener);
    if (result.isNewEntry)
        setTouchEventListenersDirty(true);

    if (Page* page = this->page())
        page->chrome().client()->needTouchEvents(true);
}

void Document::removeTouchEventListener(Node* listener, bool removeAll)
{
    Document* topDoc = topDocument();
    if (topDoc && this != topDoc) {
        topDoc->removeTouchEventListener(listener, removeAll);
        return;
    }

    if (removeAll) {
        if (m_touchEventListenersCounts.contains(listener)) {
            m_touchEventListenersCounts.removeAll(listener);
            setTouchEventListenersDirty(true);
        }
    } else {
        bool wasRemoved = m_touchEventListenersCounts.remove(listener);
        if (wasRemoved)
            setTouchEventListenersDirty(true);
    }

    if (Page* page = this->page())
        page->chrome().client()->needTouchEvents(!m_touchEventListenersCounts.isEmpty());
}

void Document::dirtyTouchEventRects()
{
    Document* topDoc = topDocument();
    if (topDoc && this != topDoc) {
        topDoc->dirtyTouchEventRects();
        return;
    }

    if (m_touchEventListenersCounts.size() == 0)
        return;
    
    setTouchEventListenersDirty(true);
}

void Document::setTouchEventListenersDirty(bool dirty)
{
    ASSERT(this == topDocument());

    m_touchEventRegionsDirty = dirty;

    if (!dirty)
        return;
    
    // We bail out now, but this will be caught when no longer in TouchEventHandling above
    if (m_handlingTouchEvent)
        return;
        
    if (!m_touchEventsChangedTimer.isActive())
        m_touchEventsChangedTimer.startOneShot(0);
}

void Document::clearTouchEventListeners()
{
    Document* topDoc = topDocument();
    if (topDoc && this != topDoc) {
        topDoc->removeTouchEventListenersInDocument(this);
        return;
    }

    setTouchEventListenersDirty(true);

    m_touchEventListenersCounts.clear();

    if (Page* page = this->page())
        page->chrome().client()->needTouchEvents(false);

    MutexLocker lock(m_touchEventsRectMutex);
    m_touchEventRects.clear();
}
    
const Document::TouchListenerMap& Document::touchEventListeners() const
{
    Document* topDoc = topDocument();
    if (topDoc && this != topDoc)
        return topDoc->touchEventListeners();

    return m_touchEventListenersCounts;
}

void Document::getTouchRects(Vector<IntRect>& rects)
{
    ASSERT(this == topDocument());

    MutexLocker lock(m_touchEventsRectMutex);
    
    rects.clear();
    rects.reserveCapacity(m_touchEventRects.size());
    
    TouchEventRectMap::const_iterator end = m_touchEventRects.end();
    for (TouchEventRectMap::const_iterator it = m_touchEventRects.begin(); it != end; ++it)
        rects.appendVector((*it).value);

    if (domWindow() && domWindow()->hasTouchEventListeners())
        rects.append(IntRect(INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX));
}

IntRect Document::eventRectRelativeToRoot(Node* node, RenderObject* renderer)
{
    // Special case the document, where we want the whole view.
    if (node && node->isDocumentNode())
        return toRenderView(renderer)->documentRect();

    // FIXME: This really should not be using the same rects used for repaint (which
    // is what absoluteClippedOverflowRect uses), but rather RenderObjects should be
    // able to produce a rect that it would use for hit testing.
    IntRect eventRegionRect = pixelSnappedIntRect(renderer->absoluteClippedOverflowRect());

    if (!node || !node->isElementNode())
        return eventRegionRect;
    
    Element* element = static_cast<Element*>(node);
    while (element && (element = element->document()->ownerElement()) && element->renderer())
        eventRegionRect = element->document()->view()->convertFromRenderer(element->renderer(), eventRegionRect);
    return eventRegionRect;
}

void Document::touchEventsChangedTimerFired(Timer<Document>*)
{
    ASSERT(this == topDocument());

    m_touchEventsChangedTimer.stop();
    
    // Rebuild the regions
    MutexLocker lock(m_touchEventsRectMutex);
    
    m_touchEventRects.clear();
    
    TouchListenerMap::const_iterator end = m_touchEventListenersCounts.end();
    for (TouchListenerMap::const_iterator it = m_touchEventListenersCounts.begin(); it != end; ++it) {
        WebCore::Node* node = (*it).key;
        
        RenderObject* renderer = node->renderer();
        if (!renderer)
            continue;
        
        Vector<IntRect> nodeRects;
        
        IntRect eventRegionRect = eventRectRelativeToRoot(node, renderer);
        nodeRects.append(eventRegionRect);
        
        // This Node may have children outside of its bounds that need to be accounted for with their own WebEventRegions.
        checkChildRenderers(renderer, eventRegionRect, nodeRects);
        
        m_touchEventRects.set(node, nodeRects);
    }

    m_touchEventRegionsDirty = false;
}

// Recurse through descendant renderers, checking for those which might project outside the bounds of
// their ancestor (whose event region quad is passed in containingQuad), and only create additional event regions
// when we detect that the child's region is outside that of the parent..
void Document::checkChildRenderers(RenderObject* o, const IntRect& containingRect, Vector<IntRect>& nodeRects)
{
    ASSERT(this == topDocument());

    for (RenderObject* currChild = o->firstChild(); currChild; currChild = currChild->nextSibling()) {
        // Don't both looking at this renderer if its node is in the event listener list.
        if (currChild->node() && m_touchEventListenersCounts.contains(currChild->node()))
            continue;
        
        if (currChild->isFloating() || currChild->isOutOfFlowPositioned() || currChild->hasTransform()) {
            IntRect currRect = eventRectRelativeToRoot(currChild->node(), currChild);
            if (!containingRect.contains(currRect))
                nodeRects.append(currRect);
            else
                currRect = containingRect;
            
            checkChildRenderers(currChild, currRect, nodeRects);
        } else
            checkChildRenderers(currChild, containingRect, nodeRects);
    }
}

void Document::removeTouchEventListenersInDocument(Document* document)
{
    ASSERT(this == topDocument());
    
    Vector<Node*> nodesToRemove;
    TouchListenerMap::const_iterator mapEnd = m_touchEventListenersCounts.end();
    for (TouchListenerMap::const_iterator it = m_touchEventListenersCounts.begin(); it != mapEnd; ++it) {
        WebCore::Node* node = (*it).key;
        
        if (node->document() == document)
            nodesToRemove.append(node);
    }
    
    Vector<Node*>::const_iterator vectorEnd = nodesToRemove.end();
    for (Vector<Node*>::const_iterator it = nodesToRemove.begin(); it != vectorEnd; ++it)
        removeTouchEventListener(*it, true);
}

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)
#endif // PLATFORM(IOS)
