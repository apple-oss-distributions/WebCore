/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef WebKitCSSKeyframeRule_h
#define WebKitCSSKeyframeRule_h

#include "CSSRule.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSMutableStyleDeclaration;

typedef int ExceptionCode;

class WebKitCSSKeyframeRule : public CSSRule {
public:
    WebKitCSSKeyframeRule(StyleBase* parent);
    virtual ~WebKitCSSKeyframeRule();

    virtual bool isKeyframeRule() { return true; }

    // Inherited from CSSRule
    virtual unsigned short type() const { return WEBKIT_KEYFRAME_RULE; }

    String keyText() const;
    void setKeyText(const String& s);

    CSSMutableStyleDeclaration* style() const { return m_style.get(); }

    virtual String cssText() const;

    // Not part of the CSSOM
    virtual bool parseString(const String&, bool = false);
    
    float key() const { return m_key/100; }
    float keyAsPercent() const { return m_key; }

    void setKey(float k) { m_key = k; }
    void setDeclaration(PassRefPtr<CSSMutableStyleDeclaration>);

    CSSMutableStyleDeclaration*         declaration()       { return m_style.get(); }
    const CSSMutableStyleDeclaration*   declaration() const { return m_style.get(); }
    
    static float keyStringToFloat(const String& s);
 
protected:
    RefPtr<CSSMutableStyleDeclaration> m_style;
    float m_key;        // 0 to 100
};

} // namespace WebCore

#endif // WebKitCSSKeyframeRule_h
