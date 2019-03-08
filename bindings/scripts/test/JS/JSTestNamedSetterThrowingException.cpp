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
#include "JSTestNamedSetterThrowingException.h"

#include "JSDOMAbstractOperations.h"
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

JSC::EncodedJSValue jsTestNamedSetterThrowingExceptionConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestNamedSetterThrowingExceptionConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::EncodedJSValue);

class JSTestNamedSetterThrowingExceptionPrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestNamedSetterThrowingExceptionPrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestNamedSetterThrowingExceptionPrototype* ptr = new (NotNull, JSC::allocateCell<JSTestNamedSetterThrowingExceptionPrototype>(vm.heap)) JSTestNamedSetterThrowingExceptionPrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestNamedSetterThrowingExceptionPrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};

using JSTestNamedSetterThrowingExceptionConstructor = JSDOMConstructorNotConstructable<JSTestNamedSetterThrowingException>;

template<> JSValue JSTestNamedSetterThrowingExceptionConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestNamedSetterThrowingExceptionConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestNamedSetterThrowingException::prototype(vm, globalObject), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(&vm, String("TestNamedSetterThrowingException"_s)), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> const ClassInfo JSTestNamedSetterThrowingExceptionConstructor::s_info = { "TestNamedSetterThrowingException", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterThrowingExceptionConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestNamedSetterThrowingExceptionPrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestNamedSetterThrowingExceptionConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestNamedSetterThrowingExceptionConstructor) } },
};

const ClassInfo JSTestNamedSetterThrowingExceptionPrototype::s_info = { "TestNamedSetterThrowingExceptionPrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterThrowingExceptionPrototype) };

void JSTestNamedSetterThrowingExceptionPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestNamedSetterThrowingException::info(), JSTestNamedSetterThrowingExceptionPrototypeTableValues, *this);
}

const ClassInfo JSTestNamedSetterThrowingException::s_info = { "TestNamedSetterThrowingException", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterThrowingException) };

JSTestNamedSetterThrowingException::JSTestNamedSetterThrowingException(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestNamedSetterThrowingException>&& impl)
    : JSDOMWrapper<TestNamedSetterThrowingException>(structure, globalObject, WTFMove(impl))
{
}

void JSTestNamedSetterThrowingException::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

}

JSObject* JSTestNamedSetterThrowingException::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestNamedSetterThrowingExceptionPrototype::create(vm, &globalObject, JSTestNamedSetterThrowingExceptionPrototype::createStructure(vm, &globalObject, globalObject.objectPrototype()));
}

JSObject* JSTestNamedSetterThrowingException::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestNamedSetterThrowingException>(vm, globalObject);
}

JSValue JSTestNamedSetterThrowingException::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestNamedSetterThrowingExceptionConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestNamedSetterThrowingException::destroy(JSC::JSCell* cell)
{
    JSTestNamedSetterThrowingException* thisObject = static_cast<JSTestNamedSetterThrowingException*>(cell);
    thisObject->JSTestNamedSetterThrowingException::~JSTestNamedSetterThrowingException();
}

bool JSTestNamedSetterThrowingException::getOwnPropertySlot(JSObject* object, ExecState* state, PropertyName propertyName, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSTestNamedSetterThrowingException*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    using GetterIDLType = IDLDOMString;
    auto getterFunctor = [] (auto& thisObject, auto propertyName) -> Optional<typename GetterIDLType::ImplementationType> {
        auto result = thisObject.wrapped().namedItem(propertyNameToAtomicString(propertyName));
        if (!GetterIDLType::isNullValue(result))
            return typename GetterIDLType::ImplementationType { GetterIDLType::extractValueFromNullable(result) };
        return WTF::nullopt;
    };
    if (auto namedProperty = accessVisibleNamedProperty<OverrideBuiltins::No>(*state, *thisObject, propertyName, getterFunctor)) {
        auto value = toJS<IDLDOMString>(*state, WTFMove(namedProperty.value()));
        slot.setValue(thisObject, static_cast<unsigned>(0), value);
        return true;
    }
    return JSObject::getOwnPropertySlot(object, state, propertyName, slot);
}

