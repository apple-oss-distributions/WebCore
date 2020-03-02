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
#include "TestPluginInterface.h"
#include <JavaScriptCore/CallData.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

class JSTestPluginInterface : public JSDOMWrapper<TestPluginInterface> {
public:
    using Base = JSDOMWrapper<TestPluginInterface>;
    static JSTestPluginInterface* create(JSC::Structure* structure, JSDOMGlobalObject* globalObject, Ref<TestPluginInterface>&& impl)
    {
        JSTestPluginInterface* ptr = new (NotNull, JSC::allocateCell<JSTestPluginInterface>(globalObject->vm().heap)) JSTestPluginInterface(structure, *globalObject, WTFMove(impl));
        ptr->finishCreation(globalObject->vm());
        return ptr;
    }

    static JSC::JSObject* createPrototype(JSC::VM&, JSDOMGlobalObject&);
    static JSC::JSObject* prototype(JSC::VM&, JSDOMGlobalObject&);
    static TestPluginInterface* toWrapped(JSC::VM&, JSC::JSValue);
    static bool getOwnPropertySlot(JSC::JSObject*, JSC::JSGlobalObject*, JSC::PropertyName, JSC::PropertySlot&);
    static bool getOwnPropertySlotByIndex(JSC::JSObject*, JSC::JSGlobalObject*, unsigned propertyName, JSC::PropertySlot&);
    static bool put(JSC::JSCell*, JSC::JSGlobalObject*, JSC::PropertyName, JSC::JSValue, JSC::PutPropertySlot&);
    static bool putByIndex(JSC::JSCell*, JSC::JSGlobalObject*, unsigned propertyName, JSC::JSValue, bool shouldThrow);
    static JSC::CallType getCallData(JSC::JSCell*, JSC::CallData&);

    static void destroy(JSC::JSCell*);

    DECLARE_INFO;

    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

    static JSC::JSValue getConstructor(JSC::VM&, const JSC::JSGlobalObject*);
    static void analyzeHeap(JSCell*, JSC::HeapAnalyzer&);
public:
    static constexpr unsigned StructureFlags = Base::StructureFlags | JSC::InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | JSC::OverridesGetCallData | JSC::OverridesGetOwnPropertySlot | JSC::ProhibitsPropertyCaching;
protected:
    JSTestPluginInterface(JSC::Structure*, JSDOMGlobalObject&, Ref<TestPluginInterface>&&);

    void finishCreation(JSC::VM&);
};

class JSTestPluginInterfaceOwner : public JSC::WeakHandleOwner {
public:
    virtual bool isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown>, void* context, JSC::SlotVisitor&, const char**);
    virtual void finalize(JSC::Handle<JSC::Unknown>, void* context);
};

inline JSC::WeakHandleOwner* wrapperOwner(DOMWrapperWorld&, TestPluginInterface*)
{
    static NeverDestroyed<JSTestPluginInterfaceOwner> owner;
    return &owner.get();
}

inline void* wrapperKey(TestPluginInterface* wrappableObject)
{
    return wrappableObject;
}

JSC::JSValue toJS(JSC::JSGlobalObject*, JSDOMGlobalObject*, TestPluginInterface&);
inline JSC::JSValue toJS(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, TestPluginInterface* impl) { return impl ? toJS(lexicalGlobalObject, globalObject, *impl) : JSC::jsNull(); }
JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject*, JSDOMGlobalObject*, Ref<TestPluginInterface>&&);
inline JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, RefPtr<TestPluginInterface>&& impl) { return impl ? toJSNewlyCreated(lexicalGlobalObject, globalObject, impl.releaseNonNull()) : JSC::jsNull(); }

template<> struct JSDOMWrapperConverterTraits<TestPluginInterface> {
    using WrapperClass = JSTestPluginInterface;
    using ToWrappedReturnType = TestPluginInterface*;
};

} // namespace WebCore
