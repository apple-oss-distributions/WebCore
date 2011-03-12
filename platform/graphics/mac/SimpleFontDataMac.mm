/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "SimpleFontData.h"

#import "BlockExceptions.h"
#import "Color.h"
#import "FloatRect.h"
#import "Font.h"
#import "FontCache.h"
#import "FontDescription.h"
#import "SharedBuffer.h"
#import "WebCoreSystemInterface.h"
#import <CoreText/CoreText.h>
#import <float.h>
#import <unicode/uchar.h>
#import <wtf/Assertions.h>
#import <wtf/StdLibExtras.h>
#import <wtf/RetainPtr.h>
#import <wtf/UnusedParam.h>

#include <wtf/UnusedParam.h>


using namespace std;

using namespace std;

namespace WebCore {
  
const float smallCapsFontSizeMultiplier = 0.7f;
static inline float scaleEmToUnits(float x, unsigned unitsPerEm) { return x / unitsPerEm; }

void SimpleFontData::platformInit()
{
    m_syntheticBoldOffset = m_platformData.m_syntheticBold ? ceilf(m_platformData.size()  / 24.0f) : 0.f;
    m_spaceGlyph = 0;
    m_spaceWidth = 0;
    m_adjustedSpaceWidth = 0;
    if (GSFontRef gsFont = m_platformData.font()) {
        m_ascent = ceilf(GSFontGetAscent(gsFont));
        m_descent = ceilf(-GSFontGetDescent(gsFont));
        m_lineSpacing = GSFontGetLineSpacing(gsFont);
        m_lineGap = GSFontGetLineGap(gsFont);
        m_xHeight = GSFontGetXHeight(gsFont);
        m_unitsPerEm = GSFontGetUnitsPerEm(gsFont);
    } else {
        CGFontRef cgFont = m_platformData.cgFont();

        m_unitsPerEm = CGFontGetUnitsPerEm(cgFont);

        float pointSize = m_platformData.size();
        m_ascent = lroundf(scaleEmToUnits(CGFontGetAscent(cgFont), m_unitsPerEm) * pointSize);
        m_descent = lroundf(-scaleEmToUnits(CGFontGetDescent(cgFont), m_unitsPerEm) * pointSize);
        m_lineGap = lroundf(scaleEmToUnits(CGFontGetLeading(cgFont), m_unitsPerEm) * pointSize);
        m_xHeight = scaleEmToUnits(CGFontGetXHeight(cgFont), m_unitsPerEm) * pointSize;

        m_lineSpacing = m_ascent + m_descent + m_lineGap;
    }

    if (!m_platformData.m_isEmoji)
        return;

    int thirdOfSize = m_platformData.size() / 3;
    m_ascent = thirdOfSize;
    m_descent = thirdOfSize;
    m_lineGap = thirdOfSize;
    m_lineSpacing = 0;
}
    

void SimpleFontData::platformCharWidthInit()
{
    m_avgCharWidth = 0;
    m_maxCharWidth = 0;
    

    // Fallback to a cross-platform estimate, which will populate these values if they are non-positive.
    initCharWidths();
}

void SimpleFontData::platformDestroy()
{
}

SimpleFontData* SimpleFontData::smallCapsFontData(const FontDescription& fontDescription) const
{
    if (!m_smallCapsFontData) {
        if (isCustomFont()) {
            FontPlatformData smallCapsFontData(m_platformData);
            smallCapsFontData.m_size = smallCapsFontData.m_size * smallCapsFontSizeMultiplier;
            m_smallCapsFontData = new SimpleFontData(smallCapsFontData, true, false);
        } else {
            BEGIN_BLOCK_OBJC_EXCEPTIONS;
            GSFontTraitMask fontTraits= GSFontGetTraits(m_platformData.font());
            FontPlatformData smallCapsFont(GSFontCreateWithName(GSFontGetFamilyName(m_platformData.font()), fontTraits, GSFontGetSize(m_platformData.font()) * smallCapsFontSizeMultiplier));
            
            // AppKit resets the type information (screen/printer) when you convert a font to a different size.
            // We have to fix up the font that we're handed back.
            UNUSED_PARAM(fontDescription);

            if (smallCapsFont.font()) {
                if (m_platformData.m_syntheticBold)
                    fontTraits |= GSBoldFontMask;
                if (m_platformData.m_syntheticOblique)
                    fontTraits |= GSItalicFontMask;

                GSFontTraitMask smallCapsFontTraits= GSFontGetTraits(smallCapsFont.font());
                smallCapsFont.m_syntheticBold = (fontTraits & GSBoldFontMask) && !(smallCapsFontTraits & GSBoldFontMask);
                smallCapsFont.m_syntheticOblique = (fontTraits & GSItalicFontMask) && !(smallCapsFontTraits & GSItalicFontMask);

                m_smallCapsFontData = fontCache()->getCachedFontData(&smallCapsFont);
            }
            END_BLOCK_OBJC_EXCEPTIONS;
        }
    }
    return m_smallCapsFontData;
}

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    UNUSED_PARAM(characters);
    UNUSED_PARAM(length);
    return 0;
}

