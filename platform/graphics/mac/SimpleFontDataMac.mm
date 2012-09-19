/*
 * Copyright (C) 2005, 2006, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
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

static bool fontHasVerticalGlyphs(CTFontRef ctFont)
{
    // The check doesn't look neat but this is what AppKit does for vertical writing...
    RetainPtr<CFArrayRef> tableTags(AdoptCF, CTFontCopyAvailableTables(ctFont, kCTFontTableOptionNoOptions));
    CFIndex numTables = CFArrayGetCount(tableTags.get());
    for (CFIndex index = 0; index < numTables; ++index) {
        CTFontTableTag tag = (CTFontTableTag)(uintptr_t)CFArrayGetValueAtIndex(tableTags.get(), index);
        if (tag == kCTFontTableVhea || tag == kCTFontTableVORG)
            return true;
    }
    return false;
}

void SimpleFontData::platformInit()
{
    m_syntheticBoldOffset = m_platformData.m_syntheticBold ? ceilf(m_platformData.size()  / 24.0f) : 0.f;
    m_spaceGlyph = 0;
    m_spaceWidth = 0;
    unsigned unitsPerEm;
    float ascent;
    float descent;
    float lineGap;
    float lineSpacing;
    float xHeight;
    if (GSFontRef gsFont = m_platformData.font()) {
        ascent = ceilf(GSFontGetAscent(gsFont));
        descent = ceilf(-GSFontGetDescent(gsFont));
        lineSpacing = GSFontGetLineSpacing(gsFont);
        lineGap = GSFontGetLineGap(gsFont);
        xHeight = GSFontGetXHeight(gsFont);
        unitsPerEm = GSFontGetUnitsPerEm(gsFont);
    } else {
        CGFontRef cgFont = m_platformData.cgFont();

        unitsPerEm = CGFontGetUnitsPerEm(cgFont);

        float pointSize = m_platformData.size();
        ascent = lroundf(scaleEmToUnits(CGFontGetAscent(cgFont), unitsPerEm) * pointSize);
        descent = lroundf(-scaleEmToUnits(-abs(CGFontGetDescent(cgFont)), unitsPerEm) * pointSize);
        lineGap = lroundf(scaleEmToUnits(CGFontGetLeading(cgFont), unitsPerEm) * pointSize);
        xHeight = scaleEmToUnits(CGFontGetXHeight(cgFont), unitsPerEm) * pointSize;

        lineSpacing = ascent + descent + lineGap;
    }

    m_fontMetrics.setUnitsPerEm(unitsPerEm);
    m_fontMetrics.setAscent(ascent);
    m_fontMetrics.setDescent(descent);
    m_fontMetrics.setLineGap(lineGap);
    m_fontMetrics.setLineSpacing(lineSpacing);
    m_fontMetrics.setXHeight(xHeight);

    if (platformData().orientation() == Vertical && !isTextOrientationFallback())
        m_hasVerticalGlyphs = fontHasVerticalGlyphs(m_platformData.ctFont());

    if (!m_platformData.m_isEmoji)
        return;

    int thirdOfSize = m_platformData.size() / 3;
    m_fontMetrics.setAscent(thirdOfSize);
    m_fontMetrics.setDescent(thirdOfSize);
    m_fontMetrics.setLineGap(thirdOfSize);
    m_fontMetrics.setLineSpacing(0);
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
    if (!isCustomFont() && m_derivedFontData) {
        // These come from the cache.
        if (m_derivedFontData->smallCaps)
            fontCache()->releaseFontData(m_derivedFontData->smallCaps.leakPtr());

        if (m_derivedFontData->emphasisMark)
            fontCache()->releaseFontData(m_derivedFontData->emphasisMark.leakPtr());
    }

#if USE(ATSUI)
    HashMap<unsigned, ATSUStyle>::iterator end = m_ATSUStyleMap.end();
    for (HashMap<unsigned, ATSUStyle>::iterator it = m_ATSUStyleMap.begin(); it != end; ++it)
        ATSUDisposeStyle(it->second);
#endif
}

PassOwnPtr<SimpleFontData> SimpleFontData::createScaledFontData(const FontDescription& fontDescription, float scaleFactor) const
{
    if (isCustomFont()) {
        FontPlatformData scaledFontData(m_platformData);
        scaledFontData.m_size = scaledFontData.m_size * scaleFactor;
        return adoptPtr(new SimpleFontData(scaledFontData, true, false));
    }

    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    float size = m_platformData.size() * scaleFactor;
    GSFontTraitMask fontTraits = GSFontGetTraits(m_platformData.font());
    RetainPtr<GSFontRef> gsFont(AdoptCF, GSFontCreateWithName(GSFontGetFamilyName(m_platformData.font()), fontTraits, size));
    FontPlatformData scaledFontData(gsFont.get(), size, m_platformData.isPrinterFont(), false, false, m_platformData.orientation());

    // AppKit resets the type information (screen/printer) when you convert a font to a different size.
    // We have to fix up the font that we're handed back.
    UNUSED_PARAM(fontDescription);

    if (scaledFontData.font()) {
        if (m_platformData.m_syntheticBold)
            fontTraits |= GSBoldFontMask;
        if (m_platformData.m_syntheticOblique)
            fontTraits |= GSItalicFontMask;

        GSFontTraitMask scaledFontTraits = GSFontGetTraits(scaledFontData.font());
        scaledFontData.m_syntheticBold = (fontTraits & GSBoldFontMask) && !(scaledFontTraits & GSBoldFontMask);
        scaledFontData.m_syntheticOblique = (fontTraits & GSItalicFontMask) && !(scaledFontTraits & GSItalicFontMask);

        return adoptPtr(fontCache()->getCachedFontData(&scaledFontData));
    }
    END_BLOCK_OBJC_EXCEPTIONS;

    return nullptr;
}

SimpleFontData* SimpleFontData::smallCapsFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->smallCaps)
        m_derivedFontData->smallCaps = createScaledFontData(fontDescription, smallCapsFontSizeMultiplier);

    return m_derivedFontData->smallCaps.get();
}

SimpleFontData* SimpleFontData::emphasisMarkFontData(const FontDescription& fontDescription) const
{
    if (!m_derivedFontData)
        m_derivedFontData = DerivedFontData::create(isCustomFont());
    if (!m_derivedFontData->emphasisMark)
        m_derivedFontData->emphasisMark = createScaledFontData(fontDescription, .5f);

    return m_derivedFontData->emphasisMark.get();
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
    boundingBox = CTFontGetBoundingRectsForGlyphs(m_platformData.ctFont(), platformData().orientation() == Vertical ? kCTFontVerticalOrientation : kCTFontHorizontalOrientation, &glyph, 0, 1);
    boundingBox.setY(-boundingBox.maxY());
    if (m_syntheticBoldOffset)
        boundingBox.setWidth(boundingBox.width() + m_syntheticBoldOffset);

    return boundingBox;
}

float SimpleFontData::platformWidthForGlyph(Glyph glyph) const
{
    CGSize advance = CGSizeZero;
    if (platformData().orientation() == Horizontal || m_isBrokenIdeographFallback) {
        if (platformData().m_isEmoji) {
            // returns the proper scaled advance for the image size - see Font::drawGlyphs
            return std::min(platformData().m_size + (platformData().m_size <= 15.0f ? 4.0f : 6.0f), 22.0f);
        } else {
            float pointSize = platformData().m_size;
            CGAffineTransform m = CGAffineTransformMakeScale(pointSize, pointSize);
            static const CGFontRenderingStyle renderingStyle = kCGFontRenderingStyleAntialiasing | kCGFontRenderingStyleSubpixelPositioning | kCGFontRenderingStyleSubpixelQuantization | kCGFontAntialiasingStyleUnfiltered;
            if (!CGFontGetGlyphAdvancesForStyle(platformData().cgFont(), &m, renderingStyle, &glyph, 1, &advance)) {
                RetainPtr<CFStringRef> fullName(AdoptCF, CGFontCopyFullName(platformData().cgFont()));
                LOG_ERROR("Unable to cache glyph widths for %@ %f", fullName.get(), pointSize);
                advance.width = 0;
            }
        }
    } else
        CTFontGetAdvancesForGlyphs(m_platformData.ctFont(), kCTFontVerticalOrientation, &glyph, &advance, 1);

    return advance.width + m_syntheticBoldOffset;
}

struct ProviderInfo {
    const UChar* characters;
    size_t length;
    CFDictionaryRef attributes;
};

static const UniChar* provideStringAndAttributes(CFIndex stringIndex, CFIndex* count, CFDictionaryRef* attributes, void* context)
{
    ProviderInfo* info = static_cast<struct ProviderInfo*>(context);
    if (stringIndex < 0 || static_cast<size_t>(stringIndex) >= info->length)
        return 0;

    *count = info->length - stringIndex;
    *attributes = info->attributes;
    return info->characters + stringIndex;
}

bool SimpleFontData::canRenderCombiningCharacterSequence(const UChar* characters, size_t length) const
{
    ASSERT(isMainThread() || pthread_main_np());

    if (!m_combiningCharacterSequenceSupport)
        m_combiningCharacterSequenceSupport = adoptPtr(new HashMap<String, bool>);

    WTF::HashMap<String, bool>::AddResult addResult = m_combiningCharacterSequenceSupport->add(String(characters, length), false);
    if (!addResult.isNewEntry)
        return addResult.iterator->second;

    RetainPtr<CGFontRef> cgFont(AdoptCF, CTFontCopyGraphicsFont(platformData().ctFont(), 0));

    ProviderInfo info = { characters, length, getCFStringAttributes(0, platformData().orientation()) };
    RetainPtr<CTLineRef> line(AdoptCF, wkCreateCTLineWithUniCharProvider(&provideStringAndAttributes, 0, &info));

    CFArrayRef runArray = CTLineGetGlyphRuns(line.get());
    CFIndex runCount = CFArrayGetCount(runArray);

    for (CFIndex r = 0; r < runCount; r++) {
        CTRunRef ctRun = static_cast<CTRunRef>(CFArrayGetValueAtIndex(runArray, r));
        ASSERT(CFGetTypeID(ctRun) == CTRunGetTypeID());
        CFDictionaryRef runAttributes = CTRunGetAttributes(ctRun);
        CTFontRef runFont = static_cast<CTFontRef>(CFDictionaryGetValue(runAttributes, kCTFontAttributeName));
        RetainPtr<CGFontRef> runCGFont(AdoptCF, CTFontCopyGraphicsFont(runFont, 0));
        if (!CFEqual(runCGFont.get(), cgFont.get()))
            return false;
    }

    addResult.iterator->second = true;
    return true;
}

} // namespace WebCore
