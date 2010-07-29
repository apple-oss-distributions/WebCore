/*
 * This file is part of the internal font implementation.
 *
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
    , m_font(gsFont)
{
    if (gsFont)
        CFRetain(gsFont);
    m_size = gsFont ? GSFontGetSize(gsFont) : 0.0f;
    m_gsFont = 0; // fixme <rdar://problem/5607116>
    m_isImageFont = false;
}

FontPlatformData::FontPlatformData(const FontPlatformData& f)
{
    m_font = f.m_font && f.m_font != reinterpret_cast<GSFontRef>(-1) ? static_cast<GSFontRef>(const_cast<void *>(CFRetain(f.m_font))) : f.m_font;
    m_syntheticBold = f.m_syntheticBold;
    m_syntheticOblique = f.m_syntheticOblique;
    m_size = f.m_size;
    m_gsFont = f.m_gsFont; //Does this need a retain/release? For now m_gsFont should always be nil, until we do: fixme <rdar://problem/5607116> Web fonts (CSS @font-face) broken in WebKit
    m_isImageFont = f.m_isImageFont;
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
    m_isImageFont = f.m_isImageFont;
    if (m_font == f.m_font)
        return *this;
    if (f.m_font && f.m_font != reinterpret_cast<GSFontRef>(-1))
        CFRetain(f.m_font);
    if (m_font && m_font != reinterpret_cast<GSFontRef>(-1))
        CFRelease(m_font);
    m_font = f.m_font;
    return *this;
}

void FontPlatformData::setFont(GSFontRef font)
{
    if (m_font == font)
        return;
    if (font)
        CFRetain(font);
    if (m_font)
        CFRelease(m_font);
    m_font = font;
    m_size = font ? GSFontGetSize(font) : 0.0f;
    m_gsFont = 0; // fixme <rdar://problem/5607116>
}


bool FontPlatformData::allowsLigatures() const
{
    if (!m_font)
        return false;
    
    RetainPtr<CTFontRef> ctFont(AdoptCF, CTFontCreateWithGraphicsFont(GSFontGetCGFont(m_font), GSFontGetSize(m_font), &CGAffineTransformIdentity, NULL));
    RetainPtr<CFCharacterSetRef> characterSet(AdoptCF, CTFontCopyCharacterSet(ctFont.get()));
    return !(characterSet.get() && CFCharacterSetIsCharacterMember(characterSet.get(), 'a'));
}

#ifndef NDEBUG
String FontPlatformData::description() const
{
    RetainPtr<CFStringRef> cgFontDescription(AdoptCF, CFCopyDescription(GSFontGetCGFont(m_font)));
    return String(cgFontDescription.get()) + " " + String::number(m_size) + (m_syntheticBold ? " synthetic bold" : "") + (m_syntheticOblique ? " syntheitic oblique" : "");
}
#endif

} // namespace WebCore
