/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#include "config.h"
#include "JSTouchListConstructor.h"

#if ENABLE(TOUCH_EVENTS)

#include "JSTouch.h"
#include "JSTouchList.h"
#include "Touch.h"
#include "TouchList.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

const ClassInfo JSTouchListConstructor::s_info = { "TouchListConstructor", &DOMConstructorWithDocument::s_info, 0, 0, CREATE_METHOD_TABLE(JSTouchListConstructor) };

JSTouchListConstructor::JSTouchListConstructor(Structure* structure, JSDOMGlobalObject* globalObject)
    : DOMConstructorWithDocument(structure, globalObject)
{
}

void JSTouchListConstructor::finishCreation(JSC::ExecState* exec, JSDOMGlobalObject* globalObject)
{
    Base::finishCreation(globalObject);
    ASSERT(inherits(&s_info));
    putDirect(exec->vm(), exec->propertyNames().prototype, JSTouchListPrototype::self(exec, globalObject), None);
}

static EncodedJSValue JSC_HOST_CALL constructTouchList(ExecState* exec)
{
    JSTouchListConstructor* jsConstructor = static_cast<JSTouchListConstructor*>(exec->callee());
    RefPtr<TouchList> touchList = TouchList::create();

    size_t sz = exec->argumentCount();
    for (size_t i = 0; i < sz; i++) {
        touchList->append(toTouch(exec->argument(i)));
    }

    return JSValue::encode(CREATE_DOM_WRAPPER(exec, jsConstructor->globalObject(), TouchList, touchList.get()));
}

ConstructType JSTouchListConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructTouchList;
    return ConstructTypeHost;
}

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)
