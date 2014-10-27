/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ProtectionSpaceCFNet_h
#define ProtectionSpaceCFNet_h

#if !PLATFORM(WIN) || !USE(CFNETWORK)
#error This header should only be included when targeting Windows and using CFNetwork.
#endif

#include "ProtectionSpaceBase.h"

namespace WebCore {

class ProtectionSpace : public ProtectionSpaceBase {
public:
    ProtectionSpace() : ProtectionSpaceBase() { }
    ProtectionSpace(const String& host, int port, ProtectionSpaceServerType serverType, const String& realm, ProtectionSpaceAuthenticationScheme authenticationScheme)
        : ProtectionSpaceBase(host, port, serverType, realm, authenticationScheme)
    {
    }

    ProtectionSpace(WTF::HashTableDeletedValueType deletedValue) : ProtectionSpaceBase(deletedValue) { }

    bool receivesCredentialSecurely() const;
};

} // namespace WebCore

#endif // ProtectionSpaceCFNet_h