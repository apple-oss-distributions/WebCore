/*
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "BitmapImage.h"

#include "FloatRect.h"
#include "ImageObserver.h"
#include "IntRect.h"
#include "PlatformString.h"
#include "Timer.h"
#include <wtf/Vector.h>
#include "MIMETypeRegistry.h"

#include "SystemTime.h"

namespace WebCore {

// Animated images >5MB are considered large enough that we'll only hang on to
// one frame at a time.
const unsigned cLargeAnimationCutoff = 5242880;

BitmapImage::BitmapImage(ImageObserver* observer)
    : Image(observer)
    , m_imageAnimationDisabled(false)
    , m_progressiveLoadChunkTime(0)
    , m_progressiveLoadChunkCount(0)
    , m_currentFrame(0)
    , m_frames(0)
    , m_frameTimer(0)
    , m_repetitionCount(0)
    , m_repetitionsComplete(0)
    , m_isSolidColor(false)
    , m_animatingImageType(true)
    , m_animationFinished(false)
    , m_allDataReceived(false)
    , m_haveSize(false)
    , m_sizeAvailable(false)
    , m_decodedSize(0)
{
    initPlatformData();
}

BitmapImage::~BitmapImage()
{
    invalidatePlatformData();
    stopAnimation();
}

void BitmapImage::destroyDecodedData(bool incremental)
{
    // Destroy the cached images and release them.
    if (m_frames.size()) {
        int sizeChange = 0;
        for (unsigned i = incremental ? m_frames.size() - 1 : 0; i < m_frames.size(); i++) {
            if (m_frames[i].m_frame) {
                sizeChange -= m_frames[i].m_bytes;
                m_frames[i].clear();
            }
        }

        // We just always invalidate our platform data, even in the incremental case.
        // This could be better, but it's not a big deal.
        m_isSolidColor = false;
        invalidatePlatformData();
        
        if (sizeChange) {
            m_decodedSize += sizeChange;
            if (imageObserver())
                imageObserver()->decodedSizeChanged(this, sizeChange);
        }
        
        if (!incremental) {
            // Reset the image source, since Image I/O has an underlying cache that it uses
            // while animating that it seems to never clear.
            m_source.clear();
            m_source.setData(m_data.get(), m_allDataReceived);
        }
    }
}

void BitmapImage::cacheFrame(size_t index, float scaleHint)
{
    size_t numFrames = frameCount();
    ASSERT(m_decodedSize == 0 || numFrames > 1);
    
    if (!m_frames.size() && shouldAnimate()) {            
        // Snag the repetition count.
        m_repetitionCount = m_source.repetitionCount();
        if (m_repetitionCount == cAnimationNone)
            m_animatingImageType = false;
    }
    
    if (m_frames.size() < numFrames)
        m_frames.grow(numFrames);

    if (m_sizeAvailable && imageObserver() && !imageObserver()->shouldDecodeFrame(this, m_size))
        return;

    m_frames[index].m_frame = m_source.createFrameAtIndex(index, scaleHint, &m_frames[index].m_scale, &m_frames[index].m_bytes);
    if (numFrames == 1 && m_frames[index].m_frame)
        checkForSolidColor();

    if (!m_frames[index].m_haveInfo)
        cacheFrameInfo(index);
    int sizeChange = m_frames[index].m_bytes;

    if (sizeChange) {
        m_decodedSize += sizeChange;
        if (imageObserver())
            imageObserver()->decodedSizeChanged(this, sizeChange);
    }
}

void BitmapImage::cacheFrameInfo(size_t index)
{
    size_t numFrames = frameCount();
    ASSERT(!m_frames[index].m_haveInfo);

    if (m_frames.size() < numFrames)
        m_frames.resize(numFrames);

    if (shouldAnimate())
        m_frames[index].m_duration = m_source.frameDurationAtIndex(index);
    m_frames[index].m_hasAlpha = m_source.frameHasAlphaAtIndex(index);
    m_frames[index].m_haveInfo = true;
}

IntSize BitmapImage::size() const
{
    if (m_sizeAvailable && !m_haveSize) {
        m_size = m_source.size();
        m_haveSize = true;
    }
    return m_size;
}

bool BitmapImage::dataChanged(bool allDataReceived)
{
    destroyDecodedData(true);
    
    // Feed all the data we've seen so far to the image decoder.
    m_allDataReceived = allDataReceived;

    static const double chunkLoadIntervals[] = {0.0, 1.0, 3.0, 6.0, 15.0};
    double interval = chunkLoadIntervals[std::min(m_progressiveLoadChunkCount, 4u)];
    
    bool needsUpdate = false;
    if (currentTime() - m_progressiveLoadChunkTime > interval) { // the first time through, the chunk time will be 0 and the image will get an update.
        needsUpdate = true;
        m_progressiveLoadChunkTime = currentTime();
        m_progressiveLoadChunkCount++;
    }
    if (needsUpdate || allDataReceived)
        m_source.setData(m_data.get(), allDataReceived);
    
    // Image properties will not be available until the first frame of the file
    // reaches kCGImageStatusIncomplete.
    return isSizeAvailable();
}

size_t BitmapImage::frameCount()
{
    return m_source.frameCount();
}

bool BitmapImage::isSizeAvailable()
{
    if (m_sizeAvailable)
        return true;

    m_sizeAvailable = m_source.isSizeAvailable();

    return m_sizeAvailable;
}

NativeImagePtr BitmapImage::frameAtIndex(size_t index)
{
    return frameAtIndex(index, 1.0f);
}

NativeImagePtr BitmapImage::frameAtIndex(size_t index, float scaleHint)
{
    if (index >= frameCount())
        return 0;

    if (index >= m_frames.size() || !m_frames[index].m_frame)
        cacheFrame(index, scaleHint);
    else if (std::min(1.0f, scaleHint) > m_frames[index].m_scale) {
        // If the image is already cached, but at too small a size, re-decode a larger version.
        int sizeChange = -m_frames[index].m_bytes;
        m_frames[index].clear();
        invalidatePlatformData();
        m_decodedSize += sizeChange;
        if (imageObserver())
            imageObserver()->decodedSizeChanged(this, sizeChange);

        cacheFrame(index, scaleHint);
    }

    return m_frames[index].m_frame;
}

float BitmapImage::frameDurationAtIndex(size_t index)
{
    if (index >= frameCount())
        return 0;

    if (index >= m_frames.size() || !m_frames[index].m_haveInfo)
        cacheFrameInfo(index);

    return m_frames[index].m_duration;
}

bool BitmapImage::frameHasAlphaAtIndex(size_t index)
{
    if (index >= frameCount())
        return 0;

    if (index >= m_frames.size() || !m_frames[index].m_haveInfo)
        cacheFrameInfo(index);

    return m_frames[index].m_hasAlpha;
}

bool BitmapImage::shouldAnimate()
{
    // This is a fix for <rdar://problem/4812660> Memory bloat problem
    // We now cap animated gif decoded size to 2MB. (width * height * 4 bytes per pixel * number of frames)
    // If larger, does not animate.
    return (m_animatingImageType && !m_animationFinished && imageObserver() && (width() * height() * 4 * frameCount() < 2097152) && !m_imageAnimationDisabled);
}

void BitmapImage::startAnimation()
{
    if (m_frameTimer || !shouldAnimate() || frameCount() <= 1)
        return;

    // Don't advance the animation until the current frame has completely loaded.
    if (!m_source.frameIsCompleteAtIndex(m_currentFrame))
        return;

    m_frameTimer = new Timer<BitmapImage>(this, &BitmapImage::advanceAnimation);
    m_frameTimer->startOneShot(frameDurationAtIndex(m_currentFrame));
}

void BitmapImage::stopAnimation()
{
    // This timer is used to animate all occurrences of this image.  Don't invalidate
    // the timer unless all renderers have stopped drawing.
    delete m_frameTimer;
    m_frameTimer = 0;
}

void BitmapImage::resetAnimation()
{
    stopAnimation();
    m_currentFrame = 0;
    m_repetitionsComplete = 0;
    m_animationFinished = false;
    int frameSize = m_size.width() * m_size.height() * 4;
    
    // For extremely large animations, when the animation is reset, we just throw everything away.
    if (frameCount() * frameSize > cLargeAnimationCutoff)
        destroyDecodedData();
}

void BitmapImage::advanceAnimation(Timer<BitmapImage>* timer)
{
    // Stop the animation.
    stopAnimation();
    
    // See if anyone is still paying attention to this animation.  If not, we don't
    // advance and will remain suspended at the current frame until the animation is resumed.
    if (imageObserver()->shouldPauseAnimation(this))
        return;

    m_currentFrame++;
    if (m_currentFrame >= frameCount()) {
        m_repetitionsComplete += 1;
        if (m_repetitionCount && m_repetitionsComplete >= m_repetitionCount) {
            m_animationFinished = true;
            m_currentFrame--;
            return;
        }
        m_currentFrame = 0;
    }

    // Notify our observer that the animation has advanced.
    imageObserver()->animationAdvanced(this);

    // For large animated images, go ahead and throw away frames as we go to save
    // footprint.
    int frameSize = m_size.width() * m_size.height() * 4;
    if (frameCount() * frameSize > cLargeAnimationCutoff) {
        // Destroy all of our frames and just redecode every time.
        destroyDecodedData();

        // Go ahead and decode the next frame.
        frameAtIndex(m_currentFrame);
    }
    
    // We do not advance the animation explicitly.  We rely on a subsequent draw of the image
    // to force a request for the next frame via startAnimation().  This allows images that move offscreen while
    // scrolling to stop animating (thus saving memory from additional decoded frames and
    // CPU time spent doing the decoding).
}

unsigned BitmapImage::animatedImageSize()
{
    if (frameCount() <= 1)
        return 0;
    return (width() * height() * 4 * frameCount());
}
    
void BitmapImage::disableImageAnimation()
{
    m_imageAnimationDisabled = true;
}
    
}
