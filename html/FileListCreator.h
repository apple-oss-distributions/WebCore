/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <wtf/Forward.h>
#include <wtf/Function.h>
#include <wtf/Ref.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/WorkQueue.h>

namespace WebCore {

struct FileChooserFileInfo;
class FileList;

class FileListCreator : public ThreadSafeRefCounted<FileListCreator> {
public:
    using CompletionHandler = Function<void(Ref<FileList>&&)>;

    enum class ShouldResolveDirectories { No, Yes };
    static Ref<FileListCreator> create(const Vector<FileChooserFileInfo>& paths, ShouldResolveDirectories shouldResolveDirectories, CompletionHandler&& completionHandler)
    {
        return adoptRef(*new FileListCreator(paths, shouldResolveDirectories, WTFMove(completionHandler)));
    }

    ~FileListCreator();

    void cancel();

private:
    FileListCreator(const Vector<FileChooserFileInfo>& paths, ShouldResolveDirectories, CompletionHandler&&);

    template<ShouldResolveDirectories shouldResolveDirectories>
    static Ref<FileList> createFileList(const Vector<FileChooserFileInfo>&);

    RefPtr<WorkQueue> m_workQueue;
    CompletionHandler m_completionHander;
};

}
