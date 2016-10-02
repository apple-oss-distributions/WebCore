/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#pragma once

#include "ActiveDOMObject.h"
#include "AsyncAudioDecoder.h"
#include "AudioBus.h"
#include "AudioDestinationNode.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "JSDOMPromise.h"
#include "MediaCanStartListener.h"
#include "MediaProducer.h"
#include "PlatformMediaSession.h"
#include <atomic>
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>
#include <wtf/RefPtr.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicStringHash.h>

namespace WebCore {

class AnalyserNode;
class AudioBuffer;
class AudioBufferCallback;
class AudioBufferSourceNode;
class AudioListener;
class AudioSummingJunction;
class BiquadFilterNode;
class ChannelMergerNode;
class ChannelSplitterNode;
class ConvolverNode;
class DelayNode;
class Document;
class DynamicsCompressorNode;
class GainNode;
class GenericEventQueue;
class HTMLMediaElement;
class MediaElementAudioSourceNode;
class MediaStreamAudioDestinationNode;
class MediaStreamAudioSourceNode;
class OscillatorNode;
class PannerNode;
class PeriodicWave;
class ScriptProcessorNode;
class WaveShaperNode;

// AudioContext is the cornerstone of the web audio API and all AudioNodes are created from it.
// For thread safety between the audio thread and the main thread, it has a rendering graph locking mechanism. 

class AudioContext : public ActiveDOMObject, public ThreadSafeRefCounted<AudioContext>, public EventTargetWithInlineData, public MediaCanStartListener, public MediaProducer, private PlatformMediaSessionClient {
public:
    // Create an AudioContext for rendering to the audio hardware.
    static RefPtr<AudioContext> create(Document&, ExceptionCode&);

    virtual ~AudioContext();

    bool isInitialized() const;
    
    bool isOfflineContext() { return m_isOfflineContext; }

    Document* document() const; // ASSERTs if document no longer exists.

    const Document* hostingDocument() const override;

    AudioDestinationNode* destination() { return m_destinationNode.get(); }
    size_t currentSampleFrame() const { return m_destinationNode->currentSampleFrame(); }
    double currentTime() const { return m_destinationNode->currentTime(); }
    float sampleRate() const { return m_destinationNode->sampleRate(); }
    unsigned long activeSourceCount() const { return static_cast<unsigned long>(m_activeSourceCount); }

    void incrementActiveSourceCount();
    void decrementActiveSourceCount();
    
    RefPtr<AudioBuffer> createBuffer(unsigned numberOfChannels, size_t numberOfFrames, float sampleRate, ExceptionCode&);
    RefPtr<AudioBuffer> createBuffer(ArrayBuffer&, bool mixToMono, ExceptionCode&);

    // Asynchronous audio file data decoding.
    void decodeAudioData(Ref<ArrayBuffer>&&, RefPtr<AudioBufferCallback>&&, RefPtr<AudioBufferCallback>&&);

    AudioListener* listener() { return m_listener.get(); }

    using ActiveDOMObject::suspend;
    using ActiveDOMObject::resume;

    typedef DOMPromise<std::nullptr_t> Promise;

    void suspend(Promise&&);
    void resume(Promise&&);
    void close(Promise&&);

    enum class State { Suspended, Running, Interrupted, Closed };
    State state() const;

