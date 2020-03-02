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
#include "TestActiveDOMObject.h"
#include <wtf/NeverDestroyed.h>

namespace WebCore {

class JSTestActiveDOMObject : public JSDOMWrapper<TestActiveDOMObject> {
public:
    using Base = JSDOMWrapper<TestActiveDOMObject>;
    static JSTestActiveDOMObject* create(JSC::Structure* structure, JSDOMGlobalObject* globalObject, Ref<TestActiveDOMObject>&& impl)
    {
        JSTestActiveDOMObject* ptr = new (NotNull, JSC::allocateCell<JSTestActiveDOMObject>(globalObject->vm().heap)) JSTestActiveDOMObject(structure, *globalObject, WTFMove(impl));
        ptr->finishCreation(globalObject->vm());
        return ptr;
    }

    static JSC::JSObject* createPrototype(JSC::VM&, JSDOMGlobalObject&);
    static JSC::JSObject* prototype(JSC::VM&, JSDOMGlobalObject&);
    static TestActiveDOMObject* toWrapped(JSC::VM&, JSC::JSValue);
    static void doPutPropertySecurityCheck(JSC::JSObject*, JSC::JSGlobalObject*, JSC::PropertyName, JSC::PutPropertySlot&);
    static void destroy(JSC::JSCell*);

    DECLARE_INFO;

    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

    static JSC::JSValue getConstructor(JSC::VM&, const JSC::JSGlobalObject*);
    static void analyzeHeap(JSCell*, JSC::HeapAnalyzer&);
public:
    static constexpr unsigned StructureFlags = Base::StructureFlags | JSC::HasPutPropertySecurityCheck | JSC::HasStaticPropertyTable;
protected:
    JSTestActiveDOMObject(JSC::Structure*, JSDOMGlobalObject&, Ref<TestActiveDOMObject>&&);

    void finishCreation(JSC::VM&);
};

class JSTestActiveDOMObjectOwner : public JSC::WeakHandleOwner {
public:
    virtual bool isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown>, void* context, JSC::SlotVisitor&, const char**);
    virtual void finalize(JSC::Handle<JSC::Unknown>, void* context);
};

inline JSC::WeakHandleOwner* wrapperOwner(DOMWrapperWorld&, TestActiveDOMObject*)
{
    static NeverDestroyed<JSTestActiveDOMObjectOwner> owner;
    return &owner.get();
}

inline void* wrapperKey(TestActiveDOMObject* wrappableObject)
{
    return wrappableObject;
}

JSC::JSValue toJS(JSC::JSGlobalObject*, JSDOMGlobalObject*, TestActiveDOMObject&);
inline JSC::JSValue toJS(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, TestActiveDOMObject* impl) { return impl ? toJS(lexicalGlobalObject, globalObject, *impl) : JSC::jsNull(); }
JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject*, JSDOMGlobalObject*, Ref<TestActiveDOMObject>&&);
inline JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, RefPtr<TestActiveDOMObject>&& impl) { return impl ? toJSNewlyCreated(lexicalGlobalObject, globalObject, impl.releaseNonNull()) : JSC::jsNull(); }

template<> struct JSDOMWrapperConverterTraits<TestActiveDOMObject> {
    using WrapperClass = JSTestActiveDOMObject;
    using ToWrappedReturnType = TestActiveDOMObject*;
};

} // namespace WebCore
