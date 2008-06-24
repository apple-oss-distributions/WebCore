/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef WebKitCSSKeyframesRule_h
#define WebKitCSSKeyframesRule_h

#include "CSSRule.h"
#include <wtf/RefPtr.h>
#include "AtomicString.h"

namespace WebCore {

class CSSRuleList;
class WebKitCSSKeyframeRule;
class String;

typedef int ExceptionCode;

class WebKitCSSKeyframesRule : public CSSRule {
public:
    WebKitCSSKeyframesRule(StyleBase* parent);
    ~WebKitCSSKeyframesRule();

    virtual bool isKeyframesRule() { return true; }

    // Inherited from CSSRule
    virtual unsigned short type() const { return WEBKIT_KEYFRAMES_RULE; }

    String name() const;
    void setName(const String&, ExceptionCode&);    
    void setName(String);    

    CSSRuleList* cssRules() { return m_lstCSSRules.get(); }

    void insertRule(const String& rule);
    void deleteRule(const String& key);
    WebKitCSSKeyframeRule* findRule(const String& key);

    virtual String cssText() const;

    /* not part of the DOM */
    unsigned length() const;
    WebKitCSSKeyframeRule*        item(unsigned index);
    const WebKitCSSKeyframeRule*  item(unsigned index) const;
    void insert(WebKitCSSKeyframeRule* rule);

protected:
    int findRuleIndex(float key) const;
    
    RefPtr<CSSRuleList> m_lstCSSRules;
    String m_name;
};

} // namespace WebCore

#endif // WebKitCSSKeyframesRule_h
