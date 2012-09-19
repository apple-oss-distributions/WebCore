/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "Color.h"
#import "KURL.h"
#import "PlatformPasteboard.h"

namespace WebCore {


PlatformPasteboard::PlatformPasteboard(const String&)
    : m_pasteboard(0)
{
}

void PlatformPasteboard::getTypes(Vector<String>&)
{
}

PassRefPtr<SharedBuffer> PlatformPasteboard::bufferForType(const String&)
{
    return 0;
}

void PlatformPasteboard::getPathnamesForType(Vector<String>&, const String&)
{
}

String PlatformPasteboard::stringForType(const String&)
{
    return String();
}

Color PlatformPasteboard::color()
{
    return Color();
}

KURL PlatformPasteboard::url()
{
    return KURL();
}

void PlatformPasteboard::copy(const String&)
{
}

void PlatformPasteboard::addTypes(const Vector<String>&)
{
}

void PlatformPasteboard::setTypes(const Vector<String>&)
{
}

void PlatformPasteboard::setBufferForType(PassRefPtr<SharedBuffer>, const String&)
{
}

void PlatformPasteboard::setPathnamesForType(const Vector<String>&, const String&)
{
}

void PlatformPasteboard::setStringForType(const String&, const String&)
{
}

int PlatformPasteboard::changeCount() const
{
    return 0;
}

String PlatformPasteboard::uniqueName()
{
    return String();
}


}
