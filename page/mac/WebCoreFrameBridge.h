/*
 * Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include <JavaScriptCore/Platform.h>

#import <Foundation/Foundation.h>
#import <GraphicsServices/GSEvent.h>
#import <GraphicsServices/GSFont.h>
#import "WAKView.h"
#import "WAKWindow.h"

#if defined(__cplusplus)
#import <WebCore/WebCoreKeyboardUIMode.h>
#import <WebCore/EditAction.h>
#import <WebCore/FrameLoaderTypes.h>
#import <WebCore/SelectionController.h>
#import <WebCore/TextAffinity.h>
#import <WebCore/TextGranularity.h>
#endif

// FIXME: USE(NPOBJECT) macro expanded to make UIKit compile
#if WTF_USE_NPOBJECT
#import <JavaScriptCore/npruntime.h>
#endif

#if defined(__cplusplus)
namespace WebCore {
    class Frame;
}
#else
@class Frame;
typedef int KeyboardUIMode;
#endif

@class DOMCSSStyleDeclaration;
@class DOMDocument;
@class DOMDocumentFragment;
@class DOMElement;
@class DOMHTMLInputElement;
@class DOMHTMLTextAreaElement;
@class DOMNode;
@class DOMRange;
@class NSMenu;
@class DOMHTMLElement;

@protocol WebCoreRenderTreeCopier;

typedef enum WebCoreDeviceType {
    WebCoreDeviceScreen,
    WebCoreDevicePrinter
} WebCoreDeviceType;

typedef enum WebScrollDirection {
    WebScrollUp,
    WebScrollDown,
    WebScrollLeft,
    WebScrollRight
} WebScrollDirection;

typedef enum WebScrollGranularity {
    WebScrollLine,
    WebScrollPage,
    WebScrollDocument,
    WebScrollWheel
} WebScrollGranularity;

@protocol WebCoreOpenPanelResultListener <NSObject>
- (void)chooseFilename:(NSString *)fileName;
- (void)cancel;
@end

// WebCoreFrameBridge objects are used by WebCore to abstract away operations that need
// to be implemented by library clients, for example WebKit. The objects are also
// used in the opposite direction, for simple access to WebCore functions without dealing
// directly with the KHTML C++ classes.

// A WebCoreFrameBridge creates and holds a reference to a Frame.

// The WebCoreFrameBridge interface contains methods for use by the non-WebCore side of the bridge.

@interface WebCoreFrameBridge : NSObject
{
@public
#if defined(__cplusplus)
    WebCore::Frame* m_frame;
#else
    Frame* m_frame;
#endif
    BOOL _shouldCreateRenderers;
    BOOL _closed;
}

#if defined(__cplusplus)
- (WebCore::Frame*)_frame; // underscore to prevent conflict with -[NSView frame]
#else
- (Frame*)_frame; // underscore to prevent conflict with -[NSView frame]
#endif

+ (WebCoreFrameBridge *)bridgeForDOMDocument:(DOMDocument *)document;

- (id)init;
- (void)close;

- (void)clearFrame;

- (NSURL *)baseURL;

- (void)installInFrame:(NSView *)view;

- (BOOL)scrollOverflowInDirection:(WebScrollDirection)direction granularity:(WebScrollGranularity)granularity;

- (void)createFrameViewWithNSView:(NSView *)view marginWidth:(int)mw marginHeight:(int)mh;

- (void)reapplyStylesForDeviceType:(WebCoreDeviceType)deviceType;
- (void)forceLayoutAdjustingViewSize:(BOOL)adjustSizeFlag;
- (void)forceLayoutWithMinimumPageWidth:(float)minPageWidth maximumPageWidth:(float)maxPageWidth adjustingViewSize:(BOOL)adjustSizeFlag;
- (void)sendOrientationChangeEvent;
- (void)sendScrollEvent;
- (BOOL)needsLayout;
- (void)setNeedsLayout;
- (void)drawRect:(NSRect)rect;
- (void)adjustPageHeightNew:(float *)newBottom top:(float)oldTop bottom:(float)oldBottom limit:(float)bottomLimit;
- (NSArray*)computePageRectsWithPrintWidthScaleFactor:(float)printWidthScaleFactor printHeight:(float)printHeight;
- (CGSize)renderedSizeOfNode:(DOMNode *)node constrainedToWidth:(float)width;

- (void)setUserStyleSheet:(NSString *)styleSheet;

- (NSObject *)copyRenderTree:(id <WebCoreRenderTreeCopier>)copier;
- (NSString *)renderTreeAsExternalRepresentation;

- (NSURL *)URLWithAttributeString:(NSString *)string;

- (DOMNode *)scrollableNodeAtViewportLocation:(CGPoint)aViewportLocation;
- (DOMNode *)approximateNodeAtViewportLocation:(CGPoint *)aViewportLocation;

- (CGRect)renderRectForPoint:(CGPoint)point isReplaced:(BOOL *)isReplaced fontSize:(float *)fontSize;

- (BOOL)hasCustomViewportArguments;
- (void)clearCustomViewportArguments;
- (NSDictionary *)viewportArguments;

- (DOMElement *)elementWithName:(NSString *)name inForm:(DOMElement *)form;
- (NSView *)viewForElement:(DOMElement *)element;
- (BOOL)elementDoesAutoComplete:(DOMElement *)element;
- (BOOL)elementIsPassword:(DOMElement *)element;
- (DOMElement *)formForElement:(DOMElement *)element;
- (DOMElement *)currentForm;
- (NSArray *)controlsInForm:(DOMElement *)form;
- (NSString *)searchForLabels:(NSArray *)labels beforeElement:(DOMElement *)element;
- (NSString *)matchLabels:(NSArray *)labels againstElement:(DOMElement *)element;

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag startInSelection:(BOOL)startInSelection;
- (unsigned)markAllMatchesForText:(NSString *)string caseSensitive:(BOOL)caseFlag limit:(unsigned)limit;
- (BOOL)markedTextMatchesAreHighlighted;
- (void)setMarkedTextMatchesAreHighlighted:(BOOL)doHighlight;
- (void)unmarkAllTextMatches;

- (void)setTextSizeMultiplier:(float)multiplier;

- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)string;
- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)string forceUserGesture:(BOOL)forceUserGesture;

- (NSString *)selectedString;

- (void)revealSelection;
- (DOMRange *)selectedDOMRange;
- (void)setSelectedDOMRange:(DOMRange *)range affinity:(NSSelectionAffinity)affinity closeTyping:(BOOL)closeTyping;
- (void)expandSelectionToElementContainingCaretSelection;
- (DOMRange *)elementRangeContainingCaretSelection;
- (void)expandSelectionToWordContainingCaretSelection;
- (void)expandSelectionToStartOfWordContainingCaretSelection;

- (unichar)characterInRelationToCaretSelection:(int)amount;
- (unichar)characterBeforeCaretSelection;
- (unichar)characterAfterCaretSelection;
- (DOMRange *)wordRangeContainingCaretSelection;
- (DOMRange *)wordRangeContainingCaretSelection;
- (NSString *)wordInRange:(DOMRange *)range;
- (int)wordOffsetInRange:(DOMRange *)range;
- (int)indexCountOfWordInRange:(DOMRange *)range;
- (BOOL)spaceFollowsWordInRange:(DOMRange *)range;
- (NSArray *)wordsInCurrentParagraph;

- (BOOL)selectionAtDocumentStart;
- (BOOL)selectionAtSentenceStart;
- (BOOL)selectionAtWordStart;
- (BOOL)rangeAtSentenceStart:(DOMRange *)range;
- (void)addAutoCorrectMarker:(DOMRange *)range;
- (void)addAutoCorrectMarker:(DOMRange *)range word:(NSString *)word correction:(NSString *)correction;

- (int)preferredHeight;

// Returns the line height of the inner node of a text control.
// For other nodes, the value is the same as lineHeight.
- (int)innerLineHeight:(DOMNode *)node;

- (void)updateLayout;

- (void)setCaretBlinks:(BOOL)flag;
- (void)setCaretVisible:(BOOL)flag;

- (void)clearDocumentContent;

- (void)setIsActive:(BOOL)flag;
- (void)setEmbeddedEditingMode:(BOOL)flag;

- (NSString *)stringForRange:(DOMRange *)range;

- (NSString *)markupStringFromNode:(DOMNode *)node nodes:(NSArray **)nodes;
- (NSString *)markupStringFromRange:(DOMRange *)range nodes:(NSArray **)nodes;

- (void)collapseSelectedDOMRangeWithAffinity:(NSSelectionAffinity)selectionAffinity;
- (NSString *)currentSentence;

- (NSRect)caretRect;
- (NSRect)rectForScrollToVisible; // return caretRect if selection is caret, selectionRect otherwise
- (NSRect)autocorrectionRect;
- (NSRect)caretRectAtNode:(DOMNode *)node offset:(int)offset affinity:(NSSelectionAffinity)affinity;
- (NSRect)firstRectForDOMRange:(DOMRange *)range;
- (void)scrollDOMRangeToVisible:(DOMRange *)range;

- (GSFontRef)fontForSelection:(BOOL *)hasMultipleFonts;

- (NSString *)stringWithData:(NSData *)data; // using the encoding of the frame's main resource
+ (NSString *)stringWithData:(NSData *)data textEncodingName:(NSString *)textEncodingName; // nil for textEncodingName means Latin-1

- (void)setShouldCreateRenderers:(BOOL)shouldCreateRenderers;

- (void)setBaseBackgroundColor:(CGColorRef)backgroundColor;
- (void)setDrawsBackground:(BOOL)drawsBackround;


#if defined(__cplusplus)
- (DOMRange *)rangeByAlteringCurrentSelection:(WebCore::SelectionController::EAlteration)alteration direction:(WebCore::SelectionController::EDirection)direction granularity:(WebCore::TextGranularity)granularity;
- (WebCore::TextGranularity)selectionGranularity;
#endif
- (void)selectNSRange:(NSRange)range;
- (DOMRange *)rangeByMovingCurrentSelection:(int)amount;
- (DOMRange *)rangeByExtendingCurrentSelection:(int)amount;
- (void)selectNSRange:(NSRange)range onElement:(DOMElement *)element;
- (DOMRange *)markedTextDOMRange;
- (void)setMarkedText:(NSString *)text forCandidates:(BOOL)forCandidates;
- (void)confirmMarkedText:(NSString *)text;
- (NSRange)selectedNSRange;
- (NSRange)markedTextNSRange;
- (DOMRange *)convertNSRangeToDOMRange:(NSRange)range;
- (NSRange)convertDOMRangeToNSRange:(DOMRange *)range;

- (DOMDocumentFragment *)documentFragmentWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString;
- (DOMDocumentFragment *)documentFragmentWithText:(NSString *)text inContext:(DOMRange *)context;
- (DOMDocumentFragment *)documentFragmentWithNodesAsParagraphs:(NSArray *)nodes;

- (void)replaceSelectionWithFragment:(DOMDocumentFragment *)fragment selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle;
- (void)replaceSelectionWithNode:(DOMNode *)node selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle;
- (void)replaceSelectionWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace;
- (void)replaceSelectionWithText:(NSString *)text selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace;

- (void)setText:(NSString *)text asChildOfElement:(DOMElement *)element breakLines:(BOOL)breakLines;

- (void)insertParagraphSeparatorInQuotedContent;

- (DOMRange *)characterRangeAtPoint:(NSPoint)point;
- (BOOL)mouseDownMayStartDrag;
- (void)setCaretColor:(CGColorRef)color;
- (void)resetSelection;
- (void)moveSelectionToPoint:(NSPoint)point useSingleLineSelectionBehavior:(BOOL)useSingleLineSelectionBehavior;
- (void)moveSelectionToPoint:(NSPoint)point;
- (void)moveSelectionToStartOrEndOfCurrentWord;

- (DOMCSSStyleDeclaration *)typingStyle;
#if defined(__cplusplus)
- (void)setTypingStyle:(DOMCSSStyleDeclaration *)style withUndoAction:(WebCore::EditAction)undoAction;
#endif


- (BOOL)getData:(NSData **)data andResponse:(NSURLResponse **)response forURL:(NSString *)URL;
- (void)getAllResourceDatas:(NSArray **)datas andResponses:(NSArray **)responses;

- (BOOL)canProvideDocumentSource;
- (BOOL)canSaveAsWebArchive;

- (void)receivedData:(NSData *)data textEncodingName:(NSString *)textEncodingName;

- (void)setLayoutInterval:(int)interval;
- (void)setMaxParseDuration:(CFTimeInterval)duration;
- (void)pauseTimeouts;
- (void)resumeTimeouts;
- (unsigned)formElementsCharacterCount;

#if ENABLE(HW_COMP)
- (BOOL)has3DContent;
// akin to pageZoomFactor; used to scale layer contents
- (float)frameScale;
- (void)setFrameScale:(float)newScale;
#endif

@end

// The WebCoreFrameBridge protocol contains methods for use by the WebCore side of the bridge.

@protocol WebCoreFrameBridge

- (NSView *)documentView;

- (NSWindow *)window;

- (NSResponder *)firstResponder;
- (void)makeFirstResponder:(NSResponder *)responder;

- (void)runOpenPanelForFileButtonWithResultListener:(id <WebCoreOpenPanelResultListener>)resultListener;

- (int)getMaximumImageSize;
- (bool)isTelephoneNumberParsingEnabled;

- (void)handleScrollViewEvent:(GSEventRef)event;

- (void)didReceiveViewportArguments:(NSDictionary *)arguments;
- (void)setNeedsScrollNotifications:(NSNumber *)aFlag;
- (void)didObserveDeferredContentChange;

- (void)didPreventDefaultForEvent:(GSEventRef)event;


- (void)issuePasteCommand;

- (void)setIsSelected:(BOOL)isSelected forView:(NSView *)view;

- (void)didReceiveDocType;
- (char *)windowState;

- (float)minimumZoomFontSize;
- (CGSize)visibleSize;

#if ENABLE(DASHBOARD_SUPPORT)
- (void)dashboardRegionsChanged:(NSMutableDictionary *)regions;
#endif
- (void)eventRegionsChanged:(NSMutableArray *)regions;
- (void)willPopupMenu:(NSMenu *)menu;

#if defined(__cplusplus)
- (NSRect)customHighlightRect:(NSString*)type forLine:(NSRect)lineRect representedNode:(WebCore::Node *)node;
- (void)paintCustomHighlight:(NSString*)type forBox:(NSRect)boxRect onLine:(NSRect)lineRect behindText:(BOOL)text entireLine:(BOOL)line representedNode:(WebCore::Node *)node;
#endif

- (BOOL)shouldCacheDecodedImages;

#if defined(__cplusplus)
- (WebCore::KeyboardUIMode)keyboardUIMode;
#endif

- (NSString*)imageTitleForFilename:(NSString*)filename size:(NSSize)size;

- (void)caretChanged;

- (BOOL)isLoading;

- (int)rotationDegrees;

// only used when ENABLE(HW_COMP) is defined
- (id)contentsHostingLayer;

@end

// This interface definition allows those who hold a WebCoreFrameBridge * to call all the methods
// in the WebCoreFrameBridge protocol without requiring the base implementation to supply the methods.
// This idiom is appropriate because WebCoreFrameBridge is an abstract class.

@interface WebCoreFrameBridge (SubclassResponsibility) <WebCoreFrameBridge>
@end

// Protocols that make up part of the interfaces above.

@protocol WebCoreRenderTreeCopier <NSObject>
- (NSObject *)nodeWithName:(NSString *)name position:(NSPoint)p rect:(NSRect)rect view:(NSView *)view children:(NSArray *)children;
@end
