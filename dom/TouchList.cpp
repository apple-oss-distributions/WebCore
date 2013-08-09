/*
 * Copyright (C) 2008, Apple Inc. All rights reserved.
 *
 * No license or rights are granted by Apple expressly or by implication,
 * estoppel, or otherwise, to Apple copyrights, patents, trademarks, trade
 * secrets or other rights.
 */

#include "config.h"
#include "TouchList.h"

#if ENABLE(TOUCH_EVENTS)

namespace WebCore {

TouchList::TouchList()
{
}

TouchList::~TouchList()
{
}

void TouchList::append(const PassRefPtr<Touch> val)
{
    m_values.append(val);
}

} // namespace WebCore

#endif // ENABLE(TOUCH_EVENTS)
