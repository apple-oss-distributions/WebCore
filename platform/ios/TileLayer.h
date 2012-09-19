/*
 * TileLayer.h
 * WebCore
 *
 * Copyright (C) 2011, Apple Inc.  All rights reserved.
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

#ifndef TileLayer_h
#define TileLayer_h


#include <QuartzCore/CALayer.h>

namespace WebCore {
class TileGrid;
};

@interface TileLayer : CALayer
{
    WebCore::TileGrid* _tileGrid;
    unsigned _paintCount;
}
@property (nonatomic) unsigned paintCount;
- (void)setTileGrid:(WebCore::TileGrid*)tileGrid;
@end

@interface TileHostLayer : CALayer
{
    WebCore::TileGrid* _tileGrid;
}
- (id)initWithTileGrid:(WebCore::TileGrid*)tileGrid;
@end

#endif
