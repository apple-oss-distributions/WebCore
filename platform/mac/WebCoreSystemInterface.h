/*
 * Copyright 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef WebCoreSystemInterface_h
#define WebCoreSystemInterface_h

#include <objc/objc.h>

#include "WebCoreSystemInterfaceIOS.h"

typedef const struct __CFString * CFStringRef;
typedef const struct __CFNumber * CFNumberRef;
typedef const struct __CFDictionary * CFDictionaryRef;
typedef struct CGPoint CGPoint;
typedef struct CGSize CGSize;
typedef struct CGRect CGRect;
typedef struct CGAffineTransform CGAffineTransform;
typedef struct CGContext *CGContextRef;
typedef struct CGImage *CGImageRef;
typedef struct CGColor *CGColorRef;
typedef struct CGFont *CGFontRef;
typedef struct CGColorSpace *CGColorSpaceRef;
typedef struct CGPattern *CGPatternRef;
typedef struct CGPath *CGMutablePathRef;
typedef unsigned short CGGlyph;
typedef struct __CFReadStream * CFReadStreamRef;
typedef struct __CFRunLoop * CFRunLoopRef;
typedef struct __CFHTTPMessage *CFHTTPMessageRef;
typedef struct _CFURLResponse *CFURLResponseRef;
typedef const struct _CFURLRequest *CFURLRequestRef;
typedef const struct __CTFont * CTFontRef;
typedef const struct __CTLine * CTLineRef;
typedef const struct __CTTypesetter * CTTypesetterRef;
typedef const struct __AXUIElement *AXUIElementRef;

typedef struct __IOSurface *IOSurfaceRef;


#include <GraphicsServices/GraphicsServices.h>

#if USE(CFNETWORK)
typedef struct OpaqueCFHTTPCookieStorage*  CFHTTPCookieStorageRef;
typedef struct _CFURLProtectionSpace* CFURLProtectionSpaceRef;
typedef struct _CFURLCredential* WKCFURLCredentialRef;
typedef struct _CFURLRequest* CFMutableURLRequestRef;
typedef const struct _CFURLRequest* CFURLRequestRef;
#endif

OBJC_CLASS AVAsset;
OBJC_CLASS CALayer;
OBJC_CLASS NSArray;
OBJC_CLASS NSButtonCell;
OBJC_CLASS NSControl;
OBJC_CLASS NSCursor;
OBJC_CLASS NSData;
OBJC_CLASS NSDate;
OBJC_CLASS NSEvent;
OBJC_CLASS NSFont;
OBJC_CLASS NSHTTPCookie;
OBJC_CLASS NSImage;
OBJC_CLASS NSMenu;
OBJC_CLASS NSMutableURLRequest;
OBJC_CLASS NSString;
OBJC_CLASS NSTextFieldCell;
OBJC_CLASS NSURL;
OBJC_CLASS NSURLConnection;
OBJC_CLASS NSURLRequest;
OBJC_CLASS NSURLResponse;
OBJC_CLASS NSView;
OBJC_CLASS NSWindow;
OBJC_CLASS QTMovie;
OBJC_CLASS QTMovieView;
OBJC_CLASS WebFilterEvaluator;
OBJC_CLASS CALayer;

extern "C" {

// In alphabetical order.

extern void (*wkAdvanceDefaultButtonPulseAnimation)(NSButtonCell *);
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
extern void (*wkCALayerEnumerateRectsBeingDrawnWithBlock)(CALayer *, CGContextRef, void (^block)(CGRect rect));
#endif

extern BOOL (*wkCGContextGetShouldSmoothFonts)(CGContextRef);
typedef enum {
    wkPatternTilingNoDistortion,
    wkPatternTilingConstantSpacingMinimalDistortion,
    wkPatternTilingConstantSpacing
} wkPatternTiling;
extern void (*wkCGContextResetClip)(CGContextRef);
extern CGPatternRef (*wkCGPatternCreateWithImageAndTransform)(CGImageRef, CGAffineTransform, int);
extern CFReadStreamRef (*wkCreateCustomCFReadStream)(void *(*formCreate)(CFReadStreamRef, void *), 
    void (*formFinalize)(CFReadStreamRef, void *), 
    Boolean (*formOpen)(CFReadStreamRef, CFStreamError *, Boolean *, void *), 
    CFIndex (*formRead)(CFReadStreamRef, UInt8 *, CFIndex, CFStreamError *, Boolean *, void *), 
    Boolean (*formCanRead)(CFReadStreamRef, void *), 
    void (*formClose)(CFReadStreamRef, void *), 
    void (*formSchedule)(CFReadStreamRef, CFRunLoopRef, CFStringRef, void *), 
    void (*formUnschedule)(CFReadStreamRef, CFRunLoopRef, CFStringRef, void *),
    void *context);
extern CFStringRef (*wkCopyCFLocalizationPreferredName)(CFStringRef);
extern NSString* (*wkCopyNSURLResponseStatusLine)(NSURLResponse*);
extern id (*wkCreateNSURLConnectionDelegateProxy)(void);
extern BOOL (*wkGetGlyphTransformedAdvances)(GSFontRef font, CGAffineTransform *m, CGGlyph *glyph, CGSize *advance);
extern NSString* (*wkGetMIMETypeForExtension)(NSString*);
extern NSDate *(*wkGetNSURLResponseLastModifiedDate)(NSURLResponse *response);
extern void (*wkSetCookieStoragePrivateBrowsingEnabled)(BOOL);
extern void (*wkSetNSURLConnectionDefersCallbacks)(NSURLConnection *, BOOL);
extern void (*wkSetNSURLRequestShouldContentSniff)(NSMutableURLRequest *, BOOL);
extern void (*wkSetBaseCTM)(CGContextRef, CGAffineTransform);
extern void (*wkSetPatternPhaseInUserSpace)(CGContextRef, CGPoint);
extern CGAffineTransform (*wkGetUserToBaseCTM)(CGContextRef);
extern void (*wkSetUpFontCache)();
extern void (*wkSignalCFReadStreamEnd)(CFReadStreamRef stream);
extern void (*wkSignalCFReadStreamError)(CFReadStreamRef stream, CFStreamError *error);
extern void (*wkSignalCFReadStreamHasBytes)(CFReadStreamRef stream);
extern unsigned (*wkInitializeMaximumHTTPConnectionCountPerHost)(unsigned preferredConnectionCount);
extern int (*wkGetHTTPPipeliningPriority)(CFURLRequestRef);
extern void (*wkSetHTTPPipeliningMaximumPriority)(int maximumPriority);
extern void (*wkSetHTTPPipeliningPriority)(CFURLRequestRef, int priority);
extern void (*wkSetHTTPPipeliningMinimumFastLanePriority)(int priority);
extern void (*wkSetCONNECTProxyForStream)(CFReadStreamRef, CFStringRef proxyHost, CFNumberRef proxyPort);
extern void (*wkSetCONNECTProxyAuthorizationForStream)(CFReadStreamRef, CFStringRef proxyAuthorizationString);
extern CFHTTPMessageRef (*wkCopyCONNECTProxyResponse)(CFReadStreamRef, CFURLRef responseURL);

extern void (*wkGetGlyphsForCharacters)(CGFontRef, const UniChar[], CGGlyph[], size_t);
extern bool (*wkGetVerticalGlyphsForCharacters)(CTFontRef, const UniChar[], CGGlyph[], size_t);

extern BOOL (*wkUseSharedMediaUI)();

extern CFIndex (*wkGetHyphenationLocationBeforeIndex)(CFStringRef string, CFIndex index);

typedef enum {
    wkEventPhaseNone = 0,
    wkEventPhaseBegan = 1,
    wkEventPhaseChanged = 2,
    wkEventPhaseEnded = 3,
} wkEventPhase;

extern int (*wkGetNSEventMomentumPhase)(NSEvent *);

extern CTLineRef (*wkCreateCTLineWithUniCharProvider)(const UniChar* (*provide)(CFIndex stringIndex, CFIndex* charCount, CFDictionaryRef* attributes, void*), void (*dispose)(const UniChar* chars, void*), void*);

extern CTTypesetterRef (*wkCreateCTTypesetterWithUniCharProviderAndOptions)(const UniChar* (*provide)(CFIndex stringIndex, CFIndex* charCount, CFDictionaryRef* attributes, void*), void (*dispose)(const UniChar* chars, void*), void*, CFDictionaryRef options);

#if PLATFORM(MAC) && USE(CA) && !PLATFORM(IOS_SIMULATOR)
extern CGContextRef (*wkIOSurfaceContextCreate)(IOSurfaceRef surface, unsigned width, unsigned height, CGColorSpaceRef colorSpace);
extern CGImageRef (*wkIOSurfaceContextCreateImage)(CGContextRef context);
#endif


extern CGSize (*wkGetViewportScreenSize)(void);
extern void (*wkSetLayerContentsScale)(CALayer *);
extern float (*wkGetScreenScaleFactor)(void);

#if defined(BUILDING_ON_SNOW_LEOPARD) || defined(BUILDING_ON_LEOPARD)
typedef struct __CFURLStorageSession* CFURLStorageSessionRef;
#else
typedef const struct __CFURLStorageSession* CFURLStorageSessionRef;
#endif
extern CFURLStorageSessionRef (*wkCreatePrivateStorageSession)(CFStringRef);
extern NSURLRequest* (*wkCopyRequestWithStorageSession)(CFURLStorageSessionRef, NSURLRequest*);

typedef struct OpaqueCFHTTPCookieStorage* CFHTTPCookieStorageRef;
extern CFHTTPCookieStorageRef (*wkCopyHTTPCookieStorage)(CFURLStorageSessionRef);
extern unsigned (*wkGetHTTPCookieAcceptPolicy)(CFHTTPCookieStorageRef);
extern void (*wkSetHTTPCookieAcceptPolicy)(CFHTTPCookieStorageRef, unsigned);
extern NSArray *(*wkHTTPCookiesForURL)(CFHTTPCookieStorageRef, NSURL *);
extern void (*wkSetHTTPCookiesForURL)(CFHTTPCookieStorageRef, NSArray *, NSURL *, NSURL *);
extern void (*wkDeleteHTTPCookie)(CFHTTPCookieStorageRef, NSHTTPCookie *);

extern CFStringRef (*wkGetCFURLResponseMIMEType)(CFURLResponseRef);
extern CFURLRef (*wkGetCFURLResponseURL)(CFURLResponseRef);
extern CFHTTPMessageRef (*wkGetCFURLResponseHTTPResponse)(CFURLResponseRef);
extern CFStringRef (*wkCopyCFURLResponseSuggestedFilename)(CFURLResponseRef);
extern void (*wkSetCFURLResponseMIMEType)(CFURLResponseRef, CFStringRef mimeType);

#if USE(CFNETWORK)
extern CFHTTPCookieStorageRef (*wkGetDefaultHTTPCookieStorage)();
extern WKCFURLCredentialRef (*wkCopyCredentialFromCFPersistentStorage)(CFURLProtectionSpaceRef protectionSpace);
extern void (*wkSetCFURLRequestShouldContentSniff)(CFMutableURLRequestRef, bool);
extern CFArrayRef (*wkCFURLRequestCopyHTTPRequestBodyParts)(CFURLRequestRef);
extern void (*wkCFURLRequestSetHTTPRequestBodyParts)(CFMutableURLRequestRef, CFArrayRef bodyParts);
extern void (*wkSetRequestStorageSession)(CFURLStorageSessionRef, CFMutableURLRequestRef);
#endif
    
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
#import <dispatch/dispatch.h>

extern dispatch_source_t (*wkCreateVMPressureDispatchOnMainQueue)(void);

#endif

#if !defined(BUILDING_ON_SNOW_LEOPARD) && !defined(BUILDING_ON_LION)
extern NSString *(*wkGetMacOSXVersionString)(void);
extern bool (*wkExecutableWasLinkedOnOrBeforeLion)(void);
#endif

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
extern void (*wkCGPathAddRoundedRect)(CGMutablePathRef path, const CGAffineTransform* matrix, CGRect rect, CGFloat cornerWidth, CGFloat cornerHeight);
#endif


}

#if !defined(BUILDING_ON_SNOW_LEOPARD)
extern void (*wkCFURLRequestAllowAllPostCaching)(CFURLRequestRef);
#endif

#endif
