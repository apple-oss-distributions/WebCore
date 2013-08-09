/*
 * TileLayer.mm
 * WebCore
 *
 * Copyright (C) 2011, Apple Inc.  All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#include "config.h"
#include "TileLayer.h"

#if PLATFORM(IOS)

#include "TileCache.h"
#include "TileGrid.h"
#include "WebCoreThread.h"

@implementation TileHostLayer
- (id)initWithTileGrid:(WebCore::TileGrid*)tileGrid
{
    self = [super init];
    if (!self)
        return nil;
    _tileGrid = tileGrid;
    [self setAnchorPoint:CGPointZero];
    return self;
}

- (id<CAAction>)actionForKey:(NSString *)key
{
    UNUSED_PARAM(key);
    // Disable all default actions
    return nil;
}

- (void)renderInContext:(CGContextRef)context
{
    if (pthread_main_np())
        WebThreadLock();
    _tileGrid->tileCache()->doLayoutTiles();
    [super renderInContext:context];
}
@end

@implementation TileLayer
@synthesize paintCount = _paintCount;
@synthesize tileGrid = _tileGrid;

static TileLayer *layerBeingPainted;

- (void)setNeedsDisplayInRect:(CGRect)rect
{
    [self setNeedsLayout];
    [super setNeedsDisplayInRect:rect];
}

- (void)layoutSublayers
{
    if (pthread_main_np())
        WebThreadLock();
    // This may trigger WebKit layout and generate more repaint rects.
    if (_tileGrid)
        _tileGrid->tileCache()->prepareToDraw();
}

- (void)drawInContext:(CGContextRef)context
{
    if (_tileGrid)
        _tileGrid->tileCache()->drawLayer(self, context);
}

- (id<CAAction>)actionForKey:(NSString *)key
{
    UNUSED_PARAM(key);
    // Disable all default actions
    return nil;
}

+ (TileLayer *)layerBeingPainted
{
    return layerBeingPainted;
}

@end

#endif
