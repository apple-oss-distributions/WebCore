/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
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

#import "WebCoreTextRenderer.h"
#import "WebCoreTextRendererFactory.h"

#import "KWQAssertions.h"

inline void WebCoreInitializeTextRun(WebCoreTextRun *run, const UniChar *characters, unsigned int length, int from, int to)
{
    run->characters = characters;
    run->length = length;
    run->from = from;
    run->to = to;
}

inline void WebCoreInitializeEmptyTextStyle(WebCoreTextStyle *style)
{
    style->padding = 0;
    style->textColor = nil;
    style->backgroundColor = nil;
    style->rtl = false;
    style->visuallyOrdered = false;
    style->letterSpacing = 0;
    style->wordSpacing = 0;
    style->smallCaps = false;
    style->applyRounding = true;
    style->attemptFontSubstitution = true;
    style->families = nil;
}

@implementation WebCoreTextRendererFactory

static WebCoreTextRendererFactory *sharedFactory;

+ (WebCoreTextRendererFactory *)sharedFactory
{
    return sharedFactory;
}

- init
{
    [super init];
    
    ASSERT(!sharedFactory);
    sharedFactory = [self retain];
    
    return self;
}

- (NSFont *)fontWithFamilies:(NSString **)families traits:(NSFontTraitMask)traits size:(float)size
{
    return nil;
}

- (BOOL)isFontFixedPitch:(NSFont *)font
{
    return NO;
}

- (id <WebCoreTextRenderer>)rendererWithFont:(NSFont *)font usingPrinterFont:(BOOL)usingPrinterFont
{
    return nil;
}

@end
