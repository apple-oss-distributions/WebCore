/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */


#ifndef WebKitPoint_h
#define WebKitPoint_h

#include <wtf/RefCounted.h>

namespace WebCore {

    class WebKitPoint : public RefCounted<WebKitPoint> {
    public:
        WebKitPoint(float x=0, float y=0) : m_x(x), m_y(y) { }

        float x() const { return m_x; }
        float y() const { return m_y; }
        void setX(float x) { m_x = x; }
        void setY(float y) { m_y = y; }

    private:
        float m_x, m_y;
    };

} // namespace WebCore

#endif // WebKitPoint_h
