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
#include "JSTestNamedSetterWithOverrideBuiltins.h"

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

JSC::EncodedJSValue jsTestNamedSetterWithOverrideBuiltinsConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::PropertyName);
bool setJSTestNamedSetterWithOverrideBuiltinsConstructor(JSC::ExecState*, JSC::EncodedJSValue, JSC::EncodedJSValue);

class JSTestNamedSetterWithOverrideBuiltinsPrototype : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestNamedSetterWithOverrideBuiltinsPrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestNamedSetterWithOverrideBuiltinsPrototype* ptr = new (NotNull, JSC::allocateCell<JSTestNamedSetterWithOverrideBuiltinsPrototype>(vm.heap)) JSTestNamedSetterWithOverrideBuiltinsPrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestNamedSetterWithOverrideBuiltinsPrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};

using JSTestNamedSetterWithOverrideBuiltinsConstructor = JSDOMConstructorNotConstructable<JSTestNamedSetterWithOverrideBuiltins>;

template<> JSValue JSTestNamedSetterWithOverrideBuiltinsConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestNamedSetterWithOverrideBuiltinsConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestNamedSetterWithOverrideBuiltins::prototype(vm, globalObject), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(&vm, String("TestNamedSetterWithOverrideBuiltins"_s)), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> const ClassInfo JSTestNamedSetterWithOverrideBuiltinsConstructor::s_info = { "TestNamedSetterWithOverrideBuiltins", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterWithOverrideBuiltinsConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestNamedSetterWithOverrideBuiltinsPrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestNamedSetterWithOverrideBuiltinsConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(setJSTestNamedSetterWithOverrideBuiltinsConstructor) } },
};

const ClassInfo JSTestNamedSetterWithOverrideBuiltinsPrototype::s_info = { "TestNamedSetterWithOverrideBuiltinsPrototype", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterWithOverrideBuiltinsPrototype) };

void JSTestNamedSetterWithOverrideBuiltinsPrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestNamedSetterWithOverrideBuiltins::info(), JSTestNamedSetterWithOverrideBuiltinsPrototypeTableValues, *this);
}

const ClassInfo JSTestNamedSetterWithOverrideBuiltins::s_info = { "TestNamedSetterWithOverrideBuiltins", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestNamedSetterWithOverrideBuiltins) };

JSTestNamedSetterWithOverrideBuiltins::JSTestNamedSetterWithOverrideBuiltins(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestNamedSetterWithOverrideBuiltins>&& impl)
    : JSDOMWrapper<TestNamedSetterWithOverrideBuiltins>(structure, globalObject, WTFMove(impl))
{
}

void JSTestNamedSetterWithOverrideBuiltins::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

}

JSObject* JSTestNamedSetterWithOverrideBuiltins::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestNamedSetterWithOverrideBuiltinsPrototype::create(vm, &globalObject, JSTestNamedSetterWithOverrideBuiltinsPrototype::createStructure(vm, &globalObject, globalObject.objectPrototype()));
}

JSObject* JSTestNamedSetterWithOverrideBuiltins::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestNamedSetterWithOverrideBuiltins>(vm, globalObject);
}

JSValue JSTestNamedSetterWithOverrideBuiltins::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestNamedSetterWithOverrideBuiltinsConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestNamedSetterWithOverrideBuiltins::destroy(JSC::JSCell* cell)
{
    JSTestNamedSetterWithOverrideBuiltins* thisObject = static_cast<JSTestNamedSetterWithOverrideBuiltins*>(cell);
    thisObject->JSTestNamedSetterWithOverrideBuiltins::~JSTestNamedSetterWithOverrideBuiltins();
}

