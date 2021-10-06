/*
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2005, 2006 Alexey Proskuryakov (ap@nypop.com)
 * Copyright (C) 2006 David Smith (catfish.man@gmail.com)
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

#import "config.h"
#import "WebCoreFrameBridge.h"

#import "AXObjectCache.h"
#import "CSSHelper.h"
#import "Cache.h"
#import "ColorMac.h"
#import "DOMImplementation.h"
#import "DOMInternal.h"
#import "DOMWindow.h"
#import "DeleteSelectionCommand.h"
#import "DocLoader.h"
#import "DocumentFragment.h"
#import "DocumentLoader.h"
#import "DocumentType.h"
#import "Editor.h"
#import "EditorClient.h"
#import "EventHandler.h"
#import "FloatRect.h"
#import "FormDataStreamMac.h"
#import "Frame.h"
#import "FrameLoader.h"
#import "FrameLoaderClient.h"
#import "FrameTree.h"
#import "FrameView.h"
#import "GraphicsContext.h"
#import "HTMLDocument.h"
#import "HTMLFormElement.h"
#import "HTMLInputElement.h"
#import "HTMLNames.h"
#import "HitTestResult.h"
#import "Image.h"
#import "LoaderNSURLExtras.h"
#import "MoveSelectionCommand.h"
#import "Page.h"
#import "PlatformMouseEvent.h"
#import "PlatformScreen.h"
#import "PluginInfoStore.h"
#import "RenderImage.h"
#import "RenderPart.h"
#import "RenderTreeAsText.h"
#import "RenderView.h"
#import "RenderWidget.h"
#import "ReplaceSelectionCommand.h"
#import "ResourceRequest.h"
#import "SelectionController.h"
#import "SimpleFontData.h"
#import "SmartReplace.h"
#import "SubresourceLoader.h"
#import "SystemTime.h"
#import "Text.h"
#import "TextEncoding.h"
#import "TextIterator.h"
#import "TextResourceDecoder.h"
#import "TypingCommand.h"
#import "WebCoreSystemInterface.h"
#import "WebCoreViewFactory.h"
#import "XMLTokenizer.h"
#import "htmlediting.h"
#import "kjs_proxy.h"
#import "kjs_window.h"
#import "markup.h"
#import "visible_units.h"
#import <JavaScriptCore/array_instance.h>
#import <JavaScriptCore/date_object.h>
#import <JavaScriptCore/runtime_root.h>
#import <wtf/RetainPtr.h>

#import <GraphicsServices/GSColor.h>
#import "WKGraphics.h"
#import "CompositeEditCommand.h"
#import "AppendNodeCommand.h"
#import "RemoveNodeCommand.h"
#import "EditingText.h"
#import "Text.h"
#import "WebCoreThreadMessage.h"
#import "WKWindowPrivate.h"
#import "ThreadSafeWrapper.h"
#import "Document.h"
#import "Node.h"
#import "FocusController.h"

@class NSView;

using namespace std;
using namespace WebCore;
using namespace HTMLNames;

using KJS::ArrayInstance;
using KJS::BooleanType;
using KJS::DateInstance;
using KJS::ExecState;
using KJS::GetterSetterType;
using KJS::JSImmediate;
using KJS::JSLock;
using KJS::JSObject;
using KJS::JSValue;
using KJS::NullType;
using KJS::NumberType;
using KJS::ObjectType;
using KJS::SavedBuiltins;
using KJS::SavedProperties;
using KJS::StringType;
using KJS::UndefinedType;
using KJS::UnspecifiedType;
using KJS::Window;

using KJS::Bindings::RootObject;

static PassRefPtr<RootObject> createRootObject(void* nativeHandle)
{
    NSView *view = (NSView *)nativeHandle;
    WebCoreFrameBridge *bridge = [[WebCoreViewFactory sharedFactory] bridgeForView:view];
    if (!bridge)
        return 0;

    Frame* frame = [bridge _frame];
    return frame->createRootObject(nativeHandle, frame->scriptProxy()->globalObject());
}

static pthread_t mainThread = 0;

static void updateRenderingForBindings(ExecState* exec, JSObject* rootObject)
{
    if (pthread_self() != mainThread)
        return;
        
    if (!rootObject)
        return;
        
    Window* window = static_cast<Window*>(rootObject);
    if (!window)
        return;

    if (Frame* frame = window->impl()->frame())
        if (Document* doc = frame->document())
            doc->updateRendering();
}

static BOOL frameHasSelection(WebCoreFrameBridge *bridge)
{
    if (!bridge)
        return NO;
    
    Frame* frame = [bridge _frame];
    if (!frame)
        return NO;
    
    if (frame->selectionController()->selection().isNone())
        return NO;
    
    // If a part has a selection, it should also have a document.        
    ASSERT(frame->document());
    
    return YES;
}


@implementation WebCoreFrameBridge

static inline WebCoreFrameBridge *bridge(Frame *frame)
{
    if (!frame)
        return nil;
    return frame->bridge();
}

- (NSString *)domain
{
    Document *doc = m_frame->document();
    if (doc)
        return doc->domain();
    return nil;
}

+ (WebCoreFrameBridge *)bridgeForDOMDocument:(DOMDocument *)document
{
    return bridge([document _document]->frame());
}

- (id)init
{
    static bool initializedKJS;
    if (!initializedKJS) {
        initializedKJS = true;

        mainThread = pthread_self();
        RootObject::setCreateRootObject(createRootObject);
        KJS::Bindings::Instance::setDidExecuteFunction(updateRenderingForBindings);
    }
    
    if (!(self = [super init]))
        return nil;

    _shouldCreateRenderers = YES;
    return self;
}

- (void)dealloc
{
    ASSERT(_closed);
    [super dealloc];
}

- (void)finalize
{
    ASSERT(_closed);
    [super finalize];
}

- (void)close
{
    [self clearFrame];
    _closed = YES;
}

- (void)addData:(NSData *)data
{
    Document *doc = m_frame->document();
    
    // Document may be nil if the part is about to redirect
    // as a result of JS executing during load, i.e. one frame
    // changing another's location before the frame's document
    // has been created. 
    if (doc) {
        doc->setShouldCreateRenderers(_shouldCreateRenderers);
        m_frame->loader()->addData((const char *)[data bytes], [data length]);
    }
}

- (BOOL)scrollOverflowInDirection:(WebScrollDirection)direction granularity:(WebScrollGranularity)granularity
{
    if (!m_frame)
        return NO;
    return m_frame->eventHandler()->scrollOverflow((ScrollDirection)direction, (ScrollGranularity)granularity);
}

- (void)clearFrame
{
    m_frame = 0;
}

- (void)createFrameViewWithNSView:(NSView *)view marginWidth:(int)mw marginHeight:(int)mh
{
    // If we own the view, delete the old one - otherwise the render m_frame will take care of deleting the view.
    if (m_frame)
        m_frame->setView(0);

    FrameView* frameView = new FrameView(m_frame);
    m_frame->setView(frameView);
    frameView->deref();

    frameView->setView(view);
    if (mw >= 0)
        frameView->setMarginWidth(mw);
    if (mh >= 0)
        frameView->setMarginHeight(mh);
}

- (NSString *)_stringWithDocumentTypeStringAndMarkupString:(NSString *)markupString
{
    return m_frame->documentTypeString() + markupString;
}

- (NSArray *)nodesFromList:(Vector<Node*> *)nodesVector
{
    size_t size = nodesVector->size();
    NSMutableArray *nodes = [NSMutableArray arrayWithCapacity:size];
    for (size_t i = 0; i < size; ++i)
        [nodes addObject:[DOMNode _wrapNode:(*nodesVector)[i]]];
    return nodes;
}

- (NSString *)markupStringFromNode:(DOMNode *)node nodes:(NSArray **)nodes
{
    // FIXME: This is never "for interchange". Is that right? See the next method.
    Vector<Node*> nodeList;
    NSString *markupString = createMarkup([node _node], IncludeNode, nodes ? &nodeList : 0);
    if (nodes)
        *nodes = [self nodesFromList:&nodeList];

    return [self _stringWithDocumentTypeStringAndMarkupString:markupString];
}

- (NSString *)markupStringFromRange:(DOMRange *)range nodes:(NSArray **)nodes
{
    // FIXME: This is always "for interchange". Is that right? See the previous method.
    Vector<Node*> nodeList;
    NSString *markupString = createMarkup([range _range], nodes ? &nodeList : 0, AnnotateForInterchange);
    if (nodes)
        *nodes = [self nodesFromList:&nodeList];

    return [self _stringWithDocumentTypeStringAndMarkupString:markupString];
}

- (NSString *)selectedString
{
    String text = m_frame->selectedText();
    text.replace('\\', m_frame->backslashAsCurrencySymbol());
    return text;
}


- (void)revealSelection
{
    m_frame->revealSelection(RenderLayer::gAlignToEdgeIfNeeded);
}

- (DOMRange *)selectedDOMRange
{
    return [DOMRange _wrapRange:m_frame->selectionController()->toRange().get()];
}

- (void)setSelectedDOMRange:(DOMRange *)range affinity:(NSSelectionAffinity)affinity closeTyping:(BOOL)closeTyping
{
    m_frame->selectionController()->setSelectedRange([range _range], (EAffinity)affinity, closeTyping);
}

- (void)expandSelectionToElementContainingCaretSelection
{
    DOMRange *range = [self elementRangeContainingCaretSelection];
    if (!range)
        return;
    Selection selection([range _range], DOWNSTREAM);
    m_frame->selectionController()->setSelection(selection);
}

- (DOMRange *)elementRangeContainingCaretSelection
{
    if (!frameHasSelection(self))
        return nil;

    Selection selection(m_frame->selectionController()->selection());
    if (selection.isNone())
        return nil;

    VisiblePosition visiblePos(selection.start(), VP_DEFAULT_AFFINITY);
    if (visiblePos.isNull())
        return nil;

    Node *node = visiblePos.deepEquivalent().node();
    Element *element = node->enclosingBlockFlowElement();
    if (!element)
        return nil;

    Position startPos(element, 0);
    Position endPos(element, element->childNodeCount());

    VisiblePosition startVisiblePos(startPos, VP_DEFAULT_AFFINITY);
    VisiblePosition endVisiblePos(endPos, VP_DEFAULT_AFFINITY);
    if (startVisiblePos.isNull() || endVisiblePos.isNull())
        return nil;

    selection.setBase(startVisiblePos);
    selection.setExtent(endVisiblePos);
     
    return [DOMRange _wrapRange:selection.toRange().get()];
}

- (unichar)characterInRelationToCaretSelection:(int)amount
{
    if (!frameHasSelection(self))
        return 0;

    Selection selection(m_frame->selectionController()->selection());
    assert(selection.isCaretOrRange());

    VisiblePosition visiblePosition(selection.start(), VP_DEFAULT_AFFINITY);

    if (amount < 0) {
        int count = abs(amount);
        for (int i = 0; i < count; i++)
            visiblePosition = visiblePosition.previous();    
        return visiblePosition.characterBefore();
    }
    else {
        for (int i = 0; i < amount; i++)
            visiblePosition = visiblePosition.next();    
        return visiblePosition.characterAfter();
    }
}

- (unichar)characterBeforeCaretSelection
{
    if (!frameHasSelection(self))
        return 0;

    Selection selection(m_frame->selectionController()->selection());
    assert(selection.isCaretOrRange());
    
    VisiblePosition visiblePosition(selection.start(), VP_DEFAULT_AFFINITY);
    return visiblePosition.characterBefore();
}

- (unichar)characterAfterCaretSelection
{
    if (!frameHasSelection(self))
        return 0;

    Selection selection(m_frame->selectionController()->selection());
    assert(selection.isCaretOrRange());
    
    VisiblePosition visiblePosition(selection.end(), VP_DEFAULT_AFFINITY);
    return visiblePosition.characterAfter();
}

- (void)expandSelectionToWordContainingCaretSelection
{
    DOMRange *range = [self wordRangeContainingCaretSelection];
    if (!range)
        return;
    Selection selection([range _range], DOWNSTREAM);
    m_frame->selectionController()->setSelection(selection);
}

- (void)expandSelectionToStartOfWordContainingCaretSelection
{
    if (!frameHasSelection(self))
        return;
        
    DOMRange *range = [self wordRangeContainingCaretSelection];
    if (!range)
        return;
    Selection expanded([range _range], DOWNSTREAM);
    Selection selection(m_frame->selectionController()->selection());
    selection.setBase(expanded.base());
    m_frame->selectionController()->setSelection(selection);
}

- (DOMRange *)_wordRangeContainingCaretSelection
{
    if (!frameHasSelection(self))
        return nil;

    Selection selection(m_frame->selectionController()->selection());
    assert(selection.isCaretOrRange());

    Position startPosBeforeExpansion(selection.start());
    Position endPosBeforeExpansion(selection.end());
    VisiblePosition endVisiblePosBeforeExpansion(endPosBeforeExpansion, VP_DEFAULT_AFFINITY);
    if (endVisiblePosBeforeExpansion.isNull())
        return nil;

    if (isEndOfParagraph(endVisiblePosBeforeExpansion)) {
        DeprecatedChar c = DeprecatedChar(endVisiblePosBeforeExpansion.characterBefore());
        unichar u = c.unicode();
        if (c.isSpace() || u == 0xA0) {
            //fprintf(stderr, "end of paragraph with space\n");
            return nil;
        }
    }

    // If at end of paragraph, move backwards one character.
    // This has the effect of selecting the word on the line (which is
    // what we want, rather than selecting past the end of the line).
    if (isEndOfParagraph(endVisiblePosBeforeExpansion) &&
        !isStartOfParagraph(endVisiblePosBeforeExpansion)) {
        m_frame->selectionController()->modify(SelectionController::MOVE, SelectionController::BACKWARD, CharacterGranularity);
    }

    m_frame->selectionController()->expandUsingGranularity(WordGranularity);
    
    selection = m_frame->selectionController()->selection();    
    Position startPos(selection.start());
    Position endPos(selection.end());

    // expansion cannot be allowed to change selection so that it is no longer
    // touches (or contains) the original, unexpanded selection.
    if (startPosBeforeExpansion.node() == startPos.node() && 
        startPosBeforeExpansion.offset() < startPos.offset()) {
        startPos = startPosBeforeExpansion;
    }
    if (endPosBeforeExpansion.node() == endPos.node() && 
        endPosBeforeExpansion.offset() > endPos.offset()) {
        endPos = endPosBeforeExpansion;
    }
    
    VisiblePosition startVisiblePos(startPos, VP_DEFAULT_AFFINITY);
    VisiblePosition endVisiblePos(endPos, VP_DEFAULT_AFFINITY);

    if (startVisiblePos.isNull() || endVisiblePos.isNull()) {
        //fprintf(stderr, "start or end is nil [1]\n");
        return nil;
    }
    
    if (isEndOfLine(endVisiblePosBeforeExpansion)) {
        VisiblePosition previous(endVisiblePos.previous());
        if (previous == endVisiblePos) {
            // empty document
            //fprintf(stderr, "empty document\n");
            return nil;
        }
        DeprecatedChar c = DeprecatedChar(previous.characterAfter());
        if (c.isSpace() || c.unicode() == 0xA0) {
            // space at end of line
            //fprintf(stderr, "space at end of line [1]\n");
            return nil;
        }
    }

    // Expansion has selected past end of line.
    // Try repositioning backwards.
    if (isEndOfLine(startVisiblePos) && isStartOfLine(endVisiblePos)) {
        VisiblePosition previous(startVisiblePos.previous());
        if (isEndOfLine(previous)) {
            // on empty line
            //fprintf(stderr, "on empty line\n");
            return nil;
        }
        DeprecatedChar c = DeprecatedChar(previous.characterAfter());
        if (c.isSpace() || c.unicode() == 0xA0) {
            // space at end of line
            //fprintf(stderr, "space at end of line [2]\n");
            return nil;
        }
        m_frame->selectionController()->moveTo(startVisiblePos);
        m_frame->selectionController()->modify(SelectionController::EXTEND, SelectionController::BACKWARD, WordGranularity);
        selection = m_frame->selectionController()->selection();
        startPos = selection.start();
        endPos = selection.end();
        startVisiblePos = VisiblePosition(startPos, VP_DEFAULT_AFFINITY);
        endVisiblePos = VisiblePosition(endPos, VP_DEFAULT_AFFINITY);
        if (startVisiblePos.isNull() || endVisiblePos.isNull()) {
            //fprintf(stderr, "start or end is nil [2]\n");
            return nil;
        }
    }

    // Now loop backwards until we find a non-space.
    while (endVisiblePos != startVisiblePos) {
        VisiblePosition previous(endVisiblePos.previous());
        DeprecatedChar c = DeprecatedChar(previous.characterAfter());
        unichar u = c.unicode();
        if (!c.isSpace() && u != 0xA0)
            break;
        endVisiblePos = previous;
    }

    Selection selectionRange(startVisiblePos, endVisiblePos);    
    
    return [DOMRange _wrapRange:selectionRange.toRange().get()];
}

- (DOMRange *)wordRangeContainingCaretSelection
{
    if (!m_frame)
        return nil;

    // _wordRangeContainingCaretSelection modifies the active selection, so save/restore it
    if (m_frame->editor() && m_frame->editor()->client())
        m_frame->editor()->client()->suppressSelectionNotifications();
    Selection selection(m_frame->selectionController()->selection());
    DOMRange *range = [self _wordRangeContainingCaretSelection];
    m_frame->selectionController()->setSelection(selection);
    if (m_frame->editor() && m_frame->editor()->client())
        m_frame->editor()->client()->restoreSelectionNotifications();
    
    return range;
}

- (NSString *)wordInRange:(DOMRange *)range
{
    if (!range)
        return nil;
    return [self stringForRange:range];
}

- (int)wordOffsetInRange:(DOMRange *)range
{
    if (!range)
        return -1;
    
    Selection selection(m_frame->selectionController()->selection());
    if (!selection.isCaret())
        return -1;

    // FIXME: This will only work in cases where the selection remains in
    // the same node after it is expanded. Improve to handle more complicated
    // cases.
    int result = selection.start().offset() - [range startOffset];
    if (result < 0)
        result = 0;
    return result;
}

- (int)indexCountOfWordInRange:(DOMRange *)range
{
    if (!range)
        return -1;

    NSString *word = [self wordInRange:range];
    if ([word length] == 0)
        return -1;
    return m_frame->indexCountOfWordPrecedingSelection(word);
}

- (BOOL)spaceFollowsWordInRange:(DOMRange *)range
{
    if (!range)
        return NO;
    VisiblePosition pos([[range endContainer] _node], [range endOffset], VP_DEFAULT_AFFINITY);
    return DeprecatedChar(pos.characterAfter()).isSpace();
}

- (NSArray *)wordsInCurrentParagraph
{
    return m_frame->wordsInCurrentParagraph();
}

- (BOOL)selectionAtDocumentStart
{
    Selection selection(m_frame->selectionController()->selection());
    if (selection.isNone())
        return NO;

    m_frame->document()->updateLayout();

    Position startPos(selection.start());
    VisiblePosition pos(startPos.node(), startPos.offset(), VP_DEFAULT_AFFINITY);
    if (pos.isNull())
        return NO;

    return isStartOfDocument(pos);
}

- (BOOL)selectionAtSentenceStart:(Selection *)sel
{
    if (!sel)
        return NO;

    Selection selection = *sel;
    if (selection.isNone())
        return NO;

    m_frame->document()->updateLayout();

    Position startPos(selection.start());
    VisiblePosition pos(startPos.node(), startPos.offset(), VP_DEFAULT_AFFINITY);
    if (pos.isNull())
        return NO;

    if (isStartOfParagraph(pos))
        return YES;
        
    BOOL result = YES;
    BOOL sawSpace = NO;
    unsigned previousCount = 0;
    for (pos = pos.previous(); !pos.isNull(); pos = pos.previous()) {
        previousCount++;
        if (isStartOfParagraph(pos)) {
            if (previousCount == 1 || (previousCount == 2 && sawSpace))
                result = NO;
            break;
        }
        DeprecatedChar c(pos.characterAfter());
        if (c.unicode()) {
            unsigned short u = c.unicode();
            if (c.isSpace() || u == 0xa0) {
                sawSpace = YES;
            }
            else {
                result = (u == '.' || u == '!' || u == '?');
                break;
            }
        }
    }
    
    return result;
}

- (BOOL)selectionAtSentenceStart
{
    Selection selection(m_frame->selectionController()->selection());
    return [self selectionAtSentenceStart:&selection];
}

- (BOOL)rangeAtSentenceStart:(DOMRange *)range
{
    if (!range)
        return NO;
    Selection selection([range _range], DOWNSTREAM);
    return [self selectionAtSentenceStart:&selection];
}

- (BOOL)selectionAtWordStart
{
    Selection selection(m_frame->selectionController()->selection());
    if (selection.isNone())
        return NO;

    m_frame->document()->updateLayout();

    Position startPos(selection.start());
    VisiblePosition pos(startPos.node(), startPos.offset(), VP_DEFAULT_AFFINITY);
    if (pos.isNull())
        return NO;

    if (isStartOfParagraph(pos))
        return YES;
        
    BOOL result = YES;
    unsigned previousCount = 0;
    for (pos = pos.previous(); !pos.isNull(); pos = pos.previous()) {
        previousCount++;
        if (isStartOfParagraph(pos)) {
            if (previousCount == 1)
                result = NO;
            break;
        }
        DeprecatedChar c(pos.characterAfter());
        unsigned short u = c.unicode();
        if (u) {
            result = c.isSpace() || u == 0xa0 || (u_ispunct(u) && u != ',' && u != '-' && u != '\'');
            break;
        }
    }
    
    return result;
}

- (void)addAutoCorrectMarker:(DOMRange *)range
{
    Document *doc = m_frame->document();
    if (!doc || !range)
        return;

    doc->addMarker([range _range], DocumentMarker::Spelling);
}

- (void)addAutoCorrectMarker:(DOMRange *)range word:(NSString *)word correction:(NSString *)correction
{
    Document *doc = m_frame->document();
    if (!doc || !range)
        return;
        
    unsigned wordLength = [word length];
    unichar wordChars[wordLength];
    [word getCharacters:wordChars];

    unsigned correctionLength = [correction length];
    unichar correctionChars[correctionLength];
    [correction getCharacters:correctionChars];
    
    VisiblePosition start([[range startContainer] _node], [range startOffset], VP_DEFAULT_AFFINITY);
    if (start.isNull())
        return;
    VisiblePosition end(start);

    SelectionController sel;
    unsigned max = min(wordLength, correctionLength);
    for (unsigned i = 0; i < max; i++) {
        if (wordChars[i] != correctionChars[i]) {
            end = end.next();
        }
        else if (start != end) {
            sel.moveTo(start, end);
            doc->addMarker(sel.toRange().get(), DocumentMarker::Spelling);
            start = end.next();
            end = start;
        }
        else {
            start = start.next();
            end = start;
        }
    }
    if (start != end) {
        sel.moveTo(start, end);
        doc->addMarker(sel.toRange().get(), DocumentMarker::Spelling);    
    }
}

- (int)preferredHeight
{
    Document *doc = m_frame->document();
    if (!doc)
        return 0;

    doc->updateLayout();
    
    Node *body = m_frame->document()->body();
    if (!body)
        return 0;
    
    RenderObject *renderer = body->renderer();
    if (!renderer || !renderer->isRenderBlock())
        return 0;
    
    RenderBlock *block = static_cast<RenderBlock *>(renderer);
    return block->height() + block->marginTop() + block->marginBottom();
}

- (int)innerLineHeight:(DOMNode *)node
{
    if (!node)
        return 0;

    Document *doc = m_frame->document();
    if (!doc)
        return 0;

    doc->updateLayout();
    
    Node *n = [node _node];
    if (!n)
        return 0;

    RenderObject *renderer = n->renderer();
    if (!renderer)
        return 0;
    
    return renderer->innerLineHeight();
}

- (void)updateLayout
{
    Document *doc = m_frame->document();
    if (!doc)
        return;
    doc->updateLayout();
    FrameView *view = m_frame->view();
    if (view)
        view->adjustViewSize();
}

- (void)setCaretBlinks:(BOOL)flag
{
    m_frame->setCaretBlinks(flag);    
}

- (void)setCaretVisible:(BOOL)flag
{
    m_frame->setCaretVisible(flag);    
}

- (void)clearDocumentContent
{
    if (!m_frame)
        return;

    Document *doc = m_frame->document();
    if (!doc)
        return;

    doc->updateLayout();
    
    Element *elem = doc->documentElement();
    if (!elem)
        return;
    
    Node *n;
    while ((n = elem->firstChild())) {
        int ec;
        elem->removeChild(n, ec);
    }
}

- (void)setIsActive:(BOOL)flag
{
    m_frame->page()->focusController()->setActive(flag);
}

- (void)setEmbeddedEditingMode:(BOOL)flag
{
    m_frame->setEmbeddedEditingMode(flag);
}


- (NSString *)stringForRange:(DOMRange *)range
{
    // This will give a system malloc'd buffer that can be turned directly into an NSString
    unsigned length;
    UChar* buf = plainTextToMallocAllocatedBuffer([range _range], length);
    
    if (!buf)
        return [NSString string];
    
    UChar backslashAsCurrencySymbol = m_frame->backslashAsCurrencySymbol();
    if (backslashAsCurrencySymbol != '\\')
        for (unsigned n = 0; n < length; n++) 
            if (buf[n] == '\\')
                buf[n] = backslashAsCurrencySymbol;

    // Transfer buffer ownership to NSString
    return [[[NSString alloc] initWithCharactersNoCopy:buf length:length freeWhenDone:YES] autorelease];
}

- (void)collapseSelectedDOMRangeWithAffinity:(NSSelectionAffinity)selectionAffinity
{
    Selection selection(m_frame->selectionController()->selection());
    if (!selection.isRange())
        return;
        
    Range *range = m_frame->selectionController()->toRange().get();
    ExceptionCode ec = 0;
    range->collapse(selectionAffinity == NSSelectionAffinityUpstream, ec);
    m_frame->selectionController()->setSelectedRange(range, static_cast<EAffinity>(selectionAffinity), false);
}

- (NSString *)currentSentence
{
    Selection selection(m_frame->selectionController()->selection());
    if (selection.isNone())
        return nil;

    Range *range = m_frame->selectionController()->toRange().get();

    ExceptionCode ec = 0;
    Node *node = range->startContainer(ec);
    ASSERT(node);

    m_frame->document()->updateLayout();

    EAffinity affinity = DOWNSTREAM;
    VisiblePosition pos(node, range->startOffset(ec), affinity);
    VisiblePosition start = startOfSentence(pos);

    selection = Selection(start, pos);
    DOMRange *selRange = [DOMRange _wrapRange:selection.toRange().get()];
    return [self stringForRange:selRange];
}

- (void)reapplyStylesForDeviceType:(WebCoreDeviceType)deviceType
{
    if (m_frame->view())
        m_frame->view()->setMediaType(deviceType == WebCoreDeviceScreen ? "screen" : "print");
    Document *doc = m_frame->document();
    if (doc)
        doc->setPrinting(deviceType == WebCoreDevicePrinter);
    m_frame->reapplyStyles();
}

- (void)forceLayoutAdjustingViewSize:(BOOL)flag
{
    m_frame->forceLayout(!flag);
    if (flag)
        m_frame->view()->adjustViewSize();
}

- (void)forceLayoutWithMinimumPageWidth:(float)minPageWidth maximumPageWidth:(float)maxPageWidth adjustingViewSize:(BOOL)flag
{
    m_frame->forceLayoutWithPageWidthRange(minPageWidth, maxPageWidth, flag);
}

- (void)sendOrientationChangeEvent
{
    if (WebThreadNotCurrent()) {
        NSInvocation *invocation = WebThreadCreateNSInvocation(self, _cmd);
        WebThreadCallAPI(invocation);
        return;
    }
    m_frame->sendOrientationChangeEvent();
}

- (void)sendScrollEvent
{
    m_frame->sendScrollEvent();
}

- (void)drawRect:(NSRect)rect
{
    PlatformGraphicsContext* platformContext = static_cast<PlatformGraphicsContext*>(WKGetCurrentGraphicsContext());
    GraphicsContext context(platformContext);
    
    m_frame->paint(&context, enclosingIntRect(rect));
}

// Used by pagination code called from AppKit when a standalone web page is printed.
- (NSArray*)computePageRectsWithPrintWidthScaleFactor:(float)printWidthScaleFactor printHeight:(float)printHeight
{
    return 0;
}

// This is to support the case where a webview is embedded in the view that's being printed
- (void)adjustPageHeightNew:(float *)newBottom top:(float)oldTop bottom:(float)oldBottom limit:(float)bottomLimit
{
    m_frame->adjustPageHeight(newBottom, oldTop, oldBottom, bottomLimit);
}

- (CGSize)renderedSizeOfNode:(DOMNode *)node constrainedToWidth:(float)width
{
    Node *n = [node _node];
    RenderObject *r = n ? n->renderer() : 0;
    float w = min((float)r->maxPrefWidth(), width);
    return r ? CGSizeMake(w, r->height()) : CGSizeMake(0,0);
}

- (void)setUserStyleSheet:(NSString *)styleSheet
{
    if (!m_frame)
        return;
    m_frame->setUserStyleSheet(styleSheet, true);
}

- (NSObject *)copyRenderNode:(RenderObject *)node copier:(id <WebCoreRenderTreeCopier>)copier
{
    NSMutableArray *children = [[NSMutableArray alloc] init];
    for (RenderObject *child = node->firstChild(); child; child = child->nextSibling()) {
        [children addObject:[self copyRenderNode:child copier:copier]];
    }
          
    NSString *name = [[NSString alloc] initWithUTF8String:node->renderName()];
    
    RenderWidget* renderWidget = node->isWidget() ? static_cast<RenderWidget*>(node) : 0;
    Widget* widget = renderWidget ? renderWidget->widget() : 0;
    NSView *view = widget ? widget->getView() : nil;
    
    int nx, ny;
    node->absolutePosition(nx, ny);
    NSObject *copiedNode = [copier nodeWithName:name
                                       position:NSMakePoint(nx,ny)
                                           rect:NSMakeRect(node->xPos(), node->yPos(), node->width(), node->height())
                                           view:view
                                       children:children];
    
    [name release];
    [children release];
    
    return copiedNode;
}

- (NSObject *)copyRenderTree:(id <WebCoreRenderTreeCopier>)copier
{
    RenderObject *renderer = m_frame->renderer();
    if (!renderer) {
        return nil;
    }
    return [self copyRenderNode:renderer copier:copier];
}

- (void)installInFrame:(NSView *)view
{
    // If this isn't the main frame, it must have a render m_frame set, or it
    // won't ever get installed in the view hierarchy.
    ASSERT(m_frame == m_frame->page()->mainFrame() || m_frame->ownerElement());

    m_frame->view()->setView(view);
    // FIXME: frame tries to do this too, is it needed?
    if (m_frame->ownerRenderer()) {
        m_frame->ownerRenderer()->setWidget(m_frame->view());
        // Now the render part owns the view, so we don't any more.
    }

    m_frame->view()->initScrollbars();
}

static HTMLInputElement* inputElementFromDOMElement(DOMElement* element)
{
    Node* node = [element _node];
    if (node->hasTagName(inputTag))
        return static_cast<HTMLInputElement*>(node);
    return nil;
}

static HTMLFormElement *formElementFromDOMElement(DOMElement *element)
{
    Node *node = [element _node];
    // This should not be necessary, but an XSL file on
    // maps.google.com crashes otherwise because it is an xslt file
    // that contains <form> elements that aren't in any namespace, so
    // they come out as generic CML elements
    if (node && node->hasTagName(formTag)) {
        return static_cast<HTMLFormElement *>(node);
    }
    return nil;
}

- (DOMElement *)elementWithName:(NSString *)name inForm:(DOMElement *)form
{
    HTMLFormElement *formElement = formElementFromDOMElement(form);
    if (formElement) {
        Vector<HTMLGenericFormElement*>& elements = formElement->formElements;
        AtomicString targetName = name;
        for (unsigned int i = 0; i < elements.size(); i++) {
            HTMLGenericFormElement *elt = elements[i];
            // Skip option elements, other duds
            if (elt->name() == targetName)
                return [DOMElement _wrapElement:elt];
        }
    }
    return nil;
}

- (NSView *)viewForElement:(DOMElement *)element
{
    RenderObject *renderer = [element _element]->renderer();
    if (renderer && renderer->isWidget()) {
        Widget *widget = static_cast<const RenderWidget*>(renderer)->widget();
        if (widget)
            return widget->getView();
    }
    return nil;
}

- (BOOL)elementDoesAutoComplete:(DOMElement *)element
{
    HTMLInputElement *inputElement = inputElementFromDOMElement(element);
    return inputElement != nil
        && inputElement->inputType() == HTMLInputElement::TEXT
        && inputElement->autoComplete();
}

- (BOOL)elementIsPassword:(DOMElement *)element
{
    HTMLInputElement *inputElement = inputElementFromDOMElement(element);
    return inputElement != nil
        && inputElement->inputType() == HTMLInputElement::PASSWORD;
}

- (DOMElement *)formForElement:(DOMElement *)element;
{
    HTMLInputElement *inputElement = inputElementFromDOMElement(element);
    if (inputElement) {
        HTMLFormElement *formElement = inputElement->form();
        if (formElement) {
            return [DOMElement _wrapElement:formElement];
        }
    }
    return nil;
}

- (DOMElement *)currentForm
{
    return [DOMElement _wrapElement:m_frame->currentForm()];
}

- (NSArray *)controlsInForm:(DOMElement *)form
{
    NSMutableArray *results = nil;
    HTMLFormElement *formElement = formElementFromDOMElement(form);
    if (formElement) {
        Vector<HTMLGenericFormElement*>& elements = formElement->formElements;
        for (unsigned int i = 0; i < elements.size(); i++) {
            if (elements.at(i)->isEnumeratable()) { // Skip option elements, other duds
                DOMElement *de = [DOMElement _wrapElement:elements.at(i)];
                if (!results) {
                    results = [NSMutableArray arrayWithObject:de];
                } else {
                    [results addObject:de];
                }
            }
        }
    }
    return results;
}

- (NSString *)searchForLabels:(NSArray *)labels beforeElement:(DOMElement *)element
{
    return m_frame->searchForLabelsBeforeElement(labels, [element _element]);
}

- (NSString *)matchLabels:(NSArray *)labels againstElement:(DOMElement *)element
{
    return m_frame->matchLabelsAgainstElement(labels, [element _element]);
}

- (NSURL *)URLWithAttributeString:(NSString *)string
{
    Document *doc = m_frame->document();
    if (!doc)
        return nil;
    // FIXME: is parseURL appropriate here?
    DeprecatedString rel = parseURL(string).deprecatedString();
    return KURL(doc->completeURL(rel)).getNSURL();
}

- (DOMNode *)scrollableNodeAtViewportLocation:(CGPoint)aViewportLocation
{
    return [DOMNode _wrapNode:m_frame->nodeRespondingToScrollWheelEvents(&aViewportLocation)];
}

- (DOMNode *)approximateNodeAtViewportLocation:(CGPoint *)aViewportLocation
{
    return [DOMNode _wrapNode:m_frame->nodeRespondingToClickEvents(aViewportLocation)];
}

- (CGRect)renderRectForPoint:(CGPoint)point isReplaced:(BOOL *)isReplaced fontSize:(float *)fontSize
{
    bool replaced = false;
    CGRect rect = m_frame->renderRectForPoint(point, &replaced, fontSize);
    *isReplaced = replaced;
    return rect;
}

- (BOOL)hasCustomViewportArguments
{
    return m_frame->viewportArguments().hasCustomArgument();
}

- (void)clearCustomViewportArguments
{
    m_frame->setViewportArguments(ViewportArguments());
}

- (NSDictionary *)viewportArguments
{
    return m_frame->dictionaryForViewportArguments(m_frame->viewportArguments());
}

- (BOOL)searchFor:(NSString *)string direction:(BOOL)forward caseSensitive:(BOOL)caseFlag wrap:(BOOL)wrapFlag startInSelection:(BOOL)startInSelection
{
    return m_frame->findString(string, forward, caseFlag, wrapFlag, startInSelection);
}

- (unsigned)markAllMatchesForText:(NSString *)string caseSensitive:(BOOL)caseFlag limit:(unsigned)limit
{
    return m_frame->markAllMatchesForText(string, caseFlag, limit);
}

- (BOOL)markedTextMatchesAreHighlighted
{
    return m_frame->markedTextMatchesAreHighlighted();
}

- (void)setMarkedTextMatchesAreHighlighted:(BOOL)doHighlight
{
    m_frame->setMarkedTextMatchesAreHighlighted(doHighlight);
}

- (void)unmarkAllTextMatches
{
    Document *doc = m_frame->document();
    if (!doc) {
        return;
    }
    doc->removeMarkers(DocumentMarker::TextMatch);
}


- (void)setTextSizeMultiplier:(float)multiplier
{
    int newZoomFactor = (int)rint(multiplier * 100);
    if (m_frame->zoomFactor() == newZoomFactor) {
        return;
    }
    m_frame->setZoomFactor(newZoomFactor);
}

- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)string
{
    return [self stringByEvaluatingJavaScriptFromString:string forceUserGesture:true];
}

- (NSString *)stringByEvaluatingJavaScriptFromString:(NSString *)string forceUserGesture:(BOOL)forceUserGesture
{
    ASSERT(m_frame->document());
    
    JSValue* result = m_frame->loader()->executeScript(string, forceUserGesture);

    if (!m_frame) // In case the script removed our frame from the page.
        return @"";

    // This bizarre set of rules matches behavior from WebKit for Safari 2.0.
    // If you don't like it, use -[WebScriptObject evaluateWebScript:] or 
    // JSEvaluateScript instead, since they have less surprising semantics.
    if (!result || !result->isBoolean() && !result->isString() && !result->isNumber())
        return @"";

    JSLock lock;
    return String(result->toString(m_frame->scriptProxy()->globalObject()->globalExec()));
}


- (NSRect)caretRect
{
    if (m_frame && m_frame->editor() && m_frame->editor()->client())
        m_frame->editor()->client()->suppressSelectionNotifications();

    NSRect rect = NSMakeRect(0, 0, 0, 0);
    SelectionController selectionController;
    Selection selection(m_frame->selectionController()->selection());
    if (selection.isNone()) {
        if (m_frame && m_frame->editor() && m_frame->editor()->client())
            m_frame->editor()->client()->restoreSelectionNotifications();
        return rect;
    }
    
    // If we ever support selection ranges, this code will need to be
    // improved. For now, collapse selection to the end.
    if (selection.isRange()) {
        Position end(selection.end());
        selection.setBase(end);
        selection.setExtent(end);
    }

    selectionController.setSelection(selection);
    FloatQuad caretQuad = selectionController.absoluteCaretQuad();

    if (m_frame && m_frame->editor() && m_frame->editor()->client())
        m_frame->editor()->client()->restoreSelectionNotifications();

    return caretQuad.boundingBox();
}

- (NSRect)rectForScrollToVisible
{
    Selection selection(m_frame->selectionController()->selection());
    if (selection.isNone())
        return CGRectZero;

    if (selection.isCaret())
        return [self caretRect];

    if (m_frame && m_frame->editor() && m_frame->editor()->client())
        m_frame->editor()->client()->suppressSelectionNotifications();

    Selection originalSelection(selection);
    Position pos;

    pos = originalSelection.start();
    selection.setBase(pos);
    selection.setExtent(pos);
    SelectionController startController;
    startController.setSelection(selection);
    FloatRect startRect(startController.absoluteCaretQuad().boundingBox());
    
    pos = originalSelection.end();
    selection.setBase(pos);
    selection.setExtent(pos);
    SelectionController endController;
    endController.setSelection(selection);
    FloatRect endRect(endController.absoluteCaretQuad().boundingBox());

    if (m_frame && m_frame->editor() && m_frame->editor()->client())
        m_frame->editor()->client()->restoreSelectionNotifications();

    return unionRect(startRect, endRect);
}

- (NSRect)autocorrectionRect
{
    Selection selection(m_frame->selectionController()->selection());
    if (!selection.isCaret())
        return NSZeroRect;

    NSRect result = NSZeroRect;

    if (m_frame && m_frame->editor() && m_frame->editor()->client())
        m_frame->editor()->client()->suppressSelectionNotifications();

    if (selection.expandUsingGranularity(WordGranularity)) {
        RefPtr<Range> range = selection.toRange();
        result = m_frame->firstRectForRange(range.get());
    }

    if (m_frame && m_frame->editor() && m_frame->editor()->client())
        m_frame->editor()->client()->restoreSelectionNotifications();

    return result;
}


- (NSRect)caretRectAtNode:(DOMNode *)node offset:(int)offset affinity:(NSSelectionAffinity)affinity
{
    RenderObject* renderer = [node _node]->renderer();
    if (!renderer)
        return NSZeroRect;
    
    FloatQuad caretQuad = renderer->absoluteCaretQuad(offset, static_cast<EAffinity>(affinity));
    return caretQuad.boundingBox();
}

- (NSRect)firstRectForDOMRange:(DOMRange *)range
{
    FloatQuad rangeQuad = m_frame->firstQuadForRange([range _range]);
    return rangeQuad.boundingBox();
}

- (void)scrollDOMRangeToVisible:(DOMRange *)range
{
    NSRect rangeRect = [self firstRectForDOMRange:range];    
    Node *startNode = [[range startContainer] _node];
        
    if (startNode && startNode->renderer()) {
        RenderLayer *layer = startNode->renderer()->enclosingLayer();
        if (layer) {
            layer->setAdjustForPurpleCaretWhenScrolling(true);
            layer->scrollRectToVisible(enclosingIntRect(rangeRect), RenderLayer::gAlignToEdgeIfNeeded, RenderLayer::gAlignToEdgeIfNeeded);
            layer->setAdjustForPurpleCaretWhenScrolling(false);
            m_frame->invalidateSelection();
        }
    }
}

- (NSURL *)baseURL
{
    return m_frame->loader()->completeURL(m_frame->document()->baseURL()).getNSURL();
}

- (NSString *)stringWithData:(NSData *)data
{
    Document* doc = m_frame->document();
    if (!doc)
        return nil;
    TextResourceDecoder* decoder = doc->decoder();
    if (!decoder)
        return nil;
    return decoder->encoding().decode(reinterpret_cast<const char*>([data bytes]), [data length]);
}

+ (NSString *)stringWithData:(NSData *)data textEncodingName:(NSString *)textEncodingName
{
    WebCore::TextEncoding encoding(textEncodingName);
    if (!encoding.isValid())
        encoding = WindowsLatin1Encoding();
    return encoding.decode(reinterpret_cast<const char*>([data bytes]), [data length]);
}

- (BOOL)needsLayout
{
    return m_frame->view() ? m_frame->view()->needsLayout() : false;
}

- (void)setNeedsLayout
{
    if (m_frame->view())
        m_frame->view()->setNeedsLayout();
}

- (NSString *)renderTreeAsExternalRepresentation
{
    return externalRepresentation(m_frame->renderer()).getNSString();
}

- (void)setShouldCreateRenderers:(BOOL)f
{
    _shouldCreateRenderers = f;
}

- (void)setBaseBackgroundColor:(CGColorRef)backgroundColor
{
    if (m_frame && m_frame->view()) {
        float r, g, b, a;
        GSColorGetRGBAComponents(backgroundColor, &r, &g, &b, &a);
        Color color = Color(makeRGBA((int)(255 * r),
                                     (int)(255 * g),
                                     (int)(255 * b),
                                     (int)(255 * a)));
        m_frame->view()->setBaseBackgroundColor(color);
    }
}


- (void)setDrawsBackground:(BOOL)drawsBackground
{
    if (m_frame && m_frame->view())
        m_frame->view()->setTransparent(!drawsBackground);
}

- (DOMRange *)rangeByAlteringCurrentSelection:(SelectionController::EAlteration)alteration direction:(SelectionController::EDirection)direction granularity:(TextGranularity)granularity
{
    if (m_frame->selectionController()->isNone())
        return nil;

    SelectionController selectionController;
    selectionController.setSelection(m_frame->selectionController()->selection());
    selectionController.modify(alteration, direction, granularity);
    return [DOMRange _wrapRange:selectionController.toRange().get()];
}

- (TextGranularity)selectionGranularity
{
    return m_frame->selectionGranularity();
}

- (NSRange)convertToNSRange:(Range *)range
{
    int exception = 0;

    if (!range || range->isDetached())
        return NSMakeRange(NSNotFound, 0);

    Element* selectionRoot = m_frame->selectionController()->rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : m_frame->document()->documentElement();
    
    // Mouse events may cause TSM to attempt to create an NSRange for a portion of the view
    // that is not inside the current editable region.  These checks ensure we don't produce
    // potentially invalid data when responding to such requests.
    if (range->startContainer(exception) != scope && !range->startContainer(exception)->isDescendantOf(scope))
        return NSMakeRange(NSNotFound, 0);
    if(range->endContainer(exception) != scope && !range->endContainer(exception)->isDescendantOf(scope))
        return NSMakeRange(NSNotFound, 0);
    
    RefPtr<Range> testRange = new Range(scope->document(), scope, 0, range->startContainer(exception), range->startOffset(exception));
    ASSERT(testRange->startContainer(exception) == scope);
    int startPosition = TextIterator::rangeLength(testRange.get());

    testRange->setEnd(range->endContainer(exception), range->endOffset(exception), exception);
    ASSERT(testRange->startContainer(exception) == scope);
    int endPosition = TextIterator::rangeLength(testRange.get());

    return NSMakeRange(startPosition, endPosition - startPosition);
}

- (PassRefPtr<Range>)convertToDOMRange:(NSRange)nsrange
{
    if (nsrange.location > INT_MAX)
        return 0;
    if (nsrange.length > INT_MAX || nsrange.location + nsrange.length > INT_MAX)
        nsrange.length = INT_MAX - nsrange.location;

    // our critical assumption is that we are only called by input methods that
    // concentrate on a given area containing the selection
    // We have to do this because of text fields and textareas. The DOM for those is not
    // directly in the document DOM, so serialization is problematic. Our solution is
    // to use the root editable element of the selection start as the positional base.
    // That fits with AppKit's idea of an input context.
    Element* selectionRoot = m_frame->selectionController()->rootEditableElement();
    Element* scope = selectionRoot ? selectionRoot : m_frame->document()->documentElement();
    return TextIterator::rangeFromLocationAndLength(scope, nsrange.location, nsrange.length);
}

- (DOMRange *)convertNSRangeToDOMRange:(NSRange)nsrange
{
    return [DOMRange _wrapRange:[self convertToDOMRange:nsrange].get()];
}

- (NSRange)convertDOMRangeToNSRange:(DOMRange *)range
{
    return [self convertToNSRange:[range _range]];
}

- (void)selectNSRange:(NSRange)range
{
    RefPtr<Range> domRange = [self convertToDOMRange:range];
    if (domRange)
        m_frame->selectionController()->setSelection(Selection(domRange.get(), SEL_DEFAULT_AFFINITY));
}


- (DOMRange *)rangeByAlteringCurrentSelection:(SelectionController::EAlteration)alteration amount:(int)amount
{
    if (m_frame->selectionController()->isNone())
        return nil;

    if (amount == 0)
        return [self selectedDOMRange];

    SelectionController selectionController;
    selectionController.setSelection(m_frame->selectionController()->selection());
    SelectionController::EDirection direction = amount > 0 ? SelectionController::FORWARD : SelectionController::BACKWARD;
    for (int i = 0; i < abs(amount); i++)
        selectionController.modify(alteration, direction, CharacterGranularity);
    return [DOMRange _wrapRange:selectionController.toRange().get()];
}

- (DOMRange *)rangeByMovingCurrentSelection:(int)amount
{
    return [self rangeByAlteringCurrentSelection:SelectionController::MOVE amount:amount];
}

- (DOMRange *)rangeByExtendingCurrentSelection:(int)amount
{
    return [self rangeByAlteringCurrentSelection:SelectionController::EXTEND amount:amount];
}

- (void)selectNSRange:(NSRange)range onElement:(DOMElement *)element
{
    Document *doc = m_frame->document();
    if (doc) {
        if (![element _node]->inDocument())
            return;
        RefPtr<Range> resultRange = doc->createRange();
        int exception = 0;
        resultRange->setStart([element _node], range.location, exception);
        resultRange->setEnd([element _node], range.location + range.length, exception);
        Selection selection = Selection(resultRange.get(), SEL_DEFAULT_AFFINITY);
        m_frame->selectionController()->setSelection(selection, YES);
        resultRange->ref();
        resultRange->deref();
    }
}

- (DOMRange *)markedTextDOMRange
{
    if (m_frame)
        return [DOMRange _wrapRange:m_frame->editor()->compositionRange().get()];
    else
        return nil;
}

- (void)setMarkedText:(NSString *)text forCandidates:(BOOL)forCandidates
{
    Vector<CompositionUnderline> underlines;
    
    if (m_frame)
        m_frame->editor()->setComposition(text, underlines, 0, [text length]);
}

- (void)confirmMarkedText:(NSString *)text;
{
    if (m_frame) {
        m_frame->editor()->client()->suppressFormNotifications();
        if (text) {
            m_frame->editor()->confirmComposition(text);
        } else {
            // FIXME: This is a hacky workaround for the keyboard calling this method too late -
            // after the selection and focus have already changed.  See <rdar://problem/5975559>
            Node *focused = m_frame->document()->focusedNode();
            Node *composition = m_frame->editor()->compositionNode();
            
            if (composition && focused && focused != composition && !composition->isDescendantOrShadowDescendantOf(focused)) {
                m_frame->editor()->confirmCompositionWithoutDisturbingSelection();
                m_frame->document()->setFocusedNode(focused);
            } else {
                m_frame->editor()->confirmComposition();
            }
        }
        m_frame->editor()->client()->restoreFormNotifications();
    }
}

- (NSRange)selectedNSRange
{
    return [self convertToNSRange:m_frame->selectionController()->toRange().get()];
}

- (DOMRange *)markDOMRange
{
    return [DOMRange _wrapRange:m_frame->mark().toRange().get()];
}

- (NSRange)markedTextNSRange
{
    return [self convertToNSRange:m_frame->editor()->compositionRange().get()];
}

// Given proposedRange, returns an extended range that includes adjacent whitespace that should
// be deleted along with the proposed range in order to preserve proper spacing and punctuation of
// the text surrounding the deletion.
- (DOMRange *)smartDeleteRangeForProposedRange:(DOMRange *)proposedRange
{
    Node *startContainer = [[proposedRange startContainer] _node];
    Node *endContainer = [[proposedRange endContainer] _node];
    if (startContainer == nil || endContainer == nil)
        return nil;

    ASSERT(startContainer->document() == endContainer->document());
    
    m_frame->document()->updateLayoutIgnorePendingStylesheets();

    Position start(startContainer, [proposedRange startOffset]);
    Position end(endContainer, [proposedRange endOffset]);
    Position newStart = start.upstream().leadingWhitespacePosition(DOWNSTREAM, true);
    if (newStart.isNull())
        newStart = start;
    Position newEnd = end.downstream().trailingWhitespacePosition(DOWNSTREAM, true);
    if (newEnd.isNull())
        newEnd = end;

    newStart = rangeCompliantEquivalent(newStart);
    newEnd = rangeCompliantEquivalent(newEnd);

    RefPtr<Range> range = m_frame->document()->createRange();
    int exception = 0;
    range->setStart(newStart.node(), newStart.offset(), exception);
    range->setEnd(newStart.node(), newStart.offset(), exception);
    return [DOMRange _wrapRange:range.get()];
}

// Determines whether whitespace needs to be added around aString to preserve proper spacing and
// punctuation when it's inserted into the receiver's text over charRange. Returns by reference
// in beforeString and afterString any whitespace that should be added, unless either or both are
// nil. Both are returned as nil if aString is nil or if smart insertion and deletion are disabled.
- (void)smartInsertForString:(NSString *)pasteString replacingRange:(DOMRange *)rangeToReplace beforeString:(NSString **)beforeString afterString:(NSString **)afterString
{
    // give back nil pointers in case of early returns
    if (beforeString)
        *beforeString = nil;
    if (afterString)
        *afterString = nil;
        
    // inspect destination
    Node *startContainer = [[rangeToReplace startContainer] _node];
    Node *endContainer = [[rangeToReplace endContainer] _node];

    Position startPos(startContainer, [rangeToReplace startOffset]);
    Position endPos(endContainer, [rangeToReplace endOffset]);

    VisiblePosition startVisiblePos = VisiblePosition(startPos, VP_DEFAULT_AFFINITY);
    VisiblePosition endVisiblePos = VisiblePosition(endPos, VP_DEFAULT_AFFINITY);
    
    // this check also ensures startContainer, startPos, endContainer, and endPos are non-null
    if (startVisiblePos.isNull() || endVisiblePos.isNull())
        return;

    bool addLeadingSpace = startPos.leadingWhitespacePosition(VP_DEFAULT_AFFINITY, true).isNull() && !isStartOfParagraph(startVisiblePos);
    if (addLeadingSpace)
        if (UChar previousChar = startVisiblePos.previous().characterAfter())
            addLeadingSpace = !isCharacterSmartReplaceExempt(previousChar, true);
    
    bool addTrailingSpace = endPos.trailingWhitespacePosition(VP_DEFAULT_AFFINITY, true).isNull() && !isEndOfParagraph(endVisiblePos);
    if (addTrailingSpace)
        if (UChar thisChar = endVisiblePos.characterAfter())
            addTrailingSpace = !isCharacterSmartReplaceExempt(thisChar, false);
    
    // inspect source
    bool hasWhitespaceAtStart = false;
    bool hasWhitespaceAtEnd = false;
    unsigned pasteLength = [pasteString length];
    if (pasteLength > 0) {
        NSCharacterSet *whiteSet = [NSCharacterSet whitespaceAndNewlineCharacterSet];
        
        if ([whiteSet characterIsMember:[pasteString characterAtIndex:0]]) {
            hasWhitespaceAtStart = YES;
        }
        if ([whiteSet characterIsMember:[pasteString characterAtIndex:(pasteLength - 1)]]) {
            hasWhitespaceAtEnd = YES;
        }
    }
    
    // issue the verdict
    if (beforeString && addLeadingSpace && !hasWhitespaceAtStart)
        *beforeString = @" ";
    if (afterString && addTrailingSpace && !hasWhitespaceAtEnd)
        *afterString = @" ";
}

- (DOMDocumentFragment *)documentFragmentWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString 
{
    if (!m_frame || !m_frame->document())
        return 0;

    return [DOMDocumentFragment _wrapDocumentFragment:createFragmentFromMarkup(m_frame->document(), markupString, baseURLString).get()];
}

- (DOMDocumentFragment *)documentFragmentWithText:(NSString *)text inContext:(DOMRange *)context
{
    return [DOMDocumentFragment _wrapDocumentFragment:createFragmentFromText([context _range], text).get()];
}

- (DOMDocumentFragment *)documentFragmentWithNodesAsParagraphs:(NSArray *)nodes
{
    if (!m_frame || !m_frame->document())
        return 0;
    
    NSEnumerator *nodeEnum = [nodes objectEnumerator];
    Vector<Node*> nodesVector;
    DOMNode *node;
    while ((node = [nodeEnum nextObject]))
        nodesVector.append([node _node]);
    
    return [DOMDocumentFragment _wrapDocumentFragment:createFragmentFromNodes(m_frame->document(), nodesVector).get()];
}

- (void)replaceSelectionWithFragment:(DOMDocumentFragment *)fragment selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle
{
    if (m_frame->selectionController()->isNone() || !fragment)
        return;
    
    applyCommand(new ReplaceSelectionCommand(m_frame->document(), [fragment _documentFragment], selectReplacement, smartReplace, matchStyle));
    m_frame->revealSelection(RenderLayer::gAlignToEdgeIfNeeded);
}

- (void)replaceSelectionWithNode:(DOMNode *)node selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace matchStyle:(BOOL)matchStyle
{
    DOMDocumentFragment *fragment = [DOMDocumentFragment _wrapDocumentFragment:m_frame->document()->createDocumentFragment().get()];
    [fragment appendChild:node];
    [self replaceSelectionWithFragment:fragment selectReplacement:selectReplacement smartReplace:smartReplace matchStyle:matchStyle];
}

- (void)replaceSelectionWithMarkupString:(NSString *)markupString baseURLString:(NSString *)baseURLString selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace
{
    DOMDocumentFragment *fragment = [self documentFragmentWithMarkupString:markupString baseURLString:baseURLString];
    [self replaceSelectionWithFragment:fragment selectReplacement:selectReplacement smartReplace:smartReplace matchStyle:NO];
}

- (void)replaceSelectionWithText:(NSString *)text selectReplacement:(BOOL)selectReplacement smartReplace:(BOOL)smartReplace
{
    [self replaceSelectionWithFragment:[self documentFragmentWithText:text
        inContext:[DOMRange _wrapRange:m_frame->selectionController()->toRange().get()]]
        selectReplacement:selectReplacement smartReplace:smartReplace matchStyle:YES];
}

- (void)setText:(NSString *)text asChildOfElement:(DOMElement *)element breakLines:(BOOL)breakLines
{
    if (!element || !m_frame || !m_frame->document())
        return;

    // clear out all current children of element
    RefPtr<Element> elem = [element _element];
    ASSERT(elem);
    elem->removeChildren();

    [self setMarkedText:@"" forCandidates:NO];

    m_frame->editor()->client()->respondToChangedContents();

    if ([text length]) {
        // insert new text
        // remove element from tree while doing it
        ExceptionCode ec;
        RefPtr<Node> parent = elem->parentNode();
        RefPtr<Node> siblingAfter = elem->nextSibling();
        if (parent)
            elem->remove(ec);    
    
        // set new text
        if (breakLines) {
            NSArray *paragraphs = [text componentsSeparatedByString:@"\n"];
            unsigned count = [paragraphs count];
            for (unsigned i = 0; i < count; i++) {
                ExceptionCode ec;
                // insert paragraph
                RefPtr<Element> paragraphNode(createDefaultParagraphElement(m_frame->document()));
                elem->appendChild(paragraphNode, ec);
                // insert text into paragraph
                NSString *paragraph = [paragraphs objectAtIndex:i];
                if ([paragraph length]) {
                    RefPtr<Text> textNode = m_frame->document()->createEditingTextNode(paragraph);
                    paragraphNode->appendChild(textNode, ec);
                }
                else {
                    RefPtr<Element> placeholder = createBlockPlaceholderElement(m_frame->document());
                    paragraphNode->appendChild(placeholder, ec);
                }
            }
        }
        else {
            ExceptionCode ec;
            RefPtr<Text> textNode = m_frame->document()->createEditingTextNode(text);
            elem->appendChild(textNode, ec);
        }
    
        // restore element to document
        if (parent) {
            if (siblingAfter)
                parent->insertBefore(elem, siblingAfter.get(), ec);
            else
                parent->appendChild(elem, ec);
        }
    }

    // set the selection to the end
    Selection selection;

    Position pos(elem.get(), elem->childNodeCount());

    VisiblePosition visiblePos(pos, VP_DEFAULT_AFFINITY);
    if (visiblePos.isNull())
        return;

    selection.setBase(visiblePos);
    selection.setExtent(visiblePos);
     
    m_frame->selectionController()->setSelection(selection);
    
    m_frame->editor()->client()->respondToChangedContents();
}

- (void)createDefaultFieldEditorDocumentStructure
{
    if (!m_frame)
        return;

    Document* doc = m_frame->document();
    if (!doc)
        return;


    // clear out all current children of the document
    doc->removeChildren();

    m_frame->editor()->client()->respondToChangedContents();

    ExceptionCode ec;

    RefPtr<Element> rootElement = doc->createElementNS(xhtmlNamespaceURI, "html", ec);
    doc->appendChild(rootElement, ec);

    RefPtr<Element> body = doc->createElementNS(xhtmlNamespaceURI, "body", ec);
    body->setAttribute(styleAttr, "margin: 0px; word-wrap: break-word; -khtml-nbsp-mode: space; -khtml-line-break: after-white-space; white-space: nowrap");
    rootElement->appendChild(body, ec);
    
    RefPtr<Element> sizeElement = doc->createElementNS(xhtmlNamespaceURI, "div", ec);
    sizeElement->setAttribute("id", "size", ec);
    sizeElement->setAttribute("contentEditable", "false", ec);
    sizeElement->setAttribute("unselectable", "on", ec);
    body->appendChild(sizeElement, ec);

    RefPtr<Element> textElement = doc->createElementNS(xhtmlNamespaceURI, "div", ec);
    textElement->setAttribute("id", "text", ec);
    textElement->setAttribute("contentEditable", "true", ec);
    textElement->setAttribute("unselectable", "off", ec);
    sizeElement->appendChild(textElement, ec);
    
    m_frame->editor()->client()->respondToChangedContents();
}


- (void)insertParagraphSeparatorInQuotedContent
{
    if (m_frame->selectionController()->isNone())
        return;
    
    TypingCommand::insertParagraphSeparatorInQuotedContent(m_frame->document());
    m_frame->revealSelection(RenderLayer::gAlignToEdgeIfNeeded);
}

- (VisiblePosition)_visiblePositionForPoint:(NSPoint)point
{
    IntPoint outerPoint(point);
    HitTestResult result = m_frame->eventHandler()->hitTestResultAtPoint(outerPoint, true);
    Node* node = result.innerNode();
    if (!node)
        return VisiblePosition();
    RenderObject* renderer = node->renderer();
    if (!renderer)
        return VisiblePosition();
    VisiblePosition visiblePos = renderer->positionForCoordinates(result.localPoint().x(), result.localPoint().y());
    if (visiblePos.isNull())
        visiblePos = VisiblePosition(Position(node, 0));
    return visiblePos;
}

- (DOMRange *)characterRangeAtPoint:(NSPoint)point
{
    VisiblePosition position = [self _visiblePositionForPoint:point];
    if (position.isNull())
        return nil;
    
    VisiblePosition previous = position.previous();
    if (previous.isNotNull()) {
        DOMRange *previousCharacterRange = [DOMRange _wrapRange:makeRange(previous, position).get()];
        NSRect rect = [self firstRectForDOMRange:previousCharacterRange];
        if (NSPointInRect(point, rect))
            return previousCharacterRange;
    }

    VisiblePosition next = position.next();
    if (next.isNotNull()) {
        DOMRange *nextCharacterRange = [DOMRange _wrapRange:makeRange(position, next).get()];
        NSRect rect = [self firstRectForDOMRange:nextCharacterRange];
        if (NSPointInRect(point, rect))
            return nextCharacterRange;
    }
    
    return nil;
}

- (BOOL)mouseDownMayStartDrag
{
    return m_frame->eventHandler()->mouseDownMayStartDrag();
}

- (void)setCaretColor:(CGColorRef)color
{
    Color qColor;
    if (color) {
        float r, g, b, a;
        GSColorGetRGBAComponents(color, &r, &g, &b, &a);
        qColor = Color(makeRGBA((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(a * 255)));
    } else {
        qColor = Color::black;
    }
    m_frame->setCaretColor(qColor);
}

static bool inSameEditableContent(const VisiblePosition &a, const VisiblePosition &b)
{
    Position ap = a.deepEquivalent();
    Node *an = ap.node();
    if (!an)
        return false;
        
    Position bp = b.deepEquivalent();
    Node *bn = bp.node();
    if (!bn)
        return false;
    
    if (!an->isContentEditable() || !bn->isContentEditable())
        return false;

    return an->rootEditableElement() == bn->rootEditableElement();
}

static bool isStartOfEditableContent(const VisiblePosition &p)
{
    return !inSameEditableContent(p, p.previous());
}

static bool isEndOfEditableContent(const VisiblePosition &p)
{
    return !inSameEditableContent(p, p.next());
}

- (void)resetSelection
{
    m_frame->selectionController()->setSelection(m_frame->selectionController()->selection());
}

- (void)moveSelectionToPoint:(NSPoint)point useSingleLineSelectionBehavior:(BOOL)useSingleLineSelectionBehavior;
{
    Selection selection([self _visiblePositionForPoint:point]);
    VisiblePosition caret(selection.start(), selection.affinity());

    Node *node = 0;

    if (caret.isNull()) {
        node = m_frame->document()->body();
    }
    else if (isEndOfEditableContent(caret) || isStartOfEditableContent(caret)) {
        node = caret.rootEditableElement();
    }
    
    if (node) {
        RenderObject *renderer = node->renderer();
        if (!renderer)
            return;
        m_frame->setSingleLineSelectionBehavior(useSingleLineSelectionBehavior);
        selection = Selection([self _visiblePositionForPoint:point]);
        m_frame->setSingleLineSelectionBehavior(false);
    }

    if (selection.isCaretOrRange())
        m_frame->selectionController()->setSelection(selection);
}

- (void)moveSelectionToPoint:(NSPoint)point
{
    [self moveSelectionToPoint:point useSingleLineSelectionBehavior:NO];
}

- (void)moveSelectionToStartOrEndOfCurrentWord
{
    // Note: this is the purple notion, not the unicode notion.
    // Here, a word starts with non-whitespace or at the start of a line and
    // ends at the next whitespace, or at the end of a line.

    // Selection rule: If the selection is before the first character or
    // just after the first character of a word that is longer than one
    // character, move to the start of the word. Otherwise, move to the
    // end of the word.

    Selection selection(m_frame->selectionController()->selection());
    if (selection.isNone())
        return;

    VisiblePosition pos(selection.start(), selection.affinity());
    if (pos.isNull())
        return;

    VisiblePosition originalPos(pos);

    CFCharacterSetRef set = CFCharacterSetGetPredefined(kCFCharacterSetWhitespaceAndNewline);

    unichar c = static_cast<unichar>(pos.characterAfter());
    if (c == 0 || CFCharacterSetIsCharacterMember(set, c)) {
        // search backward for a non-space
        while (1) {
            if (pos.isNull() || isStartOfLine(pos))
                break;
            unichar c = static_cast<unichar>(pos.characterBefore());
            if (!CFCharacterSetIsCharacterMember(set, c))
                break;
            pos = pos.previous(true);  // stay in editable content
        }
    }
    else {
        // See how far the selection extends into the current word.
        // Follow the rule stated above.
        int index = 0;
        while (1) {
            if (pos.isNull() || isStartOfLine(pos))
                break;
            unichar c = static_cast<unichar>(pos.characterBefore());
            if (CFCharacterSetIsCharacterMember(set, c))
                break;
            pos = pos.previous(true);  // stay in editable content
            index++;
            if (index > 1)
                break;
        }
        if (index > 1) {
            // search forward for a space
            pos = originalPos;
            while (1) {
                if (pos.isNull() || isEndOfLine(pos))
                    break;
                unichar c = static_cast<unichar>(pos.characterAfter());
                if (CFCharacterSetIsCharacterMember(set, c))
                    break;
                pos = pos.next(true);  // stay in editable content
            }
        }
    }

    if (pos.isNotNull() && pos != originalPos)
        m_frame->selectionController()->setSelection(pos);
}

- (DOMCSSStyleDeclaration *)typingStyle
{
    if (!m_frame || !m_frame->typingStyle())
        return nil;
    return [DOMCSSStyleDeclaration _wrapCSSStyleDeclaration:m_frame->typingStyle()->copy().get()];
}

- (void)setTypingStyle:(DOMCSSStyleDeclaration *)style withUndoAction:(EditAction)undoAction
{
    if (!m_frame)
        return;
    m_frame->computeAndSetTypingStyle([style _CSSStyleDeclaration], undoAction);
}

- (GSFontRef)fontForSelection:(BOOL *)hasMultipleFonts
{
    bool multipleFonts = false;
    GSFontRef font = nil;
    if (m_frame) {
        const SimpleFontData* fd = m_frame->editor()->fontForSelection(multipleFonts);
        if (fd)
            font = fd->getGSFont();
    }
    
    if (hasMultipleFonts)
        *hasMultipleFonts = multipleFonts;
    return font;
}


- (BOOL)getData:(NSData **)data andResponse:(NSURLResponse **)response forURL:(NSString *)url
{
    Document* doc = m_frame->document();
    if (!doc)
        return NO;

    CachedResource* resource = doc->docLoader()->cachedResource(url);
    if (!resource)
        return NO;

    SharedBuffer* buffer = resource->data();
    if (buffer)
        *data = [buffer->createNSData() autorelease];
    else
        *data = nil;

    *response = resource->response().nsURLResponse();
    return YES;
}

- (void)getAllResourceDatas:(NSArray **)datas andResponses:(NSArray **)responses
{
    Document* doc = m_frame->document();
    if (!doc) {
        NSArray* emptyArray = [NSArray array];
        *datas = emptyArray;
        *responses = emptyArray;
        return;
    }

    const HashMap<String, CachedResource*>& allResources = doc->docLoader()->allCachedResources();

    NSMutableArray *d = [[NSMutableArray alloc] initWithCapacity:allResources.size()];
    NSMutableArray *r = [[NSMutableArray alloc] initWithCapacity:allResources.size()];

    HashMap<String, CachedResource*>::const_iterator end = allResources.end();
    for (HashMap<String, CachedResource*>::const_iterator it = allResources.begin(); it != end; ++it) {
        SharedBuffer* buffer = it->second->data();
        NSData *data;
        if (buffer)
            data = buffer->createNSData();
        else
            data = [[NSData alloc] init];
        [d addObject:data];
        [data release];
        [r addObject:it->second->response().nsURLResponse()];
    }

    *datas = [d autorelease];
    *responses = [r autorelease];
}

- (BOOL)canProvideDocumentSource
{
    String mimeType = m_frame->loader()->responseMIMEType();
    
    if (WebCore::DOMImplementation::isTextMIMEType(mimeType) ||
        Image::supportsType(mimeType) ||
        PluginInfoStore::supportsMIMEType(mimeType))
        return NO;
    
    return YES;
}

- (BOOL)canSaveAsWebArchive
{
    // Currently, all documents that we can view source for
    // (HTML and XML documents) can also be saved as web archives
    return [self canProvideDocumentSource];
}

- (void)receivedData:(NSData *)data textEncodingName:(NSString *)textEncodingName
{
    // Set the encoding. This only needs to be done once, but it's harmless to do it again later.
    String encoding;
    if (m_frame)
        encoding = m_frame->loader()->documentLoader()->overrideEncoding();
    bool userChosen = !encoding.isNull();
    if (encoding.isNull())
        encoding = textEncodingName;
    m_frame->loader()->setEncoding(encoding, userChosen);
    [self addData:data];
}

- (void)setLayoutInterval:(int)interval
{
    m_frame->setLayoutInterval(interval);
}

- (void)setMaxParseDuration:(CFTimeInterval)duration
{
    m_frame->setMaxParseDuration(duration);
}

- (void)pauseTimeouts
{
    ASSERT(WebThreadIsLockedOrDisabled());
    
    if (m_frame == m_frame->page()->mainFrame()) {
        Frame *frame;
        JSLock lock;    
        //We are the main frame. Traverse all sub-frames and pause their timeouts.
        for (frame = m_frame; frame; frame = frame->tree()->traverseNext(m_frame)) {
            frame->clearTimers();
            frame->pauseTimeoutsAndSave();
        }
    }
}

- (void)resumeTimeouts
{
    ASSERT(WebThreadIsLockedOrDisabled());
    
    if (m_frame == m_frame->page()->mainFrame()) {
        Frame *frame;
        JSLock lock;    
        // We are the main frame. Traverse all sub-frames and resume their timeouts.
        for (frame = m_frame; frame; frame = frame->tree()->traverseNext(m_frame)) {
            frame->resumeSavedTimeouts();
            // Do what loading a page from pagecache does - force layout - which resumes marquee tags
            frame->forceLayout();
        }
    }
}

- (unsigned)formElementsCharacterCount
{
    Document* doc = m_frame->document();
    if (!doc)
        return 0;
    return doc->formElementsCharacterCount();
}

// -------------------

- (Frame*)_frame
{
    return m_frame;
}

#if ENABLE(HW_COMP)

- (BOOL)has3DContent
{
    return m_frame->has3DContent();
}

- (float)frameScale
{
    return m_frame->frameScale();
}

- (void)setFrameScale:(float)newScale
{
    m_frame->setFrameScale(newScale);
}

#endif

@end
