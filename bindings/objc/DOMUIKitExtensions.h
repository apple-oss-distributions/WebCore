//
//  DOMUIKitExtensions.h
//  WebCore
//
//  Copyright (C) 2007, 2008, Apple Inc.  All rights reserved.
//


//#import <WebCore/DOMCore.h>
//#import <WebCore/DOMHTML.h>

@interface DOMNode (UIKitExtensions)
- (NSArray *)borderRadii;
- (NSArray *)boundingBoxes;
- (NSArray *)absoluteQuads;     // return array of WKQuadObjects. takes transforms into account
- (CGPoint)convertToPage:(CGPoint)pt;
@end

@interface DOMHTMLAreaElement (UIKitExtensions)
- (CGRect)boundingBoxWithOwner:(DOMNode *)anOwner;
- (WKQuad)absoluteQuadWithOwner:(DOMNode *)anOwner;     // takes transforms into account
- (NSArray *)boundingBoxesWithOwner:(DOMNode *)anOwner;
- (NSArray *)absoluteQuadsWithOwner:(DOMNode *)anOwner; // return array of WKQuadObjects. takes transforms into account
@end

@interface DOMHTMLSelectElement (UIKitExtensions)
- (unsigned)completeLength;
- (DOMNode *)listItemAtIndex:(int)anIndex;
@end

typedef enum
{
    DOMHTMLTextEntryAssistanceUnspecified,
    DOMHTMLTextEntryAssistanceOn,
    DOMHTMLTextEntryAssistanceOff
} DOMHTMLTextEntryAssistance;

@interface DOMHTMLInputElement (DOMHTMLTextEntryAssistanceInternal)
- (DOMHTMLTextEntryAssistance)autocorrect;
- (DOMHTMLTextEntryAssistance)autocapitalize;
@end

@interface DOMHTMLTextAreaElement (DOMHTMLTextEntryAssistanceInternal)
- (DOMHTMLTextEntryAssistance)autocorrect;
- (DOMHTMLTextEntryAssistance)autocapitalize;
@end

@interface DOMHTMLImageElement (WebDOMHTMLImageElementOperationsPrivate)
- (NSData *)createNSDataRepresentation;
@end

