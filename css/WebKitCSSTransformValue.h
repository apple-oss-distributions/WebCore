/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef WebKitCSSTransformValue_h
#define WebKitCSSTransformValue_h

#include "CSSValueList.h"

namespace WebCore {

class WebKitCSSTransformValue : public CSSValueList
{
public:
    enum TransformOperationType {
        UnknownTransformOperation,
        ScaleTransformOperation,
        ScaleXTransformOperation,
        ScaleYTransformOperation,
        ScaleZTransformOperation,
        Scale3DTransformOperation,
        RotateTransformOperation,
        RotateXTransformOperation,
        RotateYTransformOperation,
        RotateZTransformOperation,
        Rotate3DTransformOperation,
        SkewTransformOperation,
        SkewXTransformOperation,
        SkewYTransformOperation,
        TranslateTransformOperation,
        TranslateXTransformOperation,
        TranslateYTransformOperation,
        TranslateZTransformOperation,
        Translate3DTransformOperation,
        MatrixTransformOperation,
        Matrix3DTransformOperation,
        PerspectiveTransformOperation
    };

    WebKitCSSTransformValue(TransformOperationType);
    virtual ~WebKitCSSTransformValue();

    virtual String cssText() const;
 
    TransformOperationType type() const { return m_type; }
    
protected:
    TransformOperationType m_type;
};

} // namespace WebCore

#endif // WebKitCSSTransformValue_h
