/*
 * Copyright (C) 2007, 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef LCRootLayer_h
#define LCRootLayer_h

#if ENABLE(HW_COMP)

#include <QuartzCore/QuartzCore.h>
#include "LCLayer.h"

namespace WebCore {

class LCRootLayer : public LCLayer {

protected:
    LCRootLayer(const FloatRect& inBounds);

public:
    virtual ~LCRootLayer();
    
    /* Static constructors
     * Use these so we can keep track of all layers
     * and possibly share contents
     */
    static LCRootLayer* rootLayer(const FloatRect& inBounds);

}; // class LCRootLayer

} // namespace WebCore
    
#endif // ENABLE(HW_COMP)

#endif // LCRootLayer_h