bool JSTestNamedSetterThrowingException::getOwnPropertySlotByIndex(JSObject* object, ExecState* state, unsigned index, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSTestNamedSetterThrowingException*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    auto propertyName = Identifier::from(state, index);
    using GetterIDLType = IDLDOMString;
    auto getterFunctor = [] (auto& thisObject, auto propertyName) -> Optional<typename GetterIDLType::ImplementationType> {
        auto result = thisObject.wrapped().namedItem(propertyNameToAtomicString(propertyName));
        if (!GetterIDLType::isNullValue(result))
            return typename GetterIDLType::ImplementationType { GetterIDLType::extractValueFromNullable(result) };
        return WTF::nullopt;
    };
    if (auto namedProperty = accessVisibleNamedProperty<OverrideBuiltins::No>(*state, *thisObject, propertyName, getterFunctor)) {
        auto value = toJS<IDLDOMString>(*state, WTFMove(namedProperty.value()));
        slot.setValue(thisObject, static_cast<unsigned>(0), value);
        return true;
    }
    return JSObject::getOwnPropertySlotByIndex(object, state, index, slot);
}

void JSTestNamedSetterThrowingException::getOwnPropertyNames(JSObject* object, ExecState* state, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    auto* thisObject = jsCast<JSTestNamedSetterThrowingException*>(object);
    ASSERT_GC_OBJECT_INHERITS(object, info());
    for (auto& propertyName : thisObject->wrapped().supportedPropertyNames())
        propertyNames.add(Identifier::fromString(state, propertyName));
    JSObject::getOwnPropertyNames(object, state, propertyNames, mode);
}

bool JSTestNamedSetterThrowingException::put(JSCell* cell, ExecState* state, PropertyName propertyName, JSValue value, PutPropertySlot& putPropertySlot)
{
    auto* thisObject = jsCast<JSTestNamedSetterThrowingException*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (!propertyName.isSymbol()) {
        PropertySlot slot { thisObject, PropertySlot::InternalMethodType::VMInquiry };
        JSValue prototype = thisObject->getPrototypeDirect(state->vm());
        if (!(prototype.isObject() && asObject(prototype)->getPropertySlot(state, propertyName, slot))) {
            auto throwScope = DECLARE_THROW_SCOPE(state->vm());
            auto nativeValue = convert<IDLDOMString>(*state, value);
            RETURN_IF_EXCEPTION(throwScope, true);
            propagateException(*state, throwScope, thisObject->wrapped().setNamedItem(propertyNameToString(propertyName), WTFMove(nativeValue)));
            return true;
        }
    }

    return JSObject::put(thisObject, state, propertyName, value, putPropertySlot);
}

bool JSTestNamedSetterThrowingException::putByIndex(JSCell* cell, ExecState* state, unsigned index, JSValue value, bool shouldThrow)
{
    auto* thisObject = jsCast<JSTestNamedSetterThrowingException*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    auto propertyName = Identifier::from(state, index);
    PropertySlot slot { thisObject, PropertySlot::InternalMethodType::VMInquiry };
    JSValue prototype = thisObject->getPrototypeDirect(state->vm());
    if (!(prototype.isObject() && asObject(prototype)->getPropertySlot(state, propertyName, slot))) {
        auto throwScope = DECLARE_THROW_SCOPE(state->vm());
        auto nativeValue = convert<IDLDOMString>(*state, value);
        RETURN_IF_EXCEPTION(throwScope, true);
        propagateException(*state, throwScope, thisObject->wrapped().setNamedItem(propertyNameToString(propertyName), WTFMove(nativeValue)));
        return true;
    }

    return JSObject::putByIndex(cell, state, index, value, shouldThrow);
}

