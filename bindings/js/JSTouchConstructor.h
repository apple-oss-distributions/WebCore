/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef JSTouchConstructor_h
#define JSTouchConstructor_h

#include <wtf/Platform.h>

#if ENABLE(TOUCH_EVENTS)

#include "kjs_binding.h"
#include <wtf/RefPtr.h>

namespace WebCore {

    class JSTouchConstructor : public KJS::DOMObject {
    public:
        JSTouchConstructor(KJS::ExecState*, Document*);
        virtual bool implementsConstruct() const { return true; }
        virtual KJS::JSObject *construct(KJS::ExecState*, const KJS::List& args);
    private:
        RefPtr<Document> m_doc;
    };

}

#endif // ENABLE(TOUCH_EVENTS)

#endif // JSTouchConstructor_h
