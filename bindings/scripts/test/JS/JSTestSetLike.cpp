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
#include "JSTestSetLike.h"

#include "ActiveDOMObject.h"
#include "DOMIsoSubspaces.h"
#include "JSDOMAttribute.h"
#include "JSDOMBinding.h"
#include "JSDOMConstructorNotConstructable.h"
#include "JSDOMConvertAny.h"
#include "JSDOMConvertStrings.h"
#include "JSDOMExceptionHandling.h"
#include "JSDOMOperation.h"
#include "JSDOMSetLike.h"
#include "JSDOMWrapperCache.h"
#include "ScriptExecutionContext.h"
#include "WebCoreJSClientData.h"
#include <JavaScriptCore/BuiltinNames.h>
#include <JavaScriptCore/FunctionPrototype.h>
#include <JavaScriptCore/HeapAnalyzer.h>
#include <JavaScriptCore/JSCInlines.h>
#include <JavaScriptCore/JSDestructibleObjectHeapCellType.h>
#include <JavaScriptCore/SubspaceInlines.h>
#include <wtf/GetPtr.h>
#include <wtf/PointerPreparations.h>
#include <wtf/URL.h>


namespace WebCore {
using namespace JSC;

// Functions

static JSC_DECLARE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_has);
static JSC_DECLARE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_entries);
static JSC_DECLARE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_keys);
static JSC_DECLARE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_values);
static JSC_DECLARE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_forEach);
static JSC_DECLARE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_add);
static JSC_DECLARE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_clear);
static JSC_DECLARE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_delete);

// Attributes

static JSC_DECLARE_CUSTOM_GETTER(jsTestSetLikeConstructor);
static JSC_DECLARE_CUSTOM_GETTER(jsTestSetLike_size);

class JSTestSetLikePrototype final : public JSC::JSNonFinalObject {
public:
    using Base = JSC::JSNonFinalObject;
    static JSTestSetLikePrototype* create(JSC::VM& vm, JSDOMGlobalObject* globalObject, JSC::Structure* structure)
    {
        JSTestSetLikePrototype* ptr = new (NotNull, JSC::allocateCell<JSTestSetLikePrototype>(vm.heap)) JSTestSetLikePrototype(vm, globalObject, structure);
        ptr->finishCreation(vm);
        return ptr;
    }

    DECLARE_INFO;
    template<typename CellType, JSC::SubspaceAccess>
    static JSC::IsoSubspace* subspaceFor(JSC::VM& vm)
    {
        STATIC_ASSERT_ISO_SUBSPACE_SHARABLE(JSTestSetLikePrototype, Base);
        return &vm.plainObjectSpace;
    }
    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), info());
    }

private:
    JSTestSetLikePrototype(JSC::VM& vm, JSC::JSGlobalObject*, JSC::Structure* structure)
        : JSC::JSNonFinalObject(vm, structure)
    {
    }

    void finishCreation(JSC::VM&);
};
STATIC_ASSERT_ISO_SUBSPACE_SHARABLE(JSTestSetLikePrototype, JSTestSetLikePrototype::Base);

using JSTestSetLikeDOMConstructor = JSDOMConstructorNotConstructable<JSTestSetLike>;

template<> JSValue JSTestSetLikeDOMConstructor::prototypeForStructure(JSC::VM& vm, const JSDOMGlobalObject& globalObject)
{
    UNUSED_PARAM(vm);
    return globalObject.functionPrototype();
}

template<> void JSTestSetLikeDOMConstructor::initializeProperties(VM& vm, JSDOMGlobalObject& globalObject)
{
    putDirect(vm, vm.propertyNames->prototype, JSTestSetLike::prototype(vm, globalObject), JSC::PropertyAttribute::DontDelete | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->name, jsNontrivialString(vm, "TestSetLike"_s), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(0), JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::DontEnum);
}

template<> const ClassInfo JSTestSetLikeDOMConstructor::s_info = { "TestSetLike", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestSetLikeDOMConstructor) };

/* Hash table for prototype */

