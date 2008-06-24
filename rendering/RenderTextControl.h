/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.s
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

#ifndef RenderTextControl_h
#define RenderTextControl_h

#include "PopupMenuClient.h"
#include "RenderBlock.h"

namespace WebCore {

class FontSelector;
class HTMLTextFieldInnerElement;
class HTMLTextFieldInnerTextElement;
class HTMLSearchFieldCancelButtonElement;
class HTMLSearchFieldResultsButtonElement;
class SearchPopupMenu;
class Selection;

class RenderTextControl : public RenderBlock {
public:
    RenderTextControl(Node*, bool multiLine);
    virtual ~RenderTextControl();

    virtual const char* renderName() const { return "RenderTextControl"; }

    virtual bool hasControlClip() const { return false; }
    virtual IntRect controlClipRect(int tx, int ty) const;
    virtual void calcHeight();
    virtual void calcPrefWidths();
    virtual void removeLeftoverAnonymousBlock(RenderBlock*) { }
    virtual void setStyle(RenderStyle*);
    virtual void updateFromElement();
    virtual bool canHaveChildren() const { return false; }
    virtual short baselinePosition(bool firstLine, bool isRootLineBox) const;
    virtual bool nodeAtPoint(const HitTestRequest&, HitTestResult&, int x, int y, int tx, int ty, HitTestAction);
    virtual void layout();
    virtual bool avoidsFloats() const { return true; }
    virtual void paint(PaintInfo&, int tx, int ty);
    
    virtual bool isEdited() const { return m_dirty; }
    virtual void setEdited(bool isEdited) { m_dirty = isEdited; }
    virtual bool isTextField() const { return !m_multiLine; }
    virtual bool isTextArea() const { return m_multiLine; }
    
    bool isUserEdited() const { return m_userEdited; }
    void setUserEdited(bool isUserEdited) { m_userEdited = isUserEdited; }

    int selectionStart();
    int selectionEnd();
    void setSelectionStart(int);
    void setSelectionEnd(int);
    void select();
    void setSelectionRange(int start, int end);
    Selection selection(int start, int end) const;

    void subtreeHasChanged();
    String text();
    String textWithHardLineBreaks();
    void forwardEvent(Event*);
    void selectionChanged(bool userTriggered);

    virtual bool shouldAutoscroll() const { return true; }
    virtual void autoscroll();

    // Subclassed to forward to our inner div.
    virtual int scrollLeft() const;
    virtual int scrollTop() const;
    virtual int scrollWidth() const;
    virtual int scrollHeight() const;
    virtual void setScrollLeft(int);
    virtual void setScrollTop(int);
    virtual bool scroll(ScrollDirection, ScrollGranularity, float multiplier = 1.0f);
    virtual bool isScrollable() const;

// FIXME: DDK: Use isScrollable() instead?
    bool canScroll() const;

    // Returns the line height of the inner renderer.
    virtual short innerLineHeight() const;

    VisiblePosition visiblePositionForIndex(int index);
    int indexForVisiblePosition(const VisiblePosition&);


    bool popupIsVisible() const { return m_searchPopupIsVisible; }

    
    bool placeholderIsVisible() const { return m_placeholderVisible; }

    virtual void capsLockStateMayHaveChanged();

private:

    RenderStyle* createInnerBlockStyle(RenderStyle* startStyle);
    RenderStyle* createInnerTextStyle(RenderStyle* startStyle);

    void updatePlaceholder();
    void createSubtreeIfNeeded();
    String finishText(Vector<UChar>&) const;

    RefPtr<HTMLTextFieldInnerElement> m_innerBlock;
    RefPtr<HTMLTextFieldInnerTextElement> m_innerText;

    bool m_dirty;
    bool m_multiLine;
    bool m_placeholderVisible;
    bool m_userEdited;

    bool m_searchPopupIsVisible;
    mutable Vector<String> m_recentSearches;

};

} // namespace WebCore

#endif // RenderTextControl_h
