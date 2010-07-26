/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef MediaPlayerPrivateIPhone_h
#define MediaPlayerPrivateIPhone_h

#if ENABLE(VIDEO)

#include "MediaPlayer.h"
#include "MediaPlayerProxy.h"

#include "MediaPlayerPrivate.h"
#include <wtf/RetainPtr.h>

#ifdef __OBJC__
@class NSObject;
@class WebCoreMediaPlayerNotificationHelper;
#else
class NSObject;
class WebCoreMediaPlayerNotificationHelper;
#endif

class MediaPlayerMobileInterface;

namespace WebCore {

class MediaPlayerPrivateiPhone : public MediaPlayerPrivateInterface {
public:

    static void registerMediaEngine(MediaEngineRegistrar);
    virtual ~MediaPlayerPrivateiPhone();

    void deliverNotification(MediaPlayerProxyNotificationType notification);
    bool callbacksDelayed() { return (m_delayCallbacks > 0); }
    void prepareToPlay();

private:
    MediaPlayerPrivateiPhone(MediaPlayer *player);

    // engine support
    static MediaPlayerPrivateInterface* create(MediaPlayer* player);
    static void getSupportedTypes(HashSet<String>& types);
    static MediaPlayer::SupportsType supportsType(const String& type, const String& codecs);
    static bool isAvailable();

    IntSize naturalSize() const;
    bool hasVideo() const;
    bool hasAudio() const;

    bool canLoadPoster() const { return true; }
    void setPoster(const String& url);

    void setControls(bool);

    void load(const String& url);
    void cancelLoad();

	void play();
    void pause();    

    bool paused() const;
    bool seeking() const;

    float duration() const;
    float currentTime() const;

    void seek(float time);
    void setEndTime(float time);

    float rate() const;
    void setRate(float inRate);
    float volume() const;
    void setVolume(float inVolume);
    void setMuted(bool inMute);

    int dataRate() const;

    MediaPlayer::NetworkState networkState() const;
    MediaPlayer::ReadyState readyState() const;
    
    float maxTimeBuffered() const;
    float maxTimeSeekable() const;
    PassRefPtr<TimeRanges> buffered() const;

    unsigned bytesLoaded() const;
    bool totalBytesKnown() const;
    unsigned totalBytes() const;

    void setVisible(bool);
    void setSize(const IntSize&);
    
    void paint(GraphicsContext* context, const IntRect& r);

#if USE(ACCELERATED_COMPOSITING)
    bool supportsAcceleratedRendering() const;
#endif

    void setMediaPlayerProxy(WebMediaPlayerProxy* proxy);
    void processPendingRequests();

    void setDelayCallbacks(bool doDelay) { m_delayCallbacks += (doDelay ? 1 : -1); }

    bool usingNetwork() const { return m_usingNetwork; }
    bool inFullscreen() const { return m_inFullScreen; }

private:
    MediaPlayer* m_mediaPlayer;
    RetainPtr<NSObject> m_mediaPlayerHelper;    // this is the MediaPlayerProxy.
    RetainPtr<WebCoreMediaPlayerNotificationHelper> m_objcHelper;

    MediaPlayer::NetworkState m_networkState;
    MediaPlayer::ReadyState m_readyState;

    int m_delayCallbacks;
    int m_changingVolume;
    float m_requestedRate;
    bool m_visible : 1;
    bool m_usingNetwork : 1;
    bool m_inFullScreen : 1;
    bool m_shouldPrepareToPlay : 1;
    bool m_preparingToPlay : 1;
    bool m_pauseRequested : 1;
};

}   // namespace WebCore

#endif
#endif // MediaPlayerPrivateIPhone

