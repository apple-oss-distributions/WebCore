/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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

#import "config.h"
#import "MemoryPressureHandler.h"

#import <WebCore/GCController.h>
#import <WebCore/FontCache.h>
#import <WebCore/MemoryCache.h>
#import <WebCore/PageCache.h>
#import <wtf/CurrentTime.h>
#import <wtf/FastMalloc.h>


using std::max;

namespace WebCore {

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)


void MemoryPressureHandler::releaseMemory(bool critical)
{
    int savedPageCacheCapacity = pageCache()->capacity();
    pageCache()->setCapacity(critical ? 0 : pageCache()->pageCount() / 2);
    pageCache()->setCapacity(savedPageCacheCapacity);
    pageCache()->releaseAutoreleasedPagesNow();

    NSURLCache *nsurlCache = [NSURLCache sharedURLCache];
    NSUInteger savedNsurlCacheMemoryCapacity = [nsurlCache memoryCapacity];
    [nsurlCache setMemoryCapacity:critical ? 0 : [nsurlCache currentMemoryUsage] / 2];
    [nsurlCache setMemoryCapacity:savedNsurlCacheMemoryCapacity];

    fontCache()->purgeInactiveFontData();

    memoryCache()->pruneToPercentage(critical ? 0 : 0.5f);

    gcController().discardAllCompiledCode();

    WTF::releaseFastMallocFreeMemory();
}
#endif

} // namespace WebCore
