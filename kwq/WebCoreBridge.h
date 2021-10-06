/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
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

#import <Foundation/Foundation.h>

#import <WebCore/WebCoreKeyboardAccess.h>

#import <JavaVM/jni.h>

#ifdef __cplusplus

class KWQKHTMLPart;
class KHTMLView;
class RenderArena;

namespace khtml {
    class RenderPart;
    class RenderObject;
}

typedef khtml::RenderPart KHTMLRenderPart;

#else

@class KWQKHTMLPart;
@class KHTMLView;
@class KHTMLRenderPart;
@class RenderArena;

#endif

@class WebCoreSettings;

@protocol WebCoreDOMTreeCopier;
@protocol WebCoreRenderTreeCopier;
@protocol WebCoreResourceHandle;
@protocol WebCoreResourceLoader;
@protocol WebCoreFileButton;
@protocol WebCoreFileButtonDelegate;
@protocol WebDOMDocument;
@protocol WebDOMNode;
@protocol WebDOMElement;

extern NSString *WebCoreElementFrameKey;
extern NSString *WebCoreElementImageAltStringKey;
extern NSString *WebCoreElementImageKey;
extern NSString *WebCoreElementImageRectKey;
extern NSString *WebCoreElementImageURLKey;
extern NSString *WebCoreElementIsSelectedKey;
extern NSString *WebCoreElementLinkURLKey;
extern NSString *WebCoreElementLinkTargetFrameKey;
extern NSString *WebCoreElementLinkLabelKey;
extern NSString *WebCoreElementLinkTitleKey;
extern NSString *WebCoreElementTitleKey;

extern NSString *WebCorePageCacheStateKey;

typedef enum {
    WebCoreDeviceScreen,
    WebCoreDevicePrinter
} WebCoreDeviceType;

// WebCoreBridge objects are used by WebCore to abstract away operations that need
// to be implemented by library clients, for example WebKit. The objects are also
// used in the opposite direction, for simple access to WebCore functions without dealing
// directly with the KHTML C++ classes.

// A WebCoreBridge creates and holds a reference to a KHTMLPart.

// The WebCoreBridge interface contains methods for use by the non-WebCore side of the bridge.

@interface WebCoreBridge : NSObject
{
    KWQKHTMLPart *_part;
    KHTMLRenderPart *_renderPart;
    RenderArena *_renderPartArena;
    BOOL _drawSelectionOnly;
    BOOL _shouldCreateRenderers;
}

- (void)initializeSettings:(WebCoreSettings *)settings;

- (void)setRenderPart:(KHTMLRenderPart *)renderPart;
- (KHTMLRenderPart *)renderPart;

- (void)setName:(NSString *)name;
- (NSString *)name;

- (KWQKHTMLPart *)part;

- (void)setParent:(WebCoreBridge *)parent;

- (void)provisionalLoadStarted;

- (void)openURL:(NSURL *)URL reload:(BOOL)reload
    contentType:(NSString *)contentType refresh:(NSString *)refresh lastModified:(NSDate *)lastModified
    pageCache:(NSDictionary *)pageCache;
- (void)setEncoding:(NSString *)encoding userChosen:(BOOL)userChosen;
- (void)addData:(NSData *)data;
- (void)closeURL;

- (void)didNotOpenURL:(NSURL *)URL;

- (void)saveDocumentState;
- (void)restoreDocumentState;

- (BOOL)canCachePage;
- (BOOL)saveDocumentToPageCache;

- (void)end;

- (NSURL *)URL;
- (NSString *)referrer;

- (void)installInFrame:(NSView *)view;
- (void)removeFromFrame;

- (void)scrollToAnchor:(NSString *)anchor;
- (void)scrollToAnchorWithURL:(NSURL *)URL;

- (void)createKHTMLViewWithNSView:(NSView *)view marginWidth:(int)mw marginHeight:(int)mh;

- (BOOL)isFrameSet;

