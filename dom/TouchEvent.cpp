/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#include "config.h"
#include "TouchEvent.h"

#if ENABLE(TOUCH_EVENTS)

#include "EventNames.h"
#if PLATFORM(IOS)
#include "PlatformTouchEventIOS.h"
#endif

namespace WebCore {

TouchEvent::TouchEvent()
    : m_scale(0)
    , m_rotation(0)
{
}

TouchEvent::TouchEvent( const AtomicString& type, bool canBubble, bool cancelable, AbstractView* view, int detail, 
                        int screenX, int screenY, int pageX, int pageY,
                        bool ctrlKey, bool altKey, bool shiftKey, bool metaKey,
                        TouchList* touches, TouchList* targetTouches, TouchList* changedTouches,
                        float scale, float rotation, bool isSimulated)
    : MouseRelatedEvent(type, canBubble, cancelable, view, detail, IntPoint(screenX, screenY),
                        IntPoint(pageX, pageY),
#if ENABLE(POINTER_LOCK)
                        IntPoint(0, 0),
#endif
                        ctrlKey, altKey, shiftKey, metaKey, isSimulated),
        m_touches(touches), m_targetTouches(targetTouches), m_changedTouches(changedTouches),
        m_scale(scale), m_rotation(rotation)
{
}

TouchEvent::~TouchEvent()
{
}

void TouchEvent::initTouchEvent(const AtomicString& type, bool canBubble, bool cancelable, AbstractView* view, int detail,
    int screenX, int screenY, int clientX, int clientY,
    bool ctrlKey, bool altKey, bool shiftKey, bool metaKey,
    TouchList* touches, TouchList* targetTouches, TouchList* changedTouches,
    float scale, float rotation)
{
    if (dispatched())
        return;

    initUIEvent(type, canBubble, cancelable, view, detail);

    m_touches = touches;
    m_targetTouches = targetTouches;
    m_changedTouches = changedTouches;
    m_screenLocation = IntPoint(screenX, screenY);
    m_ctrlKey = ctrlKey;
    m_altKey = altKey;
    m_shiftKey = shiftKey;
    m_metaKey = metaKey;

    initCoordinates(IntPoint(clientX, clientY));
    
    m_touches = touches;
    m_targetTouches = targetTouches;
    m_changedTouches = changedTouches;
    
    m_scale = scale;
    m_rotation = rotation;
}

#if PLATFORM(IOS)
void TouchEvent::setPlatformTouchEvent(const PlatformTouchEvent& platformEvent)
{
    m_platformEvent = adoptPtr(new PlatformTouchEvent(platformEvent));
}
#endif

const AtomicString& TouchEvent::interfaceName() const
{
    return eventNames().interfaceForTouchEvent;
}

bool TouchEvent::isTouchEvent() const
{
    return true;
}

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)
