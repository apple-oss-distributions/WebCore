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

#include "config.h"
#include "JSTestException.h"

#include "JSDOMAttribute.h"
#include "JSDOMBinding.h"
#include "JSDOMConstructorNotConstructable.h"
#include "JSDOMConvertStrings.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMWrapperCache.h"
#include "ScriptExecutionContext.h"
#include <JavaScriptCore/FunctionPrototype.h>
#include <JavaScriptCore/HeapSnapshotBuilder.h>
#include <JavaScriptCore/JSCInlines.h>
#include <wtf/GetPtr.h>
#include <wtf/PointerPreparations.h>
#include <wtf/URL.h>


namespace WebCore {
using namespace JSC;

// Attributes

JSC::EncodedJSValue jsTestExceptionConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestExceptionConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::EncodedJSValue);
JSC::EncodedJSValue jsTestExceptionName(JSC::ExecState*, JSC::EncodedJSValue, JSC::PropertyName);

class JSTestExceptionPrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestExceptionPrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestExceptionPrototype* ptr = new (NotNull, JSC::allocateCell<JSTestExceptionPrototype>(vm.heap)) JSTestExceptionPrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestExceptionPrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};

using JSTestExceptionConstructor = JSDOMConstructorNotConstructable<JSTestException>;

template<> JSValue JSTestExceptionConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestExceptionConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestException::prototype(vm, globalObject), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(&vm, String("TestException"_s)), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> const ClassInfo JSTestExceptionConstructor::s_info = { "TestException", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestExceptionConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestExceptionPrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestExceptionConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestExceptionConstructor) } },
    { "name", static_cast<unsigned>(JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::CustomAccessor | JSC::PropertyAttribute::DOMAttribute), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestExceptionName), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(0) } },
};

const ClassInfo JSTestExceptionPrototype::s_info = { "TestExceptionPrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestExceptionPrototype) };

void JSTestExceptionPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestException::info(), JSTestExceptionPrototypeTableValues, *this);
}

const ClassInfo JSTestException::s_info = { "TestException", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestException) };

JSTestException::JSTestException(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestException>&& impl)
    : JSDOMWrapper<TestException>(structure, globalObject, WTFMove(impl))
{
}

void JSTestException::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

}

JSObject* JSTestException::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestExceptionPrototype::create(vm, &globalObject, JSTestExceptionPrototype::createStructure(vm, &globalObject, globalObject.errorPrototype()));
}

JSObject* JSTestException::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestException>(vm, globalObject);
}

JSValue JSTestException::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestExceptionConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestException::destroy(JSC::JSCell* cell)
{
    JSTestException* thisObject = static_cast<JSTestException*>(cell);
    thisObject->JSTestException::~JSTestException();
}

template<> inline JSTestException* IDLAttribute<JSTestException>::cast(ExecState& state, EncodedJSValue thisValue)
{
    return jsDynamicCast<JSTestException*>(state.vm(), JSValue::decode(thisValue));
}

EncodedJSValue jsTestExceptionConstructor(ExecState* state, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestExceptionPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(state, throwScope);
    return JSValue::encode(JSTestException::getConstructor(state->vm(), prototype->globalObject()));
}

bool setJSTestExceptionConstructor(ExecState* state, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestExceptionPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(state, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(vm, vm.propertyNames->constructor, JSValue::decode(encodedValue));
}

static inline JSValue jsTestExceptionNameGetter(ExecState& state, JSTestException& thisObject, ThrowScope& throwScope)
{
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(state);
    auto& impl = thisObject.wrapped();
    JSValue result = toJS<IDLDOMString>(state, throwScope, impl.name());
    return result;
}

EncodedJSValue jsTestExceptionName(ExecState* state, EncodedJSValue thisValue, PropertyName)
{
    return IDLAttribute<JSTestException>::get<jsTestExceptionNameGetter, CastedThisErrorBehavior::Assert>(*state, thisValue, "name");
}

void JSTestException::heapSnapshot(JSCell* cell, HeapSnapshotBuilder& builder)
{
    auto* thisObject = jsCast<JSTestException*>(cell);
    builder.setWrappedObjectForCell(cell, &thisObject->wrapped());
    if (thisObject->scriptExecutionContext())
        builder.setLabelForCell(cell, String::format("url %s", thisObject->scriptExecutionContext()->url().string().utf8().data()));
    Base::heapSnapshot(cell, builder);
}

bool JSTestExceptionOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor, const char** reason)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    UNUSED_PARAM(reason);
    return false;
}

void JSTestExceptionOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsTestException = static_cast<JSTestException*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestException->wrapped(), jsTestException);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestException@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore13TestExceptionE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::ExecState*, JSDOMGlobalObject* globalObject, Ref<TestException>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    void* actualVTablePointer = *(reinterpret_cast<void**>(impl.ptr()));
#if PLATFORM(WIN)
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(__identifier("??_7TestException@WebCore@@6B@"));
#else
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(&_ZTVN7WebCore13TestExceptionE[2]);
#endif

    // If this fails TestException does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(std::is_polymorphic<TestException>::value, "TestException is not polymorphic");

    // If you hit this assertion you either have a use after free bug, or
    // TestException has subclasses. If TestException has subclasses that get passed
    // to toJS() we currently require TestException you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<TestException>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::ExecState* state, JSDOMGlobalObject* globalObject, TestException& impl)
{
    return wrap(state, globalObject, impl);
}

TestException* JSTestException::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicCast<JSTestException*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}
