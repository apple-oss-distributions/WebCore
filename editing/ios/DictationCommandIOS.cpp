/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#include "config.h"
#include "DictationCommandIOS.h"

#include "Document.h"
#include "DocumentMarkerController.h"
#include "Element.h"
#include "Position.h"
#include "Range.h"
#include "SmartReplace.h"
#include "TextIterator.h"
#include "VisibleUnits.h"

namespace WebCore {

DictationCommandIOS::DictationCommandIOS(Document* document, PassOwnPtr<Vector<Vector<String> > > dictationPhrases, RetainPtr<id> metadata)
    : CompositeEditCommand(document)
    , m_dictationPhrases(dictationPhrases)
    , m_metadata(metadata)
{
}

DictationCommandIOS::~DictationCommandIOS()
{
}

void DictationCommandIOS::doApply()
{
    VisiblePosition insertionPosition(startingSelection().visibleStart());
    
    unsigned resultLength = 0;
    size_t dictationPhraseCount = m_dictationPhrases->size();
    for (size_t i = 0; i < dictationPhraseCount; i++) {
    
        const String& firstInterpretation = m_dictationPhrases->at(i)[0];
        resultLength += firstInterpretation.length();
        inputText(firstInterpretation, true);
        
        const Vector<String>& interpretations = m_dictationPhrases->at(i);
        
        if (interpretations.size() > 1)
            document()->markers()->addDictationPhraseWithAlternativesMarker(endingSelection().toNormalizedRange().get(), interpretations);
            
        setEndingSelection(VisibleSelection(endingSelection().visibleEnd()));
    }
    
    VisiblePosition afterResults(endingSelection().visibleEnd());
    
    Element* root = afterResults.rootEditableElement();
    // FIXME: Add the result marker using a Position cached before results are inserted, instead of relying on TextIterators.
    RefPtr<Range> rangeToEnd = Range::create(document(), createLegacyEditingPosition((Node *)root, 0), afterResults.deepEquivalent());
    int endIndex = TextIterator::rangeLength(rangeToEnd.get(), true);
    int startIndex = endIndex - resultLength;
    
    if (startIndex >= 0) {
        RefPtr<Range> resultRange = TextIterator::rangeFromLocationAndLength(document()->documentElement(), startIndex, endIndex, true);
        document()->markers()->addDictationResultMarker(resultRange.get(), m_metadata);
    }
}

} // namespace WebCore
