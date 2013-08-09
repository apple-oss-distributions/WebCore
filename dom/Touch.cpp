/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#include "config.h"
#include "Touch.h"

#if ENABLE(TOUCH_EVENTS)

#include "Touch.h"

#include "DOMWindow.h"
#include "Frame.h"
#include "FrameView.h"

namespace WebCore {

// Cribbed from MouseRelatedEvent
static int contentsX(DOMWindow* abstractView)
{
    if (!abstractView)
        return 0;
    Frame* frame = abstractView->frame();
    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;
#if !PLATFORM(IOS)
    return frameView->scrollX() / frame->pageZoomFactor() / frame->frameScaleFactor();
#else
    return frameView->actualScrollX() / frame->pageZoomFactor() / frame->frameScaleFactor();
#endif
}

static int contentsY(DOMWindow* abstractView)
{
    if (!abstractView)
        return 0;
    Frame* frame = abstractView->frame();
    if (!frame)
        return 0;
    FrameView* frameView = frame->view();
    if (!frameView)
        return 0;
#if !PLATFORM(IOS)
    return frameView->scrollY() / frame->pageZoomFactor() / frame->frameScaleFactor();
#else
    return frameView->actualScrollY() / frame->pageZoomFactor() / frame->frameScaleFactor();
#endif
}

Touch::Touch(DOMWindow* view, EventTarget* target, unsigned identifier, int pageX, int pageY, int screenX, int screenY)
    : m_view(view)
    , m_target(target)
    , m_identifier(identifier)
    , m_clientX(pageX - contentsX(view))
    , m_clientY(pageY - contentsY(view))
    , m_pageX(pageX)
    , m_pageY(pageY)
    , m_screenX(screenX)
    , m_screenY(screenY)
{
#if !PLATFORM(IOS)
    float scaleFactor = frame->pageZoomFactor() * frame->frameScaleFactor();
    float x = pageX * scaleFactor;
    float y = pageY * scaleFactor;
    m_absoluteLocation = roundedLayoutPoint(FloatPoint(x, y));
#endif
}

bool Touch::updateLocation(int pageX, int pageY, int screenX, int screenY)
{
    if (pageX == m_pageX && pageY == m_pageY && screenX == m_screenX && screenY == m_screenY)
        return false;
        
    m_pageX = pageX;
    m_pageY = pageY;
    m_screenX = screenX;
    m_screenY = screenY;

    m_clientX = (pageX - contentsX(view()));
    m_clientY = (pageY - contentsY(view()));

    return true;
}

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)
