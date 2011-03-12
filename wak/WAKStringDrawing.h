//
//  WAKStringDrawing.h
//  WebKit
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2009 Apple Inc.  All rights reserved.
//

#ifndef WAKStringDrawing_h
#define WAKStringDrawing_h


#import <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>
#import <GraphicsServices/GraphicsServices.h>

typedef enum {
    // The order of the enum items is important, and it is used for >= comparisions
    WebEllipsisStyleNone = 0, // Old style, no truncation. Doesn't respect the "width" passed to it. Left in for compatability.
    WebEllipsisStyleHead = 1,
    WebEllipsisStyleTail = 2,
    WebEllipsisStyleCenter = 3,
    WebEllipsisStyleClip = 4, // Doesn't really clip, but instad truncates at the last character.
    WebEllipsisStyleWordWrap = 5, // Truncates based on the width/height passed to it.
    WebEllipsisStyleCharacterWrap = 6, // For "drawAtPoint", it is just like WebEllipsisStyleClip, since it doesn't really clip, but truncates at the last character
} WebEllipsisStyle;

typedef enum {
    WebTextAlignmentLeft = 0,
    WebTextAlignmentCenter = 1,
    WebTextAlignmentRight = 2,
} WebTextAlignment;

@protocol WebTextRenderingAttributes

@property(nonatomic, readonly)          GSFontRef font;                             // font the text should be rendered with (defaults to nil)
@property(nonatomic, readonly)          CGFloat lineSpacing;                        // set to specify the line spacing (defaults to 0.0 which indicates the default line spacing)

@property(nonatomic, readonly)          WebEllipsisStyle ellipsisStyle;             // text will be wrapped and truncated according to the line break mode (defaults to UILineBreakModeWordWrap)
@property(nonatomic, readonly)          CGFloat letterSpacing;                      // number of extra pixels to be added or subtracted between each pair of characters  (defalts to 0)
@property(nonatomic, readonly)          WebTextAlignment alignment;                 // specifies the horizontal alignment that should be used when rendering the text (defaults to UITextAlignmentLeft)
@property(nonatomic, readonly)          BOOL includeEmoji;                          // if yes, the text can include Emoji characters.
@property(nonatomic, readwrite)         CGRect truncationRect;                      // the truncation rect argument, if non-nil, will be used instead of an ellipsis character for truncation sizing.  if no truncation occurs, the truncationRect will be changed to CGRectNull.  if truncation occurs, the rect will be updated with its placement.

@property(nonatomic, readonly)         NSString **renderString;                    // An out-parameter for the actual rendered string.  Defaults to nil.

@end

@interface NSString (WebStringDrawing)

+ (void)_web_setWordRoundingEnabled:(BOOL)flag;
+ (BOOL)_web_wordRoundingEnabled;

+ (void)_web_setAscentRoundingEnabled:(BOOL)flag;
+ (BOOL)_web_ascentRoundingEnabled;

- (CGSize)_web_drawAtPoint:(CGPoint)point withFont:(GSFontRef)font;

- (CGSize)_web_sizeWithFont:(GSFontRef)font;

// Size after applying ellipsis style and clipping to width.
- (CGSize)_web_sizeWithFont:(GSFontRef)font forWidth:(float)width ellipsis:(WebEllipsisStyle)ellipsisStyle;
- (CGSize)_web_sizeWithFont:(GSFontRef)font forWidth:(float)width ellipsis:(WebEllipsisStyle)ellipsisStyle letterSpacing:(float)letterSpacing;
- (CGSize)_web_sizeWithFont:(GSFontRef)font forWidth:(float)width ellipsis:(WebEllipsisStyle)ellipsisStyle letterSpacing:(float)letterSpacing resultRange:(NSRange *)resultRangeOut;

// Draw text to fit width.  Clip or apply ellipsis according to style.
- (CGSize)_web_drawAtPoint:(CGPoint)point forWidth:(float)width withFont:(GSFontRef)font ellipsis:(WebEllipsisStyle)ellipsisStyle;
- (CGSize)_web_drawAtPoint:(CGPoint)point forWidth:(float)width withFont:(GSFontRef)font ellipsis:(WebEllipsisStyle)ellipsisStyle letterSpacing:(float)letterSpacing;
- (CGSize)_web_drawAtPoint:(CGPoint)point forWidth:(float)width withFont:(GSFontRef)font ellipsis:(WebEllipsisStyle)ellipsisStyle letterSpacing:(float)letterSpacing includeEmoji:(BOOL)includeEmoji;

// Wrap and clip to rect.
- (CGSize)_web_drawInRect:(CGRect)rect withFont:(GSFontRef)font ellipsis:(WebEllipsisStyle)ellipsisStyle alignment:(WebTextAlignment)alignment;
- (CGSize)_web_drawInRect:(CGRect)rect withFont:(GSFontRef)font ellipsis:(WebEllipsisStyle)ellipsisStyle alignment:(WebTextAlignment)alignment lineSpacing:(int)lineSpacing;
- (CGSize)_web_drawInRect:(CGRect)rect withFont:(GSFontRef)font ellipsis:(WebEllipsisStyle)ellipsisStyle alignment:(WebTextAlignment)alignment lineSpacing:(int)lineSpacing includeEmoji:(BOOL)includeEmoj truncationRect:(CGRect *)truncationRect;
- (CGSize)_web_sizeInRect:(CGRect)rect withFont:(GSFontRef)font ellipsis:(WebEllipsisStyle)ellipsisStyle;
- (CGSize)_web_sizeInRect:(CGRect)rect withFont:(GSFontRef)font ellipsis:(WebEllipsisStyle)ellipsisStyle lineSpacing:(int)lineSpacing;

// Determine the secured version of this string
- (NSString *)_web_securedStringIncludingLastCharacter:(BOOL)includingLastCharacter;

// Clip or apply ellipsis according to style. Return the string which results.
- (NSString *)_web_stringForWidth:(float)width withFont:(GSFontRef)font ellipsis:(WebEllipsisStyle)ellipsisStyle letterSpacing:(float)letterSpacing includeEmoji:(BOOL)includeEmoji;

// These methods should eventually replace all string drawing/sizing methods above

// Sizing/drawing a single line of text
- (CGSize)_web_sizeForWidth:(CGFloat)width withAttributes:(id <WebTextRenderingAttributes>)attributes;
- (CGSize)_web_drawAtPoint:(CGPoint)point forWidth:(CGFloat)width withAttributes:(id <WebTextRenderingAttributes>)attributes;

// Sizing/drawing multiline text
- (CGSize)_web_sizeInRect:(CGRect)rect withAttributes:(id <WebTextRenderingAttributes>)attributes;
- (CGSize)_web_drawInRect:(CGRect)rect withAttributes:(id <WebTextRenderingAttributes>)attributes;

@end


#endif // WAKStringDrawing_h
