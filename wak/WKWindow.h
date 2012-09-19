//
//  WKWindow.h
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc.  All rights reserved.
//

#ifndef WKWindow_h
#define WKWindow_h

#import "WKTypes.h"
#import "WKUtilities.h"
#import <CoreGraphics/CoreGraphics.h>

#ifdef __OBJC__
@class WAKWindow;
#else
typedef struct WAKWindow WAKWindow;
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct WKWindow {
    WKObject obj;
    WAKWindow* wakWindow;
    WKViewRef contentView;
    WKViewRef responderView;
    WKViewRef _nextResponder;
};

extern WKClassInfo WKWindowClassInfo;

WKWindowRef WKWindowCreate(WAKWindow* wakWindow);

void WKWindowSetContentView (WKWindowRef window, WKViewRef aView);
WKViewRef WKWindowGetContentView (WKWindowRef window);

void WKWindowClose (WKWindowRef window);

bool WKWindowMakeFirstResponder (WKWindowRef window, WKViewRef view);
WKViewRef WKWindowFirstResponder (WKWindowRef window);
WKViewRef WKWindowNewFirstResponderAfterResigning(WKWindowRef window); // Only valid to call while resigning.

void WKWindowPrepareForDrawing(WKWindowRef window);

#ifdef __cplusplus
}
#endif

#endif // WKWindow_h
