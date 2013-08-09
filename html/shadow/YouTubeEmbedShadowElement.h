/*
 * YouTubeEmbedShadowElement.h
 * WebCore
 *
 * Copyright (C) 2012, Apple Inc.  All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by
 * implication, estoppel, or otherwise, to Apple patents and
 * trademarks. For the sake of clarity, no license or rights are
 * granted by Apple expressly or by implication, estoppel, or otherwise,
 * under any Apple patents, copyrights and trademarks to underlying
 * implementations of any application programming interfaces (APIs)
 * or to any functionality that is invoked by calling any API.
 */

#ifndef YouTubeEmbedShadowElement_h
#define YouTubeEmbedShadowElement_h

#if PLATFORM(IOS)

#include "HTMLDivElement.h"
#include <wtf/Forward.h>

namespace WebCore {
    
class HTMLPlugInImageElement;

class YouTubeEmbedShadowElement : public HTMLDivElement {
public:
    static PassRefPtr<YouTubeEmbedShadowElement> create(Document*);

    virtual const AtomicString& shadowPseudoId() const;
    HTMLPlugInImageElement* pluginElement() const;
    
private:
    YouTubeEmbedShadowElement(Document*);
};

}

#endif // PLATFORM(IOS)

#endif
