/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

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
#include "CachedImage.h"

#include "BitmapImage.h"
#include "MemoryCache.h"
#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderTypes.h"
#include "FrameView.h"
#include "Settings.h"
#include "SharedBuffer.h"
#include <wtf/CurrentTime.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

#include "SystemMemory.h"

#if USE(CG)
#include "PDFDocumentImage.h"
#endif

#if ENABLE(SVG_AS_IMAGE)
#include "SVGImage.h"
#endif

#if ENABLE(DISK_IMAGE_CACHE)
#include "DiskImageCache.h"
#endif

using std::max;

namespace WebCore {

CachedImage::CachedImage(const String& url)
    : CachedResource(url, ImageResource)
    , m_image(0)
    , m_decodedDataDeletionTimer(this, &CachedImage::decodedDataDeletionTimerFired)
    , m_shouldPaintBrokenImage(true)
{
    setStatus(Unknown);
}

CachedImage::CachedImage(Image* image)
    : CachedResource(String(), ImageResource)
    , m_image(image)
    , m_decodedDataDeletionTimer(this, &CachedImage::decodedDataDeletionTimerFired)
    , m_shouldPaintBrokenImage(true)
{
    setStatus(Cached);
    setLoading(false);
}

CachedImage::~CachedImage()
{
}

void CachedImage::decodedDataDeletionTimerFired(Timer<CachedImage>*)
{
    ASSERT(!hasClients());
    destroyDecodedData();
}

void CachedImage::load(CachedResourceLoader* cachedResourceLoader)
{
    if (!cachedResourceLoader || cachedResourceLoader->autoLoadImages())
        CachedResource::load(cachedResourceLoader, true, DoSecurityCheck, true);
    else
        setLoading(false);
}

void CachedImage::didAddClient(CachedResourceClient* c)
{
    if (m_decodedDataDeletionTimer.isActive())
        m_decodedDataDeletionTimer.stop();
    
    if (m_data && !m_image && !errorOccurred()) {
        createImage();
        m_image->setData(m_data, true);
    }

    if (m_image && !m_image->isNull())
        c->imageChanged(this);

    CachedResource::didAddClient(c);
}

void CachedImage::allClientsRemoved()
{
    if (m_image && !errorOccurred())
        m_image->resetAnimation();
    if (double interval = memoryCache()->deadDecodedDataDeletionInterval())
        m_decodedDataDeletionTimer.startOneShot(interval);
}

static Image* brokenImage()
{
    DEFINE_STATIC_LOCAL(RefPtr<Image>, brokenImage, (Image::loadPlatformResource("missingImage")));
    return brokenImage.get();
}

Image* CachedImage::image() const
{
    ASSERT(!isPurgeable());

    if (errorOccurred() && m_shouldPaintBrokenImage)
        return brokenImage();

    if (m_image)
        return m_image.get();

    return Image::nullImage();
}

void CachedImage::setImageContainerSize(const IntSize& containerSize)
{
    if (m_image)
        m_image->setContainerSize(containerSize);
}

bool CachedImage::usesImageContainerSize() const
{
    if (m_image)
        return m_image->usesContainerSize();

    return false;
}

bool CachedImage::imageHasRelativeWidth() const
{
    if (m_image)
        return m_image->hasRelativeWidth();

    return false;
}

bool CachedImage::imageHasRelativeHeight() const
{
    if (m_image)
        return m_image->hasRelativeHeight();

    return false;
}

IntSize CachedImage::imageSize(float multiplier) const
{
    ASSERT(!isPurgeable());

    if (!m_image)
        return IntSize();
    if (multiplier == 1.0f)
        return m_image->size();
        
    // Don't let images that have a width/height >= 1 shrink below 1 when zoomed.
    bool hasWidth = m_image->size().width() > 0;
    bool hasHeight = m_image->size().height() > 0;
    int width = m_image->size().width() * (m_image->hasRelativeWidth() ? 1.0f : multiplier);
    int height = m_image->size().height() * (m_image->hasRelativeHeight() ? 1.0f : multiplier);
    if (hasWidth)
        width = max(1, width);
    if (hasHeight)
        height = max(1, height);
    return IntSize(width, height);
}

IntRect CachedImage::imageRect(float multiplier) const
{
    ASSERT(!isPurgeable());

    if (!m_image)
        return IntRect();
    if (multiplier == 1.0f || (!m_image->hasRelativeWidth() && !m_image->hasRelativeHeight()))
        return m_image->rect();

    float widthMultiplier = (m_image->hasRelativeWidth() ? 1.0f : multiplier);
    float heightMultiplier = (m_image->hasRelativeHeight() ? 1.0f : multiplier);

    // Don't let images that have a width/height >= 1 shrink below 1 when zoomed.
    bool hasWidth = m_image->rect().width() > 0;
    bool hasHeight = m_image->rect().height() > 0;

    int width = static_cast<int>(m_image->rect().width() * widthMultiplier);
    int height = static_cast<int>(m_image->rect().height() * heightMultiplier);
    if (hasWidth)
        width = max(1, width);
    if (hasHeight)
        height = max(1, height);

    int x = static_cast<int>(m_image->rect().x() * widthMultiplier);
    int y = static_cast<int>(m_image->rect().y() * heightMultiplier);

    return IntRect(x, y, width, height);
}

void CachedImage::notifyObservers(const IntRect* changeRect)
{
    CachedResourceClientWalker w(m_clients);
    while (CachedResourceClient* c = w.next())
        c->imageChanged(this, changeRect);
}

void CachedImage::checkShouldPaintBrokenImage()
{
    Frame* frame = m_request ? m_request->cachedResourceLoader()->frame() : 0;
    if (!frame)
        return;

    m_shouldPaintBrokenImage = frame->loader()->client()->shouldPaintBrokenImage(KURL(ParsedURLString, m_url));
}

void CachedImage::clear()
{
    destroyDecodedData();
    m_image = 0;
    setEncodedSize(0);
}

inline void CachedImage::createImage()
{
    // Create the image if it doesn't yet exist.
    if (m_image)
        return;
#if USE(CG) && !USE(WEBKIT_IMAGE_DECODERS)
    if (m_response.mimeType() == "application/pdf") {
        m_image = PDFDocumentImage::create();
        return;
    }
#endif
#if ENABLE(SVG_AS_IMAGE)
    if (m_response.mimeType() == "image/svg+xml") {
        m_image = SVGImage::create(this);
        return;
    }
#endif
    m_image = BitmapImage::create(this);
}

size_t CachedImage::maximumDecodedImageSize()
{
    Frame* frame = m_request ? m_request->cachedResourceLoader()->frame() : 0;
    if (!frame)
        return 0;
    Settings* settings = frame->settings();
    return settings ? settings->maximumDecodedImageSize() : 0;
}

void CachedImage::data(PassRefPtr<SharedBuffer> data, bool allDataReceived)
{
    m_data = data;

    createImage();

    bool sizeAvailable = false;

    // Have the image update its data from its internal buffer.
    // It will not do anything now, but will delay decoding until 
    // queried for info (like size or specific image frames).
    sizeAvailable = m_image->setData(m_data, allDataReceived);

    // Go ahead and tell our observers to try to draw if we have either
    // received all the data or the size is known.  Each chunk from the
    // network causes observers to repaint, which will force that chunk
    // to decode.
    if (sizeAvailable || allDataReceived) {
        size_t maxDecodedImageSize = maximumDecodedImageSize();
        IntSize s = imageSize(1.0f);
        size_t estimatedDecodedImageSize = s.width() * s.height() * 4; // no overflow check
        if (m_image->isNull() || (maxDecodedImageSize > 0 && estimatedDecodedImageSize > maxDecodedImageSize)) {
            error(errorOccurred() ? status() : DecodeError);
            if (inCache())
                memoryCache()->remove(this);
            return;
        }
        
        // It would be nice to only redraw the decoded band of the image, but with the current design
        // (decoding delayed until painting) that seems hard.
        notifyObservers();

        if (m_image)
            setEncodedSize(m_image->data() ? m_image->data()->size() : 0);
    }
    
    if (allDataReceived) {
        setLoading(false);
        checkNotify();
    }
}

void CachedImage::error(CachedResource::Status status)
{
    checkShouldPaintBrokenImage();
    clear();
    setStatus(status);
    ASSERT(errorOccurred());
    m_data.clear();
    notifyObservers();
    setLoading(false);
    checkNotify();
}

void CachedImage::destroyDecodedData()
{
    bool canDeleteImage = !m_image || (m_image->hasOneRef() && m_image->isBitmapImage());
    if (isSafeToMakePurgeable() && canDeleteImage && !isLoading()) {
        // Image refs the data buffer so we should not make it purgeable while the image is alive. 
        // Invoking addClient() will reconstruct the image object.
        m_image = 0;
        setDecodedSize(0);
        if (!MemoryCache::shouldMakeResourcePurgeableOnEviction())
            makePurgeable(true);
    } else if (m_image && !errorOccurred())
        m_image->destroyDecodedData();
}

void CachedImage::decodedSizeChanged(const Image* image, int delta)
{
    if (image != m_image)
        return;
    
    setDecodedSize(decodedSize() + delta);
}

void CachedImage::didDraw(const Image* image)
{
    if (image != m_image)
        return;
    
    double timeStamp = FrameView::currentPaintTimeStamp();
    if (!timeStamp) // If didDraw is called outside of a Frame paint.
        timeStamp = currentTime();
    
    CachedResource::didAccessDecodedData(timeStamp);
}

bool CachedImage::shouldPauseAnimation(const Image* image)
{
    if (image != m_image)
        return false;
    
    CachedResourceClientWalker w(m_clients);
    while (CachedResourceClient* c = w.next()) {
        if (c->willRenderImage(this))
            return false;
    }

    return true;
}

void CachedImage::animationAdvanced(const Image* image)
{
    if (image == m_image)
        notifyObservers();
}
    
bool CachedImage::shouldDecodeFrame(const Image* /*image*/, const IntSize& frameSize)
{
    unsigned fullFrameBytes = frameSize.width() * frameSize.height();

    const unsigned smallImageThreshold = 258 * 256 * 4;
    if (fullFrameBytes < smallImageThreshold)
        return true;

    if (hasEnoughMemoryFor(fullFrameBytes))
        return true;

    // Low on memory, lets try to prune the cache
    memoryCache()->prune();
    if (hasEnoughMemoryFor(fullFrameBytes))
        return true;

    // We are still low on memory, lets try some aggressive pruning of live decoded resources
    memoryCache()->pruneLiveResources(true);
#if ENABLE(DENY_LOW_MEMORY_IMAGE_DECODING)
    if (hasEnoughMemoryFor(fullFrameBytes))
        return true;

    // Still looking bad, don't decode the image at all.
    return false;
#else
    // So we allow decoding anyway. This might cause crash or jetsam but as long as WebKit
    // suffers from memory growth problems there are situations where restarting is the only way
    // to access the content user is trying to see.
    return true;
#endif
}

#if ENABLE(DISK_IMAGE_CACHE)
bool CachedImage::canUseDiskImageCache() const
{
    if (isLoading() || errorOccurred())
        return false;

    if (!m_data)
        return false;

    if (isPurgeable())
        return false;

    if (m_data->size() < diskImageCache()->minimumImageSize())
        return false;

    // "Cache-Control: no-store" resources may be marked as such because they may
    // contain sensitive information. We should not write these resources to disk.
    if (m_response.cacheControlContainsNoStore())
        return false;

    // Testing shows that PDF images did not work when memory mapped.
    // However, SVG images and Bitmap images were fine. See:
    // <rdar://problem/8591834> Disk Image Cache should support PDF Images
    if (m_response.mimeType() == "application/pdf")
        return false;

    return true;
}

void CachedImage::useDiskImageCache()
{
    ASSERT(canUseDiskImageCache());
    ASSERT(!isUsingDiskImageCache());

    m_data->allowToBeMemoryMapped();
}
#endif

void CachedImage::changedInRect(const Image* image, const IntRect& rect)
{
    if (image == m_image)
        notifyObservers(&rect);
}

CachedImageManual::CachedImageManual(const String& url, Image* image)
    : CachedImage(image)
    , m_fakeClient(new CachedResourceClient)
{
    m_url = url;
    // Use the incoming URL in the response field. This ensures that code
    // using the response directly, such as origin checks for security,
    // actually see something.
    m_response.setURL(KURL(ParsedURLString, url));
    setStatus(Cached);
    setLoading(false);
}

CachedImageManual::~CachedImageManual()
{
    delete m_fakeClient;
}

bool CachedImageManual::mustRevalidateDueToCacheHeaders(CachePolicy) const
{
    // Do not revalidate manually cached images. This mechanism is used as a
    // way to efficiently share an image from the client to content and
    // the URL for that image may not represent a resource that can be
    // retrieved by standard means. If the manual caching SPI is used, it is
    // incumbent on the client to only use valid resources.
    return false;
}

} //namespace WebCore
