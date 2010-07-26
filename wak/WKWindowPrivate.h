//
//  WKWindowPrivate.h
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc.  All rights reserved.
//

#ifndef WKWindowPrivate_h
#define WKWindowPrivate_h

#import "WKWindow.h"

#ifdef __cplusplus
extern "C" {
#endif    

bool _WKWindowHaveDirtyWindows(void);
void _WKWindowLayoutDirtyWindows(void);
void _WKWindowDrawDirtyWindows(void);

#ifdef __cplusplus
}
#endif

#endif // WKWindowPrivate_h