static const HashTableValue JSTestSetLikePrototypeTableValues[] =
{
    { "constructor", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestSetLikeConstructor), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(0) } },
    { "size", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::ReadOnly | JSC::PropertyAttribute::CustomAccessor), NoIntrinsic, { (intptr_t)static_cast<PropertySlot::GetValueFunc>(jsTestSetLike_size), (intptr_t) static_cast<PutPropertySlot::PutValueFunc>(0) } },
    { "has", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestSetLikePrototypeFunction_has), (intptr_t) (1) } },
    { "entries", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestSetLikePrototypeFunction_entries), (intptr_t) (0) } },
    { "keys", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestSetLikePrototypeFunction_keys), (intptr_t) (0) } },
    { "values", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestSetLikePrototypeFunction_values), (intptr_t) (0) } },
    { "forEach", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestSetLikePrototypeFunction_forEach), (intptr_t) (1) } },
    { "add", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestSetLikePrototypeFunction_add), (intptr_t) (1) } },
    { "clear", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestSetLikePrototypeFunction_clear), (intptr_t) (0) } },
    { "delete", static_cast<unsigned>(JSC::PropertyAttribute::DontEnum | JSC::PropertyAttribute::Function), NoIntrinsic, { (intptr_t)static_cast<RawNativeFunction>(jsTestSetLikePrototypeFunction_delete), (intptr_t) (1) } },
};

const ClassInfo JSTestSetLikePrototype::s_info = { "TestSetLike", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestSetLikePrototype) };

void JSTestSetLikePrototype::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    reifyStaticProperties(vm, JSTestSetLike::info(), JSTestSetLikePrototypeTableValues, *this);
    putDirect(vm, vm.propertyNames->iteratorSymbol, getDirect(vm, vm.propertyNames->builtinNames().entriesPublicName()), static_cast<unsigned>(JSC::PropertyAttribute::DontEnum));
    JSC_TO_STRING_TAG_WITHOUT_TRANSITION();
}

const ClassInfo JSTestSetLike::s_info = { "TestSetLike", &Base::s_info, nullptr, nullptr, CREATE_METHOD_TABLE(JSTestSetLike) };

JSTestSetLike::JSTestSetLike(Structure* structure, JSDOMGlobalObject& globalObject, Ref<TestSetLike>&& impl)
    : JSDOMWrapper<TestSetLike>(structure, globalObject, WTFMove(impl))
{
}

void JSTestSetLike::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
    ASSERT(inherits(vm, info()));

    static_assert(!std::is_base_of<ActiveDOMObject, TestSetLike>::value, "Interface is not marked as [ActiveDOMObject] even though implementation class subclasses ActiveDOMObject.");

}

JSObject* JSTestSetLike::createPrototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return JSTestSetLikePrototype::create(vm, &globalObject, JSTestSetLikePrototype::createStructure(vm, &globalObject, globalObject.objectPrototype()));
}

JSObject* JSTestSetLike::prototype(VM& vm, JSDOMGlobalObject& globalObject)
{
    return getDOMPrototype<JSTestSetLike>(vm, globalObject);
}

JSValue JSTestSetLike::getConstructor(VM& vm, const JSGlobalObject* globalObject)
{
    return getDOMConstructor<JSTestSetLikeDOMConstructor>(vm, *jsCast<const JSDOMGlobalObject*>(globalObject));
}

void JSTestSetLike::destroy(JSC::JSCell* cell)
{
    JSTestSetLike* thisObject = static_cast<JSTestSetLike*>(cell);
    thisObject->JSTestSetLike::~JSTestSetLike();
}

template<> inline JSTestSetLike* IDLAttribute<JSTestSetLike>::cast(JSGlobalObject& lexicalGlobalObject, EncodedJSValue thisValue)
{
    return jsDynamicCast<JSTestSetLike*>(JSC::getVM(&lexicalGlobalObject), JSValue::decode(thisValue));
}

template<> inline JSTestSetLike* IDLOperation<JSTestSetLike>::cast(JSGlobalObject& lexicalGlobalObject, CallFrame& callFrame)
{
    return jsDynamicCast<JSTestSetLike*>(JSC::getVM(&lexicalGlobalObject), callFrame.thisValue());
}

