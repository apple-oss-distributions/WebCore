/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef Transform3D_h
#define Transform3D_h

#include "FloatPoint.h"

namespace WebCore {

class AffineTransform;

class Transform3D {
public:
    Transform3D()                                                   { makeIdentity(); }
    Transform3D(float m11, float m12, float m13, float m14,
                float m21, float m22, float m23, float m24,
                float m31, float m32, float m33, float m34,
                float m41, float m42, float m43, float m44)         { setMatrix(m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34, m41, m42, m43, m44); }
    Transform3D(const Transform3D& t)                               { *this = t; }

    Transform3D(const AffineTransform& t);
    
    Transform3D &operator =(const Transform3D &t)
    {
        setMatrix((float*) t.m_matrix);
        return *this;
    }

    void setMatrix(float m11, float m12, float m13, float m14,
                    float m21, float m22, float m23, float m24,
                    float m31, float m32, float m33, float m34,
                    float m41, float m42, float m43, float m44)
    {
        m_matrix[0][0] = m11; m_matrix[0][1] = m12; m_matrix[0][2] = m13; m_matrix[0][3] = m14; 
        m_matrix[1][0] = m21; m_matrix[1][1] = m22; m_matrix[1][2] = m23; m_matrix[1][3] = m24; 
        m_matrix[2][0] = m31; m_matrix[2][1] = m32; m_matrix[2][2] = m33; m_matrix[2][3] = m34; 
        m_matrix[3][0] = m41; m_matrix[3][1] = m42; m_matrix[3][2] = m43; m_matrix[3][3] = m44;
    }
    
    Transform3D&    makeIdentity()  { setMatrix(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1); return *this; }
    bool isIdentity() const     { return *this == Transform3D(); }
    
    float m11() const           { return m_matrix[0][0]; }
    float m12() const           { return m_matrix[0][1]; }
    float m13() const           { return m_matrix[0][2]; }
    float m14() const           { return m_matrix[0][3]; }
    float m21() const           { return m_matrix[1][0]; }
    float m22() const           { return m_matrix[1][1]; }
    float m23() const           { return m_matrix[1][2]; }
    float m24() const           { return m_matrix[1][3]; }
    float m31() const           { return m_matrix[2][0]; }
    float m32() const           { return m_matrix[2][1]; }
    float m33() const           { return m_matrix[2][2]; }
    float m34() const           { return m_matrix[2][3]; }
    float m41() const           { return m_matrix[3][0]; }
    float m42() const           { return m_matrix[3][1]; }
    float m43() const           { return m_matrix[3][2]; }
    float m44() const           { return m_matrix[3][3]; }

    void setM11(float f)        { m_matrix[0][0] = f; }
    void setM12(float f)        { m_matrix[0][1] = f; }
    void setM13(float f)        { m_matrix[0][2] = f; }
    void setM14(float f)        { m_matrix[0][3] = f; }
    void setM21(float f)        { m_matrix[1][0] = f; }
    void setM22(float f)        { m_matrix[1][1] = f; }
    void setM23(float f)        { m_matrix[1][2] = f; }
    void setM24(float f)        { m_matrix[1][3] = f; }
    void setM31(float f)        { m_matrix[2][0] = f; }
    void setM32(float f)        { m_matrix[2][1] = f; }
    void setM33(float f)        { m_matrix[2][2] = f; }
    void setM34(float f)        { m_matrix[2][3] = f; }
    void setM41(float f)        { m_matrix[3][0] = f; }
    void setM42(float f)        { m_matrix[3][1] = f; }
    void setM43(float f)        { m_matrix[3][2] = f; }
    void setM44(float f)        { m_matrix[3][3] = f; }
    
    Transform3D& scale(float sx, float sy);
    Transform3D& scale3d(float sx, float sy, float sz);
    
    // angles are in degrees. x,y,z must form a unit vector (length 1) or results are undefined
    Transform3D& rotate3d(float x, float y, float z, float angle);
    
