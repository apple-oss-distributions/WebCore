/*
 * Copyright (C) 2007 Apple Computer, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "FontCustomPlatformData.h"

#include "SharedBuffer.h"
#include "FontPlatformData.h"

namespace WebCore {

FontCustomPlatformData::~FontCustomPlatformData()
{
    // FIXME: <rdar://problem/5607116> Web fonts broken in WebKit
    // we probably need a release method in GS when we have support for loading fonts from a memory buffer
    // CFRelease may do, see comment from andrew in <rdar://problem/5607113>
    m_gsFont = 0;
}

FontPlatformData FontCustomPlatformData::fontPlatformData(int size, bool bold, bool italic)
{
    return FontPlatformData(m_gsFont, size, bold, italic);
}

FontCustomPlatformData* createFontCustomPlatformData(SharedBuffer* buffer)
{
    GSFontRef gsFontRef = 0;

    // FIXME: <rdar://problem/5607116> Web fonts broken in WebKit
    gsFontRef = GSFontCreateWithName("Courier", 0, 1.0f);

    return new FontCustomPlatformData(gsFontRef);
}

}
