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
#import "WebCoreFrameBridge.h"
#import "BitmapImage.h"
#import "Threading.h"

#define SYNTHETIC_OBLIQUE_ANGLE 14

#ifdef __LP64__
#define URefCon void*
#else
#define URefCon UInt32
#endif

using namespace std;

namespace WebCore {



// 1600 bytes each (20 x 20 x 4 bytes pp), so 50 emoji is only ~80k.
#define EMOJI_CACHE_SIZE 50

static Image* smileImage(int imageNumber)
{   
    static HashMap<int,Image*> emojiCache;
    
    if (emojiCache.contains(imageNumber))
        return emojiCache.get(imageNumber);
    
    char name[30];
    snprintf(name, 29, "%c%c%c%c%c-%04X", 101, 109, 111, 106, 105, imageNumber);

    NSBundle *bundle = [NSBundle bundleForClass:[WebCoreFrameBridge class]];
    NSString *imagePath = [bundle pathForResource:[NSString stringWithUTF8String:name] ofType:@"png"];
    NSData *namedImageData = [NSData dataWithContentsOfFile:imagePath];
    if (namedImageData) {
        Image *image = new BitmapImage;
        image->setData(SharedBuffer::wrapNSData(namedImageData), true);
        if (emojiCache.size() > EMOJI_CACHE_SIZE) {
            deleteAllValues(emojiCache);
            emojiCache.clear(); //primitive mechanism. LRU would be better. <rdar://problem/6265136> make emoji cache LRU
        }
            
        emojiCache.add(imageNumber, image);
        return image;
    }
    return 0;
}
    
void Font::drawGlyphs(GraphicsContext* context, const SimpleFontData* font, const GlyphBuffer& glyphBuffer, int from, int numGlyphs, const FloatPoint& point) const
{
    CGContextRef cgContext = WKGetCurrentGraphicsContext();
    if (font->isImageFont()) {
        if (!context->emojiDrawingEnabled())
            return;
        float advance = 0;

        static Mutex emojiMutex;
        MutexLocker locker(emojiMutex);
        
        for (int i = from; i < from + numGlyphs; i++) {
            const Glyph glyph = glyphBuffer.glyphAt(i);
            
            const int pointSize = font->m_font.m_size;
            const int imageGlyphSize = std::min(pointSize + (pointSize <= 15 ? 2 : 4), 20); // scale images below 16 pt.
            IntRect dstRect;
            dstRect.setWidth(imageGlyphSize);
            dstRect.setHeight(imageGlyphSize);
            dstRect.setX(point.x() + 1 + advance);
            
            // these magic rules place the image glyph vertically as per HI specifications.
            if (font->m_font.m_size >= 26)
                dstRect.setY(point.y() -  20);                
            else if (font->m_font.m_size >= 16)
                dstRect.setY(point.y() -  font->m_font.m_size * 0.35 - 10);
            else
                dstRect.setY(point.y() -  font->m_font.m_size);

            Image *image = smileImage(glyph);
            if (image)
                context->drawImage(image, dstRect);
            advance += glyphBuffer.advanceAt(i);
        }
        return;
    }

    
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
