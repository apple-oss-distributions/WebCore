/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003-2020 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2009 Google, Inc. All rights reserved.
 *  Copyright (C) 2012 Ericsson AB. All rights reserved.
 *  Copyright (C) 2013 Michael Pruett <michael@68k.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#pragma once

#include "JSDOMCastedThisErrorBehavior.h"
#include "JSDOMExceptionHandling.h"

namespace WebCore {

template<typename JSClass>
class IDLAttribute {
public:
    using Setter = bool(JSC::JSGlobalObject&, JSClass&, JSC::JSValue);
    using StaticSetter = bool(JSC::JSGlobalObject&, JSC::JSValue);
    using Getter = JSC::JSValue(JSC::JSGlobalObject&, JSClass&);
    using StaticGetter = JSC::JSValue(JSC::JSGlobalObject&);
    
    static JSClass* cast(JSC::JSGlobalObject&, JSC::EncodedJSValue);

    template<Setter setter, CastedThisErrorBehavior shouldThrow = CastedThisErrorBehavior::Throw>
    static bool set(JSC::JSGlobalObject& lexicalGlobalObject, JSC::EncodedJSValue thisValue, JSC::EncodedJSValue encodedValue, const char* attributeName)
    {
        auto throwScope = DECLARE_THROW_SCOPE(JSC::getVM(&lexicalGlobalObject));

        auto* thisObject = cast(lexicalGlobalObject, thisValue);
        if (UNLIKELY(!thisObject))
            return (shouldThrow == CastedThisErrorBehavior::Throw) ? throwSetterTypeError(lexicalGlobalObject, throwScope, JSClass::info()->className, attributeName) : false;

        RELEASE_AND_RETURN(throwScope, (setter(lexicalGlobalObject, *thisObject, JSC::JSValue::decode(encodedValue))));
    }
    
    template<StaticSetter setter, CastedThisErrorBehavior shouldThrow = CastedThisErrorBehavior::Throw>
    static bool setStatic(JSC::JSGlobalObject& lexicalGlobalObject, JSC::EncodedJSValue, JSC::EncodedJSValue encodedValue, const char*)
    {
        return setter(lexicalGlobalObject, JSC::JSValue::decode(encodedValue));
    }
    
    template<Getter getter, CastedThisErrorBehavior shouldThrow = CastedThisErrorBehavior::Throw>
    static JSC::EncodedJSValue get(JSC::JSGlobalObject& lexicalGlobalObject, JSC::EncodedJSValue thisValue, const char* attributeName)
    {
        auto throwScope = DECLARE_THROW_SCOPE(JSC::getVM(&lexicalGlobalObject));

        if (shouldThrow == CastedThisErrorBehavior::Assert) {
            ASSERT(cast(lexicalGlobalObject, thisValue));
            auto* thisObject = JSC::jsCast<JSClass*>(JSC::JSValue::decode(thisValue));
            RELEASE_AND_RETURN(throwScope, (JSC::JSValue::encode(getter(lexicalGlobalObject, *thisObject))));
        }

        auto* thisObject = cast(lexicalGlobalObject, thisValue);
        if (UNLIKELY(!thisObject)) {
            if (shouldThrow == CastedThisErrorBehavior::Throw)
                return throwGetterTypeError(lexicalGlobalObject, throwScope, JSClass::info()->className, attributeName);
            if (shouldThrow == CastedThisErrorBehavior::RejectPromise)
                RELEASE_AND_RETURN(throwScope, rejectPromiseWithGetterTypeError(lexicalGlobalObject, JSClass::info()->className, attributeName));
            return JSC::JSValue::encode(JSC::jsUndefined());
        }

        RELEASE_AND_RETURN(throwScope, (JSC::JSValue::encode(getter(lexicalGlobalObject, *thisObject))));
    }
    
    template<StaticGetter getter, CastedThisErrorBehavior shouldThrow = CastedThisErrorBehavior::Throw>
    static JSC::EncodedJSValue getStatic(JSC::JSGlobalObject& lexicalGlobalObject, JSC::EncodedJSValue, const char*)
    {
        return JSC::JSValue::encode(getter(lexicalGlobalObject));
    }
};

struct AttributeSetter {
    template<typename Functor>
    static auto call(JSC::JSGlobalObject&, JSC::ThrowScope&, Functor&& functor) -> std::enable_if_t<std::is_same<void, decltype(functor())>::value>
    {
        functor();
    }

    template<typename Functor>
    static auto call(JSC::JSGlobalObject& lexicalGlobalObject, JSC::ThrowScope& throwScope, Functor&& functor) -> std::enable_if_t<!std::is_same<void, decltype(functor())>::value>
    {
        auto result = functor();
        if (!result.hasException())
            return;
        propagateException(lexicalGlobalObject, throwScope, result.releaseException());
    }
};

} // namespace WebCore
