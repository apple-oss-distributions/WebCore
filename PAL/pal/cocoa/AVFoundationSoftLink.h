/*
 * Copyright (C) 2019 Apple Inc. All rights reserved.
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

#if USE(AVFOUNDATION)

#import <AVFoundation/AVFoundation.h>
#import <wtf/SoftLinking.h>

SOFT_LINK_FRAMEWORK_FOR_HEADER(PAL, AVFoundation)

// Note: We don't define accessor macros for classes (e.g.
//    #define AVAssetCache PAL::getAVAssetCacheClass()
// because they make it difficult to use the class name in source code.

SOFT_LINK_CLASS_FOR_HEADER(PAL, AVAssetCache)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVAssetImageGenerator)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVAssetReader)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVAssetReaderSampleReferenceOutput)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVAssetResourceLoadingRequest)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVAssetWriter)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVAssetWriterInput)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVContentKeyReportGroup)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVContentKeyResponse)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVContentKeySession)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVMediaSelectionGroup)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVMediaSelectionOption)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVMetadataItem)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVMutableAudioMix)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVMutableAudioMixInputParameters)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVOutputContext)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVPlayer)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVPlayerItem)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVPlayerItemLegibleOutput)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVPlayerItemMetadataCollector)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVPlayerItemMetadataOutput)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVPlayerItemVideoOutput)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVPlayerLayer)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVSampleBufferAudioRenderer)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVSampleBufferDisplayLayer)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVSampleBufferRenderSynchronizer)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVStreamDataParser)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVURLAsset)

#if HAVE(AVSTREAMSESSION) && ENABLE(LEGACY_ENCRYPTED_MEDIA)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVStreamSession)
#endif

#if PLATFORM(IOS_FAMILY)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVAudioSession)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVPersistableContentKeyRequest)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVSpeechSynthesisVoice)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVSpeechSynthesizer)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVSpeechUtterance)
#endif

#if !PLATFORM(WATCHOS)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVRouteDetector)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVVideoPerformanceMetrics)
#endif

#if !PLATFORM(WATCHOS) && !PLATFORM(APPLETV)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVCaptureConnection)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVCaptureDevice)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVCaptureDeviceFormat)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVCaptureDeviceInput)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVCaptureOutput)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVCaptureSession)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVCaptureVideoDataOutput)
SOFT_LINK_CLASS_FOR_HEADER(PAL, AVFrameRateRange)
#endif

SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioTimePitchAlgorithmSpectral, NSString *)
#define AVAudioTimePitchAlgorithmSpectral PAL::get_AVFoundation_AVAudioTimePitchAlgorithmSpectral()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioTimePitchAlgorithmVarispeed, NSString *)
#define AVAudioTimePitchAlgorithmVarispeed PAL::get_AVFoundation_AVAudioTimePitchAlgorithmVarispeed()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaCharacteristicVisual, NSString *)
#define AVMediaCharacteristicVisual PAL::get_AVFoundation_AVMediaCharacteristicVisual()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaCharacteristicAudible, NSString *)
#define AVMediaCharacteristicAudible PAL::get_AVFoundation_AVMediaCharacteristicAudible()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaTypeClosedCaption, NSString *)
#define AVMediaTypeClosedCaption PAL::get_AVFoundation_AVMediaTypeClosedCaption()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaTypeVideo, NSString *)
#define AVMediaTypeVideo PAL::get_AVFoundation_AVMediaTypeVideo()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaTypeMuxed, NSString *)
#define AVMediaTypeMuxed PAL::get_AVFoundation_AVMediaTypeMuxed()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaTypeAudio, NSString *)
#define AVMediaTypeAudio PAL::get_AVFoundation_AVMediaTypeAudio()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaTypeMetadata, NSString *)
#define AVMediaTypeMetadata PAL::get_AVFoundation_AVMediaTypeMetadata()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVURLAssetInheritURIQueryComponentFromReferencingURIKey, NSString *)
#define AVURLAssetInheritURIQueryComponentFromReferencingURIKey PAL::get_AVFoundation_AVURLAssetInheritURIQueryComponentFromReferencingURIKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAssetImageGeneratorApertureModeCleanAperture, NSString *)
#define AVAssetImageGeneratorApertureModeCleanAperture PAL::get_AVFoundation_AVAssetImageGeneratorApertureModeCleanAperture()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVURLAssetReferenceRestrictionsKey, NSString *)
#define AVURLAssetReferenceRestrictionsKey PAL::get_AVFoundation_AVURLAssetReferenceRestrictionsKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVLayerVideoGravityResizeAspect, NSString *)
#define AVLayerVideoGravityResizeAspect PAL::get_AVFoundation_AVLayerVideoGravityResizeAspect()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVLayerVideoGravityResizeAspectFill, NSString *)
#define AVLayerVideoGravityResizeAspectFill PAL::get_AVFoundation_AVLayerVideoGravityResizeAspectFill()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVLayerVideoGravityResize, NSString *)
#define AVLayerVideoGravityResize PAL::get_AVFoundation_AVLayerVideoGravityResize()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVStreamingKeyDeliveryContentKeyType, NSString *)
#define AVStreamingKeyDeliveryContentKeyType PAL::get_AVFoundation_AVStreamingKeyDeliveryContentKeyType()

SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVCaptureDeviceWasConnectedNotification, NSString *)
#define AVCaptureDeviceWasConnectedNotification PAL::get_AVFoundation_AVCaptureDeviceWasConnectedNotification()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVCaptureDeviceWasDisconnectedNotification, NSString *)
#define AVCaptureDeviceWasDisconnectedNotification PAL::get_AVFoundation_AVCaptureDeviceWasDisconnectedNotification()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVPlayerItemDidPlayToEndTimeNotification, NSString *)
#define AVPlayerItemDidPlayToEndTimeNotification PAL::get_AVFoundation_AVPlayerItemDidPlayToEndTimeNotification()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVStreamSessionContentProtectionSessionIdentifierChangedNotification, NSString *)
#define AVStreamSessionContentProtectionSessionIdentifierChangedNotification PAL::get_AVFoundation_AVStreamSessionContentProtectionSessionIdentifierChangedNotification()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVSampleBufferDisplayLayerFailedToDecodeNotification, NSString*)
#define AVSampleBufferDisplayLayerFailedToDecodeNotification PAL::get_AVFoundation_AVSampleBufferDisplayLayerFailedToDecodeNotification()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVSampleBufferDisplayLayerFailedToDecodeNotificationErrorKey, NSString*)
#define AVSampleBufferDisplayLayerFailedToDecodeNotificationErrorKey PAL::get_AVFoundation_AVSampleBufferDisplayLayerFailedToDecodeNotificationErrorKey()

SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaCharacteristicContainsOnlyForcedSubtitles, NSString *)
#define AVMediaCharacteristicContainsOnlyForcedSubtitles PAL::get_AVFoundation_AVMediaCharacteristicContainsOnlyForcedSubtitles()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaCharacteristicLegible, NSString *)
#define AVMediaCharacteristicLegible PAL::get_AVFoundation_AVMediaCharacteristicLegible()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly, NSString *)
#define AVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly PAL::get_AVFoundation_AVPlayerItemLegibleOutputTextStylingResolutionSourceAndRulesOnly()

SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMetadataCommonKeyTitle, NSString *)
#define AVMetadataCommonKeyTitle PAL::get_AVFoundation_AVMetadataCommonKeyTitle()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMetadataKeySpaceCommon, NSString *)
#define AVMetadataKeySpaceCommon PAL::get_AVFoundation_AVMetadataKeySpaceCommon()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaTypeSubtitle, NSString *)
#define AVMediaTypeSubtitle PAL::get_AVFoundation_AVMediaTypeSubtitle()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaCharacteristicIsMainProgramContent, NSString *)
#define AVMediaCharacteristicIsMainProgramContent PAL::get_AVFoundation_AVMediaCharacteristicIsMainProgramContent()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaCharacteristicEasyToRead, NSString *)
#define AVMediaCharacteristicEasyToRead PAL::get_AVFoundation_AVMediaCharacteristicEasyToRead()

SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVURLAssetOutOfBandMIMETypeKey, NSString *)
#define AVURLAssetOutOfBandMIMETypeKey PAL::get_AVFoundation_AVURLAssetOutOfBandMIMETypeKey()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVURLAssetUseClientURLLoadingExclusively, NSString *)
#define AVURLAssetUseClientURLLoadingExclusively PAL::get_AVFoundation_AVURLAssetUseClientURLLoadingExclusively()

SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVContentKeySystemFairPlayStreaming, NSString*)
#define AVContentKeySystemFairPlayStreaming PAL::get_AVFoundation_AVContentKeySystemFairPlayStreaming()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVContentKeyRequestProtocolVersionsKey, NSString *)
#define AVContentKeyRequestProtocolVersionsKey PAL::get_AVFoundation_AVContentKeyRequestProtocolVersionsKey()

SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVVideoCodecTypeHEVCWithAlpha, NSString *)
#define AVVideoCodecTypeHEVCWithAlpha PAL::get_AVFoundation_AVVideoCodecTypeHEVCWithAlpha()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVFileTypeMPEG4, NSString *)
#define AVFileTypeMPEG4 PAL::get_AVFoundation_AVFileTypeMPEG4()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVVideoCodecKey, NSString *)
#define AVVideoCodecKey PAL::get_AVFoundation_AVVideoCodecKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVVideoCodecH264, NSString *)
#define AVVideoCodecH264 PAL::get_AVFoundation_AVVideoCodecH264()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVVideoWidthKey, NSString *)
#define AVVideoWidthKey PAL::get_AVFoundation_AVVideoWidthKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVVideoHeightKey, NSString *)
#define AVVideoHeightKey PAL::get_AVFoundation_AVVideoHeightKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVVideoExpectedSourceFrameRateKey, NSString *)
#define AVVideoExpectedSourceFrameRateKey PAL::get_AVFoundation_AVVideoExpectedSourceFrameRateKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVVideoProfileLevelKey, NSString *)
#define AVVideoProfileLevelKey PAL::get_AVFoundation_AVVideoProfileLevelKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVVideoAverageBitRateKey, NSString *)
#define AVVideoAverageBitRateKey PAL::get_AVFoundation_AVVideoAverageBitRateKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVVideoMaxKeyFrameIntervalKey, NSString *)
#define AVVideoMaxKeyFrameIntervalKey PAL::get_AVFoundation_AVVideoMaxKeyFrameIntervalKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVVideoProfileLevelH264MainAutoLevel, NSString *)
#define AVVideoProfileLevelH264MainAutoLevel PAL::get_AVFoundation_AVVideoProfileLevelH264MainAutoLevel()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVVideoCompressionPropertiesKey, NSString *)
#define AVVideoCompressionPropertiesKey PAL::get_AVFoundation_AVVideoCompressionPropertiesKey()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVEncoderBitRateKey, NSString *)
#define AVEncoderBitRateKey PAL::get_AVFoundation_AVEncoderBitRateKey()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVFormatIDKey, NSString *)
#define AVFormatIDKey PAL::get_AVFoundation_AVFormatIDKey()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVNumberOfChannelsKey, NSString *)
#define AVNumberOfChannelsKey PAL::get_AVFoundation_AVNumberOfChannelsKey()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVSampleRateKey, NSString *)
#define AVSampleRateKey PAL::get_AVFoundation_AVSampleRateKey()

SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVURLAssetCacheKey, NSString *)
#define AVURLAssetCacheKey PAL::get_AVFoundation_AVURLAssetCacheKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVURLAssetOutOfBandAlternateTracksKey, NSString *)
#define AVURLAssetOutOfBandAlternateTracksKey PAL::get_AVFoundation_AVURLAssetOutOfBandAlternateTracksKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVURLAssetUsesNoPersistentCacheKey, NSString *)
#define AVURLAssetUsesNoPersistentCacheKey PAL::get_AVFoundation_AVURLAssetUsesNoPersistentCacheKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVOutOfBandAlternateTrackDisplayNameKey, NSString *)
#define AVOutOfBandAlternateTrackDisplayNameKey PAL::get_AVFoundation_AVOutOfBandAlternateTrackDisplayNameKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVOutOfBandAlternateTrackExtendedLanguageTagKey, NSString *)
#define AVOutOfBandAlternateTrackExtendedLanguageTagKey PAL::get_AVFoundation_AVOutOfBandAlternateTrackExtendedLanguageTagKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVOutOfBandAlternateTrackIsDefaultKey, NSString *)
#define AVOutOfBandAlternateTrackIsDefaultKey PAL::get_AVFoundation_AVOutOfBandAlternateTrackIsDefaultKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVOutOfBandAlternateTrackMediaCharactersticsKey, NSString *)
#define AVOutOfBandAlternateTrackMediaCharactersticsKey PAL::get_AVFoundation_AVOutOfBandAlternateTrackMediaCharactersticsKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVOutOfBandAlternateTrackIdentifierKey, NSString *)
#define AVOutOfBandAlternateTrackIdentifierKey PAL::get_AVFoundation_AVOutOfBandAlternateTrackIdentifierKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVOutOfBandAlternateTrackSourceKey, NSString *)
#define AVOutOfBandAlternateTrackSourceKey PAL::get_AVFoundation_AVOutOfBandAlternateTrackSourceKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaCharacteristicDescribesMusicAndSoundForAccessibility, NSString *)
#define AVMediaCharacteristicDescribesMusicAndSoundForAccessibility PAL::get_AVFoundation_AVMediaCharacteristicDescribesMusicAndSoundForAccessibility()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaCharacteristicTranscribesSpokenDialogForAccessibility, NSString *)
#define AVMediaCharacteristicTranscribesSpokenDialogForAccessibility PAL::get_AVFoundation_AVMediaCharacteristicTranscribesSpokenDialogForAccessibility()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaCharacteristicIsAuxiliaryContent, NSString *)
#define AVMediaCharacteristicIsAuxiliaryContent PAL::get_AVFoundation_AVMediaCharacteristicIsAuxiliaryContent()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMediaCharacteristicDescribesVideoForAccessibility, NSString *)
#define AVMediaCharacteristicDescribesVideoForAccessibility PAL::get_AVFoundation_AVMediaCharacteristicDescribesVideoForAccessibility()

SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMetadataKeySpaceQuickTimeUserData, NSString *)
#define AVMetadataKeySpaceQuickTimeUserData PAL::get_AVFoundation_AVMetadataKeySpaceQuickTimeUserData()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMetadataKeySpaceQuickTimeMetadata, NSString *)
#define AVMetadataKeySpaceQuickTimeMetadata PAL::get_AVFoundation_AVMetadataKeySpaceQuickTimeMetadata()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMetadataKeySpaceiTunes, NSString *)
#define AVMetadataKeySpaceiTunes PAL::get_AVFoundation_AVMetadataKeySpaceiTunes()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMetadataKeySpaceID3, NSString *)
#define AVMetadataKeySpaceID3 PAL::get_AVFoundation_AVMetadataKeySpaceID3()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMetadataKeySpaceISOUserData, NSString *)
#define AVMetadataKeySpaceISOUserData PAL::get_AVFoundation_AVMetadataKeySpaceISOUserData()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVMetadataKeySpaceHLSDateRange, NSString *)
#define AVMetadataKeySpaceHLSDateRange PAL::get_AVFoundation_AVMetadataKeySpaceHLSDateRange()

#if PLATFORM(MAC)
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVStreamDataParserContentKeyRequestProtocolVersionsKey, NSString *)
#define AVStreamDataParserContentKeyRequestProtocolVersionsKey PAL::get_AVFoundation_AVStreamDataParserContentKeyRequestProtocolVersionsKey()
#endif

#if PLATFORM(IOS_FAMILY)
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVURLAssetBoundNetworkInterfaceName, NSString *)
#define AVURLAssetBoundNetworkInterfaceName PAL::get_AVFoundation_AVURLAssetBoundNetworkInterfaceName()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVURLAssetClientBundleIdentifierKey, NSString *)
#define AVURLAssetClientBundleIdentifierKey PAL::get_AVFoundation_AVURLAssetClientBundleIdentifierKey()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVURLAssetHTTPCookiesKey, NSString *)
#define AVURLAssetHTTPCookiesKey PAL::get_AVFoundation_AVURLAssetHTTPCookiesKey()
SOFT_LINK_CONSTANT_MAY_FAIL_FOR_HEADER(PAL, AVFoundation, AVURLAssetRequiresCustomURLLoadingKey, NSString *)
#define AVURLAssetRequiresCustomURLLoadingKey PAL::get_AVFoundation_AVURLAssetRequiresCustomURLLoadingKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVCaptureSessionRuntimeErrorNotification, NSString *)
#define AVCaptureSessionRuntimeErrorNotification PAL::get_AVFoundation_AVCaptureSessionRuntimeErrorNotification()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVCaptureSessionWasInterruptedNotification, NSString *)
#define AVCaptureSessionWasInterruptedNotification PAL::get_AVFoundation_AVCaptureSessionWasInterruptedNotification()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVCaptureSessionInterruptionEndedNotification, NSString *)
#define AVCaptureSessionInterruptionEndedNotification PAL::get_AVFoundation_AVCaptureSessionInterruptionEndedNotification()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVCaptureSessionInterruptionReasonKey, NSString *)
#define AVCaptureSessionInterruptionReasonKey PAL::get_AVFoundation_AVCaptureSessionInterruptionReasonKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVCaptureSessionErrorKey, NSString *)
#define AVCaptureSessionErrorKey PAL::get_AVFoundation_AVCaptureSessionErrorKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionCategoryAmbient, NSString *)
#define AVAudioSessionCategoryAmbient PAL::get_AVFoundation_AVAudioSessionCategoryAmbient()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionCategorySoloAmbient, NSString *)
#define AVAudioSessionCategorySoloAmbient PAL::get_AVFoundation_AVAudioSessionCategorySoloAmbient()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionCategoryPlayback, NSString *)
#define AVAudioSessionCategoryPlayback PAL::get_AVFoundation_AVAudioSessionCategoryPlayback()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionCategoryRecord, NSString *)
#define AVAudioSessionCategoryRecord PAL::get_AVFoundation_AVAudioSessionCategoryRecord()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionCategoryPlayAndRecord, NSString *)
#define AVAudioSessionCategoryPlayAndRecord PAL::get_AVFoundation_AVAudioSessionCategoryPlayAndRecord()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionCategoryAudioProcessing, NSString *)
#define AVAudioSessionCategoryAudioProcessing PAL::get_AVFoundation_AVAudioSessionCategoryAudioProcessing()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionModeDefault, NSString *)
#define AVAudioSessionModeDefault PAL::get_AVFoundation_AVAudioSessionModeDefault()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionModeVideoChat, NSString *)
#define AVAudioSessionModeVideoChat PAL::get_AVFoundation_AVAudioSessionModeVideoChat()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionInterruptionNotification, NSString *)
#define AVAudioSessionInterruptionNotification PAL::get_AVFoundation_AVAudioSessionInterruptionNotification()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionInterruptionTypeKey, NSString *)
#define AVAudioSessionInterruptionTypeKey PAL::get_AVFoundation_AVAudioSessionInterruptionTypeKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionInterruptionOptionKey, NSString *)
#define AVAudioSessionInterruptionOptionKey PAL::get_AVFoundation_AVAudioSessionInterruptionOptionKey()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVRouteDetectorMultipleRoutesDetectedDidChangeNotification, NSString *)
#define AVRouteDetectorMultipleRoutesDetectedDidChangeNotification PAL::get_AVFoundation_AVRouteDetectorMultipleRoutesDetectedDidChangeNotification()
SOFT_LINK_CONSTANT_FOR_HEADER(PAL, AVFoundation, AVAudioSessionMediaServicesWereResetNotification, NSString *)
#define AVAudioSessionMediaServicesWereResetNotification PAL::get_AVFoundation_AVAudioSessionMediaServicesWereResetNotification()
#endif // PLATFORM(IOS_FAMILY)

#if PLATFORM(IOS_FAMILY) && !PLATFORM(WATCHOS) && !PLATFORM(APPLETV)
SOFT_LINK_FUNCTION_FOR_HEADER(PAL, AVFoundation, AVCaptureSessionSetAuthorizedToUseCameraInMultipleForegroundAppLayout, void, (AVCaptureSession *session), (session))
#define AVCaptureSessionSetAuthorizedToUseCameraInMultipleForegroundAppLayout softLink_AVFoundation_AVCaptureSessionSetAuthorizedToUseCameraInMultipleForegroundAppLayout
#endif // PLATFORM(IOS_FAMILY) && !PLATFORM(WATCHOS) && !PLATFORM(APPLETV)

#endif // USE(AVFOUNDATION)
