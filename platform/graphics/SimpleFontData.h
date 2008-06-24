/*
 * This file is part of the internal font implementation.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
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

#ifndef SimpleFontData_h
#define SimpleFontData_h

#include "FontData.h"
#include "FontPlatformData.h"
#include "GlyphPageTreeNode.h"
#include "GlyphWidthMap.h"
#include <wtf/OwnPtr.h>

#if PLATFORM(MAC)
typedef struct OpaqueATSUStyle* ATSUStyle;
#endif

#if PLATFORM(WIN)
#include <usp10.h>
#endif

namespace WebCore {

class FontDescription;
class FontPlatformData;
class SharedBuffer;
class SVGFontData;
class WidthMap;

enum Pitch { UnknownPitch, FixedPitch, VariablePitch };

class SimpleFontData : public FontData {
public:
    SimpleFontData(const FontPlatformData&, bool customFont = false, bool loading = false, SVGFontData* data = 0);
    virtual ~SimpleFontData();

public:
    const FontPlatformData& platformData() const { return m_font; }
    SimpleFontData* smallCapsFontData(const FontDescription& fontDescription) const;

    // vertical metrics
    int ascent() const { return GSFontGetAscent(m_font.font()); }
    int descent() const { return -GSFontGetDescent(m_font.font()); }
    int lineSpacing() const { return GSFontGetLineSpacing(m_font.font()); }
    int lineGap() const { return GSFontGetLineGap(m_font.font()); }
    float xHeight() const { return GSFontGetXHeight(m_font.font()); }
    unsigned unitsPerEm() const { return GSFontGetUnitsPerEm(m_font.font()); }

    float widthForGlyph(Glyph) const;
    float platformWidthForGlyph(Glyph) const;

    virtual const SimpleFontData* fontDataForCharacter(UChar32) const;
    virtual bool containsCharacters(const UChar*, int length) const;

    void determinePitch();
    Pitch pitch() const { return m_treatAsFixedPitch ? FixedPitch : VariablePitch; }

#if ENABLE(SVG_FONTS)
    SVGFontData* svgFontData() const { return m_svgFontData.get(); }
    bool isSVGFont() const { return m_svgFontData; }
#else
    bool isSVGFont() const { return false; }
#endif

    virtual bool isCustomFont() const { return m_isCustomFont; }
    virtual bool isLoading() const { return m_isLoading; }
    virtual bool isSegmented() const;

    const GlyphData& missingGlyphData() const { return m_missingGlyphData; }

#if PLATFORM(MAC)
    GSFontRef getGSFont() const { return m_font.font(); }
#endif

#if PLATFORM(WIN)
    bool isSystemFont() const { return m_isSystemFont; }
    SCRIPT_FONTPROPERTIES* scriptFontProperties() const;
    SCRIPT_CACHE* scriptCache() const { return &m_scriptCache; }

    static void setShouldApplyMacAscentHack(bool);
    static bool shouldApplyMacAscentHack();
#endif

#if PLATFORM(GTK)
    void setFont(cairo_t*) const;
#endif

#if PLATFORM(WX)
    wxFont getWxFont() const { return m_font.font(); }
#endif

private:
    void platformInit();
    void platformDestroy();
    
    void commonInit();

public:

    FontPlatformData m_font;

    mutable GlyphWidthMap m_glyphToWidthMap;

    bool m_treatAsFixedPitch;

#if ENABLE(SVG_FONTS)
    OwnPtr<SVGFontData> m_svgFontData;
#endif

    bool m_isCustomFont;  // Whether or not we are custom font loaded via @font-face
    bool m_isLoading; // Whether or not this custom font is still in the act of loading.

    CGGlyph m_spaceGlyph;
    float m_spaceWidth;
    float m_adjustedSpaceWidth;

    GlyphData m_missingGlyphData;

    mutable SimpleFontData* m_smallCapsFontData;

#if PLATFORM(CG)
    float m_syntheticBoldOffset;
#endif

#if PLATFORM(MAC)
    GSFontRef m_smallCapsFont;
#endif

#if PLATFORM(WIN)
    bool m_isSystemFont;
    mutable SCRIPT_CACHE m_scriptCache;
    mutable SCRIPT_FONTPROPERTIES* m_scriptFontProperties;
#endif
};

} // namespace WebCore

#endif // SimpleFontData_h
