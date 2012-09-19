/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
#import "Frame.h"

#import "BlockExceptions.h"
#import "ColorMac.h"
#import "Cursor.h"
#import "DOMInternal.h"
#import "Event.h"
#import "FrameLoaderClient.h"
#import "FrameView.h"
#import "GraphicsContext.h"
#import "HTMLNames.h"
#import "HTMLTableCellElement.h"
#import "HitTestRequest.h"
#import "HitTestResult.h"
#import "KeyboardEvent.h"
#import "Logging.h"
#import "MouseEventWithHitTestResults.h"
#import "Page.h"
#import "PlatformKeyboardEvent.h"
#import "PlatformWheelEvent.h"
#import "RegularExpression.h"
#import "RenderTableCell.h"
#import "RenderView.h"
#import "Scrollbar.h"
#import "SimpleFontData.h"
#import "visible_units.h"
#import <wtf/StdLibExtras.h>

#import "EventListener.h"
#import <wtf/GetPtr.h>
#import <wtf/UnusedParam.h>

#ifndef NSView
#define NSView WAKView
#endif
#import "WAKView.h"
 
@interface NSView (WebCoreHTMLDocumentView)
- (void)drawSingleRect:(NSRect)rect;
@end
 
using namespace std;

namespace WebCore {

using namespace HTMLNames;


DragImageRef Frame::dragImageForSelection()
{
    return nil;
}

#define NSFloatValue(aFloat) [NSNumber numberWithFloat:aFloat]

NSDictionary* Frame::dictionaryForViewportArguments(const ViewportArguments& arguments) const
{
    return [NSDictionary dictionaryWithObjects:[NSArray arrayWithObjects:NSFloatValue(arguments.initialScale), NSFloatValue(arguments.minimumScale), 
                                                NSFloatValue(arguments.maximumScale), NSFloatValue(arguments.userScalable), 
                                                NSFloatValue(arguments.width), NSFloatValue(arguments.height), nil] 
                                       forKeys:[NSArray arrayWithObjects:@"initial-scale", @"minimum-scale", @"maximum-scale", @"user-scalable", @"width", @"height", nil]];
}

const ViewportArguments& Frame::viewportArguments() const
{
    return m_viewportArguments;
}

void Frame::setViewportArguments(const ViewportArguments& arguments)
{
    m_viewportArguments = arguments;
}

static int sAllLayouts = 0;

void Frame::didParse(double duration)
{
    m_parseCount++;
    m_parseDuration += duration;
}

void Frame::didLayout(bool /*firstLayout*/, double duration)
{
    sAllLayouts++;
    m_layoutCount++;
    m_layoutDuration += duration;
}

void Frame::didForcedLayout()
{
    m_forcedLayoutCount++;
}

void Frame::getPPTStats(unsigned& parseCount, unsigned& layoutCount, unsigned& forcedLayoutCount, CFTimeInterval& parseDuration, CFTimeInterval& layoutDuration)
{
    parseCount = m_parseCount;
    layoutCount = m_layoutCount;
    forcedLayoutCount = m_forcedLayoutCount;
    parseDuration = m_parseDuration;
    layoutDuration = m_layoutDuration;

//fprintf(stderr, "All Layouts: %d\n", sAllLayouts);    
}

void Frame::clearPPTStats()
{
    m_parseCount = 0;
    m_layoutCount = 0;
    m_forcedLayoutCount = 0;
    m_parseDuration = 0.0;
    m_layoutDuration = 0.0;
}

} // namespace WebCore
