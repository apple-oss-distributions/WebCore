//
//  WAKWindow.h
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc.  All rights reserved.
//

#ifndef WAKWindow_h
#define WAKWindow_h

#import "WAKAppKitStubs.h"
#import "WAKResponder.h"
#import "WAKView.h"
#import "WKContentObservation.h"
#import "WKWindow.h"
#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

@class CALayer;
@class WebEvent;

#ifdef __cplusplus
namespace WebCore {
    class TileCache;
}
typedef WebCore::TileCache TileCache;
#else
typedef struct TileCache TileCache;
#endif

typedef enum {
    kWAKWindowTilingModeNormal,
    kWAKWindowTilingModeMinimal,
    kWAKWindowTilingModePanning,
    kWAKWindowTilingModeZooming,
    kWAKWindowTilingModeDisabled
} WAKWindowTilingMode;

typedef enum {
    kWAKTilingDirectionUp,
    kWAKTilingDirectionDown,
    kWAKTilingDirectionLeft,
    kWAKTilingDirectionRight,
} WAKTilingDirection;

@interface WAKWindow : WAKResponder
{
    WKWindowRef _wkWindow;
    CALayer* _hostLayer;
    TileCache* _tileCache;
    CGRect _cachedVisibleRect;
}
// Create layer hosted window
- (id)initWithLayer:(CALayer *)hostLayer;
// Create unhosted window for manual painting
- (id)initWithFrame:(CGRect)frame;

- (CALayer*)hostLayer;

- (void)setContentView:(WAKView *)aView;
- (WAKView *)contentView;
- (void)close;
- (WAKResponder *)firstResponder;
- (NSPoint)convertBaseToScreen:(NSPoint)aPoint;
- (NSPoint)convertScreenToBase:(NSPoint)aPoint;
- (BOOL)isKeyWindow;
- (void)makeKeyWindow;
- (NSSelectionDirection)keyViewSelectionDirection;
- (BOOL)makeFirstResponder:(NSResponder *)aResponder;
- (WAKView *)_newFirstResponderAfterResigning;
- (WKWindowRef)_windowRef;
- (void)setFrame:(NSRect)frameRect display:(BOOL)flag;
- (CGRect)frame;
- (void)setScreenSize:(CGSize)size;
- (CGSize)screenSize;
- (void)setAvailableScreenSize:(CGSize)size;
- (CGSize)availableScreenSize;
- (void)setScreenScale:(CGFloat)scale;
- (CGFloat)screenScale;
- (void)sendEvent:(WebEvent *)anEvent;
- (void)sendEvent:(WebEvent *)anEvent contentChange:(WKContentChange *)aContentChange;

// Tiling support
- (void)layoutTiles;
- (void)layoutTilesNow;
- (void)setNeedsDisplay;
- (void)setNeedsDisplayInRect:(CGRect)rect;
- (BOOL)tilesOpaque;
- (void)setTilesOpaque:(BOOL)opaque;
- (NSString *)tileMinificationFilter;
- (void)setTileMinificationFilter:(NSString *)filter;
- (CGRect)visibleRect;
- (void)removeAllNonVisibleTiles;
- (void)removeAllTiles;
- (void)setTilingMode:(WAKWindowTilingMode)mode;
- (WAKWindowTilingMode)tilingMode;
- (void)setTilingDirection:(WAKTilingDirection)tilingDirection;
- (WAKTilingDirection)tilingDirection;
- (BOOL)hasPendingDraw;
- (void)hostLayerSizeChanged;

- (void)willRotate;
- (void)didRotate;

- (BOOL)useOrientationDependentFontAntialiasing;
- (void)setUseOrientationDependentFontAntialiasing:(BOOL)aa;
+ (BOOL)hasLandscapeOrientation;
+ (void)setOrientationProvider:(id)provider;

- (NSString *)recursiveDescription;
@end

#endif