JSC_DEFINE_CUSTOM_GETTER(jsTestSetLikeConstructor, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    VM& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    auto* prototype = jsDynamicCast<JSTestSetLikePrototype*>(vm, JSValue::decode(thisValue));
    if (UNLIKELY(!prototype))
        return throwVMTypeError(lexicalGlobalObject, throwScope);
    return JSValue::encode(JSTestSetLike::getConstructor(JSC::getVM(lexicalGlobalObject), prototype->globalObject()));
}

static inline JSValue jsTestSetLike_sizeGetter(JSGlobalObject& lexicalGlobalObject, JSTestSetLike& thisObject)
{
    auto& vm = JSC::getVM(&lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    RELEASE_AND_RETURN(throwScope, (toJS<IDLAny>(lexicalGlobalObject, throwScope, forwardSizeToSetLike(lexicalGlobalObject, thisObject))));
}

JSC_DEFINE_CUSTOM_GETTER(jsTestSetLike_size, (JSGlobalObject* lexicalGlobalObject, EncodedJSValue thisValue, PropertyName))
{
    return IDLAttribute<JSTestSetLike>::get<jsTestSetLike_sizeGetter>(*lexicalGlobalObject, thisValue, "size");
}

static inline JSC::EncodedJSValue jsTestSetLikePrototypeFunction_hasBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestSetLike>::ClassParameter castedThis)
{
    auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    if (UNLIKELY(callFrame->argumentCount() < 1))
        return throwVMError(lexicalGlobalObject, throwScope, createNotEnoughArgumentsError(lexicalGlobalObject));
    EnsureStillAliveScope argument0 = callFrame->uncheckedArgument(0);
    auto key = convert<IDLDOMString>(*lexicalGlobalObject, argument0.value());
    RETURN_IF_EXCEPTION(throwScope, encodedJSValue());
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLAny>(forwardHasToSetLike(*lexicalGlobalObject, *callFrame, *castedThis, WTFMove(key)))));
}

JSC_DEFINE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_has, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestSetLike>::call<jsTestSetLikePrototypeFunction_hasBody>(*lexicalGlobalObject, *callFrame, "has");
}

static inline JSC::EncodedJSValue jsTestSetLikePrototypeFunction_entriesBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestSetLike>::ClassParameter castedThis)
{
    auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLAny>(forwardEntriesToSetLike(*lexicalGlobalObject, *callFrame, *castedThis))));
}

JSC_DEFINE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_entries, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestSetLike>::call<jsTestSetLikePrototypeFunction_entriesBody>(*lexicalGlobalObject, *callFrame, "entries");
}

static inline JSC::EncodedJSValue jsTestSetLikePrototypeFunction_keysBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestSetLike>::ClassParameter castedThis)
{
    auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLAny>(forwardKeysToSetLike(*lexicalGlobalObject, *callFrame, *castedThis))));
}

JSC_DEFINE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_keys, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestSetLike>::call<jsTestSetLikePrototypeFunction_keysBody>(*lexicalGlobalObject, *callFrame, "keys");
}

static inline JSC::EncodedJSValue jsTestSetLikePrototypeFunction_valuesBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestSetLike>::ClassParameter castedThis)
{
    auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLAny>(forwardValuesToSetLike(*lexicalGlobalObject, *callFrame, *castedThis))));
}

JSC_DEFINE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_values, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestSetLike>::call<jsTestSetLikePrototypeFunction_valuesBody>(*lexicalGlobalObject, *callFrame, "values");
}

static inline JSC::EncodedJSValue jsTestSetLikePrototypeFunction_forEachBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestSetLike>::ClassParameter castedThis)
{
    auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    if (UNLIKELY(callFrame->argumentCount() < 1))
        return throwVMError(lexicalGlobalObject, throwScope, createNotEnoughArgumentsError(lexicalGlobalObject));
    EnsureStillAliveScope argument0 = callFrame->uncheckedArgument(0);
    auto callback = convert<IDLAny>(*lexicalGlobalObject, argument0.value());
    RETURN_IF_EXCEPTION(throwScope, encodedJSValue());
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLAny>(forwardForEachToSetLike(*lexicalGlobalObject, *callFrame, *castedThis, WTFMove(callback)))));
}

