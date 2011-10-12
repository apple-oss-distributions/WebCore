/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#include "config.h"
#include "SliderThumbElement.h"

#include "Event.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLParserIdioms.h"
#include "MouseEvent.h"
#include "RenderSlider.h"
#include "RenderTheme.h"
#include "StepRange.h"
#include <wtf/MathExtras.h>

#include "Document.h"
#include "Page.h"
#include "TouchEvent.h"

using namespace std;

namespace WebCore {

// FIXME: Find a way to cascade appearance (see the layout method) and get rid of this class.
class RenderSliderThumb : public RenderBlock {
public:
    RenderSliderThumb(Node*);

private:
    virtual void layout();
};

RenderSliderThumb::RenderSliderThumb(Node* node)
    : RenderBlock(node)
{
}

void RenderSliderThumb::layout()
{
    // FIXME: Hard-coding this cascade of appearance is bad, because it's something
    // that CSS usually does. We need to find a way to express this in CSS.
    RenderStyle* parentStyle = parent()->style();
    if (parentStyle->appearance() == SliderVerticalPart)
        style()->setAppearance(SliderThumbVerticalPart);
    else if (parentStyle->appearance() == SliderHorizontalPart)
        style()->setAppearance(SliderThumbHorizontalPart);
    else if (parentStyle->appearance() == MediaSliderPart)
        style()->setAppearance(MediaSliderThumbPart);
    else if (parentStyle->appearance() == MediaVolumeSliderPart)
        style()->setAppearance(MediaVolumeSliderThumbPart);

    if (style()->hasAppearance()) {
        // FIXME: This should pass the style, not the renderer, to the theme.
        theme()->adjustSliderThumbSize(this);
    }
    RenderBlock::layout();
}

void SliderThumbElement::setPositionFromValue()
{
    // Since today the code to calculate position is in the RenderSlider layout
    // path, we don't actually update the value here. Instead, we poke at the
    // renderer directly to trigger layout.
    // FIXME: Move the logic of positioning the thumb here.
    if (renderer())
        renderer()->setNeedsLayout(true);
}

RenderObject* SliderThumbElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSliderThumb(this);
}

void SliderThumbElement::dragFrom(const IntPoint& point)
{
    setPositionFromPoint(point);
}

void SliderThumbElement::setPositionFromPoint(const IntPoint& point)
{
    HTMLInputElement* input = hostInput();
    ASSERT(input);

    if (!input->renderer() || !renderer())
        return;

    IntPoint offset = roundedIntPoint(input->renderer()->absoluteToLocal(point, false, true));
    RenderStyle* sliderStyle = input->renderer()->style();
    bool isVertical = sliderStyle->appearance() == SliderVerticalPart || sliderStyle->appearance() == MediaVolumeSliderPart;

    int trackSize;
    int position;
    int currentPosition;
    if (isVertical) {
        trackSize = input->renderBox()->contentHeight() - renderBox()->height();
        position = offset.y() - renderBox()->height() / 2;
        currentPosition = renderBox()->y() - input->renderBox()->contentBoxRect().y();
    } else {
        trackSize = input->renderBox()->contentWidth() - renderBox()->width();
        position = offset.x() - renderBox()->width() / 2;
        currentPosition = renderBox()->x() - input->renderBox()->contentBoxRect().x();
    }
    position = max(0, min(position, trackSize));
    if (position == currentPosition)
        return;

    StepRange range(input);
    double fraction = static_cast<double>(position) / trackSize;
    if (isVertical)
        fraction = 1 - fraction;
    double value = range.clampValue(range.valueFromProportion(fraction));

    // FIXME: This is no longer being set from renderer. Consider updating the method name.
    input->setValueFromRenderer(serializeForNumberType(value));
    renderer()->setNeedsLayout(true);
    input->dispatchFormControlChangeEvent();
}

void SliderThumbElement::startDragging()
{
    if (Frame* frame = document()->frame()) {
        frame->eventHandler()->setCapturingMouseEventsNode(this);
        m_inDragMode = true;
    }
}