void SimpleFontData::determinePitch()
{
    GSFontRef f = m_platformData.font();
    m_treatAsFixedPitch = false;
    // GSFont is null in the case of SVG fonts for example.
    if (f) {
        const char *fullName = GSFontGetFullName(f);
        const char *familyName = GSFontGetFamilyName(f);
        m_treatAsFixedPitch = (GSFontIsFixedPitch(f) || (fullName && (strcasecmp(fullName, "Osaka-Mono") == 0 || strcasecmp(fullName, "MS-PGothic") == 0)));        
        if (familyName && strcasecmp(familyName, "Courier New") == 0) // Special case Courier New to not be treated as fixed pitch, as this will make use of a hacked space width which is undesireable for iPhone (see rdar://6269783)
            m_treatAsFixedPitch = false;
    }
}

FloatRect SimpleFontData::platformBoundsForGlyph(Glyph glyph) const
{
    FloatRect boundingBox;
#ifndef BUILDING_ON_TIGER
    CGRect box;
    CGFontGetGlyphBBoxes(platformData().cgFont(), &glyph, 1, &box);
    float pointSize = platformData().m_size;
    CGFloat scale = pointSize / unitsPerEm();
    boundingBox = CGRectApplyAffineTransform(box, CGAffineTransformMakeScale(scale, -scale));
#else
    // FIXME: Custom fonts don't have NSFonts, so this function doesn't compute correct bounds for these on Tiger.
    if (!m_platformData.font())
        return boundingBox;
    boundingBox = [m_platformData.font() boundingRectForGlyph:glyph];
#endif
    if (m_syntheticBoldOffset)
        boundingBox.setWidth(boundingBox.width() + m_syntheticBoldOffset);

    return boundingBox;
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    if (platformData().m_isEmoji) {
        // returns the proper scaled advance for the image size - see Font::drawGlyphs
        return std::min(platformData().m_size + (platformData().m_size <= 15.0f ? 4.0f : 6.0f), 22.0f);
    }
    float pointSize = platformData().m_size;
    CGAffineTransform m = CGAffineTransformMakeScale(pointSize, pointSize);
    CGSize advance;
    static const CGFontRenderingStyle renderingStyle = kCGFontRenderingStyleAntialiasing | kCGFontRenderingStyleSubpixelPositioning | kCGFontRenderingStyleSubpixelQuantization | kCGFontAntialiasingStyleUnfiltered;
    if (!CGFontGetGlyphAdvancesForStyle(platformData().cgFont(), &m, renderingStyle, &glyph, 1, &advance)) {
        RetainPtr<CFStringRef> fullName(AdoptCF, CGFontCopyFullName(platformData().cgFont()));
        LOG_ERROR("Unable to cache glyph widths for %@ %f", fullName.get(), pointSize);
        advance.width = 0;
    }
    return advance.width + m_syntheticBoldOffset;
}

} // namespace WebCore
