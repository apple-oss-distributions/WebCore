/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 */

#include "config.h"
#include "JSWebKitCSSMatrixConstructor.h"

#include "Document.h"
#include "WebKitCSSMatrix.h"
#include "JSWebKitCSSMatrix.h"
#include "Text.h"

namespace WebCore {

using namespace KJS;

JSWebKitCSSMatrixConstructor::JSWebKitCSSMatrixConstructor(ExecState* exec, Document* d)
    : KJS::DOMObject(exec->lexicalGlobalObject()->objectPrototype())
    , m_doc(d)
{
    put(exec, exec->propertyNames().length, jsNumber(4), ReadOnly|DontDelete|DontEnum);
}

bool JSWebKitCSSMatrixConstructor::implementsConstruct() const
{
    return true;
}

JSObject* JSWebKitCSSMatrixConstructor::construct(ExecState* exec, const List& args)
{
    String s;
    if (args.size() >= 1) {
        s = args[0]->getString();
    }
    return static_cast<JSObject*>(toJS(exec, new WebKitCSSMatrix(s)));
}

} // namespace WebCore
