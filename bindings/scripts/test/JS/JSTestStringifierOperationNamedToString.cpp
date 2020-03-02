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
#include "JSTestStringifierOperationNamedToString.h"

#include "ActiveDOMObject.h"
#include "JSDOMBinding.h"
#include "JSDOMConstructorNotConstructable.h"
#include "JSDOMConvertStrings.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMOperation.h"
#include "JSDOMWrapperCache.h"
#include "ScriptExecutionContext.h"
#include <JavaScriptCore/FunctionPrototype.h>
#include <JavaScriptCore/HeapAnalyzer.h>
#include <JavaScriptCore/JSCInlines.h>
#include <wtf/GetPtr.h>
#include <wtf/PointerPreparations.h>
#include <wtf/URL.h>


namespace WebCore {
using namespace JSC;

// Functions

JSC::EncodedJSValue JSC_HOST_CALL jsTestStringifierOperationNamedToStringPrototypeFunctionToString(JSC::JSGlobalObject*, JSC::CallFrame*);

// Attributes

JSC::EncodedJSValue jsTestStringifierOperationNamedToStringConstructor(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestStringifierOperationNamedToStringConstructor(JSC::JSGlobalObject*, JSC::EncodedJSValue, JSC::EncodedJSValue);

class JSTestStringifierOperationNamedToStringPrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestStringifierOperationNamedToStringPrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestStringifierOperationNamedToStringPrototype* ptr = new (NotNull, JSC::allocateCell<JSTestStringifierOperationNamedToStringPrototype>(vm.heap)) JSTestStringifierOperationNamedToStringPrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestStringifierOperationNamedToStringPrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};
STATIC_ASSERT_ISO_SUBSPACE_SHARABLE(JSTestStringifierOperationNamedToStringPrototype, JSTestStringifierOperationNamedToStringPrototype::Base);

using JSTestStringifierOperationNamedToStringConstructor = JSDOMConstructorNotConstructable<JSTestStringifierOperationNamedToString>;

template<> JSValue JSTestStringifierOperationNamedToStringConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestStringifierOperationNamedToStringConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestStringifierOperationNamedToString::prototype(vm, globalObject), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(vm, String("TestStringifierOperationNamedToString"_s)), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> const ClassInfo JSTestStringifierOperationNamedToStringConstructor::s_info = { "TestStringifierOperationNamedToString", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestStringifierOperationNamedToStringConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestStringifierOperationNamedToStringPrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestStringifierOperationNamedToStringConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestStringifierOperationNamedToStringConstructor) } },
    { "toString", static_cast<unsigned>(JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestStringifierOperationNamedToStringPrototypeFunctionToString), (intptr_t) (0) } },
};

const ClassInfo JSTestStringifierOperationNamedToStringPrototype::s_info = { "TestStringifierOperationNamedToStringPrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestStringifierOperationNamedToStringPrototype) };

void JSTestStringifierOperationNamedToStringPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestStringifierOperationNamedToString::info(), JSTestStringifierOperationNamedToStringPrototypeTableValues, *this);
}

const ClassInfo JSTestStringifierOperationNamedToString::s_info = { "TestStringifierOperationNamedToString", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestStringifierOperationNamedToString) };

JSTestStringifierOperationNamedToString::JSTestStringifierOperationNamedToString(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestStringifierOperationNamedToString>&& impl)
    : JSDOMWrapper<TestStringifierOperationNamedToString>(structure, globalObject, WTFMove(impl))
{
}

void JSTestStringifierOperationNamedToString::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

    static_assert(!std::is_base_of<ActiveDOMObject, TestStringifierOperationNamedToString>::value, "Interface is not marked as [ActiveDOMObject] even though implementation class subclasses ActiveDOMObject.");

}

JSObject* JSTestStringifierOperationNamedToString::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestStringifierOperationNamedToStringPrototype::create(vm, &globalObject, JSTestStringifierOperationNamedToStringPrototype::createStructure(vm, &globalObject, globalObject.objectPrototype()));
}

JSObject* JSTestStringifierOperationNamedToString::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestStringifierOperationNamedToString>(vm, globalObject);
}

