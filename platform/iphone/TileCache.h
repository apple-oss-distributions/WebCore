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


#include "IntPoint.h"
#include "IntRect.h"
#include "IntSize.h"
#include "Timer.h"
#include "WAKWindow.h"
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RetainPtr.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

#ifdef __OBJC__
@class CALayer;
@class NSString;
@class TileLayer;
@class WebThreadCaller;
#else
class CALayer;
class NSString;
class TileLayer;
class WebThreadCaller;
#endif

namespace WebCore {

    typedef const NSString* TileMinificationFilter;

    class TileCache : Noncopyable {
    public:
        TileCache(WAKWindow*);
        ~TileCache();
        
        void setNeedsDisplay();
        void setNeedsDisplayInRect(const IntRect&);
        
        void layoutTiles();
        void layoutTilesNow();
        void removeAllNonVisibleTiles();
        void removeAllTiles();
        
        unsigned tileCount() const;
        
        bool tilesOpaque() const;
        void setTilesOpaque(bool);
        
        TileMinificationFilter tileMinificationFilter() const;
        void setTileMinificationFilter(TileMinificationFilter);
        
        enum TilingMode {
            Normal,
            Minimal,
            Panning,
            Zooming,
            Disabled
        };
        TilingMode tilingMode() const;
        void setTilingMode(TilingMode);

        void setTilingDirection(WAKTilingDirection);
        WAKTilingDirection tilingDirection() const;

        bool hasPendingDraw() const;
        
        void hostLayerSizeChanged();

        // Internal
        void updateTilingMode();
        void doLayoutTiles();
        void drawLayer(CALayer* layer, CGContextRef context);
        void prepareToDraw();

    private:
        // Refcount the tiles so they work nicely in vector and we know when to remove the tile layer from the parent.
        class Tile : public RefCounted<Tile> {
        public:
            static PassRefPtr<Tile> create(TileCache* cache, const IntRect& rect) { return adoptRef<Tile>(new Tile(cache, rect)); }
            ~Tile();

            TileLayer* tileLayer() const { return m_tileLayer.get(); }
            void invalidateRect(const IntRect& rectInSurface);
            IntRect rect() const { return m_rect; }
            void setRect(const IntRect& tileRect);

        private:
            Tile(TileCache*, const IntRect&);
            
            TileCache* m_tileCache;
            RetainPtr<TileLayer> m_tileLayer;
            IntRect m_rect;
        };

        class TileIndex {
        public:
            TileIndex(unsigned x, unsigned y) : m_x(x), m_y(y) { }
            bool operator==(const TileCache::TileIndex& o) const { return m_x == o.m_x && m_y == o.m_y; }
            unsigned x() const { return m_x; }
            unsigned y() const { return m_y; }
        private:
            unsigned m_x;
            unsigned m_y;
        };
        
        CALayer* tileHostLayer() const;

        IntRect bounds() const;
        IntRect visibleRect() const;

        void createTiles();
        void dropInvalidTiles();
        void dropTilesOutsideRect(const IntRect& keepRect);
        PassRefPtr<Tile> tileForIndex(const TileIndex&) const;
        IntRect tileRectForIndex(const TileIndex&) const;
        PassRefPtr<Tile> tileForPoint(const IntPoint& point) const;
        double tileDistance2(const IntRect& visibleRect, const IntRect& tileRect);
        bool tilesCover(const IntRect& rect) const;
        void centerTileGridOrigin(const IntRect& visibleRect);
        TileIndex tileIndexForPoint(const IntPoint& point) const;
        bool pointsOnSameTile(const IntPoint&, const IntPoint&) const;
        void adjustForPageBounds(IntRect& rect) const;
        bool isPaintingSuspended() const;
        bool isTileCreationSuspended() const;
        bool checkDoSingleTileLayout();
        void flushSavedDisplayRects();
        void invalidateTiles(const IntRect& dirtyRect);
        void calculateCoverAndKeepRectForMemoryLevel(const IntRect& visibleRect, IntRect& keepRect, IntRect& coverRect, bool& centerGrid);
        
        void tileCreationTimerFired(Timer<TileCache>*);
        
        RetainPtr<CALayer> m_tileHostLayer;
        WAKWindow* m_window;
        
        RetainPtr<WebThreadCaller> m_webThreadCaller;
        
        TilingMode m_tilingMode;
        WAKTilingDirection m_tilingDirection;
        TileMinificationFilter m_tileMinificationFilter;
        
        IntPoint m_tileGridOrigin;
        IntSize m_tileSize;
        bool m_tilesOpaque;
    
        bool m_didCallWillStartScrollingOrZooming;
        
        struct TileIndexHash {
            static unsigned hash(const TileIndex& i) { return WTF::intHash(static_cast<uint64_t>(i.x()) << 32 | i.y()); }
            static bool equal(const TileIndex& a, const TileIndex& b) { return a == b; }
            static const bool safeToCompareToEmptyOrDeleted = true;
        };
        struct TileIndexHashTraits : WTF::GenericHashTraits<TileIndex> {
            static const bool emptyValueIsZero = false;
            static const bool needsDestruction = false;
            static TileIndex emptyValue() { return TileIndex(-1, 0); }
            static void constructDeletedValue(TileIndex& slot) { slot = TileIndex(-1, -1); }
            static bool isDeletedValue(TileIndex value) { return value == TileIndex(-1, -1); }
        };
        typedef HashMap<TileIndex, RefPtr<Tile>, TileIndexHash, TileIndexHashTraits> TileMap;
        TileMap m_tiles;
        
        Timer<TileCache> m_tileCreationTimer;

        Vector<IntRect> m_savedDisplayRects;
        
        mutable Mutex m_tileMutex;
        mutable Mutex m_savedDisplayRectMutex;
    };
}


#endif // TileCache_h

