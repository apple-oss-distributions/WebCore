//
//  WAKWindowPrivate.h
//
//  Copyright (C) 2005, 2006, 2007, 2009 Apple Inc.  All rights reserved.
//

#ifndef WAKWindowPrivate_h
#define WAKWindowPrivate_h

#import "WAKWindow.h"

@interface WAKWindow (WAKPrivate)
+ (WAKWindow *)_wrapperForWindowRef:(WKWindowRef)_windowRef;
@end

#endif // WAKWindowPrivate_h
