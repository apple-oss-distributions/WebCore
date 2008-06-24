/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef WKTiledLayer_h
#define WKTiledLayer_h

#if ENABLE(HW_COMP)

#define HAVE_CATILED_LAYER    1

#if HAVE_CATILED_LAYER

#import <QuartzCore/QuartzCore.h>

#import "WKLayer.h"

@interface WKTiledLayer : CATiledLayer 
{
    WebCore::LCLayer*       _layerOwner;
}

// implements WKLayerAdditions
// implements ExtendedDescription

@end

#endif // HAVE_CATILED_LAYER

#endif // ENABLE(HW_COMP)

#endif // WKTiledLayer_h