JSC_DEFINE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_forEach, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestSetLike>::call<jsTestSetLikePrototypeFunction_forEachBody>(*lexicalGlobalObject, *callFrame, "forEach");
}

static inline JSC::EncodedJSValue jsTestSetLikePrototypeFunction_addBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestSetLike>::ClassParameter castedThis)
{
    auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    if (UNLIKELY(callFrame->argumentCount() < 1))
        return throwVMError(lexicalGlobalObject, throwScope, createNotEnoughArgumentsError(lexicalGlobalObject));
    EnsureStillAliveScope argument0 = callFrame->uncheckedArgument(0);
    auto key = convert<IDLDOMString>(*lexicalGlobalObject, argument0.value());
    RETURN_IF_EXCEPTION(throwScope, encodedJSValue());
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLAny>(forwardAddToSetLike(*lexicalGlobalObject, *callFrame, *castedThis, WTFMove(key)))));
}

JSC_DEFINE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_add, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestSetLike>::call<jsTestSetLikePrototypeFunction_addBody>(*lexicalGlobalObject, *callFrame, "add");
}

static inline JSC::EncodedJSValue jsTestSetLikePrototypeFunction_clearBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestSetLike>::ClassParameter castedThis)
{
    auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    throwScope.release();
    forwardClearToSetLike(*lexicalGlobalObject, *callFrame, *castedThis);
    return JSValue::encode(jsUndefined());
}

JSC_DEFINE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_clear, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestSetLike>::call<jsTestSetLikePrototypeFunction_clearBody>(*lexicalGlobalObject, *callFrame, "clear");
}

static inline JSC::EncodedJSValue jsTestSetLikePrototypeFunction_deleteBody(JSC::JSGlobalObject* lexicalGlobalObject, JSC::CallFrame* callFrame, typename IDLOperation<JSTestSetLike>::ClassParameter castedThis)
{
    auto& vm = JSC::getVM(lexicalGlobalObject);
    auto throwScope = DECLARE_THROW_SCOPE(vm);
    UNUSED_PARAM(throwScope);
    UNUSED_PARAM(callFrame);
    if (UNLIKELY(callFrame->argumentCount() < 1))
        return throwVMError(lexicalGlobalObject, throwScope, createNotEnoughArgumentsError(lexicalGlobalObject));
    EnsureStillAliveScope argument0 = callFrame->uncheckedArgument(0);
    auto key = convert<IDLDOMString>(*lexicalGlobalObject, argument0.value());
    RETURN_IF_EXCEPTION(throwScope, encodedJSValue());
    RELEASE_AND_RETURN(throwScope, JSValue::encode(toJS<IDLAny>(forwardDeleteToSetLike(*lexicalGlobalObject, *callFrame, *castedThis, WTFMove(key)))));
}

JSC_DEFINE_HOST_FUNCTION(jsTestSetLikePrototypeFunction_delete, (JSGlobalObject* lexicalGlobalObject, CallFrame* callFrame))
{
    return IDLOperation<JSTestSetLike>::call<jsTestSetLikePrototypeFunction_deleteBody>(*lexicalGlobalObject, *callFrame, "delete");
}