bool JSTestNamedSetterWithOverrideBuiltins::getOwnPropertySlot(JSObject* object, ExecState* state, PropertyName propertyName, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithOverrideBuiltins*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    using GetterIDLType = IDLDOMString;
    auto getterFunctor = [] (auto& thisObject, auto propertyName) -> Optional<typename GetterIDLType::ImplementationType> {
        auto result = thisObject.wrapped().namedItem(propertyNameToAtomicString(propertyName));
        if (!GetterIDLType::isNullValue(result))
            return typename GetterIDLType::ImplementationType { GetterIDLType::extractValueFromNullable(result) };
        return WTF::nullopt;
    };
    if (auto namedProperty = accessVisibleNamedProperty<OverrideBuiltins::Yes>(*state, *thisObject, propertyName, getterFunctor)) {
        auto value = toJS<IDLDOMString>(*state, WTFMove(namedProperty.value()));
        slot.setValue(thisObject, static_cast<unsigned>(0), value);
        return true;
    }
    return JSObject::getOwnPropertySlot(object, state, propertyName, slot);
}

bool JSTestNamedSetterWithOverrideBuiltins::getOwnPropertySlotByIndex(JSObject* object, ExecState* state, unsigned index, PropertySlot& slot)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithOverrideBuiltins*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    auto propertyName = Identifier::from(state, index);
    using GetterIDLType = IDLDOMString;
    auto getterFunctor = [] (auto& thisObject, auto propertyName) -> Optional<typename GetterIDLType::ImplementationType> {
        auto result = thisObject.wrapped().namedItem(propertyNameToAtomicString(propertyName));
        if (!GetterIDLType::isNullValue(result))
            return typename GetterIDLType::ImplementationType { GetterIDLType::extractValueFromNullable(result) };
        return WTF::nullopt;
    };
    if (auto namedProperty = accessVisibleNamedProperty<OverrideBuiltins::Yes>(*state, *thisObject, propertyName, getterFunctor)) {
        auto value = toJS<IDLDOMString>(*state, WTFMove(namedProperty.value()));
        slot.setValue(thisObject, static_cast<unsigned>(0), value);
        return true;
    }
    return JSObject::getOwnPropertySlotByIndex(object, state, index, slot);
}

void JSTestNamedSetterWithOverrideBuiltins::getOwnPropertyNames(JSObject* object, ExecState* state, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithOverrideBuiltins*>(object);
    ASSERT_GC_OBJECT_INHERITS(object, info());
    for (auto& propertyName : thisObject->wrapped().supportedPropertyNames())
        propertyNames.add(Identifier::fromString(state, propertyName));
    JSObject::getOwnPropertyNames(object, state, propertyNames, mode);
}

bool JSTestNamedSetterWithOverrideBuiltins::put(JSCell* cell, ExecState* state, PropertyName propertyName, JSValue value, PutPropertySlot& putPropertySlot)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithOverrideBuiltins*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (!propertyName.isSymbol()) {
        auto throwScope = DECLARE_THROW_SCOPE(state->vm());
        auto nativeValue = convert<IDLDOMString>(*state, value);
        RETURN_IF_EXCEPTION(throwScope, true);
        thisObject->wrapped().setNamedItem(propertyNameToString(propertyName), WTFMove(nativeValue));
        return true;
    }

    return JSObject::put(thisObject, state, propertyName, value, putPropertySlot);
}

bool JSTestNamedSetterWithOverrideBuiltins::putByIndex(JSCell* cell, ExecState* state, unsigned index, JSValue value, bool)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithOverrideBuiltins*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    auto propertyName = Identifier::from(state, index);
    auto throwScope = DECLARE_THROW_SCOPE(state->vm());
    auto nativeValue = convert<IDLDOMString>(*state, value);
    RETURN_IF_EXCEPTION(throwScope, true);
    thisObject->wrapped().setNamedItem(propertyNameToString(propertyName), WTFMove(nativeValue));
    return true;
}

