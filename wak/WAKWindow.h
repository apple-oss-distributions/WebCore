//
//  WAKWindow.h
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc.  All rights reserved.
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
    kWAKWindowTilingModeDisabled,
    kWAKWindowTilingModeScrollToTop,
} WAKWindowTilingMode;

typedef enum {
    kWAKTilingDirectionUp,
    kWAKTilingDirectionDown,
    kWAKTilingDirectionLeft,
    kWAKTilingDirectionRight,
} WAKTilingDirection;

extern NSString * const WAKWindowScreenScaleDidChangeNotification;

@interface WAKWindow : WAKResponder
{
    WKWindowRef _wkWindow;
    CALayer* _hostLayer;
    TileCache* _tileCache;
    CGRect _cachedVisibleRect;
    CALayer *_rootLayer;

    CGSize _screenSize;
    CGSize _availableScreenSize;
    CGFloat _screenScale;

    CGRect _frame;

    BOOL _useOrientationDependentFontAntialiasing;
}

@property (nonatomic, assign) BOOL useOrientationDependentFontAntialiasing;

// Create layer hosted window
- (id)initWithLayer:(CALayer *)hostLayer;
// Create unhosted window for manual painting
- (id)initWithFrame:(CGRect)frame;

- (CALayer*)hostLayer;

- (void)setContentView:(WAKView *)aView;
- (WAKView *)contentView;
- (void)close;
- (WAKView *)firstResponder;
- (BOOL)makeViewFirstResponder:(WAKView *)view;
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
- (void)setContentRect:(CGRect)rect;
- (void)setScreenSize:(CGSize)size;
- (CGSize)screenSize;
- (void)setAvailableScreenSize:(CGSize)size;
- (CGSize)availableScreenSize;
- (void)setScreenScale:(CGFloat)scale;
- (CGFloat)screenScale;
- (void)setRootLayer:(CALayer *)layer;
- (CALayer *)rootLayer;
- (void)sendEvent:(WebEvent *)anEvent;
- (void)sendEventSynchronously:(WebEvent *)anEvent;
- (void)sendMouseMoveEvent:(WebEvent *)anEvent contentChange:(WKContentChange *)aContentChange;

// Tiling support
- (void)layoutTiles;
- (void)layoutTilesNow;
- (void)layoutTilesNowForRect:(CGRect)rect;
- (void)setNeedsDisplay;
- (void)setNeedsDisplayInRect:(CGRect)rect;
- (BOOL)tilesOpaque;
- (void)setTilesOpaque:(BOOL)opaque;
- (CGRect)visibleRect;
- (void)removeAllNonVisibleTiles;
- (void)removeAllTiles;
- (void)removeForegroundTiles;
- (void)setTilingMode:(WAKWindowTilingMode)mode;
- (WAKWindowTilingMode)tilingMode;
- (void)setTilingDirection:(WAKTilingDirection)tilingDirection;
- (WAKTilingDirection)tilingDirection;
- (BOOL)hasPendingDraw;
- (void)displayRect:(NSRect)rect;
- (void)setZoomedOutTileScale:(float)scale;
- (float)zoomedOutTileScale;
- (void)setCurrentTileScale:(float)scale;
- (float)currentTileScale;
- (void)setKeepsZoomedOutTiles:(BOOL)keepsZoomedOutTiles;
- (BOOL)keepsZoomedOutTiles;

- (void)dumpTiles;

- (void)setTileBordersVisible:(BOOL)visible;
- (void)setTilePaintCountsVisible:(BOOL)visible;
- (void)setAcceleratedDrawingEnabled:(BOOL)enabled;

- (void)willRotate;
- (void)didRotate;

- (BOOL)useOrientationDependentFontAntialiasing;
- (void)setUseOrientationDependentFontAntialiasing:(BOOL)aa;
+ (BOOL)hasLandscapeOrientation;
+ (void)setOrientationProvider:(id)provider;

+ (WebEvent *)currentEvent;

- (NSString *)recursiveDescription;
@end

#endif

