/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef WKLayer_h
#define WKLayer_h

#if ENABLE(HW_COMP)

#import <QuartzCore/QuartzCore.h>

namespace WebCore {
class LCLayer;
}

@interface CALayer(WKLayerAdditions)

- (void)setLayerOwner:(WebCore::LCLayer*)aLayer;
- (WebCore::LCLayer*)layerOwner;

- (float)contentsScale;
- (void)setContentsScale:(float)newScale;

@end


// for debug logging
@interface CALayer(ExtendedDescription)

- (NSString*)descriptionWithPrefix:(NSString*)inPrefix;

@end


@interface WKLayer : CALayer 
{
    WebCore::LCLayer* _layerOwner;
    float _contentsScale;
}

// implements WKLayerAdditions
// implements ExtendedDescription

@end

#endif // ENABLE(HW_COMP)

#endif // WKLayer_h


