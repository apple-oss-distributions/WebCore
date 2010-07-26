//
//  WAKViewPrivate.h
//
//  Copyright (C) 2005, 2006, 2007, 2009 Apple Inc.  All rights reserved.
//

#ifndef WAKViewPrivate_h
#define WAKViewPrivate_h

#import "WAKView.h"

@interface WAKView (WAKPrivate)
- (WKViewRef)_viewRef;
+ (void)_addViewWrapper:(WAKView *)view;
+ (void)_removeViewWrapper:(WAKView *)view;
+ (WAKView *)_wrapperForViewRef:(WKViewRef)_viewRef;
- (void)_handleEvent:(WebEvent *)event;
- (BOOL)_handleResponderCall:(WKViewResponderCallbackType)type;
- (NSMutableSet *)_subviewReferences;
@end

#endif // WAKViewPrivate_h
