/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef DictationCommandIOS_h
#define DictationCommandIOS_h

#include "CompositeEditCommand.h"

#import <wtf/RetainPtr.h>

typedef struct objc_object *id;

namespace WebCore {

class DictationCommandIOS : public CompositeEditCommand {
public:
    static PassRefPtr<DictationCommandIOS> create(Document* document, PassOwnPtr<Vector<Vector<String> > > dictationPhrase, RetainPtr<id> metadata)
    {
        return adoptRef(new DictationCommandIOS(document, dictationPhrase, metadata));
    }
    
    virtual ~DictationCommandIOS();
private:
    DictationCommandIOS(Document* document, PassOwnPtr<Vector<Vector<String> > > dictationPhrase, RetainPtr<id> metadata);
    
    virtual void doApply();
    virtual EditAction editingAction() const { return EditActionDictation; }
    
    OwnPtr<Vector<Vector<String> > > m_dictationPhrases;
    RetainPtr<id> m_metadata;
};

} // namespace WebCore

#endif
