/*
 * Copyright 2006, 2007, 2008 Apple Computer, Inc. All rights reserved.
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

#import "config.h"
#import "WebCoreSystemInterface.h"

BOOL (*wkCGContextGetShouldSmoothFonts)(CGContextRef);
void (*wkClearGlyphVector)(void* glyphs);
OSStatus (*wkConvertCharToGlyphs)(void* styleGroup, const UniChar*, unsigned numCharacters, void* glyphs);
NSString* (*wkGetMIMETypeForExtension)(NSString*);
NSDate *(*wkGetNSURLResponseLastModifiedDate)(NSURLResponse *response);
void (*wkSetPatternBaseCTM)(CGContextRef, CGAffineTransform);
void (*wkSetPatternPhaseInUserSpace)(CGContextRef, CGPoint point);
void (*wkSetUpFontCache)(size_t);
void (*wkSignalCFReadStreamEnd)(CFReadStreamRef stream);
void (*wkSignalCFReadStreamHasBytes)(CFReadStreamRef stream);
void (*wkSignalCFReadStreamError)(CFReadStreamRef stream, CFStreamError *error);
CFReadStreamRef (*wkCreateCustomCFReadStream)(void *(*formCreate)(CFReadStreamRef, void *), 
    void (*formFinalize)(CFReadStreamRef, void *), 
    Boolean (*formOpen)(CFReadStreamRef, CFStreamError *, Boolean *, void *), 
    CFIndex (*formRead)(CFReadStreamRef, UInt8 *, CFIndex, CFStreamError *, Boolean *, void *), 
    Boolean (*formCanRead)(CFReadStreamRef, void *), 
    void (*formClose)(CFReadStreamRef, void *), 
    void (*formSchedule)(CFReadStreamRef, CFRunLoopRef, CFStringRef, void *), 
    void (*formUnschedule)(CFReadStreamRef, CFRunLoopRef, CFStringRef, void *),
    void *context);
void (*wkSetNSURLConnectionDefersCallbacks)(NSURLConnection *, BOOL);
void (*wkSetNSURLRequestShouldContentSniff)(NSMutableURLRequest *, BOOL);
id (*wkCreateNSURLConnectionDelegateProxy)(void);
BOOL (*wkSupportsMultipartXMixedReplace)(NSMutableURLRequest *);
Class (*wkNSURLProtocolClassForReqest)(NSURLRequest *);
float (*wkSecondsSinceLastInputEvent)(void);

void (*wkAssistControl)(id anAssistedControl);
