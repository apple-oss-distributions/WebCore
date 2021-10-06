/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "MediaRecorder.h"

#if ENABLE(MEDIA_STREAM)

#include "Blob.h"
#include "BlobEvent.h"
#include "Document.h"
#include "EventNames.h"
#include "MediaRecorderErrorEvent.h"
#include "MediaRecorderPrivate.h"
#include "MediaRecorderProvider.h"
#include "Page.h"
#include "SharedBuffer.h"
#include "WindowEventLoop.h"
#include <wtf/IsoMallocInlines.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(MediaRecorder);

MediaRecorder::CreatorFunction MediaRecorder::m_customCreator = nullptr;

bool MediaRecorder::isTypeSupported(Document& document, const String& value)
{
#if PLATFORM(COCOA)
    auto* page = document.page();
    return page && page->mediaRecorderProvider().isSupported(value);
#else
    UNUSED_PARAM(document);
    return false;
#endif

}

ExceptionOr<Ref<MediaRecorder>> MediaRecorder::create(Document& document, Ref<MediaStream>&& stream, Options&& options)
{
    if (!isTypeSupported(document, options.mimeType))
        return Exception { NotSupportedError, "mimeType is not supported" };

    auto result = MediaRecorder::createMediaRecorderPrivate(document, stream->privateStream(), options);
    if (result.hasException())
        return result.releaseException();

    auto recorder = adoptRef(*new MediaRecorder(document, WTFMove(stream), WTFMove(options)));
    recorder->suspendIfNeeded();
    return recorder;
}

void MediaRecorder::setCustomPrivateRecorderCreator(CreatorFunction creator)
{
    m_customCreator = creator;
}

ExceptionOr<std::unique_ptr<MediaRecorderPrivate>> MediaRecorder::createMediaRecorderPrivate(Document& document, MediaStreamPrivate& stream, const Options& options)
{
#if !PLATFORM(COCOA)
    UNUSED_PARAM(stream);
#endif

    if (m_customCreator)
        return m_customCreator(stream, options);

    auto* page = document.page();
    if (!page)
        return Exception { InvalidStateError };

#if PLATFORM(COCOA)
    auto result = page->mediaRecorderProvider().createMediaRecorderPrivate(stream, options);
#else
    std::unique_ptr<MediaRecorderPrivate> result;
#endif
    if (!result)
        return Exception { NotSupportedError, "The MediaRecorder is unsupported on this platform"_s };
    return result;
}

MediaRecorder::MediaRecorder(Document& document, Ref<MediaStream>&& stream, Options&& options)
    : ActiveDOMObject(document)
    , m_options(WTFMove(options))
    , m_stream(WTFMove(stream))
    , m_timeSliceTimer([this] { requestData(); })
{
    m_tracks = WTF::map(m_stream->getTracks(), [] (auto&& track) -> Ref<MediaStreamTrackPrivate> {
        return track->privateTrack();
    });
    m_stream->privateStream().addObserver(*this);
}

MediaRecorder::~MediaRecorder()
{
    m_stream->privateStream().removeObserver(*this);
    stopRecordingInternal();
}

Document* MediaRecorder::document() const
{
    return downcast<Document>(scriptExecutionContext());
}

void MediaRecorder::stop()
{
    m_isActive = false;
    stopRecordingInternal();
}

void MediaRecorder::suspend(ReasonForSuspension reason)
{
    if (reason != ReasonForSuspension::BackForwardCache)
        return;

    if (!m_isActive || state() == RecordingState::Inactive)
        return;

    stopRecordingInternal();

    queueTaskToDispatchEvent(*this, TaskSource::Networking, MediaRecorderErrorEvent::create(eventNames().errorEvent, Exception { UnknownError, "MediaStream recording was interrupted"_s }));
}

const char* MediaRecorder::activeDOMObjectName() const
{
    return "MediaRecorder";
}

ExceptionOr<void> MediaRecorder::startRecording(Optional<unsigned> timeSlice)
{
    if (!m_isActive)
        return Exception { InvalidStateError, "The MediaRecorder is not active"_s };

    if (state() != RecordingState::Inactive)
        return Exception { InvalidStateError, "The MediaRecorder's state must be inactive in order to start recording"_s };

    ASSERT(!m_private);
    auto result = createMediaRecorderPrivate(*document(), m_stream->privateStream(), m_options);

    ASSERT(!result.hasException());
    if (result.hasException())
        return result.releaseException();

    m_private = result.releaseReturnValue();
    m_private->startRecording([this, pendingActivity = makePendingActivity(*this)](auto&& mimeTypeOrException) mutable {
        if (!m_isActive)
            return;

        if (mimeTypeOrException.hasException()) {
            stopRecordingInternal();
            queueTaskKeepingObjectAlive(*this, TaskSource::Networking, [this, exception = mimeTypeOrException.releaseException()]() mutable {
                if (!m_isActive)
                    return;
                dispatchError(WTFMove(exception));
            });
            return;
        }

        queueTaskKeepingObjectAlive(*this, TaskSource::Networking, [this, mimeType = mimeTypeOrException.releaseReturnValue()]() mutable {
            if (!m_isActive)
                return;
            m_options.mimeType = WTFMove(mimeType);
            dispatchEvent(Event::create(eventNames().startEvent, Event::CanBubble::No, Event::IsCancelable::No));
        });
    });

    for (auto& track : m_tracks)
        track->addObserver(*this);

    m_state = RecordingState::Recording;
    m_timeSlice = timeSlice;
    if (m_timeSlice)
        m_timeSliceTimer.startOneShot(Seconds::fromMilliseconds(*m_timeSlice));
    return { };
}

