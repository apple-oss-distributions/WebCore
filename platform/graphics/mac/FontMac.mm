/**
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2006 Apple Computer, Inc.
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
#import "Font.h"

#import "BlockExceptions.h"
#import "CharacterNames.h"
#import "FontFallbackList.h"
#import "GlyphBuffer.h"
#import "GraphicsContext.h"
#import "IntRect.h"
#import "Logging.h"
#import "ShapeArabic.h"
#import "SimpleFontData.h"
#import "WebCoreSystemInterface.h"
#import "WebCoreTextRenderer.h"

#import "WKGraphics.h"
#import <GraphicsServices/GraphicsServices.h>

#define SYNTHETIC_OBLIQUE_ANGLE 14

#ifdef __LP64__
#define URefCon void*
#else
#define URefCon UInt32
#endif

using namespace std;

namespace WebCore {



void Font::drawGlyphs(GraphicsContext* context, const SimpleFontData* font, const GlyphBuffer& glyphBuffer, int from, int numGlyphs, const FloatPoint& point, bool setColor) const
{
    CGContextRef cgContext = WKGetCurrentGraphicsContext();

    bool originalShouldUseFontSmoothing = CGContextGetShouldSmoothFonts(cgContext);
    bool newShouldUseFontSmoothing = WebCoreShouldUseFontSmoothing();
    
    if (originalShouldUseFontSmoothing != newShouldUseFontSmoothing)
        CGContextSetShouldSmoothFonts(cgContext, newShouldUseFontSmoothing);

#if !PLATFORM(IPHONE_SIMULATOR)
    // Font smoothing style
    CGFontSmoothingStyle originalFontSmoothingStyle = CGContextGetFontSmoothingStyle(cgContext);
    CGFontSmoothingStyle fontSmoothingStyle = WebCoreFontSmoothingStyle();
    if (newShouldUseFontSmoothing && fontSmoothingStyle != originalFontSmoothingStyle)
        CGContextSetFontSmoothingStyle(cgContext, fontSmoothingStyle);
    
    // Font antialiasing style
    CGFontAntialiasingStyle originalFontAntialiasingStyle = CGContextGetFontAntialiasingStyle(cgContext);
    CGFontAntialiasingStyle fontAntialiasingStyle = WebCoreFontAntialiasingStyle();
    if (fontAntialiasingStyle != originalFontAntialiasingStyle)
        CGContextSetFontAntialiasingStyle(cgContext, fontAntialiasingStyle);
#endif
    
    const FontPlatformData& platformData = font->platformData();
    
    GSFontSetFont(cgContext, platformData.font());
    float fontSize = GSFontGetSize(platformData.font());
    CGAffineTransform matrix = CGAffineTransformMakeScale(fontSize, fontSize);
    // Always flipped on Purple.
    matrix.b = -matrix.b;
    matrix.d = -matrix.d;    
    if (platformData.m_syntheticOblique)
        matrix = CGAffineTransformConcat(matrix, CGAffineTransformMake(1, 0, -tanf(SYNTHETIC_OBLIQUE_ANGLE * acosf(0) / 90), 1, 0, 0)); 
    CGContextSetTextMatrix(cgContext, matrix);

    CGContextSetFontSize(cgContext, 1.0f);
    
    CGContextSetTextPosition(cgContext, point.x(), point.y());
    CGContextShowGlyphsWithAdvances(cgContext, glyphBuffer.glyphs(from), glyphBuffer.advances(from), numGlyphs);
    if (font->m_syntheticBoldOffset) {
        CGContextSetTextPosition(cgContext, point.x() + font->m_syntheticBoldOffset, point.y());
        CGContextShowGlyphsWithAdvances(cgContext, glyphBuffer.glyphs(from), glyphBuffer.advances(from), numGlyphs);
    }

    if (originalShouldUseFontSmoothing != newShouldUseFontSmoothing)
        CGContextSetShouldSmoothFonts(cgContext, originalShouldUseFontSmoothing);
        
#if !PLATFORM(IPHONE_SIMULATOR)
    if (newShouldUseFontSmoothing && fontSmoothingStyle != originalFontSmoothingStyle)
        CGContextSetFontSmoothingStyle(cgContext, originalFontSmoothingStyle);
    
    if (fontAntialiasingStyle != originalFontAntialiasingStyle)
        CGContextSetFontAntialiasingStyle(cgContext, originalFontAntialiasingStyle);
#endif
}

}
