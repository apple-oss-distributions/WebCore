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

@class WebEvent;

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
    CGRect frame;
    WKViewRef contentView;
    WKViewRef responderView;
    WKViewRef _nextResponder;
    CGSize screenSize;
    CGSize availableScreenSize;
    CGFloat screenScale;
    unsigned int useOrientationDependentFontAntialiasing:1;
};

extern WKClassInfo WKWindowClassInfo;

WKWindowRef WKWindowCreate(WAKWindow* wakWindow, CGRect contentRect);

void WKWindowSetScreenSize(WKWindowRef window, CGSize size);
CGSize WKWindowGetScreenSize(WKWindowRef window);

void WKWindowSetAvailableScreenSize(WKWindowRef window, CGSize size);
CGSize WKWindowGetAvailableScreenSize(WKWindowRef window);

void WKWindowSetScreenScale(WKWindowRef window, CGFloat scale);
CGFloat WKWindowGetScreenScale(WKWindowRef window);

void WKWindowSetContentView (WKWindowRef window, WKViewRef aView);
WKViewRef WKWindowGetContentView (WKWindowRef window);

void WKWindowSetContentRect(WKWindowRef window, CGRect contentRect);
CGRect WKWindowGetContentRect(WKWindowRef window);

void WKWindowClose (WKWindowRef window);

bool WKWindowMakeFirstResponder (WKWindowRef window, WKViewRef view);
WKViewRef WKWindowFirstResponder (WKWindowRef window);
WKViewRef WKWindowNewFirstResponderAfterResigning(WKWindowRef window); // Only valid to call while resigning.
void WKWindowSendEvent (WKWindowRef window, WebEvent *event);

CGPoint WKWindowConvertBaseToScreen (WKWindowRef window, CGPoint point);
CGPoint WKWindowConvertScreenToBase (WKWindowRef window, CGPoint point);

void WKWindowSetFrame(WKWindowRef window, CGRect frame, bool display);

WebEvent *WKEventGetCurrentEvent(void);

void WKWindowPrepareForDrawing(WKWindowRef window);

void WKWindowSetNeedsDisplay(WKWindowRef window, bool flag);
void WKWindowSetNeedsDisplayInRect(WKWindowRef window, CGRect rect);
    
void WKWindowDrawRect(WKWindowRef window, CGRect dirtyRect);

#ifdef __cplusplus
}
#endif

#endif // WKWindow_h
