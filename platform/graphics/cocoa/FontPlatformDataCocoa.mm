/*
 * This file is part of the internal font implementation.
 *
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (c) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#import "config.h"
#import "FontPlatformData.h"

#import "PlatformString.h"
#import "WebCoreSystemInterface.h"
#import <CoreText/CoreText.h>

namespace WebCore {

// These CoreText Text Spacing feature selectors are not defined in CoreText.
enum TextSpacingCTFeatureSelector { TextSpacingProportional, TextSpacingFullWidth, TextSpacingHalfWidth, TextSpacingThirdWidth, TextSpacingQuarterWidth };


FontPlatformData::FontPlatformData(GSFontRef gsFont, float size, bool isPrinterFont, bool syntheticBold, bool syntheticOblique, FontOrientation orientation,
                                   TextOrientation textOrientation, FontWidthVariant widthVariant)
    : m_syntheticBold(syntheticBold)
    , m_syntheticOblique(syntheticOblique)
    , m_orientation(orientation)
    , m_textOrientation(textOrientation)
    , m_isEmoji(false)
    , m_size(size)
    , m_widthVariant(widthVariant)
    , m_font(gsFont)
    , m_cgFont(GSFontGetCGFont(gsFont))
    , m_isColorBitmapFont(GSFontHasColorGlyphs(gsFont))
    , m_isCompositeFontReference(false)
    , m_isPrinterFont(isPrinterFont)
{
    ASSERT_ARG(gsFont, gsFont);

    CFRetain(gsFont);
}

FontPlatformData::FontPlatformData(CTFontRef ctFont, float size, bool syntheticBold, bool syntheticOblique, FontOrientation orientation, TextOrientation textOrientation, FontWidthVariant widthVariant)
    : m_syntheticBold(syntheticBold)
    , m_syntheticOblique(syntheticOblique)
    , m_orientation(orientation)
    , m_textOrientation(textOrientation)
    , m_isEmoji(false)
    , m_size(size)
    , m_widthVariant(widthVariant)
    , m_cgFont(AdoptCF, CTFontCopyGraphicsFont(ctFont, 0))
    , m_isColorBitmapFont(false)
{
    ASSERT_ARG(ctFont, ctFont);

    const char* postScriptName = CGFontGetPostScriptName(m_cgFont.get());
    if (!postScriptName)
        return;

    m_font = GSFontCreateWithName(postScriptName, 0, m_size);
    m_isColorBitmapFont = GSFontHasColorGlyphs(m_font);
    m_isEmoji = !strcmp("AppleColorEmoji", postScriptName);
}

FontPlatformData:: ~FontPlatformData()
{
    if (m_font && m_font != reinterpret_cast<GSFontRef>(-1))
        CFRelease(m_font);
}

void FontPlatformData::platformDataInit(const FontPlatformData& f)
{
    m_font = f.m_font && f.m_font != reinterpret_cast<GSFontRef>(-1) ? static_cast<GSFontRef>(const_cast<void *>(CFRetain(f.m_font))) : f.m_font;

    m_isEmoji = f.m_isEmoji;
    m_cgFont = f.m_cgFont;
    m_CTFont = f.m_CTFont;

#if PLATFORM(CHROMIUM) && OS(DARWIN)
    m_inMemoryFont = f.m_inMemoryFont;
#endif
}

const FontPlatformData& FontPlatformData::platformDataAssign(const FontPlatformData& f)
{
    m_cgFont = f.m_cgFont;
    m_isEmoji = f.m_isEmoji;
    if (m_font == f.m_font)
        return *this;
    if (f.m_font && f.m_font != reinterpret_cast<GSFontRef>(-1))
        CFRetain(f.m_font);
    if (m_font && m_font != reinterpret_cast<GSFontRef>(-1))
        CFRelease(m_font);
    m_font = f.m_font;
    m_CTFont = f.m_CTFont;
#if PLATFORM(CHROMIUM) && OS(DARWIN)
    m_inMemoryFont = f.m_inMemoryFont;
#endif
    return *this;
}

bool FontPlatformData::platformIsEqual(const FontPlatformData& other) const
{
    if (m_font || other.m_font) {
#ifndef NDEBUG
        if (m_font == other.m_font)
            ASSERT(m_isEmoji == other.m_isEmoji);
#endif
        return m_font == other.m_font;
    }
#ifndef NDEBUG
    if (m_cgFont == other.m_cgFont)
        ASSERT(m_isEmoji == other.m_isEmoji);
#endif
    return m_cgFont == other.m_cgFont;
}

void FontPlatformData::setFont(GSFontRef font)
{
    ASSERT_ARG(font, font);
    ASSERT(m_font != reinterpret_cast<GSFontRef>(-1));

    if (m_font == font)
        return;

    CFRetain(font);
    if (m_font)
        CFRelease(m_font);
    m_font = font;
    m_size = GSFontGetSize(font);
    m_cgFont = GSFontGetCGFont(font);
    m_isColorBitmapFont = GSFontHasColorGlyphs(font);
}


bool FontPlatformData::allowsLigatures() const
{
    if (!m_font)
        return false;

    RetainPtr<CFCharacterSetRef> characterSet(AdoptCF, CTFontCopyCharacterSet(ctFont()));
    return !(characterSet.get() && CFCharacterSetIsCharacterMember(characterSet.get(), 'a'));
}

inline int mapFontWidthVariantToCTFeatureSelector(FontWidthVariant variant)
{
    switch(variant) {
    case RegularWidth:
        return TextSpacingProportional;

    case HalfWidth:
        return TextSpacingHalfWidth;

    case ThirdWidth:
        return TextSpacingThirdWidth;

    case QuarterWidth:
        return TextSpacingQuarterWidth;
    }

    ASSERT_NOT_REACHED();
    return TextSpacingProportional;
}


static CTFontDescriptorRef cascadeToLastResortFontDescriptor()
{
    static CTFontDescriptorRef descriptor;
    if (descriptor)
        return descriptor;

    const void* keys[] = { kCTFontCascadeListAttribute };
    const void* descriptors[] = { CTFontDescriptorCreateWithNameAndSize(CFSTR("LastResort"), 0) };
    const void* values[] = { CFArrayCreate(kCFAllocatorDefault, descriptors, WTF_ARRAY_LENGTH(descriptors), &kCFTypeArrayCallBacks) };
    RetainPtr<CFDictionaryRef> attributes(AdoptCF, CFDictionaryCreate(kCFAllocatorDefault, keys, values, WTF_ARRAY_LENGTH(keys), &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks));

    descriptor = CTFontDescriptorCreateWithAttributes(attributes.get());

    return descriptor;
}


CTFontRef FontPlatformData::ctFont() const
{
    if (m_CTFont)
        return m_CTFont.get();

    // Apple Color Emoji size is adjusted (and then re-adjusted by Core Text) and capped.
    CGFloat size = !m_isEmoji ? m_size : m_size <= 15 ? 4 * (m_size + 2) / static_cast<CGFloat>(5) : 16;
    m_CTFont.adoptCF(CTFontCreateWithGraphicsFont(m_cgFont.get(), size, 0, cascadeToLastResortFontDescriptor()));

    if (m_widthVariant != RegularWidth) {
        int featureTypeValue = kTextSpacingType;
        int featureSelectorValue = mapFontWidthVariantToCTFeatureSelector(m_widthVariant);
        RetainPtr<CTFontDescriptorRef> sourceDescriptor(AdoptCF, CTFontCopyFontDescriptor(m_CTFont.get()));
        RetainPtr<CFNumberRef> featureType(AdoptCF, CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &featureTypeValue));
        RetainPtr<CFNumberRef> featureSelector(AdoptCF, CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &featureSelectorValue));
        RetainPtr<CTFontDescriptorRef> newDescriptor(AdoptCF, CTFontDescriptorCreateCopyWithFeature(sourceDescriptor.get(), featureType.get(), featureSelector.get()));
        RetainPtr<CTFontRef> newFont(AdoptCF, CTFontCreateWithFontDescriptor(newDescriptor.get(), m_size, 0));

        if (newFont)
            m_CTFont = newFont;
    }

    return m_CTFont.get();
}

#ifndef NDEBUG
String FontPlatformData::description() const
{
    RetainPtr<CFStringRef> cgFontDescription(AdoptCF, CFCopyDescription(cgFont()));
    return String(cgFontDescription.get()) + " " + String::number(m_size)
            + (m_syntheticBold ? " synthetic bold" : "") + (m_syntheticOblique ? " synthetic oblique" : "") + (m_orientation ? " vertical orientation" : "");
}
#endif

} // namespace WebCore