    // The AudioNode create methods are called on the main thread (from JavaScript).
    Ref<AudioBufferSourceNode> createBufferSource();
#if ENABLE(VIDEO)
    RefPtr<MediaElementAudioSourceNode> createMediaElementSource(HTMLMediaElement&, ExceptionCode&);
#endif
#if ENABLE(MEDIA_STREAM)
    RefPtr<MediaStreamAudioSourceNode> createMediaStreamSource(MediaStream&, ExceptionCode&);
    Ref<MediaStreamAudioDestinationNode> createMediaStreamDestination();
#endif
    Ref<GainNode> createGain();
    Ref<BiquadFilterNode> createBiquadFilter();
    Ref<WaveShaperNode> createWaveShaper();
    RefPtr<DelayNode> createDelay(double maxDelayTime, ExceptionCode&);
    Ref<PannerNode> createPanner();
    Ref<ConvolverNode> createConvolver();
    Ref<DynamicsCompressorNode> createDynamicsCompressor();
    Ref<AnalyserNode> createAnalyser();
    RefPtr<ScriptProcessorNode> createScriptProcessor(size_t bufferSize, size_t numberOfInputChannels, size_t numberOfOutputChannels, ExceptionCode&);
    RefPtr<ChannelSplitterNode> createChannelSplitter(size_t numberOfOutputs, ExceptionCode&);
    RefPtr<ChannelMergerNode> createChannelMerger(size_t numberOfInputs, ExceptionCode&);
    Ref<OscillatorNode> createOscillator();
    RefPtr<PeriodicWave> createPeriodicWave(Float32Array& real, Float32Array& imaginary, ExceptionCode&);

    // When a source node has no more processing to do (has finished playing), then it tells the context to dereference it.
    void notifyNodeFinishedProcessing(AudioNode*);

    // Called at the start of each render quantum.
    void handlePreRenderTasks();

    // Called at the end of each render quantum.
    void handlePostRenderTasks();

    // Called periodically at the end of each render quantum to dereference finished source nodes.
    void derefFinishedSourceNodes();

    // We schedule deletion of all marked nodes at the end of each realtime render quantum.
    void markForDeletion(AudioNode*);
    void deleteMarkedNodes();

    // AudioContext can pull node(s) at the end of each render quantum even when they are not connected to any downstream nodes.
    // These two methods are called by the nodes who want to add/remove themselves into/from the automatic pull lists.
    void addAutomaticPullNode(AudioNode*);
    void removeAutomaticPullNode(AudioNode*);

    // Called right before handlePostRenderTasks() to handle nodes which need to be pulled even when they are not connected to anything.
    void processAutomaticPullNodes(size_t framesToProcess);

    // Keeps track of the number of connections made.
    void incrementConnectionCount()
    {
        ASSERT(isMainThread());
        m_connectionCount++;
    }

    unsigned connectionCount() const { return m_connectionCount; }

    //
    // Thread Safety and Graph Locking:
    //
    
    void setAudioThread(ThreadIdentifier thread) { m_audioThread = thread; } // FIXME: check either not initialized or the same
    ThreadIdentifier audioThread() const { return m_audioThread; }
    bool isAudioThread() const;

    // Returns true only after the audio thread has been started and then shutdown.
    bool isAudioThreadFinished() { return m_isAudioThreadFinished; }
    
    // mustReleaseLock is set to true if we acquired the lock in this method call and caller must unlock(), false if it was previously acquired.
    void lock(bool& mustReleaseLock);

    // Returns true if we own the lock.
    // mustReleaseLock is set to true if we acquired the lock in this method call and caller must unlock(), false if it was previously acquired.
    bool tryLock(bool& mustReleaseLock);

    void unlock();

    // Returns true if this thread owns the context's lock.
    bool isGraphOwner() const;

    // Returns the maximum number of channels we can support.
    static unsigned maxNumberOfChannels() { return MaxNumberOfChannels; }

    class AutoLocker {
    public:
        explicit AutoLocker(AudioContext& context)
            : m_context(context)
        {
            m_context.lock(m_mustReleaseLock);
        }
        
        ~AutoLocker()
        {
            if (m_mustReleaseLock)
                m_context.unlock();
        }

    private:
        AudioContext& m_context;
        bool m_mustReleaseLock;
    };
    
    // In AudioNode::deref() a tryLock() is used for calling finishDeref(), but if it fails keep track here.
    void addDeferredFinishDeref(AudioNode*);

    // In the audio thread at the start of each render cycle, we'll call handleDeferredFinishDerefs().
    void handleDeferredFinishDerefs();

