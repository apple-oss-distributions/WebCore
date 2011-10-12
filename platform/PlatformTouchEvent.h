/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * Permission is granted by Apple to use this file to the extent
 * necessary to relink with LGPL WebKit files.
 *
 * No license or rights are granted by Apple expressly or by
 * implication, estoppel, or otherwise, to Apple patents and
 * trademarks. For the sake of clarity, no license or rights are
 * granted by Apple expressly or by implication, estoppel, or otherwise,
 * under any Apple patents, copyrights and trademarks to underlying
 * implementations of any application programming interfaces (APIs)
 * or to any functionality that is invoked by calling any API.
 */

#ifndef PlatformTouchEvent_h
#define PlatformTouchEvent_h

#include <wtf/Platform.h>


#ifdef __OBJC__
@class WebEvent;
#else
class WebEvent;
#endif

#include <wtf/Vector.h>
#include "IntPoint.h"

namespace WebCore {

enum TouchEventType { TouchEventBegin, TouchEventChange, TouchEventEnd, TouchEventCancel };
enum TouchPhaseType { TouchPhaseBegan, TouchPhaseMoved, TouchPhaseStationary, TouchPhaseEnded, TouchPhaseCancelled };

class PlatformTouchEvent {
public:
    PlatformTouchEvent(WebEvent *);
    PlatformTouchEvent(const PlatformTouchEvent& other)
        : m_type(other.m_type)
        , m_touchCount(other.m_touchCount)
        , m_touchLocations(other.m_touchLocations)
        , m_touchGlobalLocations(other.m_touchGlobalLocations)
        , m_touchIdentifiers(other.m_touchIdentifiers)
        , m_touchPhases(other.m_touchPhases)
        , m_gestureScale(other.m_gestureScale)
        , m_gestureRotation(other.m_gestureRotation)
        , m_isGesture(other.m_isGesture)
        , m_position(other.m_position)
        , m_globalPosition(other.m_globalPosition)
        , m_shiftKey(other.m_shiftKey)
        , m_ctrlKey(other.m_ctrlKey)
        , m_altKey(other.m_altKey)
        , m_metaKey(other.m_metaKey)
    {
    }
    
    TouchEventType eventType() const { return m_type; }
    unsigned touchCount() const { return m_touchCount; }
    IntPoint touchLocationAtIndex(unsigned i) const { return m_touchLocations[i]; }
    IntPoint globalTouchLocationAtIndex(unsigned i) const { return m_touchGlobalLocations[i]; }
    unsigned touchIdentifierAtIndex(unsigned i) const { return m_touchIdentifiers[i]; }
    TouchPhaseType touchPhaseAtIndex(unsigned i) const { return m_touchPhases[i]; }
    
    bool isGesture() const { return m_isGesture; }
    
    float scale() const { return m_gestureScale; }
    float rotation() const { return m_gestureRotation; }

    const IntPoint& pos() const { return m_position; }
    int x() const { return m_position.x(); }
    int y() const { return m_position.y(); }
    int globalX() const { return m_globalPosition.x(); }
    int globalY() const { return m_globalPosition.y(); }

    bool shiftKey() const { return m_shiftKey; }
    bool ctrlKey() const { return m_ctrlKey; }
    bool altKey() const { return m_altKey; }
    bool metaKey() const { return m_metaKey; }

protected:
    TouchEventType m_type;
    unsigned m_touchCount;
    Vector<IntPoint> m_touchLocations;
    Vector<IntPoint> m_touchGlobalLocations;
    Vector<unsigned> m_touchIdentifiers;
    Vector<TouchPhaseType> m_touchPhases;
    float m_gestureScale;
    float m_gestureRotation;
    bool m_isGesture;
    IntPoint m_position;
    IntPoint m_globalPosition;
    bool m_shiftKey;
    bool m_ctrlKey;
    bool m_altKey;
    bool m_metaKey;
};

} // namespace WebCore


#endif // PlatformTouchEvent_h
