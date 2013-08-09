/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#include "config.h"
#include "GestureEvent.h"

#if ENABLE(IOS_GESTURE_EVENTS)

#include "EventNames.h"

namespace WebCore {

GestureEvent::GestureEvent( const AtomicString& type, bool canBubble, bool cancelable, AbstractView* view, int detail,
                            int screenX, int screenY, int pageX, int pageY,
                            bool ctrlKey, bool altKey, bool shiftKey, bool metaKey,
                            EventTarget* target, float scale, float rotation, bool isSimulated)
    : MouseRelatedEvent(type, canBubble, cancelable, view, detail, IntPoint(screenX, screenY),
                        IntPoint(pageX, pageY), ctrlKey, altKey, shiftKey, metaKey, isSimulated),
    m_scale(scale), m_rotation(rotation)
{
    setTarget(target);
}

void GestureEvent::initGestureEvent(const AtomicString& type, bool canBubble, bool cancelable, AbstractView* view, int detail,
    int screenX, int screenY, int clientX, int clientY,
    bool ctrlKey, bool altKey, bool shiftKey, bool metaKey,
    EventTarget* target, float scale, float rotation)
{
    if (dispatched())
        return;

    initUIEvent(type, canBubble, cancelable, view, detail);

    m_screenLocation = IntPoint(screenX, screenY);
    m_ctrlKey = ctrlKey;
    m_altKey = altKey;
    m_shiftKey = shiftKey;
    m_metaKey = metaKey;

    initCoordinates(IntPoint(clientX, clientY));

    setTarget(target);
    
    m_scale = scale;
    m_rotation = rotation;
}

const AtomicString& GestureEvent::interfaceName() const
{
    return eventNames().interfaceForGestureEvent;
}

} // namespace WebCore

#endif // ENABLE(IOS_GESTURE_EVENTS)
