/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 */

#ifndef PopupMenuIPhone_h
#define PopupMenuIPhone_h

#include "IntRect.h"
#include "PopupMenu.h"

namespace WebCore {

class FrameView;
class PopupMenuClient;

class PopupMenuIPhone : public PopupMenu {
public:
    PopupMenuIPhone(PopupMenuClient*);

    virtual void show(const IntRect&, FrameView*, int index);
    virtual void hide();
    virtual void updateFromElement();
    virtual void disconnectClient();

private:
    PopupMenuClient* client() const { return m_popupClient; }

    PopupMenuClient* m_popupClient;
};

} // namespace WebCore

#endif // PopupMenuIPhone_h
