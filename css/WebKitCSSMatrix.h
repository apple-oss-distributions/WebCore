/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef WebKitCSSMatrix_h
#define WebKitCSSMatrix_h

#include "StyleBase.h"
#include "PlatformString.h"
#include "Transform3D.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

typedef int ExceptionCode;

class WebKitCSSMatrix : public StyleBase {
public:
    WebKitCSSMatrix();
    WebKitCSSMatrix(const WebKitCSSMatrix& m);
    WebKitCSSMatrix(const Transform3D& m);
    WebKitCSSMatrix(const String& s);
    virtual ~WebKitCSSMatrix();

    float m11() const { return m_matrix.m11(); }
    float m12() const { return m_matrix.m12(); }
    float m13() const { return m_matrix.m13(); }
    float m14() const { return m_matrix.m14(); }
    float m21() const { return m_matrix.m21(); }
    float m22() const { return m_matrix.m22(); }
    float m23() const { return m_matrix.m23(); }
    float m24() const { return m_matrix.m24(); }
    float m31() const { return m_matrix.m31(); }
    float m32() const { return m_matrix.m32(); }
    float m33() const { return m_matrix.m33(); }
    float m34() const { return m_matrix.m34(); }
    float m41() const { return m_matrix.m41(); }
    float m42() const { return m_matrix.m42(); }
    float m43() const { return m_matrix.m43(); }
    float m44() const { return m_matrix.m44(); }
    
    void setM11(float f)   { m_matrix.setM11(f); }
    void setM12(float f)   { m_matrix.setM12(f); }
    void setM13(float f)   { m_matrix.setM13(f); }
    void setM14(float f)   { m_matrix.setM14(f); }
    void setM21(float f)   { m_matrix.setM21(f); }
    void setM22(float f)   { m_matrix.setM22(f); }
    void setM23(float f)   { m_matrix.setM23(f); }
    void setM24(float f)   { m_matrix.setM24(f); }
    void setM31(float f)   { m_matrix.setM31(f); }
    void setM32(float f)   { m_matrix.setM32(f); }
    void setM33(float f)   { m_matrix.setM33(f); }
    void setM34(float f)   { m_matrix.setM34(f); }
    void setM41(float f)   { m_matrix.setM41(f); }
    void setM42(float f)   { m_matrix.setM42(f); }
    void setM43(float f)   { m_matrix.setM43(f); }
    void setM44(float f)   { m_matrix.setM44(f); }
 
    void                setMatrixValue(const String& string);
    
    // this = this * secondMatrix (i.e. multRight)
    WebKitCSSMatrix*    multiply(WebKitCSSMatrix* secondMatrix);
    
    // FIXME: we really should have an exception here, for when matrix is not invertible
    WebKitCSSMatrix*    inverse();
    WebKitCSSMatrix*    translate(float x, float y, float z);
    WebKitCSSMatrix*    scale(float scaleX, float scaleY, float scaleZ);
    WebKitCSSMatrix*    rotate(float rotZ, float rotY, float rotZ);
    WebKitCSSMatrix*    rotateAxisAngle(float x, float y, float z, float angle);
    
    const Transform3D&    transform() { return m_matrix; }
    
    String toString();
    
protected:
    Transform3D m_matrix;
};

} // namespace WebCore

#endif // WebKitCSSMatrix_h
