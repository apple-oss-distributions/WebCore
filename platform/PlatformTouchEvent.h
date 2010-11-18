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
    
    TouchEventType eventType() const { return m_type; }
    unsigned touchCount() const { return m_touchCount; }
    IntPoint touchLocationAtIndex(unsigned i) const { return m_touchLocations[i]; }
    IntPoint globalTouchLocationAtIndex(unsigned i) const { return m_touchGlobalLocations[i]; }
    unsigned touchIdentifierAtIndex(unsigned i) const { return m_touchIdentifiers[i]; }
    TouchPhaseType touchPhaseAtIndex(unsigned i) const { return m_touchPhases[i]; }
    
    bool isGesture() const { return m_isGesture; }
    
    float scale() const { return m_gestureScale; }
    float rotation() const { return m_gestureRotation; }

private:
    TouchEventType m_type;
    unsigned m_touchCount;
    Vector<IntPoint> m_touchLocations;
    Vector<IntPoint> m_touchGlobalLocations;
    Vector<unsigned> m_touchIdentifiers;
    Vector<TouchPhaseType> m_touchPhases;
    bool m_isGesture;
    float m_gestureScale;
    float m_gestureRotation;
};

} // namespace WebCore


#endif // PlatformTouchEvent_h
