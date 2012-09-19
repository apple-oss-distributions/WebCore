/*
 * TileCache.h
 * WebCore
 *
 * Copyright (C) 2009, Apple Inc.  All rights reserved.
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

#ifndef TileCache_h
#define TileCache_h


#include "FloatRect.h"
#include "IntPoint.h"
#include "IntRect.h"
#include "IntSize.h"
#include "Timer.h"
#include "WAKWindow.h"
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RetainPtr.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

#ifdef __OBJC__
@class TileCacheTombstone;
@class TileLayer;
#else
class TileCacheTombstone;
class TileLayer;
#endif

namespace WebCore {

class TileGrid;

class TileCache {
    WTF_MAKE_NONCOPYABLE(TileCache);
public:
    TileCache(WAKWindow*);
    ~TileCache();

    CGFloat screenScale() const;

    void setNeedsDisplay();
    void setNeedsDisplayInRect(const IntRect&);
    
    void layoutTiles();
    void layoutTilesNow();
    void layoutTilesNowForRect(const IntRect&);
    void removeAllNonVisibleTiles();
    void removeAllTiles();
    void removeForegroundTiles();
    
    void setTileBordersVisible(bool);
    bool tileBordersVisible() const { return m_tileBordersVisible; }
    
    void setTilePaintCountersVisible(bool);
    bool tilePaintCountersVisible() const { return m_tilePaintCountersVisible; }

    void setAcceleratedDrawingEnabled(bool enabled) { m_acceleratedDrawingEnabled = enabled; }
    bool acceleratedDrawingEnabled() const { return m_acceleratedDrawingEnabled; }

    void setKeepsZoomedOutTiles(bool);
    bool keepsZoomedOutTiles() const { return m_keepsZoomedOutTiles; }

    void setZoomedOutScale(float);
    float zoomedOutScale() const;
    
    void setCurrentScale(float);
    float currentScale() const;
    
    bool tilesOpaque() const;
    void setTilesOpaque(bool);
    
    enum TilingMode {
        Normal,
        Minimal,
        Panning,
        Zooming,
        Disabled,
        ScrollToTop
    };
    TilingMode tilingMode() const { return m_tilingMode; }
    void setTilingMode(TilingMode);

    void setTilingDirection(WAKTilingDirection);
    WAKTilingDirection tilingDirection() const;

    bool hasPendingDraw() const;
    
    void hostLayerSizeChanged();
    
    static void setLayerPoolCapacity(unsigned capacity);
    static void drainLayerPool();

    // Logging
    void dumpTiles();

    // Internal
    void doLayoutTiles();
    void drawLayer(TileLayer* layer, CGContextRef context);
    void prepareToDraw();
    void finishedCreatingTiles(bool createMore);
    FloatRect visibleRectInLayer(CALayer *layer) const;
    CALayer* hostLayer() const;
    unsigned tileCapacityForGrid(TileGrid*);

private:
    TileGrid* activeTileGrid() const;
    TileGrid* inactiveTileGrid() const;

    void updateTilingMode();
    bool isTileInvalidationSuspended() const;
    bool isTileCreationSuspended() const;
    void flushSavedDisplayRects();
    void invalidateTiles(const IntRect& dirtyRect);
    void setZoomedOutScaleInternal(float scale);
    void commitScaleChange();
    void bringActiveTileGridToFront();
    void adjustTileGridTransforms();
    void removeAllNonVisibleTilesInternal();
    void createTilesInActiveGrid();

    void tileCreationTimerFired(Timer<TileCache>*);
    
    WAKWindow* m_window;

    bool m_keepsZoomedOutTiles;
    
    bool m_hasPendingLayoutTiles;
    bool m_hasPendingUpdateTilingMode;
    // Ensure there are no async calls on a dead tile cache.
    RetainPtr<TileCacheTombstone> m_tombstone;
    
    TilingMode m_tilingMode;
    WAKTilingDirection m_tilingDirection;
    
    IntSize m_tileSize;
    bool m_tilesOpaque;

    bool m_tileBordersVisible;
    bool m_tilePaintCountersVisible;
    bool m_acceleratedDrawingEnabled;

    bool m_didCallWillStartScrollingOrZooming;
    OwnPtr<TileGrid> m_zoomedOutTileGrid;
    OwnPtr<TileGrid> m_zoomedInTileGrid;
    
    Timer<TileCache> m_tileCreationTimer;

    Vector<IntRect> m_savedDisplayRects;

    float m_currentScale;

    float m_pendingScale;
    float m_pendingZoomedOutScale;

    mutable Mutex m_tileMutex;
    mutable Mutex m_savedDisplayRectMutex;
};

}


#endif // TileCache_h

