/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Apple Inc.  All rights reserved.
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
#include "Color.h"

#if PLATFORM(CG)

#include <wtf/Assertions.h>
#include <wtf/RetainPtr.h>
#include <CoreGraphics/CoreGraphics.h>
#include <CoreGraphics/CGColorTransform.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {


CGColorRef createCGColorWithDeviceWhite(CGFloat w, CGFloat a)
{
    DEFINE_STATIC_LOCAL(RetainPtr<CGColorSpaceRef>, graySpace, (AdoptCF, CGColorSpaceCreateDeviceGray()));
    const CGFloat components[] = { w, a };
    return CGColorCreate(graySpace.get(), components);
}

CGColorSpaceRef deviceRGBColorSpace()
{
    DEFINE_STATIC_LOCAL(RetainPtr<CGColorSpaceRef>, colorSpace, (AdoptCF, CGColorSpaceCreateDeviceRGB()));
    return colorSpace.get();
}

static CGColorRef createCGColorWithDeviceRGBA(CGColorRef sourceColor)
{
    if (!sourceColor || CFEqual(CGColorGetColorSpace(sourceColor), deviceRGBColorSpace()))
        return CGColorRetain(sourceColor);
    
    RetainPtr<CGColorTransformRef> colorTransform(AdoptCF, CGColorTransformCreate(deviceRGBColorSpace(), NULL));
    if (!colorTransform)
        return CGColorRetain(sourceColor);
    
    // CGColorTransformConvertColor() returns a +1 retained object.
    return CGColorTransformConvertColor(colorTransform.get(), sourceColor, kCGRenderingIntentDefault);
}

static CGColorRef createCGColorWithDeviceRGBA(CGFloat r, CGFloat g, CGFloat b, CGFloat a)
{
    const CGFloat components[] = { r, g, b, a };
    return CGColorCreate(deviceRGBColorSpace(), components);
}


Color::Color(CGColorRef color)
{
    if (!color) {
        m_color = 0;
        m_valid = false;
        return;
    }

    RetainPtr<CGColorRef> correctedColor(AdoptCF, createCGColorWithDeviceRGBA(color));
    if (!correctedColor)
        correctedColor = color;

    size_t numComponents = CGColorGetNumberOfComponents(correctedColor.get());
    const CGFloat* components = CGColorGetComponents(correctedColor.get());

    float r = 0;
    float g = 0;
    float b = 0;
    float a = 0;

    switch (numComponents) {
    case 2:
        r = g = b = components[0];
        a = components[1];
        break;
    case 4:
        r = components[0];
        g = components[1];
        b = components[2];
        a = components[3];
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    m_color = makeRGBA(r * 255, g * 255, b * 255, a * 255);
}

#if OS(WINDOWS)

CGColorRef createCGColor(const Color& c)
{
    CGColorRef color = NULL;
    CMProfileRef prof = NULL;
    CMGetSystemProfile(&prof);

    RetainPtr<CGColorSpaceRef> rgbSpace(AdoptCF, CGColorSpaceCreateWithPlatformColorSpace(prof));

    if (rgbSpace) {
        CGFloat components[4] = { static_cast<CGFloat>(c.red()) / 255, static_cast<CGFloat>(c.green()) / 255,
                                  static_cast<CGFloat>(c.blue()) / 255, static_cast<CGFloat>(c.alpha()) / 255 };
        color = CGColorCreate(rgbSpace.get(), components);
    }

    CMCloseProfile(prof);

    return color;
}

#endif // OS(WINDOWS)


CGColorRef createCGColor(const Color &color)
{
    RGBA32 c = color.rgb();
    switch (c) {
        case 0: {
            DEFINE_STATIC_LOCAL(RetainPtr<CGColorRef>, clearColor, (AdoptCF, createCGColorWithDeviceRGBA(0.f, 0.f, 0.f, 0.f)));
            return CGColorRetain(clearColor.get());
        }
        case Color::black: {
            DEFINE_STATIC_LOCAL(RetainPtr<CGColorRef>, blackColor, (AdoptCF, createCGColorWithDeviceRGBA(0.f, 0.f, 0.f, 1.f)));
            return CGColorRetain(blackColor.get());
        }
        case Color::white: {
            DEFINE_STATIC_LOCAL(RetainPtr<CGColorRef>, whiteColor, (AdoptCF, createCGColorWithDeviceRGBA(1.f, 1.f, 1.f, 1.f)));
            return CGColorRetain(whiteColor.get());
        }
        default: {
            const int cacheSize = 32;
            static RGBA32 cachedRGBAValues[cacheSize];
            static RetainPtr<CGColorRef>* cachedColors = new RetainPtr<CGColorRef>[cacheSize];

            for (int i = 0; i != cacheSize; ++i) {
                if (cachedRGBAValues[i] == c)
                    return CGColorRetain(cachedColors[i].get());
            }

            CGColorRef result =  createCGColorWithDeviceRGBA(color.red() / 255.f, color.green() / 255.f, color.blue() / 255.f, color.alpha() / 255.f);

            static int cursor;
            cachedRGBAValues[cursor] = c;
            cachedColors[cursor] = result;
            if (++cursor == cacheSize)
                cursor = 0;
            
            return result;
        }
    }

    return NULL;
}


}

#endif // PLATFORM(CG)
