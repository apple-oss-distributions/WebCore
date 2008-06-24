/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "JSWebKitPointConstructor.h"

#include "Document.h"
#include "WebKitPoint.h"
#include "JSWebKitPoint.h"

namespace WebCore {

using namespace KJS;

JSWebKitPointConstructor::JSWebKitPointConstructor(ExecState* exec, Document* d)
    : KJS::DOMObject(exec->lexicalGlobalObject()->objectPrototype())
    , m_doc(d)
{
    put(exec, exec->propertyNames().length, jsNumber(4), ReadOnly|DontDelete|DontEnum);
}

bool JSWebKitPointConstructor::implementsConstruct() const
{
    return true;
}

JSObject* JSWebKitPointConstructor::construct(ExecState* exec, const List& args)
{
    float x = 0;
    float y = 0;
    if (args.size() >= 2) {
        x = (float) args[0]->getNumber();
        y = (float) args[1]->getNumber();
        if (isnan(x))
            x = 0;
        if (isnan(y))
            y = 0;
    }
    return static_cast<JSObject*>(toJS(exec, new WebKitPoint(x, y)));
}

} // namespace WebCore
