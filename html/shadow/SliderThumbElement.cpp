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

#include "CSSValueKeywords.h"
#include "Event.h"
#include "Frame.h"
#include "HTMLInputElement.h"
#include "HTMLParserIdioms.h"
#include "MouseEvent.h"
#include "RenderDeprecatedFlexibleBox.h"
#include "RenderSlider.h"
#include "RenderTheme.h"
#include "ShadowRoot.h"
#include "ShadowTree.h"
#include "StepRange.h"
#include <wtf/MathExtras.h>

#include "Document.h"
#include "Page.h"
#include "TouchEvent.h"

using namespace std;

namespace WebCore {

inline static double sliderPosition(HTMLInputElement* element)
{
    StepRange range(element);
    return range.proportionFromValue(range.valueFromElement(element));
}

inline static bool hasVerticalAppearance(HTMLInputElement* input)
{
    ASSERT(input->renderer());
    RenderStyle* sliderStyle = input->renderer()->style();
    return sliderStyle->appearance() == SliderVerticalPart || sliderStyle->appearance() == MediaVolumeSliderPart;
}

SliderThumbElement* sliderThumbElementOf(Node* node)
{
    ASSERT(node);
    ShadowRoot* shadow = node->toInputElement()->shadowTree()->oldestShadowRoot();
    ASSERT(shadow);
    Node* thumb = shadow->firstChild()->firstChild()->firstChild();
    ASSERT(thumb);
    return toSliderThumbElement(thumb);
}

// --------------------------------

RenderSliderThumb::RenderSliderThumb(Node* node)
    : RenderBlock(node)
{
}

void RenderSliderThumb::updateAppearance(RenderStyle* parentStyle)
{
    if (parentStyle->appearance() == SliderVerticalPart)
        style()->setAppearance(SliderThumbVerticalPart);
    else if (parentStyle->appearance() == SliderHorizontalPart)
        style()->setAppearance(SliderThumbHorizontalPart);
    else if (parentStyle->appearance() == MediaSliderPart)
        style()->setAppearance(MediaSliderThumbPart);
    else if (parentStyle->appearance() == MediaVolumeSliderPart)
        style()->setAppearance(MediaVolumeSliderThumbPart);
    else if (parentStyle->appearance() == MediaFullScreenVolumeSliderPart)
        style()->setAppearance(MediaFullScreenVolumeSliderThumbPart);
    if (style()->hasAppearance())
        theme()->adjustSliderThumbSize(style());
}

bool RenderSliderThumb::isSliderThumb() const
{
    return true;
}

void RenderSliderThumb::layout()
{
    // Do not cast node() to SliderThumbElement. This renderer is used for
    // TrackLimitElement too.
    HTMLInputElement* input = node()->shadowAncestorNode()->toInputElement();
    bool isVertical = style()->appearance() == SliderThumbVerticalPart || style()->appearance() == MediaVolumeSliderThumbPart;

    double fraction = sliderPosition(input) * 100;
    if (isVertical)
        style()->setTop(Length(100 - fraction, Percent));
    else if (style()->isLeftToRightDirection())
        style()->setLeft(Length(fraction, Percent));
    else
        style()->setRight(Length(fraction, Percent));

    RenderBlock::layout();
}

// --------------------------------

// FIXME: Find a way to cascade appearance and adjust heights, and get rid of this class.
// http://webkit.org/b/62535
class RenderSliderContainer : public RenderDeprecatedFlexibleBox {
public:
    RenderSliderContainer(Node* node)
        : RenderDeprecatedFlexibleBox(node) { }

private:
    virtual void layout();
};

void RenderSliderContainer::layout()
{
    HTMLInputElement* input = node()->shadowAncestorNode()->toInputElement();
    bool isVertical = hasVerticalAppearance(input);
    style()->setBoxOrient(isVertical ? VERTICAL : HORIZONTAL);
    // Sets the concrete height if the height of the <input> is not fixed or a
    // percentage value because a percentage height value of this box won't be
    // based on the <input> height in such case.
    Length inputHeight = input->renderer()->style()->height();
    RenderObject* trackRenderer = node()->firstChild()->renderer();
    if (!isVertical && input->renderer()->isSlider() && !inputHeight.isFixed() && !inputHeight.isPercent()) {
        RenderObject* thumbRenderer = input->shadowTree()->oldestShadowRoot()->firstChild()->firstChild()->firstChild()->renderer();
        if (thumbRenderer) {
            style()->setHeight(thumbRenderer->style()->height());
            if (trackRenderer)
                trackRenderer->style()->setHeight(thumbRenderer->style()->height());
        }
    } else {
        style()->setHeight(Length(100, Percent));
        if (trackRenderer)
            trackRenderer->style()->setHeight(Length());
    }

    RenderDeprecatedFlexibleBox::layout();

    // Percentage 'top' for the thumb doesn't work if the parent style has no
    // concrete height.
    Node* track = node()->firstChild();
    if (track && track->renderer()->isBox()) {
        RenderBox* trackBox = track->renderBox();
        trackBox->style()->setHeight(Length(trackBox->height() - trackBox->borderAndPaddingHeight(), Fixed));
    }
}

// --------------------------------

void SliderThumbElement::setPositionFromValue()
{
    // Since the code to calculate position is in the RenderSliderThumb layout
    // path, we don't actually update the value here. Instead, we poke at the
    // renderer directly to trigger layout.
    if (renderer())
        renderer()->setNeedsLayout(true);
}

RenderObject* SliderThumbElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSliderThumb(this);
}

bool SliderThumbElement::isEnabledFormControl() const
{
    return hostInput()->isEnabledFormControl();
}

