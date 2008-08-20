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
#import <float.h>
#import <unicode/uchar.h>
#import <wtf/Assertions.h>
#import <wtf/RetainPtr.h>


namespace WebCore {
  
const float smallCapsFontSizeMultiplier = 0.7f;
const float contextDPI = 72.0f;
static inline float scaleEmToUnits(float x, unsigned unitsPerEm) { return x * (contextDPI / (contextDPI * unitsPerEm)); }

void SimpleFontData::platformInit()
{
    m_syntheticBoldOffset = m_font.m_syntheticBold ? ceilf(GSFontGetSize(m_font.font())  / 24.0f) : 0.f;
    m_spaceGlyph = 0;
    m_spaceWidth = 0;
    m_smallCapsFont = 0;
    m_adjustedSpaceWidth = 0;
    m_ascent = GSFontGetAscent(m_font.font());
    m_descent = -GSFontGetDescent(m_font.font());
    m_lineSpacing = GSFontGetLineSpacing(m_font.font());
    m_lineGap = GSFontGetLineGap(m_font.font());
    m_xHeight = GSFontGetXHeight(m_font.font());
    m_unitsPerEm = GSFontGetUnitsPerEm(m_font.font());    
}

void SimpleFontData::platformDestroy()
{
}

SimpleFontData* SimpleFontData::smallCapsFontData(const FontDescription& fontDescription) const
{
    if (!m_smallCapsFontData) {
        if (isCustomFont()) {
            FontPlatformData smallCapsFontData(m_font);
            smallCapsFontData.m_size = smallCapsFontData.m_size * smallCapsFontSizeMultiplier;
            m_smallCapsFontData = new SimpleFontData(smallCapsFontData, true, false);
        } else {
            BEGIN_BLOCK_OBJC_EXCEPTIONS;
            GSFontTraitMask fontTraits= GSFontGetTraits(m_font.font());
            FontPlatformData smallCapsFont(GSFontCreateWithName(GSFontGetFamilyName(m_font.font()), fontTraits, GSFontGetSize(m_font.font()) * smallCapsFontSizeMultiplier));
            // <rdar://problem/4567212> PURPLE TODO: Implement this for real
            // Francisco Says: Maybe fixed now
            
            // AppKit resets the type information (screen/printer) when you convert a font to a different size.
            // We have to fix up the font that we're handed back.

            if (smallCapsFont.font()) {
                if (m_font.m_syntheticBold)
                    fontTraits |= GSBoldFontMask;
                if (m_font.m_syntheticOblique)
                    fontTraits |= GSItalicFontMask;

                GSFontTraitMask smallCapsFontTraits= GSFontGetTraits(smallCapsFont.font());
                smallCapsFont.m_syntheticBold = (fontTraits & GSBoldFontMask) && !(smallCapsFontTraits & GSBoldFontMask);
                smallCapsFont.m_syntheticOblique = (fontTraits & GSItalicFontMask) && !(smallCapsFontTraits & GSItalicFontMask);

                m_smallCapsFontData = FontCache::getCachedFontData(&smallCapsFont);
            }
            END_BLOCK_OBJC_EXCEPTIONS;
        }
    }
    return m_smallCapsFontData;
}

bool SimpleFontData::containsCharacters(const UChar* characters, int length) const
{
    return 0;
}

void SimpleFontData::determinePitch()
{
    GSFontRef f = m_font.font();
    m_treatAsFixedPitch = false;
    // GSFont is null in the case of SVG fonts for example.
    if (f) {
        const char *fullName = GSFontGetFullName(f);
        m_treatAsFixedPitch = (GSFontIsFixedPitch(f) || (fullName && (strcasecmp(fullName, "Osaka-Mono") == 0 || strcasecmp(fullName, "MS-PGothic") == 0)));        
    }
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    GSFontRef font = m_font.font();
    float pointSize = GSFontGetSize(font);
    CGAffineTransform m = CGAffineTransformMakeScale(pointSize, pointSize);
    CGSize advance;
    if (!GSFontGetGlyphTransformedAdvances(font, &m, kCGFontRenderingModeAntialiased, &glyph, 1, &advance)) {      
        LOG_ERROR("Unable to cache glyph widths for %@ %f", GSFontGetFullName(font), pointSize);
        advance.width = 0;
    }
    return advance.width + m_syntheticBoldOffset;
}


}