    // angles are in degrees. 
    Transform3D& rotate3d(float rx, float ry, float rz);
    Transform3D& translate(float tx, float ty);
    Transform3D& translate3d(float tx, float ty, float tz);
    Transform3D& skew(float sx, float sy);
    Transform3D& applyPerspective(float p);
    
    bool hasPerspective() const { return m_matrix[2][3] != 0.0f; }
    
    // this = mat * this
    Transform3D& multLeft(const Transform3D& mat);
    
    // multiply passed 2D point by matrix (assume z=0)
    void multVecMatrix(float x, float y, float& dstX, float& dstY) const;
    
    // return the inverse of matrix
    Transform3D inverse() const;

    // return an affine transform (lossy!)
    AffineTransform affineTransform() const;
    
    //Equality comparison operator
    friend bool operator ==(const Transform3D &m1, const Transform3D &m2)
    {
        return (m1.m_matrix[0][0] == m2.m_matrix[0][0] &&
                m1.m_matrix[0][1] == m2.m_matrix[0][1] &&
                m1.m_matrix[0][2] == m2.m_matrix[0][2] &&
                m1.m_matrix[0][3] == m2.m_matrix[0][3] &&
                m1.m_matrix[1][0] == m2.m_matrix[1][0] &&
                m1.m_matrix[1][1] == m2.m_matrix[1][1] &&
                m1.m_matrix[1][2] == m2.m_matrix[1][2] &&
                m1.m_matrix[1][3] == m2.m_matrix[1][3] &&
                m1.m_matrix[2][0] == m2.m_matrix[2][0] &&
                m1.m_matrix[2][1] == m2.m_matrix[2][1] &&
                m1.m_matrix[2][2] == m2.m_matrix[2][2] &&
                m1.m_matrix[2][3] == m2.m_matrix[2][3] &&
                m1.m_matrix[3][0] == m2.m_matrix[3][0] &&
                m1.m_matrix[3][1] == m2.m_matrix[3][1] &&
                m1.m_matrix[3][2] == m2.m_matrix[3][2] &&
                m1.m_matrix[3][3] == m2.m_matrix[3][3]);
    }

    friend bool operator !=(const Transform3D &m1, const Transform3D &m2)
    { return !(m1 == m2); }

    friend bool operator <(const Transform3D &m1, const Transform3D &m2)
    {
        return (m1.m_matrix[0][0] < m2.m_matrix[0][0] &&
                m1.m_matrix[0][1] < m2.m_matrix[0][1] &&
                m1.m_matrix[0][2] < m2.m_matrix[0][2] &&
                m1.m_matrix[0][3] < m2.m_matrix[0][3] &&
                m1.m_matrix[1][0] < m2.m_matrix[1][0] &&
                m1.m_matrix[1][1] < m2.m_matrix[1][1] &&
                m1.m_matrix[1][2] < m2.m_matrix[1][2] &&
                m1.m_matrix[1][3] < m2.m_matrix[1][3] &&
                m1.m_matrix[2][0] < m2.m_matrix[2][0] &&
                m1.m_matrix[2][1] < m2.m_matrix[2][1] &&
                m1.m_matrix[2][2] < m2.m_matrix[2][2] &&
                m1.m_matrix[2][3] < m2.m_matrix[2][3] &&
                m1.m_matrix[3][0] < m2.m_matrix[3][0] &&
                m1.m_matrix[3][1] < m2.m_matrix[3][1] &&
                m1.m_matrix[3][2] < m2.m_matrix[3][2] &&
                m1.m_matrix[3][3] < m2.m_matrix[3][3]);
    }
     
private:
    void setMatrix(const float* m)        { if (m && m != (float*) m_matrix) memcpy(m_matrix, m, 16*sizeof(float)); }
    
    float   m_matrix[4][4];
};

} // namespace WebCore

#endif // Transform3D_h