JSC::IsoSubspace* JSTestSetLike::subspaceForImpl(JSC::VM& vm)
{
    auto& clientData = *static_cast<JSVMClientData*>(vm.clientData);
    auto& spaces = clientData.subspaces();
    if (auto* space = spaces.m_subspaceForTestSetLike.get())
        return space;
    static_assert(std::is_base_of_v<JSC::JSDestructibleObject, JSTestSetLike> || !JSTestSetLike::needsDestruction);
    if constexpr (std::is_base_of_v<JSC::JSDestructibleObject, JSTestSetLike>)
        spaces.m_subspaceForTestSetLike = makeUnique<IsoSubspace> ISO_SUBSPACE_INIT(vm.heap, vm.destructibleObjectHeapCellType.get(), JSTestSetLike);
    else
        spaces.m_subspaceForTestSetLike = makeUnique<IsoSubspace> ISO_SUBSPACE_INIT(vm.heap, vm.cellHeapCellType.get(), JSTestSetLike);
    auto* space = spaces.m_subspaceForTestSetLike.get();
IGNORE_WARNINGS_BEGIN("unreachable-code")
IGNORE_WARNINGS_BEGIN("tautological-compare")
    if (&JSTestSetLike::visitOutputConstraints != &JSC::JSCell::visitOutputConstraints)
        clientData.outputConstraintSpaces().append(space);
IGNORE_WARNINGS_END
IGNORE_WARNINGS_END
    return space;
}

void JSTestSetLike::analyzeHeap(JSCell* cell, HeapAnalyzer& analyzer)
{
    auto* thisObject = jsCast<JSTestSetLike*>(cell);
    analyzer.setWrappedObjectForCell(cell, &thisObject->wrapped());
    if (thisObject->scriptExecutionContext())
        analyzer.setLabelForCell(cell, "url " + thisObject->scriptExecutionContext()->url().string());
    Base::analyzeHeap(cell, analyzer);
}

bool JSTestSetLikeOwner::isReachableFromOpaqueRoots(JSC::Handle<JSC::Unknown> handle, void*, SlotVisitor& visitor, const char** reason)
{
    UNUSED_PARAM(handle);
    UNUSED_PARAM(visitor);
    UNUSED_PARAM(reason);
    return false;
}

void JSTestSetLikeOwner::finalize(JSC::Handle<JSC::Unknown> handle, void* context)
{
    auto* jsTestSetLike = static_cast<JSTestSetLike*>(handle.slot()->asCell());
    auto& world = *static_cast<DOMWrapperWorld*>(context);
    uncacheWrapper(world, &jsTestSetLike->wrapped(), jsTestSetLike);
}

#if ENABLE(BINDING_INTEGRITY)
#if PLATFORM(WIN)
#pragma warning(disable: 4483)
extern "C" { extern void (*const __identifier("??_7TestSetLike@WebCore@@6B@")[])(); }
#else
extern "C" { extern void* _ZTVN7WebCore11TestSetLikeE[]; }
#endif
#endif

JSC::JSValue toJSNewlyCreated(JSC::JSGlobalObject*, JSDOMGlobalObject* globalObject, Ref<TestSetLike>&& impl)
{

#if ENABLE(BINDING_INTEGRITY)
    const void* actualVTablePointer = getVTablePointer(impl.ptr());
#if PLATFORM(WIN)
    void* expectedVTablePointer = __identifier("??_7TestSetLike@WebCore@@6B@");
#else
    void* expectedVTablePointer = &_ZTVN7WebCore11TestSetLikeE[2];
#endif

    // If this fails TestSetLike does not have a vtable, so you need to add the
    // ImplementationLacksVTable attribute to the interface definition
    static_assert(std::is_polymorphic<TestSetLike>::value, "TestSetLike is not polymorphic");

    // If you hit this assertion you either have a use after free bug, or
    // TestSetLike has subclasses. If TestSetLike has subclasses that get passed
    // to toJS() we currently require TestSetLike you to opt out of binding hardening
    // by adding the SkipVTableValidation attribute to the interface IDL definition
    RELEASE_ASSERT(actualVTablePointer == expectedVTablePointer);
#endif
    return createWrapper<TestSetLike>(globalObject, WTFMove(impl));
}

JSC::JSValue toJS(JSC::JSGlobalObject* lexicalGlobalObject, JSDOMGlobalObject* globalObject, TestSetLike& impl)
{
    return wrap(lexicalGlobalObject, globalObject, impl);
}

TestSetLike* JSTestSetLike::toWrapped(JSC::VM& vm, JSC::JSValue value)
{
    if (auto* wrapper = jsDynamicCast<JSTestSetLike*>(vm, value))
        return &wrapper->wrapped();
    return nullptr;
}

}