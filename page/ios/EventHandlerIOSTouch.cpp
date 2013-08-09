/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#include "config.h"
#include "EventHandler.h"

#if ENABLE(TOUCH_EVENTS)

#include "Chrome.h"
#include "ChromeClient.h"
#include "Document.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameView.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "Page.h"
#include "RenderBox.h"
#include "RenderLayer.h"
#include "RenderView.h"
#include "UserGestureIndicator.h"
#include "WebCoreSystemInterface.h"

#include "PlatformTouchEventIOS.h"
#include "GestureEvent.h"
#include "TouchEvent.h"

#include <CoreFoundation/CFPriv.h>

namespace WebCore {

bool EventHandler::dispatchTouchEvent(const PlatformTouchEvent& platformEvent, const AtomicString& eventType, const EventTargetTouchMap& touches, float scale, float rotation)
{
    FrameView* view = m_frame->view();
    if (!view)
        return false;

    // Iterate through nodes that need event delivered
    bool handled = false;
    EventTargetTouchMap::const_iterator end = touches.end();
    for (EventTargetTouchMap::const_iterator it = touches.begin(); it != end; ++it) {
        RefPtr<EventTarget> evtTarget = (*it).key;

        RefPtr<TouchList> touches = TouchList::create();
        RefPtr<TouchList> targetTouches = TouchList::create();
        RefPtr<TouchList> changedTouches = TouchList::create();
        
        // changedTouches
        const TouchArray* nodeTouches = (*it).value;
        for (size_t i = 0; i < nodeTouches->size(); ++i)
            changedTouches->append(nodeTouches->at(i));
        
        // touches and targetTouches
        for (size_t i = 0; i < m_touches.size(); ++i) {
            Touch* touch = m_touches[i].get();
            touches->append(touch);
            if (touch->target() == evtTarget)
                targetTouches->append(touch);
        }

        // Build a TouchEvent from the Touches for this EventTarget
        RefPtr<TouchEvent> touchEvent = TouchEvent::create(eventType,
            true, // canBubble
            true, // cancelable
            m_frame->document()->defaultView(),
            0, // detail
            platformEvent.globalX(), // screenX
            platformEvent.globalY(), // screenY
            platformEvent.globalX(), // "windowX" will be converted to pageX in MouseRelatedEvent::MouseRelatedEvent
            platformEvent.globalY(), // "windowY" will be converted to pageY in MouseRelatedEvent::MouseRelatedEvent
            platformEvent.ctrlKey(),
            platformEvent.altKey(),
            platformEvent.shiftKey(),
            platformEvent.metaKey(), 
            touches.get(),
            targetTouches.get(),
            changedTouches.get(),
            scale, rotation);
        
        touchEvent->setPlatformTouchEvent(platformEvent);

        // Deliver the TouchEvent
        ExceptionCode ec = 0;
        evtTarget->dispatchEvent(touchEvent, ec);
        
        if ((touchEvent->defaultPrevented() || touchEvent->defaultHandled()) && m_frame->page()) {
            m_frame->page()->chrome().client()->didPreventDefaultForEvent();
            handled = true;
        }
    }
    
    return handled;
}

#if ENABLE(IOS_GESTURE_EVENTS)
bool EventHandler::dispatchGestureEvent(const PlatformTouchEvent& platformEvent, const AtomicString& eventType, const EventTargetSet& targets, float scale, float rotation)
{
    FrameView* view = m_frame->view();
    if (!view)
        return false;

    // Iterate through nodes that need event delivered
    bool handled = false;
    EventTargetSet::const_iterator end = targets.end();
    for (EventTargetSet::const_iterator it = targets.begin(); it != end; ++it) {
        RefPtr<EventTarget> evtTarget = *it;

        RefPtr<GestureEvent> gestureEvent = GestureEvent::create(eventType,
            true, // canBubble
            true, // cancelable
            m_frame->document()->defaultView(),
            0, // detail
            platformEvent.globalX(), // screenX
            platformEvent.globalY(), // screenY
            platformEvent.globalX(), // "windowX" will be converted to pageX in MouseRelatedEvent::MouseRelatedEvent
            platformEvent.globalY(), // "windowY" will be converted to pageY in MouseRelatedEvent::MouseRelatedEvent
            platformEvent.ctrlKey(),
            platformEvent.altKey(),
            platformEvent.shiftKey(),
            platformEvent.metaKey(), 
            evtTarget.get(),
            scale, rotation);

        ExceptionCode ec = 0;
        evtTarget->dispatchEvent(gestureEvent, ec);
        
        if (gestureEvent->defaultPrevented() && m_frame->page()) {
            m_frame->page()->chrome().client()->didPreventDefaultForEvent();
            handled = true;
        }
    }
    
    return handled;
}
#endif

static bool allTouchesAreTouchPhaseBegan(const PlatformTouchEvent& e)
{
    for (unsigned touchIndex = 0; touchIndex < e.touchCount(); touchIndex++) {
        if (e.touchPhaseAtIndex(touchIndex) != PlatformTouchPoint::TouchPhaseBegan)
            return false;
    }
    return true;
}

static EventHandler::TouchArray* getTargetTouches(EventTarget* evtTarget, EventHandler::EventTargetTouchMap& touches)
{
    EventHandler::TouchArray* targetTouches = touches.get(evtTarget);
    if (!targetTouches) {
        targetTouches = new EventHandler::TouchArray;
        touches.set(evtTarget, targetTouches);
    }
    return targetTouches;
}

static bool shouldPassTouchEventsToSubframes()
{
    // Preserve <rdar://problem/7992472> behavior for third party applications. See <rdar://problem/8463725>.
    static bool shouldPassTouchEvents = iosExecutableWasLinkedOnOrAfterVersion(wkIOSSystemVersion_4_2);
    return shouldPassTouchEvents;
}

static bool shouldSendTouchEventAboveShadowRoot()
{
    // Preserve <rdar://problem/6757734> behavior for third party applications. See: <rdar://problem/9083142>.
    static bool shouldSendTouchEventAboveShadow = iosExecutableWasLinkedOnOrAfterVersion(wkIOSSystemVersion_5_0);
    return shouldSendTouchEventAboveShadow;
}

static PassRefPtr<Touch> touchWithIdentifier(EventHandler::TouchArray& touches, unsigned identifier)
{
    for (size_t i = 0; i < touches.size(); ++i) {
        Touch* currTouch = touches[i].get();
        if (currTouch->identifier() == identifier)
            return currTouch;
    }
    return 0;
}

bool EventHandler::handleTouchEvent(const PlatformTouchEvent& e)
{
    Document* doc = m_frame->document();
    if (!doc)
        return false;

    UserGestureIndicator gestureIndicator(DefinitelyProcessingNewUserGesture);

    RefPtr<FrameView> view = m_frame->view();
    if (!view)
        return false;

    view->resetDeferredRepaintDelay();

    EventTargetTouchMap touchesBegan;
    EventTargetTouchMap touchesMoved;
    EventTargetTouchMap touchesEnded;
    
    // Copy node touches into touchesEnded; they will be removed as they are found in the new set of touches so that
    // the nodes that remain in touchesEnded are those that are no longer being touched.
    {
        for (size_t i = 0; i < m_touches.size(); ++i) {
            Touch* touch = m_touches[i].get();
            EventTarget* evtTarget = touch->target();
            if (!evtTarget) {
                ASSERT_NOT_REACHED();
                continue;
            }

            getTargetTouches(evtTarget, touchesEnded)->append(touch);
        }
    }
    
    EventTargetSet gesturesStarted;
    EventTargetSet gesturesChanged;
    EventTargetSet gesturesEnded = m_gestureTargets; // Remove nodes from gesturesEnded as found

    bool wasInGesture = m_touches.size() > 1;
    bool gestureEnding = wasInGesture && !e.isGesture();
    bool gestureUnknown = (m_gestureInitialDiameter == GestureUnknown);
    bool inTouchEventHandling = false;
    
    for (unsigned touchIndex = 0; touchIndex < e.touchCount(); touchIndex++) {
        unsigned touchIdentifier = e.touchIdentifierAtIndex(touchIndex);

        IntPoint screenPoint = e.touchLocationAtIndex(touchIndex);
        IntPoint contentsPos = view->windowToContents(screenPoint);
        PlatformTouchPoint::TouchPhaseType touchPhase = e.touchPhaseAtIndex(touchIndex);
        
        inTouchEventHandling |= ((touchPhase != PlatformTouchPoint::TouchPhaseEnded) && (touchPhase != PlatformTouchPoint::TouchPhaseCancelled));

        RefPtr<Touch> touch = touchWithIdentifier(m_touches, touchIdentifier);
        if (!touch) {
            // This is a new touch.
            ASSERT(touchPhase == PlatformTouchPoint::TouchPhaseBegan || touchPhase == PlatformTouchPoint::TouchPhaseMoved || touchPhase == PlatformTouchPoint::TouchPhaseEnded);

            if (m_touches.size() == 0)
                m_firstTouchID = touchIdentifier;
                        
            // Find the Node under the touch.            
            bool isFirstTouch = (m_firstTouchID == touchIdentifier);
            HitTestRequest request(isFirstTouch ? HitTestRequest::Active : HitTestRequest::ReadOnly);
            HitTestResult result(contentsPos);
            if (!m_frame->contentRenderer())
                continue;
            m_frame->contentRenderer()->hitTest(request, result);
            if (!request.readOnly())
                m_frame->document()->updateHoverActiveState(request, result.innerElement());
            Node* node = result.innerNode();
            if (!node)
                continue;
            
            if (shouldPassTouchEventsToSubframes()) {
                // If this is the first touch of a new touch, do subframe hittesting
                if (isFirstTouch && e.type() == PlatformEvent::TouchStart && allTouchesAreTouchPhaseBegan(e))
                    m_touchEventTargetSubframe = EventHandler::subframeForTargetNode(node);
                if (m_touchEventTargetSubframe && isFirstTouch) {
                    // Pass the entire touch event to the subframe for handling.
                    return m_touchEventTargetSubframe->eventHandler()->handleTouchEvent(e);
                }
            }

            Node *touchedNode = node;
            bool sendTouch = false;
            if (m_frame->document()->domWindow()->hasTouchEventListeners())
                sendTouch = true;
            else {
                do {
                    sendTouch = doc->touchEventListeners().contains(touchedNode);
                    touchedNode = shouldSendTouchEventAboveShadowRoot() ? touchedNode->parentOrShadowHostNode() : touchedNode->parentNode();
                } while (touchedNode && !sendTouch);
            }

            if (sendTouch) {
                // If the target node is a text node, dispatch on the parent node. This is the way mouse events work.
                // FIXME: why are we duplicating code from mouse events? We should share.
                if (node && node->isTextNode())
                    node = node->parentNode();
                if (node)
                    node = node->deprecatedShadowAncestorNode();
                
                // Create touch
                touch = Touch::create(m_frame->document()->defaultView(), node, touchIdentifier, contentsPos.x(), contentsPos.y(), screenPoint.x(), screenPoint.y());
                m_touches.append(touch);

                // Send ontouchstart to node
                getTargetTouches(node, touchesBegan)->append(touch);
            } else {
                if (isFirstTouch) {
                    // The touch hitTest in a Node that doesn't have a touch event handler,
                    // but hit testing has enabled ":active", so we need to 'un-activate'
                    HitTestRequest request(HitTestRequest::Release);
                    HitTestResult result(contentsPos);
                    m_frame->contentRenderer()->hitTest(request, result);
                    m_frame->document()->updateHoverActiveState(request, result.innerElement());
                }
                // It is possible, in a gesture, for multiple touches to begin at
                // the same time and one of those touches to not have a touch event
                // handler. In that case, create the touch because it might move
                // or end, and we expect the touch to exist.
                if (e.isGesture()) {
                    touch = Touch::create(m_frame->document()->defaultView(), node, touchIdentifier, contentsPos.x(), contentsPos.y(), screenPoint.x(), screenPoint.y());
                    m_touches.append(touch);
                }
            }
        } else {
            // This is a touch that is already being tracked (and sometimes TouchPhaseBegan comes through).
            if ((touchPhase == PlatformTouchPoint::TouchPhaseBegan) || (touchPhase == PlatformTouchPoint::TouchPhaseMoved) || (touchPhase == PlatformTouchPoint::TouchPhaseStationary)) {
                EventTarget* evtTarget = touch->target();
                ASSERT(evtTarget);
                
                if (touch->updateLocation(contentsPos.x(), contentsPos.y(), screenPoint.x(), screenPoint.y())) {
                    // Send ontouchmove to node if location changed
                    // It is possible that a "Stationary" touch on the screen may have changed its
                    // location in the viewport if the browser view scaled underneath the touch.
                    ASSERT(touchPhase == PlatformTouchPoint::TouchPhaseMoved || touchPhase == PlatformTouchPoint::TouchPhaseStationary);
                    getTargetTouches(evtTarget, touchesMoved)->append(touch);
                }

                // Remove from touchEnded set
                TouchArray* nodeTouches = touchesEnded.get(evtTarget);
                if (nodeTouches) {
                    size_t nodeIndex = nodeTouches->find(touch);
                    if (nodeIndex != WTF::notFound)
                        nodeTouches->remove(nodeIndex);
                    if (nodeTouches->size() == 0) {
                        touchesEnded.remove(evtTarget);
                        delete nodeTouches;
                    }
                }
            } else
                ASSERT((touchPhase == PlatformTouchPoint::TouchPhaseEnded) || (touchPhase == PlatformTouchPoint::TouchPhaseCancelled));

            if (e.isGesture()) {
                RefPtr<EventTarget> target = touch->target();
                if (!wasInGesture || !m_gestureTargets.contains(target)) {
                    // Gestures started, or first gesture event for this target                    
                    gesturesStarted.add(target);
                    m_gestureTargets.add(target);
                } else {
                    // Gestures changed
                    if (e.isGesture())
                        gesturesChanged.add(target);
                }
                gesturesEnded.remove(target);
            }
        }
    }

    doc->setInTouchEventHandling(inTouchEventHandling);
    
    // Remove touches that ended
    {
        EventTargetTouchMap::iterator end = touchesEnded.end();
        for (EventTargetTouchMap::iterator it = touchesEnded.begin(); it != end; ++it) {
            TouchArray* targetTouches = (*it).value;
            for (size_t i = 0; i < targetTouches->size(); ++i) {
                RefPtr<Touch> touch = targetTouches->at(i);
                if (touch->identifier() == m_firstTouchID) {
                    HitTestRequest request(HitTestRequest::Release);
                    HitTestResult result(view->windowToContents(IntPoint(touch->clientX(), touch->clientY())));
                    m_frame->contentRenderer()->hitTest(request, result);
                    m_frame->document()->updateHoverActiveState(request, result.innerElement());
                }
                
                size_t touchIndex = m_touches.find(touch);
                if (touchIndex != WTF::notFound)
                    m_touches.remove(touchIndex);
            }
        }
    }

    // Remove gestures that ended
    {
        EventTargetSet::iterator end = gesturesEnded.end();
        for (EventTargetSet::iterator it = gesturesEnded.begin(); it != end; ++it) {
            m_gestureTargets.remove(*it);
        }
    }
    
    if (e.isGesture()) {
        // Lazily set these; they are not set until e.isGesture() returns true, but
        //  we're saying that gestures start when there is > 1 touch.
        if (gestureUnknown) {
            // This is why GestureUnknown == 0.0 (as when we're still
            //  not in gesture, scale returns 0.0).
            m_gestureInitialDiameter = e.scale();
            m_gestureInitialRotation = e.rotation();
        }
    }
    if (!gestureEnding && e.isGesture()) {
        m_gestureLastDiameter = e.scale();
        m_gestureLastRotation = e.rotation();
    }
    
    float scale = (!gestureUnknown) ? m_gestureLastDiameter / m_gestureInitialDiameter : 1.0f;
    float rotation = (!gestureUnknown) ? m_gestureLastRotation - m_gestureInitialRotation : 0.0f;
    
    bool handled = false;

#if ENABLE(IOS_GESTURE_EVENTS)
    handled |= dispatchGestureEvent(e, eventNames().gestureendEvent, gesturesEnded, scale, rotation);
    handled |= dispatchGestureEvent(e, eventNames().gesturestartEvent, gesturesStarted, scale, rotation);
    handled |= dispatchGestureEvent(e, eventNames().gesturechangeEvent, gesturesChanged, scale, rotation);
#endif

    handled |= dispatchTouchEvent(e, (e.type() == PlatformEvent::TouchCancel) ? eventNames().touchcancelEvent : eventNames().touchendEvent, touchesEnded, scale, rotation);
    handled |= dispatchTouchEvent(e, eventNames().touchstartEvent, touchesBegan, scale, rotation);
    handled |= dispatchTouchEvent(e, eventNames().touchmoveEvent, touchesMoved, scale, rotation);
    
    deleteAllValues(touchesEnded);
    deleteAllValues(touchesBegan);
    deleteAllValues(touchesMoved);
    
    if (gestureEnding) {
        m_gestureInitialDiameter = GestureUnknown;
        m_gestureInitialRotation = GestureUnknown;
        m_gestureLastDiameter = GestureUnknown;
        m_gestureLastRotation = GestureUnknown;
    }
    
    return handled;
}

static RenderLayer* layerForNode(Node* node)
{
    if (!node)
        return 0;

    RenderObject* renderer = node->renderer();
    if (!renderer)
        return 0;

    return renderer->enclosingLayer();
}

void EventHandler::defaultTouchEventHandler(Node* startNode, TouchEvent* touchEvent)
{
    if (!startNode || !touchEvent)
        return;

    // Touch events created from JS won't have a platformEvent.
    if (!touchEvent->platformTouchEvent())
        return;

    bool handled = false;
    if (RenderLayer* layer = layerForNode(startNode)) {
        FrameView* frameView = m_frame->view();
        if (frameView && frameView->containsScrollableArea(layer))
            handled = layer->handleTouchEvent(*(touchEvent->platformTouchEvent()));
    }
    
    if (handled)
        touchEvent->setDefaultHandled();
}

}

#endif // ENABLE(TOUCH_EVENTS)