    // Only accessed when the graph lock is held.
    void markSummingJunctionDirty(AudioSummingJunction*);
    void markAudioNodeOutputDirty(AudioNodeOutput*);

    // Must be called on main thread.
    void removeMarkedSummingJunction(AudioSummingJunction*);

    // EventTarget
    EventTargetInterface eventTargetInterface() const final { return AudioContextEventTargetInterfaceType; }
    ScriptExecutionContext* scriptExecutionContext() const final;

    // Reconcile ref/deref which are defined both in ThreadSafeRefCounted and EventTarget.
    using ThreadSafeRefCounted::ref;
    using ThreadSafeRefCounted::deref;

    void startRendering();
    void fireCompletionEvent();
    
    static unsigned s_hardwareContextCount;

    // Restrictions to change default behaviors.
    enum BehaviorRestrictionFlags {
        NoRestrictions = 0,
        RequireUserGestureForAudioStartRestriction = 1 << 0,
        RequirePageConsentForAudioStartRestriction = 1 << 1,
    };
    typedef unsigned BehaviorRestrictions;

    BehaviorRestrictions behaviorRestrictions() const { return m_restrictions; }
    void addBehaviorRestriction(BehaviorRestrictions restriction) { m_restrictions |= restriction; }
    void removeBehaviorRestriction(BehaviorRestrictions restriction) { m_restrictions &= ~restriction; }

    void isPlayingAudioDidChange();

    void nodeWillBeginPlayback();

protected:
    explicit AudioContext(Document&);
    AudioContext(Document&, unsigned numberOfChannels, size_t numberOfFrames, float sampleRate);
    
    static bool isSampleRateRangeGood(float sampleRate);
    
private:
    void constructCommon();

    void lazyInitialize();
    void uninitialize();

    bool willBeginPlayback();
    bool willPausePlayback();

    bool userGestureRequiredForAudioStart() const { return m_restrictions & RequireUserGestureForAudioStartRestriction; }
    bool pageConsentRequiredForAudioStart() const { return m_restrictions & RequirePageConsentForAudioStartRestriction; }

    void setState(State);

    void clear();

    void scheduleNodeDeletion();

    void mediaCanStart() override;

    // MediaProducer
    MediaProducer::MediaStateFlags mediaState() const override;
    void pageMutedStateDidChange() override;

    // The context itself keeps a reference to all source nodes.  The source nodes, then reference all nodes they're connected to.
    // In turn, these nodes reference all nodes they're connected to.  All nodes are ultimately connected to the AudioDestinationNode.
    // When the context dereferences a source node, it will be deactivated from the rendering graph along with all other nodes it is
    // uniquely connected to.  See the AudioNode::ref() and AudioNode::deref() methods for more details.
    void refNode(AudioNode&);
    void derefNode(AudioNode&);

    // ActiveDOMObject API.
    void stop() override;
    bool canSuspendForDocumentSuspension() const override;
    const char* activeDOMObjectName() const override;

    // When the context goes away, there might still be some sources which haven't finished playing.
    // Make sure to dereference them here.
    void derefUnfinishedSourceNodes();

    // PlatformMediaSessionClient
    PlatformMediaSession::MediaType mediaType() const override { return PlatformMediaSession::WebAudio; }
    PlatformMediaSession::MediaType presentationType() const override { return PlatformMediaSession::WebAudio; }
    PlatformMediaSession::CharacteristicsFlags characteristics() const override { return m_state == State::Running ? PlatformMediaSession::HasAudio : PlatformMediaSession::HasNothing; }
    void mayResumePlayback(bool shouldResume) override;
    void suspendPlayback() override;
    bool canReceiveRemoteControlCommands() const override { return false; }
    void didReceiveRemoteControlCommand(PlatformMediaSession::RemoteControlCommandType, const PlatformMediaSession::RemoteCommandArgument*) override { }
    bool supportsSeeking() const override { return false; }
    bool shouldOverrideBackgroundPlaybackRestriction(PlatformMediaSession::InterruptionType) const override { return false; }
    String sourceApplicationIdentifier() const override;

