/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#ifndef RenderVideo_h
#define RenderVideo_h

#if ENABLE(VIDEO)

#include "RenderMedia.h"

namespace WebCore {
    
class HTMLMediaElement;

#if ENABLE(HW_COMP)
class LCLayer;
#endif

class RenderVideo : public RenderMedia {
public:
    RenderVideo(HTMLMediaElement*);
    virtual ~RenderVideo();

    virtual const char* renderName() const { return "RenderVideo"; }
    virtual bool isVideo() const { return true; }

    virtual void paintReplaced(PaintInfo& paintInfo, int tx, int ty);

    virtual void layout();

    virtual int calcReplacedWidth() const;
    virtual int calcReplacedHeight() const;

    virtual void calcPrefWidths();
    
    void videoSizeChanged();
#if ENABLE(HW_COMP)
    void videoAdvanced();
#endif
    
    void updateFromElement();

#if ENABLE(HW_COMP)
    virtual bool requiresLayer() { return true; }
    virtual void compositingStateChanged();
    LCLayer* compositingLayer();
#endif

private:
    int calcAspectRatioWidth() const;
    int calcAspectRatioHeight() const;

    bool isWidthSpecified() const;
    bool isHeightSpecified() const;
    
    IntRect videoBox() const;

    void updatePlayer();

#if ENABLE(HW_COMP)
    void updateLayerBounds();

    void resizeTimerFired(Timer<RenderVideo>*);

    Timer<RenderVideo> m_resizeTimer;
#endif

};

} // namespace WebCore

#endif
#endif // RenderVideo_h
