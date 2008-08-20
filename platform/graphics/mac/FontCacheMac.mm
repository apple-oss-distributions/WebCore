/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#import "FontCache.h"

#import "Font.h"
#import "SimpleFontData.h"
#import "FontPlatformData.h"
#import "WebCoreSystemInterface.h"
#import "WebFontCache.h"
#import "CharacterNames.h"

#ifdef BUILDING_ON_TIGER
typedef int NSInteger;
#endif

namespace WebCore {


#define MINIMUM_GLYPH_CACHE_SIZE 1536 * 1024

void FontCache::platformInit()
{
	size_t s = 1024*1024;

    wkSetUpFontCache(s);
}

const SimpleFontData* FontCache::getFontDataForCharacters(const Font& font, const UChar* characters, int length)
{
    //printf("Purple looks for font for unicode char %u\n", *characters);
    // Unlike OS X, our fallback font on Purple is Arial Unicode, which doesn't have some apple-specific glyphs like F8FF.
    // Fall back to the Apple Fallback font in this case.
    if (length > 0 && requiresCustomFallbackFont(*characters))
        return getCachedFontData(getCustomFallbackFont(*characters, font));

    UChar32 c = *characters;
    if (length > 1 && U16_IS_LEAD(c) && U16_IS_TRAIL(characters[1]))
        c = U16_GET_SUPPLEMENTARY(c, characters[1]);

    bool useCJFont = false;
    bool useKoreanFont = false;
    bool useCyrillicFont = false;
    if (length > 0) {
        do {
            // This isn't a loop but a way to efficiently check for ranges of characters.

            // The following ranges are Korean Hangul and should be rendered by Apple Gothic
            // U+1100 - U+11FF
            // U+3130 - U+318F
            // U+AC00 - U+D7A3

            // These are Cyrillic and should be rendered by Helvetica Neue
            // U+0400 - U+052F
            
            if (c < 0x400)
                break;
            if (c <= 0x52F)
                useCyrillicFont = true;
            if (c < 0x1100)
                break;
            if (c <= 0x11FF) {
                useKoreanFont = true;
                break;
            }
            if (c < 0x2E80)
                break;
            if (c < 0x3130) {
                useCJFont = true;
                break;
            }
            if (c <= 0x318F) {
                useKoreanFont = true;
                break;
            }
            if (c < 0xAC00) {
                useCJFont = true;
                break;
            }
            if (c <= 0xD7A3) {
                useKoreanFont = true;
                break;
            }
            if ( c <= 0xDFFF) {
                useCJFont = true;
                break;
            }
            if (c <= 0xF8FF)
                break;
            if (c < 0xFFF0) {
                useCJFont = true;
                break;
            }
            if (c >=0x20000 && c <= 0x2FFFF)
                useCJFont = true;
        } while (0);
    }
        
	FontPlatformData* platformFont = NULL;
    
	if (useCJFont) {
		// by default, Chinese font is preferred,  fall back on Japanese
		
		enum {
			kCJKFontUseHiragino	= 0,
			kCJKFontUseSTHeitiSC,
			kCJKFontUseSTHeitiTC,
			kCJKFontUseSTHeitiJ,
			kCJKFontUseSTHeitiK,
			kCJKFontsUseHKGPW3UI
		};
		static AtomicString cjkPlain[] = { 	
            AtomicString("HiraKakuProN-W3"),
            AtomicString("STHeitiSC-Light"),
            AtomicString("STHeitiTC-Light"),
            AtomicString("STHeitiJ-Light"),
            AtomicString("STHeitiK-Light"),
            AtomicString("HKGPW3UI")};
        
		static AtomicString cjkBold[] = { 	
            AtomicString("HiraKakuProN-W6"),
            AtomicString("STHeitiSC-Medium"),
            AtomicString("STHeitiTC-Medium"),
            AtomicString("STHeitiJ-Medium"),
            AtomicString("STHeitiK-Medium"),
            AtomicString("HKGPW3UI")};
		
		//default below is for Simplified Chinese user: zh-Hans - note that Hiragino is the
		//the secondary font as we want its for Hiragana and Katakana. The other CJK fonts
		//do not, and should not, contain Hiragana or Katakana glyphs.
		static int	preferredCJKFont=kCJKFontUseSTHeitiSC;
		static int	secondaryCJKFont=kCJKFontsUseHKGPW3UI;
		
		static int CJKFontInitialized = 0;
		if ( CJKFontInitialized == 0 ) {
			CJKFontInitialized = 1;
			// Testing: languageName = (CFStringRef)@"ja";
			NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
			NSArray *languages = [defaults stringArrayForKey:@"AppleLanguages"];
			
            if( languages ) {
                CFStringRef languageName = NULL;
                for( NSString *language in languages) {
                    languageName = CFLocaleCreateCanonicalLanguageIdentifierFromString(NULL, (CFStringRef)language);
                    if ( CFEqual(languageName, CFSTR("zh-Hans")) )
                        break;                                  //Simplified Chinese - default settings
                    else if ( CFEqual(languageName, CFSTR("ja")) ) {	
                        preferredCJKFont=kCJKFontUseHiragino;	//japanese - prefer Hiragino and STHeiti Japanse Variant
                        secondaryCJKFont=kCJKFontUseSTHeitiJ;
                        break;
                    } else if ( CFEqual(languageName, CFSTR("ko")) ) {	
                        preferredCJKFont=kCJKFontUseSTHeitiK;	//korean - prefer STHeiti Korean Variant 
                        break;
                    } else if ( CFEqual(languageName, CFSTR("zh-Hant")) ) {
                        preferredCJKFont=kCJKFontUseSTHeitiTC;	//Traditional Chinese - prefer STHeiti Traditional Variant
                        break;
                    }
                    CFRelease(languageName);
                    languageName = NULL;
                }
                if( languageName )
                    CFRelease(languageName);
            }
		}
        
		platformFont = getCachedFontPlatformData(font.fontDescription(), font.fontDescription().bold() ? cjkBold[preferredCJKFont] : cjkPlain[preferredCJKFont]);
		
		CGGlyph glyphs[2];
		// CGFontGetGlyphsForUnichars takes UTF-16 buffer. Should only be 1 codepoint but since we may pass in two UTF-16 characters,
		// make room for 2 glyphs just to be safe.
		GSFontGetGlyphsForUnichars(platformFont->font(), characters, glyphs, length);
		
		if ( glyphs[0] == 0 )
			platformFont = getCachedFontPlatformData(font.fontDescription(), font.fontDescription().bold() ? cjkBold[secondaryCJKFont] : cjkPlain[secondaryCJKFont]);
    } else if (useKoreanFont) {
        static AtomicString koreanFont("AppleGothic");
        platformFont = getCachedFontPlatformData(font.fontDescription(), koreanFont);    
    } else if (useCyrillicFont) {
        static AtomicString cyrillicPlain("HelveticaNeue");
        static AtomicString cyrillicBold("HelveticaNeue-Bold");
        platformFont = getCachedFontPlatformData(font.fontDescription(), font.fontDescription().bold() ? cyrillicBold : cyrillicPlain);
    }

	if ( platformFont != NULL )
		return getCachedFontData(platformFont);

    return getCachedFontData(getLastResortFallbackFont(font.fontDescription()));
}

FontPlatformData* FontCache::getSimilarFontPlatformData(const Font& font)
{
    // Attempt to find an appropriate font using a match based on 
    // the presence of keywords in the the requested names.  For example, we'll
    // match any name that contains "Arabic" to Geeza Pro.
    FontPlatformData* platformData = 0;
    const FontFamily* currFamily = &font.fontDescription().family();
    while (currFamily && !platformData) {
        if (currFamily->family().length()) {
            static String matchWords[3] = { String("Arabic"), String("Pashto"), String("Urdu") };
            static AtomicString geezaStr("Geeza Pro");
            for (int j = 0; j < 3 && !platformData; ++j)
                if (currFamily->family().contains(matchWords[j], false))
                    platformData = getCachedFontPlatformData(font.fontDescription(), geezaStr);
        }
        currFamily = currFamily->next();
    }

    return platformData;
}

FontPlatformData* FontCache::getLastResortFallbackFont(const FontDescription& fontDescription)
{
    static AtomicString arialUnicodeMSStr("ArialUnicodeMS");
    FontPlatformData* platformFont = getCachedFontPlatformData(fontDescription, arialUnicodeMSStr);    
    return platformFont;
}

    
FontPlatformData* FontCache::getCustomFallbackFont(const UInt32 c, const Font& font)
{
    assert(requiresCustomFallbackFont(c));
    
    if (c == AppleLogo) {
        static AtomicString helveticaStr("Helvetica");
        FontPlatformData* platformFont = getCachedFontPlatformData(font.fontDescription(), helveticaStr);
        return platformFont;
    } else if (c == BigDot) {
        static AtomicString lockClockStr("LockClock-Light");
        FontPlatformData* platformFont = getCachedFontPlatformData(font.fontDescription(), lockClockStr);
        return platformFont;
    }
    return NULL;
}
    
    
inline bool FontCache::requiresCustomFallbackFont(const UInt32 c)
{
    return (c == AppleLogo ||
            c == BigDot);
}

    

bool FontCache::fontExists(const FontDescription& fontDescription, const AtomicString& family)
{
    GSFontTraitMask traits = 0;
    if (fontDescription.italic())
        traits |= GSItalicFontMask;
    if (fontDescription.bold())
        traits |= GSBoldFontMask;
    float size = fontDescription.computedPixelSize();
    
    GSFontRef gsFont= [WebFontCache createFontWithFamily:family traits:traits size:size];
    return gsFont != 0;
}

FontPlatformData* FontCache::createFontPlatformData(const FontDescription& fontDescription, const AtomicString& family)
{
    // Special case for "Courier" font. We have only an oblique variant on Purple,
    // so disallow its use here. We'll fall back on "Courier New". 5116477.
    static AtomicString courier("Courier");
    if (equalIgnoringCase(family, courier))
        return 0;    
    
    GSFontTraitMask traits = 0;
    if (fontDescription.italic())
        traits |= GSItalicFontMask;
    if (fontDescription.bold())
        traits |= GSBoldFontMask;
    float size = fontDescription.computedPixelSize();
	
	GSFontRef gsFont= [WebFontCache createFontWithFamily: family traits: traits size: size];
	if (!gsFont)
		return 0;
	
    GSFontTraitMask actualTraits = 0;
    if (fontDescription.bold() || fontDescription.italic())
        actualTraits = GSFontGetTraits(gsFont);
    
    FontPlatformData* result = new FontPlatformData;
    
    // Use the correct font for print vs. screen.
    result->setFont(gsFont);
    result->m_syntheticBold = (traits & GSBoldFontMask) && !(actualTraits & GSBoldFontMask);
    result->m_syntheticOblique = (traits & GSItalicFontMask) && !(actualTraits & GSItalicFontMask);	
    return result;
}

} // namespace WebCore
