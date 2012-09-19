/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 */

#ifndef PopupMenuIOS_h
#define PopupMenuIOS_h

#include "IntRect.h"
#include "PopupMenu.h"

namespace WebCore {

class FrameView;
class PopupMenuClient;

class PopupMenuIOS : public PopupMenu {
public:
    PopupMenuIOS(PopupMenuClient*);

    virtual void show(const IntRect&, FrameView*, int index) OVERRIDE;
    virtual void hide() OVERRIDE;
    virtual void updateFromElement() OVERRIDE;
    virtual void disconnectClient() OVERRIDE;

private:
    PopupMenuClient* client() const { return m_popupClient; }

    PopupMenuClient* m_popupClient;
};

} // namespace WebCore

#endif // PopupMenuIOS_h
