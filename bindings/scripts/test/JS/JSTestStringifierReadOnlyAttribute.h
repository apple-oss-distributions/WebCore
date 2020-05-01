/*
    This file is part of the WebKit open source project.
    This file has been generated by generate-bindings.pl. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#pragma once

#include "JSDOMWrapper.h"
#include "TestStringifierReadOnlyAttribute.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

class JSTestStringifierReadOnlyAttribute : public JSDOMWrapper<TestStringifierReadOnlyAttribute> {
public:
    using Base = JSDOMWrapper<TestStringifierReadOnlyAttribute>;
    static JSTestStringifierReadOnlyAttribute* create(JSC::Structure* structure, JSDOMGlobalObject* globalObject, Ref<TestStringifierReadOnlyAttribute>&& impl)
    {
        JSTestStringifierReadOnlyAttribute* ptr = new (NotNull, JSC::allocateCell<JSTestStringifierReadOnlyAttribute>(globalObject->vm().heap)) JSTestStringifierReadOnlyAttribute(structure, *globalObject, WTFMove(impl));
        ptr->finishCreation(globalObject->vm());
        return ptr;
    }

    static JSC::JSObject* createPrototype(JSC::VM&, JSDOMGlobalObject&);
    static JSC::JSObject* prototype(JSC::VM&, JSDOMGlobalObject&);
    static TestStringifierReadOnlyAttribute* toWrapped(JSC::VM&, JSC::JSValue);
    static void destroy(JSC::JSCell*);

    DECLARE_INFO;

    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info(), JSC::NonArray);
    }

    static JSC::JSValue getConstructor(JSC::VM&, const JSC::JSGlobalObject*);
    static void analyzeHeap(JSCell*, JSC::HeapAnalyzer&);
protected:
    JSTestStringifierReadOnlyAttribute(JSC::Structure*, JSDOMGlobalObject&, Ref<TestStringifierReadOnlyAttribute>&&);

    void finishCreation(JSC::VM&);
};

class JSTestStringifierReadOnlyAttributeOwner : public JSC::WeakHandleOwner {
public:
    virtual bool isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown>, void* context, JSC::SlotVisitor&, const char**);
    virtual void finalize(JSC::Handle<JSC::Unknown>, void* context);
};

inline JSC::WeakHandleOwner* wrapperOwner(DOMWrapperWorld&, TestStringifierReadOnlyAttribute*)
{
    static NeverDestroyed<JSTestStringifierReadOnlyAttributeOwner> owner;
    return &owner.get();
}

inline void* wrapperKey(TestStringifierReadOnlyAttribute* wrappableObject)
{
    return wrappableObject;
}

JSC::JSValue toJS(JSC::JSGlobalObject*, JSDOMGlobalObject*, TestStringifierReadOnlyAttribute&);
inline JSC::JSValue toJS(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, TestStringifierReadOnlyAttribute* impl) { return impl ? toJS(lexicalGlobalObject, globalObject, *impl) : JSC::jsNull(); }
JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject*, JSDOMGlobalObject*, Ref<TestStringifierReadOnlyAttribute>&&);
inline JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, RefPtr<TestStringifierReadOnlyAttribute>&& impl) { return impl ? toJSNewlyCreated(lexicalGlobalObject, globalObject, impl.releaseNonNull()) : JSC::jsNull(); }

template<> struct JSDOMWrapperConverterTraits<TestStringifierReadOnlyAttribute> {
    using WrapperClass = JSTestStringifierReadOnlyAttribute;
    using ToWrappedReturnType = TestStringifierReadOnlyAttribute*;
};

} // namespace WebCore