- (void)reapplyStylesForDeviceType:(WebCoreDeviceType)deviceType;
- (void)forceLayoutAdjustingViewSize:(BOOL)adjustSizeFlag;
- (void)forceLayoutWithMinimumPageWidth:(float)minPageWidth maximumPageWidth:(float)maxPageWidth adjustingViewSize:(BOOL)adjustSizeFlag;
- (void)sendResizeEvent;
- (void)sendScrollEvent;
- (BOOL)needsLayout;
- (void)setNeedsLayout;
- (void)drawRect:(NSRect)rect;
- (void)adjustPageHeightNew:(float *)newBottom top:(float)oldTop bottom:(float)oldBottom limit:(float)bottomLimit;
- (NSArray*)computePageRectsWithPrintWidthScaleFactor:(float)printWidthScaleFactor printHeight:(float)printHeight;

- (void)setUsesInactiveTextBackgroundColor:(BOOL)uses;
- (BOOL)usesInactiveTextBackgroundColor;

- (void)setShowsFirstResponder:(BOOL)flag;

- (void)mouseDown:(NSEvent *)event;
- (void)mouseUp:(NSEvent *)event;
- (void)mouseMoved:(NSEvent *)event;
- (void)mouseDragged:(NSEvent *)event;

- (BOOL)sendContextMenuEvent:(NSEvent *)event; // return YES if event is eaten by WebCore

- (NSView *)nextKeyView;
- (NSView *)previousKeyView;

- (NSView *)nextKeyViewInsideWebFrameViews;
- (NSView *)previousKeyViewInsideWebFrameViews;

- (NSObject *)copyDOMTree:(id <WebCoreDOMTreeCopier>)copier;
- (NSObject *)copyRenderTree:(id <WebCoreRenderTreeCopier>)copier;
- (NSString *)renderTreeAsExternalRepresentation;

- (NSDictionary *)elementAtPoint:(NSPoint)point;
- (id <WebDOMElement>)elementWithName:(NSString *)name inForm:(id <WebDOMElement>)form;
- (id <WebDOMElement>)elementForView:(NSView *)view;
- (BOOL)elementDoesAutoComplete:(id <WebDOMElement>)element;
- (BOOL)elementIsPassword:(id <WebDOMElement>)element;
- (id <WebDOMElement>)formForElement:(id <WebDOMElement>)element;
- (id <WebDOMElement>)currentForm;
- (NSArray *)controlsInForm:(id <WebDOMElement>)form;
- (NSString *)searchForLabels:(NSArray *)labels beforeElement:(id <WebDOMElement>)element;
- (NSString *)matchLabels:(NSArray *)labels againstElement:(id <WebDOMElement>)element;

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag;
- (void)jumpToSelection;

- (void)setTextSizeMultiplier:(float)multiplier;

- (CFStringEncoding)textEncoding;

- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)string;

- (id <WebDOMDocument>)DOMDocument;

- (void)setSelectionFrom:(id <WebDOMNode>)start startOffset:(int)startOffset to:(id <WebDOMNode>)end endOffset:(int) endOffset;

- (NSString *)selectedString;
- (NSAttributedString *)selectedAttributedString;

- (void)selectAll;
- (void)deselectAll;
- (void)deselectText;

- (NSRect)selectionRect;
- (NSRect)visibleSelectionRect;
- (NSImage *)selectionImage;

- (id <WebDOMNode>)selectionStart;
- (int)selectionStartOffset;
- (id <WebDOMNode>)selectionEnd;
- (int)selectionEndOffset;

- (NSAttributedString *)attributedStringFrom:(id <WebDOMNode>)startNode startOffset:(int)startOffset to:(id <WebDOMNode>)endNode endOffset:(int)endOffset;

+ (NSString *)stringWithData:(NSData *)data textEncoding:(CFStringEncoding)textEncoding;
+ (NSString *)stringWithData:(NSData *)data textEncodingName:(NSString *)textEncodingName;

- (BOOL)interceptKeyEvent:(NSEvent *)event toView:(NSView *)view;

- (void)setShouldCreateRenderers:(BOOL)f;
- (BOOL)shouldCreateRenderers;

- (int)numPendingOrLoadingRequests;

- (NSColor *)bodyBackgroundColor;

- (void)adjustViewSize;

+ (void)updateAllViews;

- (id)accessibilityTree;

@end

// The WebCoreBridge protocol contains methods for use by the WebCore side of the bridge.

@protocol WebCoreBridge