bool JSTestNamedSetterWithOverrideBuiltins::defineOwnProperty(JSObject* object, ExecState* state, PropertyName propertyName, const PropertyDescriptor& propertyDescriptor, bool shouldThrow)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithOverrideBuiltins*>(object);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());

    if (!propertyName.isSymbol()) {
        if (!propertyDescriptor.isDataDescriptor())
            return false;
        auto throwScope = DECLARE_THROW_SCOPE(state->vm());
        auto nativeValue = convert<IDLDOMString>(*state, propertyDescriptor.value());
        RETURN_IF_EXCEPTION(throwScope, true);
        thisObject->wrapped().setNamedItem(propertyNameToString(propertyName), WTFMove(nativeValue));
        return true;
    }

    PropertyDescriptor newPropertyDescriptor = propertyDescriptor;
    newPropertyDescriptor.setConfigurable(true);
    return JSObject::defineOwnProperty(object, state, propertyName, newPropertyDescriptor, shouldThrow);
}

EncodedJSValue jsTestNamedSetterWithOverrideBuiltinsConstructor(ExecState* state, EncodedJSValue thisValue, PropertyName)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestNamedSetterWithOverrideBuiltinsPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(state, throwScope);
    return JSValue::encode(JSTestNamedSetterWithOverrideBuiltins::getConstructor(state->vm(), prototype->globalObject()));
}

bool setJSTestNamedSetterWithOverrideBuiltinsConstructor(ExecState* state, EncodedJSValue thisValue, EncodedJSValue encodedValue)
{
    VM& vm = state->vm();
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestNamedSetterWithOverrideBuiltinsPrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype)) {
        throwVMTypeError(state, throwScope);
        return false;
    }
    // Shadowing a built-in constructor
    return prototype->putDirect(vm, vm.propertyNames->constructor, JSValue::decode(encodedValue));
}

void JSTestNamedSetterWithOverrideBuiltins::heapSnapshot(JSCell* cell, HeapSnapshotBuilder& builder)
{
    auto* thisObject = jsCast<JSTestNamedSetterWithOverrideBuiltins*>(cell);
    builder.setWrappedObjectForCell(cell, &thisObject->wrapped());
    if (thisObject->scriptExecutionContext())
        builder.setLabelForCell(cell, String::format("url %s", thisObject->scriptExecutionContext()->url().string().utf8().data()));
    Base::heapSnapshot(cell, builder);
}

bool JSTestNamedSetterWithOverrideBuiltinsOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor, const char** reason)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    UNUSED_PARAM(reason);
    return false;
}

void JSTestNamedSetterWithOverrideBuiltinsOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsTestNamedSetterWithOverrideBuiltins = static_cast<JSTestNamedSetterWithOverrideBuiltins*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestNamedSetterWithOverrideBuiltins->wrapped(), jsTestNamedSetterWithOverrideBuiltins);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestNamedSetterWithOverrideBuiltins@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore35TestNamedSetterWithOverrideBuiltinsE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::ExecState*, JSDOMGlobalObject* globalObject, Ref<TestNamedSetterWithOverrideBuiltins>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    void* actualVTablePointer = *(reinterpret_cast<void**>(impl.ptr()));
#if PLATFORM(WIN)
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(__identifier("??_7TestNamedSetterWithOverrideBuiltins@WebCore@@6B@"));
#else
    void* expectedVTablePointer = WTF_PREPARE_VTBL_POINTER_FOR_INSPECTION(&_ZTVN7WebCore35TestNamedSetterWithOverrideBuiltinsE[2]);
#endif

    // If this fails TestNamedSetterWithOverrideBuiltins does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(std::is_polymorphic<TestNamedSetterWithOverrideBuiltins>::value, "TestNamedSetterWithOverrideBuiltins is not polymorphic");

    // If you hit this assertion you either have a use after free bug, or
    // TestNamedSetterWithOverrideBuiltins has subclasses. If TestNamedSetterWithOverrideBuiltins has subclasses that get passed
    // to toJS() we currently require TestNamedSetterWithOverrideBuiltins you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<TestNamedSetterWithOverrideBuiltins>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::ExecState* state, JSDOMGlobalObject* globalObject, TestNamedSetterWithOverrideBuiltins& impl)
{
    return wrap(state, globalObject, impl);
}

TestNamedSetterWithOverrideBuiltins* JSTestNamedSetterWithOverrideBuiltins::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicCast<JSTestNamedSetterWithOverrideBuiltins*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}