JSValue JSTestStringifierOperationNamedToString::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestStringifierOperationNamedToStringConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestStringifierOperationNamedToString::destroy(JSC::JSCell* cell)
{
    JSTestStringifierOperationNamedToString* thisObject = static_cast<JSTestStringifierOperationNamedToString*>(cell);
    thisObject->JSTestStringifierOperationNamedToString::~JSTestStringifierOperationNamedToString();
}

template<> inline JSTestStringifierOperationNamedToString* IDLOperation<JSTestStringifierOperationNamedToString>::cast(JSGlobalObject& lexicalGlobalObject, CallFrame& callFrame)
{
    return jsDynamicCast<JSTestStringifierOperationNamedToString*>(JSC::getVM(&lexicalGlobalObject), callFrame.thisValue());
}

EncodedJSValue jsTestStringifierOperationNamedToStringConstructor(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestStringifierOperationNamedToStringPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(lexicalGlobalObject, throwScope);
    return JSValue::encode(JSTestStringifierOperationNamedToString::getConstructor(JSC::getVM(lexicalGlobalObject), prototype->globalObject()));
}

bool setJSTestStringifierOperationNamedToStringConstructor(JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestStringifierOperationNamedToStringPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(lexicalGlobalObject, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(vm, vm.propertyNames->constructor, JSValue::decode(encodedValue));
}

static inline JSC::EncodedJSValue jsTestStringifierOperationNamedToStringPrototypeFunctionToStringBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestStringifierOperationNamedToString>::ClassParameter castedThis, JSC::ThrowScope& throwScope)
{
    UNUSED_PARAM(lexicalGlobalObject);
    UNUSED_PARAM(callFrame);
    UNUSED_PARAM(throwScope);
    auto& impl = castedThis->wrapped();
    return JSValue::encode(toJS<IDLDOMString>(*lexicalGlobalObject, impl.toString()));
}

EncodedJSValue JSC_HOST_CALL jsTestStringifierOperationNamedToStringPrototypeFunctionToString(JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame)
{
    return IDLOperation<JSTestStringifierOperationNamedToString>::call<jsTestStringifierOperationNamedToStringPrototypeFunctionToStringBody>(*lexicalGlobalObject, *callFrame, "toString");
}

void JSTestStringifierOperationNamedToString::analyzeHeap(JSCell* cell, HeapAnalyzer& analyzer)
{
    auto* thisObject = jsCast<JSTestStringifierOperationNamedToString*>(cell);
    analyzer.setWrappedObjectForCell(cell, &thisObject->wrapped());
    if (thisObject->scriptExecutionContext())
        analyzer.setLabelForCell(cell, "url " + thisObject->scriptExecutionContext()->url().string());
    Base::analyzeHeap(cell, analyzer);
}

bool JSTestStringifierOperationNamedToStringOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor, const char** reason)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    UNUSED_PARAM(reason);
    return false;
}

void JSTestStringifierOperationNamedToStringOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsTestStringifierOperationNamedToString = static_cast<JSTestStringifierOperationNamedToString*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestStringifierOperationNamedToString->wrapped(), jsTestStringifierOperationNamedToString);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestStringifierOperationNamedToString@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore37TestStringifierOperationNamedToStringE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject*, JSDOMGlobalObject* globalObject, Ref<TestStringifierOperationNamedToString>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    void* actualVTablePointer = *(reinterpret_cast<void**>(impl.ptr()));
#if PLATFORM(WIN)
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(__identifier("??_7TestStringifierOperationNamedToString@WebCore@@6B@"));
#else
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(&_ZTVN7WebCore37TestStringifierOperationNamedToStringE[2]);
#endif

    // If this fails TestStringifierOperationNamedToString does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(std::is_polymorphic<TestStringifierOperationNamedToString>::value, "TestStringifierOperationNamedToString is not polymorphic");

    // If you hit this assertion you either have a use after free bug, or
    // TestStringifierOperationNamedToString has subclasses. If TestStringifierOperationNamedToString has subclasses that get passed
    // to toJS() we currently require TestStringifierOperationNamedToString you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<TestStringifierOperationNamedToString>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, TestStringifierOperationNamedToString& impl)
{
    return wrap(lexicalGlobalObject, globalObject, impl);
}

TestStringifierOperationNamedToString* JSTestStringifierOperationNamedToString::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicCast<JSTestStringifierOperationNamedToString*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}
