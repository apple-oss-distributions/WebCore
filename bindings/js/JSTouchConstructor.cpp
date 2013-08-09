/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#include "config.h"
#include "JSTouchConstructor.h"

#if ENABLE(TOUCH_EVENTS)

#include "JSDOMWindow.h"
#include "JSNode.h"
#include "JSTouch.h"
#include "ScriptExecutionContext.h"
#include "Touch.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

const ClassInfo JSTouchConstructor::s_info = { "TouchConstructor", &DOMConstructorWithDocument::s_info, 0, 0, CREATE_METHOD_TABLE(JSTouchConstructor) };

JSTouchConstructor::JSTouchConstructor(Structure* structure, JSDOMGlobalObject* globalObject)
    : DOMConstructorWithDocument(structure, globalObject)
{
}

void JSTouchConstructor::finishCreation(ExecState* exec, JSDOMGlobalObject* globalObject)
{
    Base::finishCreation(globalObject);
    ASSERT(inherits(&s_info));
    putDirect(exec->vm(), exec->propertyNames().prototype, JSTouchPrototype::self(exec, globalObject), None);
}

static EncodedJSValue JSC_HOST_CALL constructTouch(ExecState* exec)
{
    JSTouchConstructor* jsConstructor = static_cast<JSTouchConstructor*>(exec->callee());
    DOMWindow* view = toDOMWindow(exec->argument(0));
    Node* target = toNode(exec->argument(1));

    RefPtr<Touch> touch = Touch::create(view, target, 
        exec->argument(2).toInt32(exec), exec->argument(3).toInt32(exec), exec->argument(4).toInt32(exec), 
        exec->argument(5).toInt32(exec), exec->argument(6).toInt32(exec));

    return JSValue::encode(CREATE_DOM_WRAPPER(exec, jsConstructor->globalObject(), Touch, touch.get()));
}

ConstructType JSTouchConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructTouch;
    return ConstructTypeHost;
}

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)
