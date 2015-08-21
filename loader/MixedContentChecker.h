/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MixedContentChecker_h
#define MixedContentChecker_h

#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

class Frame;
class FrameLoaderClient;
class URL;
class SecurityOrigin;

class MixedContentChecker {
    WTF_MAKE_NONCOPYABLE(MixedContentChecker);
public:
    enum class ContentType {
        Active,
        ActiveCanWarn,
    };

    MixedContentChecker(Frame&);

    bool canDisplayInsecureContent(SecurityOrigin*, ContentType, const URL&) const;
    bool canRunInsecureContent(SecurityOrigin*, const URL&) const;
    static bool isMixedContent(SecurityOrigin*, const URL&);

private:
    // FIXME: This should probably have a separate client from FrameLoader.
    FrameLoaderClient& client() const;

    void logWarning(bool allowed, const String& action, const URL&) const;

    Frame& m_frame;
};

} // namespace WebCore

#endif // MixedContentChecker_h

