/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Permission is granted by Apple to use this file to the extent
 * necessary to relink with LGPL WebKit files.
 *
 * No license or rights are granted by Apple expressly or by
 * implication, estoppel, or otherwise, to Apple patents and
 * trademarks. For the sake of clarity, no license or rights are
 * granted by Apple expressly or by implication, estoppel, or otherwise,
 * under any Apple patents, copyrights and trademarks to underlying
 * implementations of any application programming interfaces (APIs)
 * or to any functionality that is invoked by calling any API.
 */

#ifndef WebEventRegion_h
#define WebEventRegion_h

#import <wtf/Platform.h>


#import <CoreGraphics/CGGeometry.h>
#import <Foundation/NSObject.h>

@interface WebEventRegion : NSObject <NSCopying>
{
    CGPoint p1, p2, p3, p4;
}
- (id)initWithPoints:(CGPoint) inP1:(CGPoint) inP2:(CGPoint) inP3:(CGPoint) inP4;
- (BOOL)hitTest:(CGPoint)point;
@end


#endif // WebEventRegion_h
