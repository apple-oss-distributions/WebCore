/*
 * This file is part of the internal font implementation.
 *
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

FontPlatformData::FontPlatformData(GSFontRef gsFont, bool syntheticBold, bool syntheticOblique)
    : m_syntheticBold(syntheticBold)
    , m_syntheticOblique(syntheticOblique)
    , m_atsuFontID(0)
    , m_isEmoji(false)
    , m_size(GSFontGetSize(gsFont))
    , m_font(gsFont)
    , m_cgFont(GSFontGetCGFont(gsFont))
    , m_isColorBitmapFont(GSFontHasColorGlyphs(gsFont))
{
    ASSERT_ARG(gsFont, gsFont);

    CFRetain(gsFont);
}

FontPlatformData::FontPlatformData(const FontPlatformData& f)
{
    m_font = f.m_font && f.m_font != reinterpret_cast<GSFontRef>(-1) ? static_cast<GSFontRef>(const_cast<void *>(CFRetain(f.m_font))) : f.m_font;
    m_syntheticBold = f.m_syntheticBold;
    m_syntheticOblique = f.m_syntheticOblique;
    m_size = f.m_size;
    m_cgFont = f.m_cgFont;
    m_atsuFontID = f.m_atsuFontID;
    m_isEmoji = f.m_isEmoji;
    m_isColorBitmapFont = f.m_isColorBitmapFont;
#if USE(CORE_TEXT)
    m_CTFont = f.m_CTFont;
#endif
}

FontPlatformData:: ~FontPlatformData()
{
    if (m_font && m_font != reinterpret_cast<GSFontRef>(-1))
        CFRelease(m_font);
}

const FontPlatformData& FontPlatformData::operator=(const FontPlatformData& f)
{
    m_syntheticBold = f.m_syntheticBold;
    m_syntheticOblique = f.m_syntheticOblique;
    m_size = f.m_size;
    m_cgFont = f.m_cgFont;
    m_atsuFontID = f.m_atsuFontID;
    m_isEmoji = f.m_isEmoji;
    if (m_font == f.m_font)
        return *this;
    if (f.m_font && f.m_font != reinterpret_cast<GSFontRef>(-1))
        CFRetain(f.m_font);
    if (m_font && m_font != reinterpret_cast<GSFontRef>(-1))
        CFRelease(m_font);
    m_font = f.m_font;
    m_isColorBitmapFont = f.m_isColorBitmapFont;
#if USE(CORE_TEXT)
    m_CTFont = f.m_CTFont;
#endif
    return *this;
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

#if USE(CORE_TEXT)
CTFontRef FontPlatformData::ctFont() const
{
    if (!m_CTFont) {
        // Apple Color Emoji size is restricted to 20 or less.
        CGFloat size = !m_isEmoji ? m_size : std::min<CGFloat>(m_size, 20);
        m_CTFont.adoptCF(CTFontCreateWithGraphicsFont(m_cgFont.get(), size, 0, 0));
    }
    return m_CTFont.get();
}
#endif // USE(CORE_TEXT)

#ifndef NDEBUG
String FontPlatformData::description() const
{
    RetainPtr<CFStringRef> cgFontDescription(AdoptCF, CFCopyDescription(m_cgFont.get()));
    return String(cgFontDescription.get()) + " " + String::number(m_size) + (m_syntheticBold ? " synthetic bold" : "") + (m_syntheticOblique ? " syntheitic oblique" : "");
}
#endif

} // namespace WebCore
