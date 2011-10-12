/*
 * TileGridTile.h
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

#ifndef TileGridTile_h
#define TileGridTile_h


#include "IntRect.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>

@class TileLayer;

namespace WebCore {
    
class TileGrid;

// Refcount the tiles so they work nicely in vector and we know when to remove the tile layer from the parent.
class TileGridTile : public RefCounted<TileGridTile> {
public:
    static PassRefPtr<TileGridTile> create(TileGrid* grid, const IntRect& rect) { return adoptRef<TileGridTile>(new TileGridTile(grid, rect)); }
    ~TileGridTile();
    
    TileLayer* tileLayer() const { return m_tileLayer.get(); }
    void invalidateRect(const IntRect& rectInSurface);
    IntRect rect() const { return m_rect; }
    void setRect(const IntRect& tileRect);
    void showBorder(bool);
    
private:
    TileGridTile(TileGrid*, const IntRect&);
    
    TileGrid* m_tileGrid;
    RetainPtr<TileLayer> m_tileLayer;
    IntRect m_rect;
};

}

#endif
