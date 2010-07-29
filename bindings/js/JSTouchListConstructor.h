/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * Permission is granted by Apple to use this file to the extent
 * necessary to relink with LGPL WebKit files.
 *
 * No license or rights are granted by Apple expressly or by
 * implication, estoppel, or otherwise, to Apple patents and
 * trademarks. For the sake of clarity, no license or rights are
 * granted by Apple expressly or by implication, estoppel, or otherwise,
 * under any Apple patents, copyrights and trademarks to underlying
 * implementations of any application programming interfaces (APIs)
 * or to any functionality that is invoked by calling any API.
 */

#ifndef JSTouchListConstructor_h
#define JSTouchListConstructor_h


#include "JSDOMBinding.h"
#include "JSDocument.h"

namespace WebCore {

class JSTouchListConstructor : public DOMConstructorObject {
public:
    JSTouchListConstructor(JSC::ExecState*, JSDOMGlobalObject*);

    static const JSC::ClassInfo s_info;

private:
    virtual JSC::ConstructType getConstructData(JSC::ConstructData&);
    virtual const JSC::ClassInfo* classInfo() const { return &s_info; }
};

}


#endif // JSTouchListConstructor_h
