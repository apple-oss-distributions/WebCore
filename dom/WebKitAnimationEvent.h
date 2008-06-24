/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */


#ifndef WebKitAnimationEvent_h
#define WebKitAnimationEvent_h

#include "Event.h"

namespace WebCore {
        
    class WebKitAnimationEvent : public Event {
    public:
        WebKitAnimationEvent();
        WebKitAnimationEvent(const AtomicString& type, const String& animationName, double elapsedTime);
        virtual ~WebKitAnimationEvent();
        
        void initWebKitAnimationEvent(const AtomicString& type, 
                                bool canBubbleArg,
                                bool cancelableArg,
                                const String& animationName,
                                double elapsedTime);
                        
        const String& animationName() const;
        double elapsedTime() const;
        
        virtual bool isWebKitAnimationEvent() const { return true; }
        
    private:
        String m_animationName;
        double m_elapsedTime;
    };
    
} // namespace WebCore

#endif // WebKitAnimationEvent_h
