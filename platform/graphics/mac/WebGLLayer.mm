/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)
#if ENABLE(WEBGL)

#import "WebGLLayer.h"

#import "GraphicsContext3D.h"
#import "GraphicsLayer.h"
#import <wtf/FastMalloc.h>
#import <wtf/RetainPtr.h>
#import <wtf/UnusedParam.h>

using namespace WebCore;

@implementation WebGLLayer

-(id)initWithGraphicsContext3D:(GraphicsContext3D*)context
{
    m_context = context;
    self = [super init];
    return self;
}


-(CGImageRef)copyImageSnapshotWithColorSpace:(CGColorSpaceRef)colorSpace
{
    UNUSED_PARAM(colorSpace);
    return 0;
}

- (void)display
{
    m_context->endPaint();
    if (m_layerOwner)
        m_layerOwner->layerDidDisplay(self);
}

@end

@implementation WebGLLayer(WebGLLayerAdditions)

-(void)setLayerOwner:(GraphicsLayer*)aLayer
{
    m_layerOwner = aLayer;
}

-(GraphicsLayer*)layerOwner
{
    return m_layerOwner;
}

@end

#endif // ENABLE(WEBGL)
#endif // USE(ACCELERATED_COMPOSITING)