ExceptionOr<void> MediaRecorder::stopRecording()
{
    if (state() == RecordingState::Inactive)
        return Exception { InvalidStateError, "The MediaRecorder's state cannot be inactive"_s };

    stopRecordingInternal();
    fetchData([this](auto&& buffer, auto& mimeType) {
        if (!m_isActive)
            return;
        dispatchEvent(BlobEvent::create(eventNames().dataavailableEvent, Event::CanBubble::No, Event::IsCancelable::No, buffer ? Blob::create(scriptExecutionContext(), buffer.releaseNonNull(), mimeType) : Blob::create(scriptExecutionContext())));
        if (!m_isActive)
            return;
        dispatchEvent(Event::create(eventNames().stopEvent, Event::CanBubble::No, Event::IsCancelable::No));
    }, TakePrivateRecorder::Yes);
    return { };
}

ExceptionOr<void> MediaRecorder::requestData()
{
    if (state() == RecordingState::Inactive)
        return Exception { InvalidStateError, "The MediaRecorder's state cannot be inactive"_s };

    if (m_timeSliceTimer.isActive())
        m_timeSliceTimer.stop();

    fetchData([this](auto&& buffer, auto& mimeType) {
        if (!m_isActive)
            return;

        dispatchEvent(BlobEvent::create(eventNames().dataavailableEvent, Event::CanBubble::No, Event::IsCancelable::No, buffer ? Blob::create(scriptExecutionContext(), buffer.releaseNonNull(), mimeType) : Blob::create(scriptExecutionContext())));

        if (m_isActive && m_timeSlice)
            m_timeSliceTimer.startOneShot(Seconds::fromMilliseconds(*m_timeSlice));
    }, TakePrivateRecorder::No);
    return { };
}

void MediaRecorder::fetchData(FetchDataCallback&& callback, TakePrivateRecorder takeRecorder)
{
    auto& privateRecorder = *m_private;

    std::unique_ptr<MediaRecorderPrivate> takenPrivateRecorder;
    if (takeRecorder == TakePrivateRecorder::Yes)
        takenPrivateRecorder = WTFMove(m_private);

    auto fetchDataCallback = [this, privateRecorder = WTFMove(takenPrivateRecorder), callback = WTFMove(callback)](auto&& buffer, auto& mimeType) mutable {
        queueTaskKeepingObjectAlive(*this, TaskSource::Networking, [buffer = WTFMove(buffer), mimeType, callback = WTFMove(callback)]() mutable {
            callback(WTFMove(buffer), mimeType);
        });
    };

    if (m_isFetchingData) {
        m_pendingFetchDataTasks.append(WTFMove(fetchDataCallback));
        return;
    }

    m_isFetchingData = true;
    privateRecorder.fetchData([this, pendingActivity = makePendingActivity(*this), callback = WTFMove(fetchDataCallback)](auto&& buffer, auto& mimeType) mutable {
        m_isFetchingData = false;
        callback(WTFMove(buffer), mimeType);
        for (auto& task : std::exchange(m_pendingFetchDataTasks, { }))
            task({ }, mimeType);
    });
}

void MediaRecorder::stopRecordingInternal()
{
    if (state() != RecordingState::Recording)
        return;

    for (auto& track : m_tracks)
        track->removeObserver(*this);

    m_state = RecordingState::Inactive;
    m_private->stopRecording();
}

void MediaRecorder::handleTrackChange()
{
    queueTaskKeepingObjectAlive(*this, TaskSource::Networking, [this] {
        if (!m_isActive || state() == RecordingState::Inactive)
            return;
        stopRecordingInternal();
        dispatchError(Exception { UnknownError, "Track cannot be added to or removed from the MediaStream while recording is happening"_s });
    });
}

void MediaRecorder::dispatchError(Exception&& exception)
{
    if (!m_isActive)
        return;
    dispatchEvent(MediaRecorderErrorEvent::create(eventNames().errorEvent, WTFMove(exception)));
}

void MediaRecorder::trackEnded(MediaStreamTrackPrivate&)
{
    auto position = m_tracks.findMatching([](auto& track) {
        return !track->ended();
    });
    if (position != notFound)
        return;

    stopRecording();
}

void MediaRecorder::trackMutedChanged(MediaStreamTrackPrivate& track)
{
    if (m_private)
        m_private->trackMutedChanged(track);
}

void MediaRecorder::trackEnabledChanged(MediaStreamTrackPrivate& track)
{
    if (m_private)
        m_private->trackEnabledChanged(track);
}

bool MediaRecorder::virtualHasPendingActivity() const
{
    return m_state != RecordingState::Inactive;
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
