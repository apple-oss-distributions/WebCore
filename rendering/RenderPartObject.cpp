/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 * Copyright (C) 2004, 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
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

#include "config.h"
#include "RenderPartObject.h"

#include "Frame.h"
#include "FrameLoaderClient.h"
#include "HTMLEmbedElement.h"
#include "HTMLIFrameElement.h"
#include "HTMLNames.h"
#include "HTMLObjectElement.h"
#include "HTMLParamElement.h"
#include "MIMETypeRegistry.h"
#include "Page.h"
#include "PluginData.h"
#include "RenderView.h"
#include "RenderWidgetProtector.h"
#include "Text.h"

#include "Settings.h"

#if ENABLE(PLUGIN_PROXY_FOR_VIDEO)
#include "HTMLVideoElement.h"
#endif

namespace WebCore {

using namespace HTMLNames;

RenderPartObject::RenderPartObject(Element* element)
    : RenderPart(element)
    , m_didResizeFrameToContent(false)
{
}

void RenderPartObject::calcWidth()
{
    RenderPart::calcWidth();
    if (!suggestResizeFrameToContent())
        return;
    if (!style()->width().isFixed() || static_cast<HTMLIFrameElement*>(node())->scrollingMode() != ScrollbarAlwaysOff)
        setWidth(max(width(), static_cast<FrameView*>(widget())->contentsWidth()));
}
    
void RenderPartObject::calcHeight()
{
    RenderPart::calcHeight();
    if (!suggestResizeFrameToContent())
        return;
    if (!style()->height().isFixed() || static_cast<HTMLIFrameElement*>(node())->scrollingMode() != ScrollbarAlwaysOff)
        setHeight(max(height(), static_cast<FrameView*>(widget())->contentsHeight()));
}
  
bool RenderPartObject::suggestResizeFrameToContent() const
{
    Document* document = node() ? node()->document() : 0;
    return widget() && widget()->isFrameView() && document && node()->hasTagName(iframeTag) && document->frame() && 
        document->frame()->settings() && document->frame()->settings()->flatFrameSetLayoutEnabled() &&
        !style()->width().isZero() && !style()->height().isZero() &&
        (static_cast<HTMLIFrameElement*>(node())->scrollingMode() != ScrollbarAlwaysOff || !style()->width().isFixed() || !style()->height().isFixed());
}

void RenderPartObject::layout()
{
    ASSERT(needsLayout());

    FrameView* childFrameView;
    RenderView* childRoot;
    bool shouldResize = suggestResizeFrameToContent();
    
    if (shouldResize) {
        childFrameView = static_cast<FrameView*>(widget());
        childRoot = childFrameView->frame()->contentRenderer();
        if (childRoot && childRoot->prefWidthsDirty())
            childRoot->calcPrefWidths();
    }

    RenderPart::calcWidth();
    RenderPart::calcHeight();
    adjustOverflowForBoxShadowAndReflect();

    // Determine if the frame is off the screen; if so, then don't resize it.
    // Positioned elements that are positioned from the root node will have valid dimensions at this point.
    if (shouldResize && isPositioned() && containingBlock() == view() && ((x() + width() <= 0) || (y() + height() <= 0)))
        shouldResize = false;

    if (shouldResize) {
        bool scrolling = static_cast<HTMLIFrameElement*>(node())->scrollingMode() != ScrollbarAlwaysOff;
        if (childRoot && (scrolling || !style()->width().isFixed()))
            setWidth(max(width(), childRoot->minPrefWidth()));

        updateWidgetPosition();
        do
            childFrameView->layout();
        while (childFrameView->layoutPending() || (childRoot && childRoot->needsLayout()));
        
        if (scrolling || !style()->height().isFixed())
            setHeight(max(height(), childFrameView->contentsHeight()));
        if (scrolling || !style()->width().isFixed())
            setWidth(max(width(), childFrameView->contentsWidth()));
        
        updateWidgetPosition();
        
        m_didResizeFrameToContent = true;
        
        ASSERT(!childFrameView->layoutPending());
        ASSERT(!childRoot || !childRoot->needsLayout());
        ASSERT(!childRoot || !childRoot->firstChild() || !childRoot->firstChild()->firstChild() || !childRoot->firstChild()->firstChild()->needsLayout());
    } else {
        if (m_didResizeFrameToContent && widget()) {
            //  Acid3 test 46: We have to trigger a relayout to update media queries if we were in resized mode earlier.
            updateWidgetPosition();
            static_cast<FrameView*>(widget())->layout();
        }
        
        m_didResizeFrameToContent = false;
        
        RenderPart::layout();
    }

    setNeedsLayout(false);
}

void RenderPartObject::viewCleared()
{
    if (node() && widget() && widget()->isFrameView()) {
        FrameView* view = static_cast<FrameView*>(widget());
        int marginw = -1;
        int marginh = -1;
        if (node()->hasTagName(iframeTag)) {
            HTMLIFrameElement* frame = static_cast<HTMLIFrameElement*>(node());
            marginw = frame->getMarginWidth();
            marginh = frame->getMarginHeight();
        }
        if (marginw != -1)
            view->setMarginWidth(marginw);
        if (marginh != -1)
            view->setMarginHeight(marginh);
    }
}

}
