/*
 * Copyright (C) 2018 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InlineContentBreaker.h"

#if ENABLE(LAYOUT_FORMATTING_CONTEXT)

#include "FontCascade.h"
#include "Hyphenation.h"
#include "InlineItem.h"
#include "InlineTextItem.h"
#include "TextUtil.h"

namespace WebCore {
namespace Layout {

static inline bool isTextContent(const InlineContentBreaker::ContinuousContent& continuousContent)
{
    // <span>text</span> is considered a text run even with the [container start][container end] inline items.
    // Due to commit boundary rules, we just need to check the first non-typeless inline item (can't have both [img] and [text])
    for (auto& run : continuousContent.runs()) {
        auto& inlineItem = run.inlineItem;
        if (inlineItem.isInlineBoxStart() || inlineItem.isInlineBoxEnd())
            continue;
        return inlineItem.isText();
    }
    return false;
}

static inline bool isVisuallyEmptyWhitespaceContent(const InlineContentBreaker::ContinuousContent& continuousContent)
{
    // [<span></span> ] [<span> </span>] [ <span style="padding: 0px;"></span>] are all considered visually empty whitespace content.
    // [<span style="border: 1px solid red"></span> ] while this is whitespace content only, it is not considered visually empty.
    // Due to commit boundary rules, we just need to check the first non-typeless inline item (can't have both [img] and [text])
    for (auto& run : continuousContent.runs()) {
        auto& inlineItem = run.inlineItem;
        // FIXME: check for padding border etc.
        if (inlineItem.isInlineBoxStart() || inlineItem.isInlineBoxEnd())
            continue;
        return inlineItem.isText() && downcast<InlineTextItem>(inlineItem).isWhitespace();
    }
    return false;
}

static inline bool isNonContentRunsOnly(const InlineContentBreaker::ContinuousContent& continuousContent)
{
    // <span></span> <- non content runs.
    for (auto& run : continuousContent.runs()) {
        auto& inlineItem = run.inlineItem;
        if (inlineItem.isInlineBoxStart() || inlineItem.isInlineBoxEnd())
            continue;
        return false;
    }
    return true;
}

static inline Optional<size_t> firstTextRunIndex(const InlineContentBreaker::ContinuousContent& continuousContent)
{
    auto& runs = continuousContent.runs();
    for (size_t index = 0; index < runs.size(); ++index) {
        if (runs[index].inlineItem.isText())
            return index;
    }
    return { };
}

static inline bool isWrappingAllowed(const InlineItem& inlineItem)
{
    // Do not try to wrap overflown 'pre' and 'no-wrap' content to next line.
    auto& styleToUse = inlineItem.isBox() ? inlineItem.layoutBox().parent().style() : inlineItem.layoutBox().style();
    return styleToUse.whiteSpace() != WhiteSpace::Pre && styleToUse.whiteSpace() != WhiteSpace::NoWrap;
}

static inline Optional<size_t> lastWrapOpportunityIndex(const InlineContentBreaker::ContinuousContent::RunList& runList)
{
    // <span style="white-space: pre">no_wrap</span><span>yes wrap</span><span style="white-space: pre">no_wrap</span>.
    // [container start][no_wrap][container end][container start][yes] <- continuous content
    // [ ] <- continuous content
    // [wrap][container end][container start][no_wrap][container end] <- continuous content
    // Return #0 as the index where the second continuous content can wrap at.
    ASSERT(!runList.isEmpty());
    auto lastItemIndex = runList.size() - 1;
    return isWrappingAllowed(runList[lastItemIndex].inlineItem) ? makeOptional(lastItemIndex) : WTF::nullopt;
}

bool InlineContentBreaker::shouldKeepEndOfLineWhitespace(const ContinuousContent& continuousContent) const
{
    // Grab the style and check for white-space property to decide whether we should let this whitespace content overflow the current line.
    // Note that the "keep" in this context means we let the whitespace content sit on the current line.
    // It might very well get collapsed when we close the line (normal/nowrap/pre-line).
    // See https://www.w3.org/TR/css-text-3/#white-space-property
    auto whitespace = continuousContent.runs()[*firstTextRunIndex(continuousContent)].inlineItem.style().whiteSpace();
    return whitespace == WhiteSpace::Normal || whitespace == WhiteSpace::NoWrap || whitespace == WhiteSpace::PreWrap || whitespace == WhiteSpace::PreLine;
}

InlineContentBreaker::Result InlineContentBreaker::processInlineContent(const ContinuousContent& candidateContent, const LineStatus& lineStatus)
{
    auto processCandidateContent = [&] {
        if (candidateContent.logicalWidth() <= lineStatus.availableWidth)
            return Result { Result::Action::Keep };
#if USE_FLOAT_AS_INLINE_LAYOUT_UNIT
        // Preferred width computation sums up floats while line breaker subtracts them. This can lead to epsilon-scale differences.
        if (WTF::areEssentiallyEqual(candidateContent.logicalWidth(), lineStatus.availableWidth))
            return Result { Result::Action::Keep };
#endif
        return processOverflowingContent(candidateContent, lineStatus);
    };

    auto result = processCandidateContent();
    if (result.action == Result::Action::Keep) {
        // If this is not the end of the line, hold on to the last eligible line wrap opportunity so that we could revert back
        // to this position if no other line breaking opportunity exists in this content.
        if (auto lastLineWrapOpportunityIndex = lastWrapOpportunityIndex(candidateContent.runs())) {
            auto isEligibleLineWrapOpportunity = [&] (auto& candidateItem) {
                // Just check for leading preserved whitespace for now.
                if (!lineStatus.isEmpty || !is<InlineTextItem>(candidateItem))
                    return true;
                auto inlineTextItem = downcast<InlineTextItem>(candidateItem);
                return !inlineTextItem.isWhitespace() || InlineTextItem::shouldPreserveSpacesAndTabs(inlineTextItem);
            };
            auto& lastWrapOpportunityCandidateItem = candidateContent.runs()[*lastLineWrapOpportunityIndex].inlineItem;
            if (isEligibleLineWrapOpportunity(lastWrapOpportunityCandidateItem)) {
                result.lastWrapOpportunityItem = &lastWrapOpportunityCandidateItem;
                m_hasWrapOpportunityAtPreviousPosition = true;
            }
        }
    } else if (result.action == Result::Action::Wrap) {
        if (lineStatus.trailingSoftHyphenWidth && isTextContent(candidateContent)) {
            // A trailing soft hyphen with a wrapped text content turns into a visible hyphen.
            // Let's check if there's enough space for the hyphen character.
            auto hyphenOverflows = *lineStatus.trailingSoftHyphenWidth > lineStatus.availableWidth; 
            auto action = hyphenOverflows ? Result::Action::RevertToLastNonOverflowingWrapOpportunity : Result::Action::WrapWithHyphen;
            result = { action, IsEndOfLine::Yes };
        }
    }
    return result;
}

struct TrailingTextContent {
    size_t runIndex { 0 };
    bool hasOverflow { false }; // Trailing content still overflows the line.
    Optional<InlineContentBreaker::PartialRun> partialRun;
};

InlineContentBreaker::Result InlineContentBreaker::processOverflowingContent(const ContinuousContent& overflowContent, const LineStatus& lineStatus) const
{
    auto continuousContent = ContinuousContent { overflowContent };
    ASSERT(!continuousContent.runs().isEmpty());

    ASSERT(continuousContent.logicalWidth() > lineStatus.availableWidth);
    if (continuousContent.hasTrailingCollapsibleContent()) {
        ASSERT(isTextContent(continuousContent));
        // First check if the content fits without the trailing collapsible part.
        if (continuousContent.nonCollapsibleLogicalWidth() <= lineStatus.availableWidth)
            return { Result::Action::Keep, IsEndOfLine::No };
        // Now check if we can trim the line too.
        if (lineStatus.hasFullyCollapsibleTrailingRun && continuousContent.isFullyCollapsible()) {
            // If this new content is fully collapsible, it should surely fit.
            return { Result::Action::Keep, IsEndOfLine::No };
        }
    } else if (lineStatus.collapsibleWidth && isNonContentRunsOnly(continuousContent)) {
        // Let's see if the non-content runs fit when the line has trailing collapsible content.
        // "text content <span style="padding: 1px"></span>" <- the <span></span> runs could fit after collapsing the trailing whitespace.
        if (continuousContent.logicalWidth() <= lineStatus.availableWidth + lineStatus.collapsibleWidth)
            return { Result::Action::Keep };
    }
    if (isVisuallyEmptyWhitespaceContent(continuousContent) && shouldKeepEndOfLineWhitespace(continuousContent)) {
        // This overflowing content apparently falls into the remove/hang end-of-line-spaces category.
        // see https://www.w3.org/TR/css-text-3/#white-space-property matrix
        return { Result::Action::Keep };
    }

    if (isTextContent(continuousContent)) {
        if (auto trailingContent = processOverflowingTextContent(continuousContent, lineStatus)) {
            if (!trailingContent->runIndex && trailingContent->hasOverflow) {
                // We tried to break the content but the available space can't even accommodate the first character.
                // 1. Wrap the content over to the next line when we've got content on the line already.
                // 2. Keep the first character on the empty line (or keep the whole run if it has only one character/completely empty).
                if (!lineStatus.isEmpty)
                    return { Result::Action::Wrap, IsEndOfLine::Yes };
                auto leadingTextRunIndex = *firstTextRunIndex(continuousContent);
                auto& inlineTextItem = downcast<InlineTextItem>(continuousContent.runs()[leadingTextRunIndex].inlineItem);
                if (inlineTextItem.length() <= 1)
                    return Result { Result::Action::Keep, IsEndOfLine::Yes };
                auto firstCharacterWidth = TextUtil::width(inlineTextItem, inlineTextItem.start(), inlineTextItem.start() + 1, lineStatus.contentLogicalRight);
                auto firstCharacterRun = PartialRun { 1, firstCharacterWidth };
                return { Result::Action::Break, IsEndOfLine::Yes, Result::PartialTrailingContent { leadingTextRunIndex, firstCharacterRun } };
            }
            auto trailingPartialContent = Result::PartialTrailingContent { trailingContent->runIndex, trailingContent->partialRun };
            return { Result::Action::Break, IsEndOfLine::Yes, trailingPartialContent };
        }
    }
    // If we are not allowed to break this overflowing content, we still need to decide whether keep it or wrap it to the next line.
    if (lineStatus.isEmpty) {
        ASSERT(!m_hasWrapOpportunityAtPreviousPosition);
        return { Result::Action::Keep, IsEndOfLine::No };
    }
    // Now either wrap this content over to the next line or revert back to an earlier wrapping opportunity, or not wrap at all.
    auto shouldWrapThisContentToNextLine = [&] {
        // The individual runs in this continuous content don't break, let's check if we are allowed to wrap this content to next line (e.g. pre would prevent us from wrapping).
        auto& runs = continuousContent.runs();
        auto& lastInlineItem = runs.last().inlineItem;
        // Parent style drives the wrapping behavior here.
        // e.g. <div style="white-space: nowrap">some text<div style="display: inline-block; white-space: pre-wrap"></div></div>.
        // While the inline-block has pre-wrap which allows wrapping, the content lives in a nowrap context.
        if (lastInlineItem.isBox() || lastInlineItem.isInlineBoxStart() || lastInlineItem.isInlineBoxEnd())
            return isWrappingAllowed(lastInlineItem);
        if (lastInlineItem.isText()) {
            if (runs.size() == 1) {
                // Fast path for the most common case of an individual text item.
                return isWrappingAllowed(lastInlineItem);
            }
            for (auto& run : WTF::makeReversedRange(runs)) {
                auto& inlineItem = run.inlineItem;
                if (inlineItem.isInlineBoxStart() || inlineItem.isInlineBoxStart())
                    return isWrappingAllowed(inlineItem);
                ASSERT(!inlineItem.isBox());
            }
            // This must be a set of individual text runs. We could just check the last item.
            return isWrappingAllowed(lastInlineItem);
        }
        ASSERT_NOT_REACHED();
        return true;
    };
    if (shouldWrapThisContentToNextLine())
        return { Result::Action::Wrap, IsEndOfLine::Yes };
    if (m_hasWrapOpportunityAtPreviousPosition)
        return { Result::Action::RevertToLastWrapOpportunity, IsEndOfLine::Yes };
    return { Result::Action::Keep, IsEndOfLine::No };
}

Optional<TrailingTextContent> InlineContentBreaker::processOverflowingTextContent(const ContinuousContent& continuousContent, const LineStatus& lineStatus) const
{
    auto isBreakableRun = [] (auto& run) {
        ASSERT(run.inlineItem.isText() || run.inlineItem.isInlineBoxStart() || run.inlineItem.isInlineBoxEnd());
        if (!run.inlineItem.isText()) {
            // Can't break horizontal spacing -> e.g. <span style="padding-right: 100px;">textcontent</span>, if the [container end] is the overflown inline item
            // we need to check if there's another inline item beyond the [container end] to split.
            return false;
        }
        // Check if this text run needs to stay on the current line.  
        return isWrappingAllowed(run.inlineItem);
    };

    // Check where the overflow occurs and use the corresponding style to figure out the breaking behaviour.
    // <span style="word-break: normal">first</span><span style="word-break: break-all">second</span><span style="word-break: normal">third</span>
    auto& runs = continuousContent.runs();
    auto accumulatedRunWidth = InlineLayoutUnit { }; 
    size_t index = 0;
    while (index < runs.size()) {
        auto& run = runs[index];
        ASSERT(run.inlineItem.isText() || run.inlineItem.isInlineBoxStart() || run.inlineItem.isInlineBoxEnd());
        if (accumulatedRunWidth + run.logicalWidth > lineStatus.availableWidth && isBreakableRun(run)) {
            // At this point the available width can very well be negative e.g. when some part of the continuous text content can not be broken into parts ->
            // <span style="word-break: keep-all">textcontentwithnobreak</span><span>textcontentwithyesbreak</span>
            // When the first span computes longer than the available space, by the time we get to the second span, the adjusted available space becomes negative.
            auto adjustedAvailableWidth = std::max<InlineLayoutUnit>(0, lineStatus.availableWidth - accumulatedRunWidth);
            if (auto partialRun = tryBreakingTextRun(run, lineStatus.contentLogicalRight + accumulatedRunWidth, adjustedAvailableWidth)) {
                if (partialRun->length)
                    return TrailingTextContent { index, false, partialRun };
                if (index) {
                    // When the content is wrapped at the run boundary, the trailing run is the previous run.
                    auto trailingCandidateIndex = index - 1;
                    // Try not break content at inline box boundary
                    // e.g. <span>fits</span><span>overflows</span>
                    // when the text "overflows" completely overflows, let's not wrap the content at '<span>'.
                    auto isAtInlineBox = runs[trailingCandidateIndex].inlineItem.isInlineBoxStart();
                    if (isAtInlineBox) {
                        if (!trailingCandidateIndex) {
                            // This content starts at an inline box and clearly does not fit.
                            // Let's wrap this content over to the next line
                            // e.g <span>this_content_does_not_fit</span>
                            return TrailingTextContent { 0, true, { } };
                        }
                        --trailingCandidateIndex;
                    }
                    return TrailingTextContent { trailingCandidateIndex, false, { } };
                }
                // Sometimes we can't accommodate even the very first character.
                return TrailingTextContent { 0, true, { } };
            }
            // If this run is not breakable, we need to check if any previous run is breakable.
            break;
        }
        accumulatedRunWidth += run.logicalWidth;
        ++index;
    }
    // We did not manage to break the run that actually overflows the line.
    // Let's try to find the last breakable position starting from the overflowing run and wrap it at the content boundary (as it surely fits).
    while (index--) {
        auto& run = runs[index];
        accumulatedRunWidth -= run.logicalWidth;
        if (isBreakableRun(run)) {
            ASSERT(run.inlineItem.isText());
            if (auto partialRun = tryBreakingTextRun(run, lineStatus.contentLogicalRight + accumulatedRunWidth, maxInlineLayoutUnit())) {
                // We know this run fits, so if wrapping is allowed on the run, it should return a non-empty left-side.
                ASSERT(partialRun->length);
                return TrailingTextContent { index, false, partialRun };
            }
        }
    }
    // Give up, there's no breakable run in here.
    return { };
}

InlineContentBreaker::WordBreakRule InlineContentBreaker::wordBreakBehavior(const RenderStyle& style) const
{
    // Disregard any prohibition against line breaks mandated by the word-break property.
    // The different wrapping opportunities must not be prioritized. Hyphenation is not applied.
    if (style.lineBreak() == LineBreak::Anywhere)
        return WordBreakRule::AtArbitraryPosition;
    // Breaking is allowed within “words”.
    if (style.wordBreak() == WordBreak::BreakAll)
        return WordBreakRule::AtArbitraryPosition;
    // Breaking is forbidden within “words”.
    if (style.wordBreak() == WordBreak::KeepAll)
        return WordBreakRule::NoBreak;
    // For compatibility with legacy content, the word-break property also supports a deprecated break-word keyword.
    // When specified, this has the same effect as word-break: normal and overflow-wrap: anywhere, regardless of the actual value of the overflow-wrap property.
    if (style.wordBreak() == WordBreak::BreakWord && !m_hasWrapOpportunityAtPreviousPosition)
        return WordBreakRule::AtArbitraryPosition;
    // OverflowWrap::Break: An otherwise unbreakable sequence of characters may be broken at an arbitrary point if there are no otherwise-acceptable break points in the line.
    if (style.overflowWrap() == OverflowWrap::Break && !m_hasWrapOpportunityAtPreviousPosition)
        return WordBreakRule::AtArbitraryPosition;

    if (!n_hyphenationIsDisabled && style.hyphens() == Hyphens::Auto && canHyphenate(style.computedLocale()))
        return WordBreakRule::OnlyHyphenationAllowed;

    return WordBreakRule::NoBreak;
}

Optional<InlineContentBreaker::PartialRun> InlineContentBreaker::tryBreakingTextRun(const ContinuousContent::Run& overflowingRun, InlineLayoutUnit logicalLeft, InlineLayoutUnit availableWidth) const
{
    ASSERT(overflowingRun.inlineItem.isText());
    auto& inlineTextItem = downcast<InlineTextItem>(overflowingRun.inlineItem);
    auto& style = inlineTextItem.style();
    auto findLastBreakablePosition = availableWidth == maxInlineLayoutUnit();

    auto breakRule = wordBreakBehavior(style);
    if (breakRule == WordBreakRule::AtArbitraryPosition) {
        if (!inlineTextItem.length()) {
            // Empty text runs may be breakable based on style, but in practice we can't really split them any further.
            return PartialRun { };
        }
        if (findLastBreakablePosition) {
            // When the run can be split at arbitrary position,
            // let's just return the entire run when it is intended to fit on the line.
            ASSERT(inlineTextItem.length());
            auto trailingPartialRunWidth = TextUtil::width(inlineTextItem, logicalLeft);
            return PartialRun { inlineTextItem.length() - 1, trailingPartialRunWidth };
        }
        auto splitData = TextUtil::split(inlineTextItem, overflowingRun.logicalWidth, availableWidth, logicalLeft);
        return PartialRun { splitData.length, splitData.logicalWidth };
    }

    if (breakRule == WordBreakRule::OnlyHyphenationAllowed) {
        // Find the hyphen position as follows:
        // 1. Split the text by taking the hyphen width into account
        // 2. Find the last hyphen position before the split position
        auto runLength = inlineTextItem.length();
        unsigned limitBefore = style.hyphenationLimitBefore() == RenderStyle::initialHyphenationLimitBefore() ? 0 : style.hyphenationLimitBefore();
        unsigned limitAfter = style.hyphenationLimitAfter() == RenderStyle::initialHyphenationLimitAfter() ? 0 : style.hyphenationLimitAfter();
        // Check if this run can accommodate the before/after limits at all before start measuring text.
        if (limitBefore >= runLength || limitAfter >= runLength || limitBefore + limitAfter > runLength)
            return { };

        unsigned leftSideLength = runLength;
        auto& fontCascade = style.fontCascade();
        auto hyphenWidth = InlineLayoutUnit { fontCascade.width(TextRun { StringView { style.hyphenString() } }) };
        if (!findLastBreakablePosition) {
            auto availableWidthExcludingHyphen = availableWidth - hyphenWidth;
            if (availableWidthExcludingHyphen <= 0 || !enoughWidthForHyphenation(availableWidthExcludingHyphen, fontCascade.pixelSize()))
                return { };
            leftSideLength = TextUtil::split(inlineTextItem, overflowingRun.logicalWidth, availableWidthExcludingHyphen, logicalLeft).length;
        }
        if (leftSideLength < limitBefore)
            return { };
        // Adjust before index to accommodate the limit-after value (it's the last potential hyphen location in this run).
        auto hyphenBefore = std::min(leftSideLength, runLength - limitAfter) + 1;
        unsigned hyphenLocation = lastHyphenLocation(StringView(inlineTextItem.inlineTextBox().content()).substring(inlineTextItem.start(), inlineTextItem.length()), hyphenBefore, style.computedLocale());
        if (!hyphenLocation || hyphenLocation < limitBefore)
            return { };
        // hyphenLocation is relative to the start of this InlineItemText.
        ASSERT(inlineTextItem.start() + hyphenLocation < inlineTextItem.end());
        auto trailingPartialRunWidthWithHyphen = TextUtil::width(inlineTextItem, inlineTextItem.start(), inlineTextItem.start() + hyphenLocation, logicalLeft); 
        return PartialRun { hyphenLocation, trailingPartialRunWidthWithHyphen, hyphenWidth };
    }

    ASSERT(breakRule == WordBreakRule::NoBreak);
    return { };
}

void InlineContentBreaker::ContinuousContent::append(const InlineItem& inlineItem, InlineLayoutUnit logicalWidth, Optional<InlineLayoutUnit> collapsibleWidth)
{
    m_runs.append({ inlineItem, logicalWidth });
    m_logicalWidth += logicalWidth;
    if (!collapsibleWidth) {
        m_collapsibleLogicalWidth = { };
        return;
    }
    if (*collapsibleWidth == logicalWidth) {
        // Fully collapsible run.
        m_collapsibleLogicalWidth += logicalWidth;
        ASSERT(m_collapsibleLogicalWidth <= m_logicalWidth);
        return;
    }
    // Partially collapsible run.
    m_collapsibleLogicalWidth = *collapsibleWidth;
    ASSERT(m_collapsibleLogicalWidth <= m_logicalWidth);
}

void InlineContentBreaker::ContinuousContent::reset()
{
    m_logicalWidth = { };
    m_collapsibleLogicalWidth = { };
    m_runs.clear();
}

}
}
#endif
