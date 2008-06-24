/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef FloatQuad_h
#define FloatQuad_h

#include "FloatPoint.h"
#include "FloatRect.h"


namespace WebCore {

// A FloatQuad is a collection of 4 points that result from mapping
// a rectangle through various, possible non-affine transforms.
// The 4 points are in order of the corners of the original rect
// in clockwise order, starting with the top left.
class FloatQuad {
public:
    FloatQuad() { }

    FloatQuad(const FloatPoint& p1, const FloatPoint& p2, const FloatPoint& p3, const FloatPoint& p4)
    : m_p1(p1)
    , m_p2(p2)
    , m_p3(p3)
    , m_p4(p4) { }

    FloatQuad(const FloatRect& inRect)
    : m_p1(inRect.location())
    , m_p2(inRect.right(), inRect.y())
    , m_p3(inRect.right(), inRect.bottom())
    , m_p4(inRect.x(), inRect.bottom()) { }

    FloatPoint p1() const   { return m_p1; }
    FloatPoint p2() const   { return m_p2; }
    FloatPoint p3() const   { return m_p3; }
    FloatPoint p4() const   { return m_p4; }

    void setP1(const FloatPoint& inPoint)   { m_p1 = inPoint; }
    void setP2(const FloatPoint& inPoint)   { m_p2 = inPoint; }
    void setP3(const FloatPoint& inPoint)   { m_p3 = inPoint; }
    void setP4(const FloatPoint& inPoint)   { m_p4 = inPoint; }

    // use enclosingIntRect() to get an intregral rect if necessary.
    FloatRect   boundingBox() const;

    void move(float inDeltaX, float inDeltaY);

private:
    FloatPoint m_p1;
    FloatPoint m_p2;
    FloatPoint m_p3;
    FloatPoint m_p4;

};


}   // namespace WebCore


#endif // FloatQuad_h