- (NSArray *)childFrames; // WebCoreBridge objects
- (WebCoreBridge *)mainFrame;
- (WebCoreBridge *)findFrameNamed:(NSString *)name;
/* Creates a name for an frame unnamed in the HTML.  It should produce repeatable results for loads of the same frameset. */
- (NSString *)generateFrameName;
- (void)frameDetached;
- (NSView *)documentView;

- (void)loadURL:(NSURL *)URL referrer:(NSString *)referrer reload:(BOOL)reload onLoadEvent:(BOOL)onLoad target:(NSString *)target triggeringEvent:(NSEvent *)event form:(NSObject <WebDOMElement> *)form formValues:(NSDictionary *)values;
- (void)postWithURL:(NSURL *)URL referrer:(NSString *)referrer target:(NSString *)target data:(NSData *)data contentType:(NSString *)contentType triggeringEvent:(NSEvent *)event form:(NSObject <WebDOMElement> *)form formValues:(NSDictionary *)values;

- (WebCoreBridge *)createWindowWithURL:(NSURL *)URL frameName:(NSString *)name;
- (void)showWindow;

- (NSString *)userAgentForURL:(NSURL *)URL;

- (void)setTitle:(NSString *)title;
- (void)setStatusText:(NSString *)status;

- (void)setIconURL:(NSURL *)URL;
- (void)setIconURL:(NSURL *)URL withType:(NSString *)string;

- (WebCoreBridge *)createChildFrameNamed:(NSString *)frameName withURL:(NSURL *)URL
    renderPart:(KHTMLRenderPart *)renderPart
    allowsScrolling:(BOOL)allowsScrolling marginWidth:(int)width marginHeight:(int)height;

- (BOOL)areToolbarsVisible;
- (void)setToolbarsVisible:(BOOL)visible;
- (BOOL)isStatusBarVisible;
- (void)setStatusBarVisible:(BOOL)visible;
- (BOOL)areScrollbarsVisible;
- (void)setScrollbarsVisible:(BOOL)visible;
- (NSWindow *)window;
- (void)setWindowFrame:(NSRect)frame;
- (NSRect)windowFrame;
- (void)setWindowContentRect:(NSRect)frame;
- (NSRect)windowContentRect;

- (void)setWindowIsResizable:(BOOL)resizable;
- (BOOL)windowIsResizable;

- (NSResponder *)firstResponder;
- (void)makeFirstResponder:(NSResponder *)view;

- (void)closeWindowSoon;

- (void)runJavaScriptAlertPanelWithMessage:(NSString *)message;
- (BOOL)runJavaScriptConfirmPanelWithMessage:(NSString *)message;
- (BOOL)runJavaScriptTextInputPanelWithPrompt:(NSString *)prompt defaultText:(NSString *)defaultText returningText:(NSString **)result;

- (id <WebCoreResourceHandle>)startLoadingResource:(id <WebCoreResourceLoader>)loader withURL:(NSURL *)URL customHeaders:(NSDictionary *)customHeaders;
- (id <WebCoreResourceHandle>)startLoadingResource:(id <WebCoreResourceLoader>)loader withURL:(NSURL *)URL customHeaders:(NSDictionary *)customHeaders postData:(NSData *)data;
- (void)objectLoadedFromCacheWithURL:(NSURL *)URL response:(id)response size:(unsigned)bytes;

- (NSData *)syncLoadResourceWithURL:(NSURL *)URL customHeaders:(NSDictionary *)requestHeaders postData:(NSData *)postData finalURL:(NSURL **)finalNSURL responseHeaders:(NSDictionary **)responseHeaderDict statusCode:(int *)statusCode;

- (BOOL)isReloading;
- (time_t)expiresTimeForResponse:(NSURLResponse *)response;

- (void)reportClientRedirectToURL:(NSURL *)URL delay:(NSTimeInterval)seconds fireDate:(NSDate *)date lockHistory:(BOOL)lockHistory isJavaScriptFormAction:(BOOL)isJavaScriptFormAction;
- (void)reportClientRedirectCancelled:(BOOL)cancelWithLoadInProgress;

- (void)focusWindow;
- (void)unfocusWindow;

