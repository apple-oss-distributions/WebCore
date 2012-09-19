/*
 * Copyright (C) 2011, 2012 Apple, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "LegacyNumberInputType.h"


namespace WebCore {

PassOwnPtr<InputType> LegacyNumberInputType::create(HTMLInputElement* element)
{
    return adoptPtr(new LegacyNumberInputType(element));
}

bool LegacyNumberInputType::isTextType() const
{
    // Pretend to also be a text field for maxlength.
    return true;
}

bool LegacyNumberInputType::typeMismatchFor(const String& value) const
{
    return TextFieldInputType::typeMismatchFor(value);
}

bool LegacyNumberInputType::typeMismatch() const
{
    return TextFieldInputType::typeMismatch();
}

bool LegacyNumberInputType::supportsRangeLimitation() const
{
    return TextFieldInputType::supportsRangeLimitation();
}

String LegacyNumberInputType::visibleValue() const
{
    return TextFieldInputType::visibleValue();
}

String LegacyNumberInputType::convertFromVisibleValue(const String& value) const
{
    return TextFieldInputType::convertFromVisibleValue(value);
}

bool LegacyNumberInputType::isAcceptableValue(const String& value)
{
    return TextFieldInputType::isAcceptableValue(value);
}

String LegacyNumberInputType::sanitizeValue(const String& value) const
{
    return TextFieldInputType::sanitizeValue(value);
}

bool LegacyNumberInputType::hasUnacceptableValue()
{
    return TextFieldInputType::hasUnacceptableValue();
}

} // namespace WebCore

