/*
 * TileCache.h
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

#ifndef TileGrid_h
#define TileGrid_h

#if PLATFORM(IOS)

#include "IntPoint.h"
#include "IntPointHash.h"
#include "IntRect.h"
#include "IntSize.h"
#include "TileCache.h"
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RetainPtr.h>

#define LOG_TILING 0

@class CALayer;

namespace WebCore {

class TileGridTile;

class TileGrid {
    WTF_MAKE_NONCOPYABLE(TileGrid);
public:
    typedef IntPoint TileIndex;

    static PassOwnPtr<TileGrid> create(TileCache* cache, const IntSize& tileSize) { return adoptPtr(new TileGrid(cache, tileSize)); }
    
    ~TileGrid();
    
    TileCache* tileCache() const { return m_tileCache; }
    
    CALayer *tileHostLayer() const;
    IntRect bounds() const;
    unsigned tileCount() const;
    
    float scale() const { return m_scale; }
    void setScale(float scale) { m_scale = scale; }
    
    IntRect visibleRect() const;
    
    void createTiles(TileCache::SynchronousTileCreationMode);

    void dropAllTiles();
    void dropInvalidTiles();
    void dropTilesOutsideRect(const IntRect& keepRect);
    void dropTilesIntersectingRect(const IntRect& dropRect);
    // Drops tiles that intersect dropRect but do not intersect keepRect.
    void dropTilesBetweenRects(const IntRect& dropRect, const IntRect& keepRect);
    bool dropDistantTiles(unsigned tilesNeeded, double shortestDistance);

    void addTilesCoveringRect(const IntRect& rect);

    bool tilesCover(const IntRect& rect) const;
    void centerTileGridOrigin(const IntRect& visibleRect);
    void invalidateTiles(const IntRect& dirtyRect);
    
    void updateTileOpacity();
    void updateTileBorderVisibility();
    void updateHostLayerSize();
    bool checkDoSingleTileLayout();
    
    bool hasTiles() const { return !m_tiles.isEmpty(); }
    
    IntRect calculateCoverRect(const IntRect& visibleRect, bool& centerGrid);

    // Logging
    void dumpTiles();

private:
    double tileDistance2(const IntRect& visibleRect, const IntRect& tileRect) const;
    unsigned tileByteSize() const;

    void addTileForIndex(const TileIndex&);
    
    PassRefPtr<TileGridTile> tileForIndex(const TileIndex&) const;
    IntRect tileRectForIndex(const TileIndex&) const;
    PassRefPtr<TileGridTile> tileForPoint(const IntPoint& point) const;
    TileIndex tileIndexForPoint(const IntPoint& point) const;
    
    IntRect adjustCoverRectForPageBounds(const IntRect& rect) const;
    bool shouldUseMinimalTileCoverage() const;

private:        
    TileGrid(TileCache*, const IntSize&);
    
    TileCache* m_tileCache;
    RetainPtr<CALayer> m_tileHostLayer;
    
    IntPoint m_origin;
    IntSize m_tileSize;
    
    float m_scale;

    typedef HashMap<TileIndex, RefPtr<TileGridTile> > TileMap;
    TileMap m_tiles;
    
    IntRect m_validBounds;
};

static inline IntPoint topLeft(const IntRect& rect)
{
    return rect.location();
}

static inline IntPoint bottomRight(const IntRect& rect)
{
    return IntPoint(rect.maxX() - 1, rect.maxY() - 1);
}

}

#endif
#endif