void SliderThumbElement::stopDragging()
{
    if (!m_inDragMode)
        return;

    if (Frame* frame = document()->frame())
        frame->eventHandler()->setCapturingMouseEventsNode(0);
    m_inDragMode = false;
    if (renderer())
        renderer()->setNeedsLayout(true);
}


void SliderThumbElement::detach()
{
    if (m_inDragMode) {
        if (Frame* frame = document()->frame())
            frame->eventHandler()->setCapturingMouseEventsNode(0);      
    }
    // This balances the add in attach. The document's touch
    // rect list is a counted set, so it would be nice to
    // be balanced here, as this element could be attached
    // and detached multiple times.
    document()->removeTouchEventListener(this);
    HTMLDivElement::detach();
}

unsigned SliderThumbElement::exclusiveTouchIdentifier() const
{
    return m_exclusiveTouchIdentifier;
}

void SliderThumbElement::setExclusiveTouchIdentifier(unsigned identifier)
{
    ASSERT(m_exclusiveTouchIdentifier == NoIdentifier);
    m_exclusiveTouchIdentifier = identifier;
}

void SliderThumbElement::clearExclusiveTouchIdentifier()
{
    m_exclusiveTouchIdentifier = NoIdentifier;
}

static Touch* findTouchWithIdentifier(TouchList* list, unsigned identifier)
{
    for (unsigned i = 0; i < list->length(); ++i) {
        Touch* touch = list->item(i);
        if (touch->identifier() == identifier)
            return touch;
    }

    return 0;
}

void SliderThumbElement::handleTouchStart(TouchEvent* touchEvent)
{
    TouchList* targetTouches = touchEvent->targetTouches();
    if (targetTouches->length() != 1)
        return;

    // Ignore the touch if it is not really inside the thumb.
    Touch* touch = targetTouches->item(0);
    IntRect boundingBox = renderer()->absoluteBoundingBoxRect();
    if (!boundingBox.contains(touch->pageX(), touch->pageY()))
        return;

    setExclusiveTouchIdentifier(touch->identifier());

    startDragging();
    touchEvent->setDefaultHandled();
}

void SliderThumbElement::handleTouchMove(TouchEvent* touchEvent)
{
    unsigned identifier = exclusiveTouchIdentifier();
    if (identifier == NoIdentifier)
        return;

    Touch* touch = findTouchWithIdentifier(touchEvent->targetTouches(), identifier);
    if (!touch)
        return;

    if (m_inDragMode)
        setPositionFromPoint(IntPoint(touch->pageX(), touch->pageY()));
    touchEvent->setDefaultHandled();
}

void SliderThumbElement::handleTouchEndAndCancel(TouchEvent* touchEvent)
{
    unsigned identifier = exclusiveTouchIdentifier();
    if (identifier == NoIdentifier)
        return;

    // If our exclusive touch still exists, it was not the touch
    // that ended, so we should not stop dragging.
    Touch* exclusiveTouch = findTouchWithIdentifier(touchEvent->targetTouches(), identifier);
    if (exclusiveTouch)
        return;

    clearExclusiveTouchIdentifier();

    stopDragging();
}

void SliderThumbElement::attach()
{
    HTMLDivElement::attach();

    // This adds the thumb rect to the Document's list
    // of touch rects, so that we receive touch events.
    document()->addTouchEventListener(this);
}

void SliderThumbElement::handleTouchEvent(TouchEvent* touchEvent)
{
    const AtomicString& eventType = touchEvent->type();
    if (eventType == eventNames().touchstartEvent) {
        handleTouchStart(touchEvent);
        return;
    } else if (eventType == eventNames().touchendEvent || eventType == eventNames().touchcancelEvent) {
        handleTouchEndAndCancel(touchEvent);
        return;
    } else if (eventType == eventNames().touchmoveEvent) {
        handleTouchMove(touchEvent);
        return;
    }

    HTMLDivElement::defaultEventHandler(touchEvent);
}

HTMLInputElement* SliderThumbElement::hostInput()
{
    ASSERT(parentNode());
    return static_cast<HTMLInputElement*>(parentNode()->shadowHost());
}

const AtomicString& SliderThumbElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, sliderThumb, ("-webkit-slider-thumb"));
    return sliderThumb;
}

}