    // EventTarget
    void refEventTarget() override { ref(); }
    void derefEventTarget() override { deref(); }

    void handleDirtyAudioSummingJunctions();
    void handleDirtyAudioNodeOutputs();

    void addReaction(State, Promise&&);
    void updateAutomaticPullNodes();

    // Only accessed in the audio thread.
    Vector<AudioNode*> m_finishedNodes;

    // We don't use RefPtr<AudioNode> here because AudioNode has a more complex ref() / deref() implementation
    // with an optional argument for refType.  We need to use the special refType: RefTypeConnection
    // Either accessed when the graph lock is held, or on the main thread when the audio thread has finished.
    Vector<AudioNode*> m_referencedNodes;

    // Accumulate nodes which need to be deleted here.
    // This is copied to m_nodesToDelete at the end of a render cycle in handlePostRenderTasks(), where we're assured of a stable graph
    // state which will have no references to any of the nodes in m_nodesToDelete once the context lock is released
    // (when handlePostRenderTasks() has completed).
    Vector<AudioNode*> m_nodesMarkedForDeletion;

    // They will be scheduled for deletion (on the main thread) at the end of a render cycle (in realtime thread).
    Vector<AudioNode*> m_nodesToDelete;

    bool m_isDeletionScheduled { false };
    bool m_isStopScheduled { false };
    bool m_isInitialized { false };
    bool m_isAudioThreadFinished { false };
    bool m_automaticPullNodesNeedUpdating { false };
    bool m_isOfflineContext { false };

    // Only accessed when the graph lock is held.
    HashSet<AudioSummingJunction*> m_dirtySummingJunctions;
    HashSet<AudioNodeOutput*> m_dirtyAudioNodeOutputs;

    // For the sake of thread safety, we maintain a seperate Vector of automatic pull nodes for rendering in m_renderingAutomaticPullNodes.
    // It will be copied from m_automaticPullNodes by updateAutomaticPullNodes() at the very start or end of the rendering quantum.
    HashSet<AudioNode*> m_automaticPullNodes;
    Vector<AudioNode*> m_renderingAutomaticPullNodes;
    // Only accessed in the audio thread.
    Vector<AudioNode*> m_deferredFinishDerefList;
    Vector<Vector<Promise>> m_stateReactions;

    std::unique_ptr<PlatformMediaSession> m_mediaSession;
    std::unique_ptr<GenericEventQueue> m_eventQueue;

    RefPtr<AudioBuffer> m_renderTarget;
    RefPtr<AudioDestinationNode> m_destinationNode;
    RefPtr<AudioListener> m_listener;

    unsigned m_connectionCount { 0 };

    // Graph locking.
    Lock m_contextGraphMutex;
    volatile ThreadIdentifier m_audioThread { 0 };
    volatile ThreadIdentifier m_graphOwnerThread; // if the lock is held then this is the thread which owns it, otherwise == UndefinedThreadIdentifier

    AsyncAudioDecoder m_audioDecoder;

    // This is considering 32 is large enough for multiple channels audio. 
    // It is somewhat arbitrary and could be increased if necessary.
    enum { MaxNumberOfChannels = 32 };

    // Number of AudioBufferSourceNodes that are active (playing).
    std::atomic<int> m_activeSourceCount { 0 };

    BehaviorRestrictions m_restrictions { NoRestrictions };

    State m_state { State::Suspended };
};

// FIXME: Find out why these ==/!= functions are needed and remove them if possible.

inline bool operator==(const AudioContext& lhs, const AudioContext& rhs)
{
    return &lhs == &rhs;
}

inline bool operator!=(const AudioContext& lhs, const AudioContext& rhs)
{
    return &lhs != &rhs;
}

inline AudioContext::State AudioContext::state() const
{
    return m_state;
}

} // WebCore