- (NSView *)nextKeyViewOutsideWebFrameViews;
- (NSView *)previousKeyViewOutsideWebFrameViews;

- (BOOL)defersLoading;
- (void)setDefersLoading:(BOOL)loading;
- (void)saveDocumentState:(NSArray *)documentState;
- (NSArray *)documentState;

- (void)setNeedsReapplyStyles;

// OK to be an NSString rather than an NSURL.
// This URL is only used for coloring visited links.
- (NSString *)requestedURLString;
- (NSString *)incomingReferrer;

- (NSView *)viewForPluginWithURL:(NSURL *)URL
                      attributes:(NSArray *)attributesArray
                         baseURL:(NSURL *)baseURL
                        MIMEType:(NSString *)MIMEType;
- (NSView *)viewForJavaAppletWithFrame:(NSRect)frame
                            attributes:(NSDictionary *)attributes
                               baseURL:(NSURL *)baseURL;

- (BOOL)saveDocumentToPageCache:(id)documentInfo;

- (int)getObjectCacheSize;

- (BOOL)frameRequiredForMIMEType:(NSString*)MIMEType URL:(NSURL *)URL;

- (void)loadEmptyDocumentSynchronously;

- (NSString *)MIMETypeForPath:(NSString *)path;

- (void)handleMouseDragged:(NSEvent *)event;
- (void)handleAutoscrollForMouseDragged:(NSEvent *)event;
- (BOOL)mayStartDragWithMouseDragged:(NSEvent *)event;

- (int)historyLength;
- (void)goBackOrForward:(int)distance;

- (void)controlTextDidBeginEditing:(NSNotification *)obj;
- (void)controlTextDidEndEditing:(NSNotification *)obj;
- (void)controlTextDidChange:(NSNotification *)obj;

- (BOOL)control:(NSControl *)control textShouldBeginEditing:(NSText *)fieldEditor;
- (BOOL)control:(NSControl *)control textShouldEndEditing:(NSText *)fieldEditor;
- (BOOL)control:(NSControl *)control didFailToFormatString:(NSString *)string errorDescription:(NSString *)error;
- (void)control:(NSControl *)control didFailToValidatePartialString:(NSString *)string errorDescription:(NSString *)error;
- (BOOL)control:(NSControl *)control isValidObject:(id)obj;
- (BOOL)control:(NSControl *)control textView:(NSTextView *)textView doCommandBySelector:(SEL)commandSelector;

- (NSView <WebCoreFileButton> *)fileButtonWithDelegate:(id <WebCoreFileButtonDelegate>)delegate;

- (void)setHasBorder:(BOOL)hasBorder;

- (WebCoreKeyboardUIMode)keyboardUIMode;

- (void)didSetName:(NSString *)name;

- (NSFileWrapper *)fileWrapperForURL:(NSURL *)URL;

- (void)print;

- (jobject)pollForAppletInView:(NSView *)view;

@end

// This interface definition allows those who hold a WebCoreBridge * to call all the methods
// in the WebCoreBridge protocol without requiring the base implementation to supply the methods.
// This idiom is appropriate because WebCoreBridge is an abstract class.

@interface WebCoreBridge (SubclassResponsibility) <WebCoreBridge>
@end

@protocol WebCoreDOMTreeCopier <NSObject>
- (NSObject *)nodeWithName:(NSString *)name value:(NSString *)value source:(NSString *)source children:(NSArray *)children;
@end

@protocol WebCoreRenderTreeCopier <NSObject>
- (NSObject *)nodeWithName:(NSString *)name position:(NSPoint)p rect:(NSRect)rect view:(NSView *)view children:(NSArray *)children;
@end

@protocol WebCoreFileButton <NSObject>
- (void)setFilename:(NSString *)filename;
- (void)performClick;
- (NSString *)filename;
- (float)baseline;
- (void)setVisualFrame:(NSRect)rect;
- (NSRect)visualFrame;
- (NSSize)bestVisualFrameSizeForCharacterCount:(int)count;
@end

@protocol WebCoreFileButtonDelegate <NSObject>
- (void)filenameChanged:(NSString *)filename;
- (void)focusChanged:(BOOL)nowHasFocus;
- (void)clicked;
@end
