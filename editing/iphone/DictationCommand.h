/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#ifndef DictationCommand_h
#define DictationCommand_h

#include "CompositeEditCommand.h"

#import <wtf/RetainPtr.h>

typedef struct objc_object *id;

namespace WebCore {

class DictationCommand : public CompositeEditCommand {
public:
    static PassRefPtr<DictationCommand> create(Document* document, PassOwnPtr<Vector<Vector<String> > > dictationPhrase, RetainPtr<id> metadata)
    {
        return adoptRef(new DictationCommand(document, dictationPhrase, metadata));
    }
    
    virtual ~DictationCommand();
private:
    DictationCommand(Document* document, PassOwnPtr<Vector<Vector<String> > > dictationPhrase, RetainPtr<id> metadata);
    
    virtual void doApply();
    virtual EditAction editingAction() const { return EditActionDictation; }
    
    OwnPtr<Vector<Vector<String> > > m_dictationPhrases;
    RetainPtr<id> m_metadata;
};

} // namespace WebCore

#endif