bool SliderThumbElement::isReadOnlyFormControl() const
{
    return hostInput()->isReadOnlyFormControl();
}

Node* SliderThumbElement::focusDelegate()
{
    return hostInput();
}

void SliderThumbElement::dragFrom(const LayoutPoint& point)
{
    setPositionFromPoint(point);
}

void SliderThumbElement::setPositionFromPoint(const LayoutPoint& point)
{
    HTMLInputElement* input = hostInput();

    if (!input->renderer() || !renderer())
        return;

    LayoutPoint offset = roundedLayoutPoint(input->renderer()->absoluteToLocal(point, false, true));
    bool isVertical = hasVerticalAppearance(input);
    LayoutUnit trackSize;
    LayoutUnit position;
    LayoutUnit currentPosition;
    // We need to calculate currentPosition from absolute points becaue the
    // renderer for this node is usually on a layer and renderBox()->x() and
    // y() are unusable.
    // FIXME: This should probably respect transforms.
    LayoutPoint absoluteThumbOrigin = renderBox()->absoluteBoundingBoxRectIgnoringTransforms().location();
    LayoutPoint absoluteSliderContentOrigin = roundedLayoutPoint(input->renderer()->localToAbsolute());
    if (isVertical) {
        trackSize = input->renderBox()->contentHeight() - renderBox()->height();
        position = offset.y() - renderBox()->height() / 2;
        currentPosition = absoluteThumbOrigin.y() - absoluteSliderContentOrigin.y();
    } else {
        trackSize = input->renderBox()->contentWidth() - renderBox()->width();
        position = offset.x() - renderBox()->width() / 2;
        currentPosition = absoluteThumbOrigin.x() - absoluteSliderContentOrigin.x();
    }
    position = max<LayoutUnit>(0, min(position, trackSize));
    if (position == currentPosition)
        return;

    StepRange range(input);
    double fraction = static_cast<double>(position) / trackSize;
    if (isVertical || !renderBox()->style()->isLeftToRightDirection())
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
    unregisterForTouchEvents();
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

    if (shouldAcceptTouchEvents())
        registerForTouchEvents();
}

void SliderThumbElement::handleTouchEvent(TouchEvent* touchEvent)
{
    HTMLInputElement* input = hostInput();
    if (input->isReadOnlyFormControl() || !input->isEnabledFormControl()) {
        clearExclusiveTouchIdentifier();
        stopDragging();
        touchEvent->setDefaultHandled();
        HTMLDivElement::defaultEventHandler(touchEvent);
        return;
    }

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

bool SliderThumbElement::shouldAcceptTouchEvents()
{
    if (!attached())
        return false;

    if (!isEnabledFormControl())
        return false;

    return true;
}

void SliderThumbElement::registerForTouchEvents()
{
    if (m_isRegisteredAsTouchEventListener)
        return;

    ASSERT(shouldAcceptTouchEvents());

    document()->addTouchEventListener(this);
    m_isRegisteredAsTouchEventListener = true;
}

void SliderThumbElement::unregisterForTouchEvents()
{
    if (!m_isRegisteredAsTouchEventListener)
        return;

    clearExclusiveTouchIdentifier();
    stopDragging();

    document()->removeTouchEventListener(this);
    m_isRegisteredAsTouchEventListener = false;
}

void SliderThumbElement::disabledAttributeChanged()
{
    if (shouldAcceptTouchEvents())
        registerForTouchEvents();
    else
        unregisterForTouchEvents();
}

HTMLInputElement* SliderThumbElement::hostInput() const
{
    // Only HTMLInputElement creates SliderThumbElement instances as its shadow nodes.
    // So, shadowAncestorNode() must be an HTMLInputElement.
    return shadowAncestorNode()->toInputElement();
}

const AtomicString& SliderThumbElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, sliderThumb, ("-webkit-slider-thumb"));
    return sliderThumb;
}

// --------------------------------

inline TrackLimiterElement::TrackLimiterElement(Document* document)
    : HTMLDivElement(HTMLNames::divTag, document)
{
}

PassRefPtr<TrackLimiterElement> TrackLimiterElement::create(Document* document)
{
    RefPtr<TrackLimiterElement> element = adoptRef(new TrackLimiterElement(document));

    element->setInlineStyleProperty(CSSPropertyVisibility, CSSValueHidden);
    element->setInlineStyleProperty(CSSPropertyPosition, CSSValueStatic);

    return element.release();
}

RenderObject* TrackLimiterElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSliderThumb(this);
}

const AtomicString& TrackLimiterElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, sliderThumb, ("-webkit-slider-thumb"));
    return sliderThumb;
}

TrackLimiterElement* trackLimiterElementOf(Node* node)
{
    ASSERT(node);
    ASSERT(node->toInputElement()->hasShadowRoot());
    ShadowRoot* shadow = node->toInputElement()->shadowTree()->oldestShadowRoot();
    ASSERT(shadow);
    Node* limiter = shadow->firstChild()->lastChild();
    ASSERT(limiter);
    return static_cast<TrackLimiterElement*>(limiter);
}

// --------------------------------

inline SliderContainerElement::SliderContainerElement(Document* document)
    : HTMLDivElement(HTMLNames::divTag, document)
{
}

PassRefPtr<SliderContainerElement> SliderContainerElement::create(Document* document)
{
    return adoptRef(new SliderContainerElement(document));
}

RenderObject* SliderContainerElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderSliderContainer(this);
}

const AtomicString& SliderContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, sliderThumb, ("-webkit-slider-container"));
    return sliderThumb;
}

}
