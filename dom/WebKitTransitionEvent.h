/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef WebKitTransitionEvent_h
#define WebKitTransitionEvent_h

#include "Event.h"

namespace WebCore {
        
    class WebKitTransitionEvent : public Event {
    public:
        WebKitTransitionEvent();
        WebKitTransitionEvent(const AtomicString& type, const String& propertyName, double elapsedTime);
        virtual ~WebKitTransitionEvent();
        
        void initWebKitTransitionEvent(const AtomicString& type, 
                                bool canBubbleArg,
                                bool cancelableArg,
                                const String& propertyName,
                                double elapsedTime);
                        
        const String& propertyName() const;
        double elapsedTime() const;
        
        virtual bool isWebKitTransitionEvent() const { return true; }
        
    private:
        String m_propertyName;
        double m_elapsedTime;
    };
    
} // namespace WebCore

#endif // WebKitTransitionEvent_h
