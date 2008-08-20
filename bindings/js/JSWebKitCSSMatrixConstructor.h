/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef JSWebKitCSSMatrixConstructor_h
#define JSWebKitCSSMatrixConstructor_h

#include "kjs_binding.h"
#include <wtf/RefPtr.h>

namespace WebCore {

    class JSWebKitCSSMatrixConstructor : public KJS::DOMObject {
    public:
        JSWebKitCSSMatrixConstructor(KJS::ExecState*, Document*);
        virtual bool implementsConstruct() const;
        virtual KJS::JSObject *construct(KJS::ExecState*, const KJS::List& args);
    private:
        RefPtr<Document> m_doc;
    };

}

#endif // JSWebKitCSSMatrixConstructor_h
