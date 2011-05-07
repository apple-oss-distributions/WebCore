/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#include "config.h"
#include "DiskImageCache.h"

#if ENABLE(DISK_IMAGE_CACHE)

#include "FileSystem.h"
#include "Logging.h"
#include "WebCoreThread.h"
#include "WebCoreThreadMessage.h"
#include <errno.h>
#include <sys/mman.h>
#include <wtf/text/CString.h>

@interface UpdateBufferOnWebThreadDispatcher : NSObject {
    RefPtr<WebCore::SharedBuffer> m_buffer;
}
- (id)initWithBuffer:(PassRefPtr<WebCore::SharedBuffer>)buffer;
- (void)runAndDispose;
@end

@implementation UpdateBufferOnWebThreadDispatcher

- (id)initWithBuffer:(PassRefPtr<WebCore::SharedBuffer>)buffer
{
    ASSERT(buffer);
    self = [super init];
    if (!self)
        return nil;

    m_buffer = buffer;
    return self;
}

- (void)runAndDispose
{
    // Notify the buffer in the case of a successful mapping.
    m_buffer->markAsMemoryMapped();

    // Stop holding on to the buffer, we don't want to keep it alive.
    m_buffer.clear();

    // Dispose of ourselves.
    [self release];
}

@end

namespace WebCore {

DiskImageCache* diskImageCache()
{
    DEFINE_STATIC_LOCAL(DiskImageCache, staticCache, ());
    return &staticCache;
}

void DiskImageCache::Entry::map(const String& path)
{
    ASSERT(m_buffer.get());
    ASSERT(!m_mapping);

    // Optimization: Don't map the buffer if we are the only object holding a
    // reference to the buffer. Just remove our reference and return. When the
    // buffer deconstructs it will remove itself. The removal is asynchronous,
    // so we don't need to worry about this entry being deleted immediately,
    // so our caller is safe to check if the entry was mapped or not.
    if (m_buffer.get()->hasOneRef()) {
        m_buffer.clear();
        return;
    }

    m_path = path;
    m_size = m_buffer->size();

    // Open the file for reading and writing.
    PlatformFileHandle handle = open(m_path.utf8().data(), O_CREAT | O_RDWR | O_TRUNC, (mode_t)0600);
    if (!isHandleValid(handle))
        return;

    // Write the data to the file.
    if (writeToFile(handle, m_buffer->data(), m_size) == -1) {
        closeFile(handle);
        deleteFile(m_path);
        return;
    }

    // Seek back to the beginning.
    if (seekFile(handle, 0, SeekFromBeginning) == -1) {
        closeFile(handle);
        deleteFile(m_path);
        return;
    }

    // Perform Memory mapping for reading.
    // NOTE: This must not conflict with the open() above, which must also open for reading.
    m_mapping = mmap(0, m_size, PROT_READ, MAP_SHARED, handle, 0);
    closeFile(handle);
    if (m_mapping == MAP_FAILED) {
        LOG(DiskImageCache, "DiskImageCache: mapping failed (%d): (%s)", errno, strerror(errno));
        m_mapping = NULL;
        deleteFile(m_path);
        return;
    }

    // Notify the buffer in the case of a successful mapping.
    // This should happen on the WebThread, because this is being run
    // asynchronously inside a dispatch queue.
    ASSERT(WebThreadNotCurrent());
    id dispatcher = [[UpdateBufferOnWebThreadDispatcher alloc] initWithBuffer:m_buffer.release()];
    NSInvocation *invocation = WebThreadMakeNSInvocation(dispatcher, @selector(runAndDispose));
    WebThreadCallAPI(invocation);
}

void DiskImageCache::Entry::unmap()
{
    if (!m_mapping) {
        ASSERT(!m_size);
        return;
    }

    if (munmap(m_mapping, m_size) == -1)
        LOG_ERROR("DiskImageCache: Could not munmap a memory mapped file with id (%d)", m_id);

    m_mapping = NULL;
    m_size = 0;
}

void DiskImageCache::Entry::removeFile()
{
    ASSERT(!m_mapping);
    ASSERT(!m_size);

    if (!deleteFile(m_path))
        LOG_ERROR("DiskImageCache: Could not delete memory mapped file (%s)", m_path.utf8().data());
}


DiskImageCache::DiskImageCache()
    : m_enabled(false)
    , m_size(0)
    , m_maximumCacheSize(100 * 1024 * 1024)
    , m_minimumImageSize(100 * 1024)
    , m_nextAvailableId(DiskImageCache::invalidDiskCacheId + 1)
{
}


disk_cache_id_t DiskImageCache::writeItem(PassRefPtr<SharedBuffer> item)
{
    if (!isEnabled() || !createDirectoryIfNeeded())
        return DiskImageCache::invalidDiskCacheId;

    // We are already full, cannot add anything until something is removed.
    if (isFull()) {
        LOG(DiskImageCache, "DiskImageCache: could not process an item because the cache was full at (%d). The \"max\" being (%d)", m_size, m_maximumCacheSize);
        return DiskImageCache::invalidDiskCacheId;
    }

    // Create an entry.
    disk_cache_id_t id = nextAvailableId();
    RefPtr<DiskImageCache::Entry> entry = DiskImageCache::Entry::create(item, id);
    m_table.add(id, entry);

    // Create a temporary file path.
    String path = temporaryFile();
    LOG(DiskImageCache, "DiskImageCache: creating entry (%d) at (%s)", id, path.utf8().data());
    if (path.isNull())
        return DiskImageCache::invalidDiskCacheId;

    // Map to disk asynchronously.
    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        // The cache became full since the time we were added to the queue. Don't map.
        if (diskImageCache()->isFull())
            return;

        entry->map(path);

        // Update the size on a sucessful mapping.
        if (entry->isMapped())
            diskImageCache()->updateSize(entry->size());
    });

    return id;
}

void DiskImageCache::updateSize(unsigned delta)
{
    MutexLocker lock(m_mutex);
    m_size += delta;
}

void DiskImageCache::removeItem(disk_cache_id_t id)
{
    LOG(DiskImageCache, "DiskImageCache: removeItem (%d)", id);
    RefPtr<DiskImageCache::Entry> entry = m_table.get(id);
    m_table.remove(id);
    if (!entry->isMapped())
        return;

    updateSize(-(entry->size()));

    dispatch_async(dispatch_get_global_queue(0, 0), ^{
        entry->unmap();
        entry->removeFile();
    });
}

void* DiskImageCache::dataForItem(disk_cache_id_t id)
{
    ASSERT(id);

    RefPtr<DiskImageCache::Entry> entry = m_table.get(id);
    ASSERT(entry->isMapped());
    return entry->data();
}

bool DiskImageCache::createDirectoryIfNeeded()
{
    if (!m_cacheDirectory.isNull())
        return true;

    m_cacheDirectory = temporaryDirectory();
    LOG(DiskImageCache, "DiskImageCache: Created temporary directory (%s)", m_cacheDirectory.utf8().data());
    if (m_cacheDirectory.isNull()) {
        LOG_ERROR("DiskImageCache: could not create cache directory");
        return false;
    }

    if (m_client)
        m_client->didCreateDiskImageCacheDirectory(m_cacheDirectory);

    return true;
}

disk_cache_id_t DiskImageCache::nextAvailableId()
{
    disk_cache_id_t nextId = m_nextAvailableId;
    m_nextAvailableId++;
    return nextId;
}

} // namespace WebCore

#endif // ENABLE(DISK_IMAGE_CACHE)