bool JSTestNamedSetterThrowingException::defineOwnProperty(JSObject* object, ExecState* state, PropertyName propertyName, const PropertyDescriptor& propertyDescriptor, bool shouldThrow)
{
    auto* thisObject = jsCast<JSTestNamedSetterThrowingException*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (!propertyName.isSymbol()) {
        PropertySlot slot { thisObject, PropertySlot::InternalMethodType::VMInquiry };
        if (!JSObject::getOwnPropertySlot(thisObject, state, propertyName, slot)) {
            if (!propertyDescriptor.isDataDescriptor())
                return false;
            auto throwScope = DECLARE_THROW_SCOPE(state->vm());
            auto nativeValue = convert<IDLDOMString>(*state, propertyDescriptor.value());
            RETURN_IF_EXCEPTION(throwScope, true);
            propagateException(*state, throwScope, thisObject->wrapped().setNamedItem(propertyNameToString(propertyName), WTFMove(nativeValue)));
            return true;
        }
    }

    PropertyDescriptor newPropertyDescriptor = propertyDescriptor;
    newPropertyDescriptor.setConfigurable(true);
    return JSObject::defineOwnProperty(object, state, propertyName, newPropertyDescriptor, shouldThrow);
}

EncodedJSValue jsTestNamedSetterThrowingExceptionConstructor(ExecState* state, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestNamedSetterThrowingExceptionPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(state, throwScope);
    return JSValue::encode(JSTestNamedSetterThrowingException::getConstructor(state->vm(), prototype->globalObject()));
}

bool setJSTestNamedSetterThrowingExceptionConstructor(ExecState* state, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestNamedSetterThrowingExceptionPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(state, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(vm, vm.propertyNames->constructor, JSValue::decode(encodedValue));
}

void JSTestNamedSetterThrowingException::heapSnapshot(JSCell* cell, HeapSnapshotBuilder& builder)
{
    auto* thisObject = jsCast<JSTestNamedSetterThrowingException*>(cell);
    builder.setWrappedObjectForCell(cell, &thisObject->wrapped());
    if (thisObject->scriptExecutionContext())
        builder.setLabelForCell(cell, String::format("url %s", thisObject->scriptExecutionContext()->url().string().utf8().data()));
    Base::heapSnapshot(cell, builder);
}

bool JSTestNamedSetterThrowingExceptionOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor, const char** reason)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    UNUSED_PARAM(reason);
    return false;
}

void JSTestNamedSetterThrowingExceptionOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsTestNamedSetterThrowingException = static_cast<JSTestNamedSetterThrowingException*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestNamedSetterThrowingException->wrapped(), jsTestNamedSetterThrowingException);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestNamedSetterThrowingException@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore32TestNamedSetterThrowingExceptionE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::ExecState*, JSDOMGlobalObject* globalObject, Ref<TestNamedSetterThrowingException>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    void* actualVTablePointer = *(reinterpret_cast<void**>(impl.ptr()));
#if PLATFORM(WIN)
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(__identifier("??_7TestNamedSetterThrowingException@WebCore@@6B@"));
#else
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(&_ZTVN7WebCore32TestNamedSetterThrowingExceptionE[2]);
#endif

    // If this fails TestNamedSetterThrowingException does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(std::is_polymorphic<TestNamedSetterThrowingException>::value, "TestNamedSetterThrowingException is not polymorphic");

    // If you hit this assertion you either have a use after free bug, or
    // TestNamedSetterThrowingException has subclasses. If TestNamedSetterThrowingException has subclasses that get passed
    // to toJS() we currently require TestNamedSetterThrowingException you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<TestNamedSetterThrowingException>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::ExecState* state, JSDOMGlobalObject* globalObject, TestNamedSetterThrowingException& impl)
{
    return wrap(state, globalObject, impl);
}

TestNamedSetterThrowingException* JSTestNamedSetterThrowingException::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicCast<JSTestNamedSetterThrowingException*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}
