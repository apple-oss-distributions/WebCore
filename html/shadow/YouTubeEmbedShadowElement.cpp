/*
 * YouTubeEmbedShadowElement.cpp
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

#include "config.h"
#include "YouTubeEmbedShadowElement.h"

#if PLATFORM(IOS)

#include "HTMLEmbedElement.h"

namespace WebCore {

using namespace HTMLNames;

PassRefPtr<YouTubeEmbedShadowElement> YouTubeEmbedShadowElement::create(Document* doc)
{
    return adoptRef(new YouTubeEmbedShadowElement(doc));
}

YouTubeEmbedShadowElement::YouTubeEmbedShadowElement(Document* document) 
    : HTMLDivElement(HTMLNames::divTag, document)
{
}

HTMLPlugInImageElement* YouTubeEmbedShadowElement::pluginElement() const
{
    Node* node = const_cast<YouTubeEmbedShadowElement*>(this)->deprecatedShadowAncestorNode();
    ASSERT(!node || embedTag == toElement(node)->tagQName() || objectTag == toElement(node)->tagQName());
    return static_cast<HTMLPlugInImageElement*>(node);
}

const AtomicString& YouTubeEmbedShadowElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, pseudoId, ("-apple-youtube-shadow-iframe"));
    return pseudoId;
}

}

#endif
