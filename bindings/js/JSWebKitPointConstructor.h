/*
 * Copyright (C) 2007, 2008, Apple Computer, Inc.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef JSWebKitPointConstructor_h
#define JSWebKitPointConstructor_h

#include "kjs_binding.h"
#include <wtf/RefPtr.h>

namespace WebCore {

    class JSWebKitPointConstructor : public KJS::DOMObject {
    public:
        JSWebKitPointConstructor(KJS::ExecState*, Document*);
        virtual bool implementsConstruct() const;
        virtual KJS::JSObject *construct(KJS::ExecState*, const KJS::List& args);
    private:
        RefPtr<Document> m_doc;
    };

}

#endif // JSWebKitPointConstructor_h
