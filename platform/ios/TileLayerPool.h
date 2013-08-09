/*
 * TileLayerPool.h
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

#ifndef TileLayerPool_h
#define TileLayerPool_h

#if PLATFORM(IOS)

#include "IntSize.h"
#include "IntSizeHash.h"
#include "Timer.h"
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/Threading.h>
#include <wtf/Vector.h>

@class TileLayer;

namespace WebCore {
    
class TileLayerPool {
    WTF_MAKE_NONCOPYABLE(TileLayerPool);
public:
    static TileLayerPool* sharedPool();
    
    void addLayer(const RetainPtr<TileLayer>&);
    RetainPtr<TileLayer> takeLayerWithSize(const IntSize&);
    
    // The maximum size of all queued layers in bytes.
    unsigned capacity() const { return m_capacity; }
    void setCapacity(unsigned);
    void drain();
    
    unsigned decayedCapacity() const;
    
    static unsigned bytesBackingLayerWithPixelSize(const IntSize& size);
    
private:
    TileLayerPool();
    
    typedef Deque<RetainPtr<TileLayer> > LayerList;
    
    bool canReuseLayerWithSize(const IntSize& size) const { return m_capacity && !size.isEmpty(); }
    void schedulePrune();
    void prune();
    typedef enum { LeaveUnchanged, MarkAsUsed } AccessType;
    LayerList& listOfLayersWithSize(const IntSize&, AccessType accessType = LeaveUnchanged);
    
    HashMap<IntSize, LayerList> m_reuseLists;
    // Ordered by recent use.  The last size is the most recently used.
    Vector<IntSize> m_sizesInPruneOrder;
    unsigned m_totalBytes;
    unsigned m_capacity;
    Mutex m_layerPoolMutex;
    
    double m_lastAddTime;
    bool m_needsPrune;
};

}

#endif
#endif
