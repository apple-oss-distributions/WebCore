/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#import "config.h"
#import "WebEventRegion.h"
 
#if ENABLE(TOUCH_EVENTS)

#import "FloatQuad.h"

using namespace WebCore;

@interface WebEventRegion(Private)
- (FloatQuad)quad;
@end

@implementation WebEventRegion

- (id)initWithPoints:(CGPoint)inP1 :(CGPoint)inP2 :(CGPoint)inP3 :(CGPoint)inP4
{
    if (!(self = [super init]))
        return nil;
        
    p1 = inP1;
    p2 = inP2;
    p3 = inP3;
    p4 = inP4;
    return self;
}

- (id)copyWithZone:(NSZone *)zone
{
    UNUSED_PARAM(zone);
    return [self retain];
}

- (NSString *)description
{
    return [NSString stringWithFormat:@"p1:{%g, %g} p2:{%g, %g} p3:{%g, %g} p4:{%g, %g}", p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y];
}

- (BOOL)hitTest:(CGPoint)point
{
    return [self quad].containsPoint(point);
}

- (BOOL)isEqual:(id)other
{
    return  CGPointEqualToPoint(p1, ((WebEventRegion *)other)->p1) &&
            CGPointEqualToPoint(p2, ((WebEventRegion *)other)->p2) &&
            CGPointEqualToPoint(p3, ((WebEventRegion *)other)->p3) &&
            CGPointEqualToPoint(p4, ((WebEventRegion *)other)->p4);
}

- (FloatQuad)quad
{
    return FloatQuad(p1, p2, p3, p4);
}

@end

#endif // ENABLE(TOUCH_EVENTS)
