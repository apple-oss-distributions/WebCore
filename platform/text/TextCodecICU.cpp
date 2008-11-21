/*
 * Copyright (C) 2004, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "TextCodecICU.h"

#include "CharacterNames.h"
#include "CString.h"
#include "PlatformString.h"
#include <unicode/ucnv.h>
#include <unicode/ucnv_cb.h>
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>

#include <wtf/ASCIICType.h>

using std::auto_ptr;
using std::min;

namespace WebCore {

const size_t ConversionBufferSize = 16384;

// from TextEncodingRegistry.cpp
const size_t maxEncodingNameLength = 63;
    
static UConverter* cachedConverterICU;

static auto_ptr<TextCodec> newTextCodecICU(const TextEncoding& encoding, const void*)
{
    return auto_ptr<TextCodec>(new TextCodecICU(encoding));
}

void TextCodecICU::registerBaseEncodingNames(EncodingNameRegistrar registrar)
{
    registrar("UTF-8", "UTF-8");
}

void TextCodecICU::registerBaseCodecs(TextCodecRegistrar registrar)
{
    registrar("UTF-8", newTextCodecICU, 0);
}

// FIXME: Registering all the encodings we get from ucnv_getAvailableName
// includes encodings we don't want or need. For example: UTF16_PlatformEndian,
// UTF16_OppositeEndian, UTF32_PlatformEndian, UTF32_OppositeEndian, and all
// the encodings with commas and version numbers.

void TextCodecICU::registerExtendedEncodingNames(EncodingNameRegistrar registrar)
{
    // We register Hebrew with logical ordering using a separate name.
    // Otherwise, this would share the same canonical name as the
    // visual ordering case, and then TextEncoding could not tell them
    // apart; ICU works with either name.
    registrar("ISO-8859-8-I", "ISO-8859-8-I");

// on iPhone, looping through all encodings at startup takes too long.
// instead we will register encodings "on demand"
// FIXME: back this out asap: <rdar://problem/5912708> ICU should support intreating through all encodings quickly (without 177 stats to disk)
}

void TextCodecICU::registerExtendedCodecs(TextCodecRegistrar registrar)
{
    // See comment above in registerEncodingNames.
    registrar("ISO-8859-8-I", newTextCodecICU, 0);

// on iPhone, looping through all encodings at startup takes too long.
// instead we will register encodings "on demand"
}

// back this code out ASAP when both of these are fixed:
// <rdar://problem/5912708> ICU should support intreating through all encodings quickly (without 177 stats to disk)
// <rdar://problem/5825153> Reduce or eliminate use of TEC (use NSString / CFString)
// see encodings test page: http://smontagu.damowmow.com/encodingtest.html
void TextCodecICU::registerEncodingByName(const char *name, EncodingNameRegistrar nameRegistrar, TextCodecRegistrar codecRegistrar)
{
    const unsigned nonMacCount = 30;
    const char* macAliases[][2] = {
        {"macroman", "macintosh"},
        {"xmacroman", "macintosh"},
        {"cnbig5", "Big5"},
        {"cngb", "EUC-CN"},
        {"csISO88598I", "ISO_8859-8-I"},
        {"csgb231280", "EUC-CN"},
        {"dos720", "cp864"},
        {"dos874", "cp874"},
        {"jis7", "ISO-2022-JP"},
        {"koi", "KOI8-R"},
        {"logical", "ISO-8859-8-I"},
        {"unicode11utf8", "UTF-8"},
        {"unicode20utf8", "UTF-8"},
        {"visual", "ISO-8859-8"},
        {"winarabic", "windows-1256"},
        {"winbaltic", "windows-1257"},
        {"wincyrillic", "windows-1251"},
        {"windows874", "cp874"},
        {"wingreek", "windows-1253"},
        {"winhebrew", "windows-1255"},
        {"winlatin2", "windows-1250"},
        {"winturkish", "windows-1254"},
        {"winvietnamese", "windows-1258"},
        {"xcp1250", "windows-1250"},
        {"xcp1251", "windows-1251"},
        {"xeuc", "EUC-JP"},
        {"xeuccn", "EUC-CN"},
        {"xgbk", "EUC-CN"},
        {"xunicode20utf8", "UTF-8"},
        {"xxbig5", "Big5"},
        // A.B. adding a few more Mac encodings missing 'cause we don't have TextCodecMac right now
        // luckily, they are supported in ICU, just need to alias them.
        // this handles encodings that OS X uses TEC (TextCodecMac)
        // http://publib.boulder.ibm.com/infocenter/wmbhelp/v6r0m0/index.jsp?topic=/com.ibm.etools.mft.eb.doc/ac00408_.htm
        {"xmaccyrillic", "macos-7_3-10.2"},
        {"maccyrillic", "macos-7_3-10.2"},
        {"xmacgreek", "macos-6_2-10.4"},
        {"macgreek", "macos-6-10.2"},
        {"xmaccentraleurroman", "macos-29-10.2"},
        {"maccentraleurroman", "macos-29-10.2"},
        {"xmacturkish", "macos-35-10.2 "},
        {"macturkish", "macos-35-10.2 "},
        {"softbanksjis", "softbank-sjis"},

        {0, 0}};
    unsigned i;
    
    // in the encoding name hash, the names are canonicalized.
    // While looking for macintosh names below, we first need to canonicalize.
    char canonicalName[maxEncodingNameLength + 1];
    i = 0;
    unsigned j = 0;
    do {
        while (j < maxEncodingNameLength && name[j] && !WTF::isASCIIAlphanumeric(name[j]))
            j++;
            
        if (j < maxEncodingNameLength && name[j]) {
            canonicalName[i] = WTF::toASCIILower(name[j]);
            i++;
            j++;
        }
    } while (j < maxEncodingNameLength && name[j]);
    canonicalName[i] = 0;
    
    const char* macName = name;
    
    // ICU doesn't know about mac aliases
    // map from mac alias to ICU standard name
    // we will then register the mac aliases later (see bottom of this method)
    i = 0;
    bool isMacOnlyCodec = false;
    while (macAliases[i][0] != 0) {
        if (strcmp(canonicalName, macAliases[i][0]) == 0) {
            macName = macAliases[i][1];
            if (i >= nonMacCount)
                isMacOnlyCodec = true;
            break;
        }
        i++;
    }
    
    UErrorCode error = U_ZERO_ERROR;    
    // FIXME: Should we use the "MIME" standard instead of "IANA"?
    const char* standardName = ucnv_getStandardName(macName, "IANA", &error);
    if (!U_SUCCESS(error))
        return;
    if (!standardName) {
        if (isMacOnlyCodec)
            standardName = macName;
        else
            return;
    }

    const char* standardNameForCodec = standardName;
    // 1. Treat GB2312 encoding as GBK (its more modern superset), to match other browsers.
    // 2. On the Web, GB2312 is encoded as EUC-CN or HZ, while ICU provides a native encoding
    //    for encoding GB_2312-80 and several others. So, we need to override this behavior, too.
    if (strcmp(standardName, "GB2312") == 0 || strcmp(standardName, "GB_2312-80") == 0)
        standardName = "GBK";

    nameRegistrar(standardName, standardName);
    
    uint16_t numAliases = ucnv_countAliases(name, &error);
    ASSERT(U_SUCCESS(error));
    if (U_SUCCESS(error))
        for (uint16_t j = 0; j < numAliases; ++j) {
            error = U_ZERO_ERROR;
            const char* alias = ucnv_getAlias(name, j, &error);
            ASSERT(U_SUCCESS(error));
            if (U_SUCCESS(error) && alias != standardName)
                nameRegistrar(alias, standardName);
        }
    
    codecRegistrar(standardNameForCodec, newTextCodecICU, 0);
    
    i = 0;
    while (macAliases[i][0] != 0) {
        if (strcmp(macName, macAliases[i][1]) == 0)
            nameRegistrar(macAliases[i][0], macAliases[i][1]);            
        i++;
    }
}
    
TextCodecICU::TextCodecICU(const TextEncoding& encoding)
    : m_encoding(encoding)
    , m_numBufferedBytes(0)
    , m_converterICU(0)
    , m_needsGBKFallbacks(false)
{
}

TextCodecICU::~TextCodecICU()
{
    releaseICUConverter();
}

void TextCodecICU::releaseICUConverter() const
{
    if (m_converterICU) {
        if (cachedConverterICU)
            ucnv_close(cachedConverterICU);
        cachedConverterICU = m_converterICU;
        m_converterICU = 0;
    }
}

void TextCodecICU::createICUConverter() const
{
    ASSERT(!m_converterICU);

    const char* name = m_encoding.name();
    m_needsGBKFallbacks = name[0] == 'G' && name[1] == 'B' && name[2] == 'K' && !name[3];

    UErrorCode err;

    if (cachedConverterICU) {
        err = U_ZERO_ERROR;
        const char* cachedName = ucnv_getName(cachedConverterICU, &err);
        if (U_SUCCESS(err) && m_encoding == cachedName) {
            m_converterICU = cachedConverterICU;
            cachedConverterICU = 0;
            return;
        }
    }

    err = U_ZERO_ERROR;
    m_converterICU = ucnv_open(m_encoding.name(), &err);
#if !LOG_DISABLED
    if (err == U_AMBIGUOUS_ALIAS_WARNING)
        LOG_ERROR("ICU ambiguous alias warning for encoding: %s", m_encoding.name());
#endif
    if (m_converterICU)
        ucnv_setFallback(m_converterICU, TRUE);
}

String TextCodecICU::decode(const char* bytes, size_t length, bool flush)
{
    // Get a converter for the passed-in encoding.
    if (!m_converterICU) {
        createICUConverter();
        ASSERT(m_converterICU);
        if (!m_converterICU) {
            LOG_ERROR("error creating ICU encoder even though encoding was in table");
            return String();
        }
    }

    Vector<UChar> result;

    UChar buffer[ConversionBufferSize];
    const char* source = reinterpret_cast<const char*>(bytes);
    const char* sourceLimit = source + length;
    int32_t* offsets = NULL;
    UErrorCode err;

    do {
        UChar* target = buffer;
        const UChar* targetLimit = target + ConversionBufferSize;
        err = U_ZERO_ERROR;
        ucnv_toUnicode(m_converterICU, &target, targetLimit, &source, sourceLimit, offsets, flush, &err);
        int count = target - buffer;
        result.append(buffer, count);
    } while (err == U_BUFFER_OVERFLOW_ERROR);

    if (U_FAILURE(err)) {
        // flush the converter so it can be reused, and not be bothered by this error.
        do {
            UChar *target = buffer;
            const UChar *targetLimit = target + ConversionBufferSize;
            err = U_ZERO_ERROR;
            ucnv_toUnicode(m_converterICU, &target, targetLimit, &source, sourceLimit, offsets, true, &err);
        } while (source < sourceLimit);
        LOG_ERROR("ICU conversion error");
        return String();
    }

    String resultString = String::adopt(result);

    // <http://bugs.webkit.org/show_bug.cgi?id=17014>
    // Simplified Chinese pages use the code A3A0 to mean "full-width space", but ICU decodes it as U+E5E5.
    if (m_encoding == "GBK" || m_encoding == "gb18030")
        resultString.replace(0xE5E5, ideographicSpace);

    return resultString;
}

// We need to apply these fallbacks ourselves as they are not currently supported by ICU and
// they were provided by the old TEC encoding path
// Needed to fix <rdar://problem/4708689>
static HashMap<UChar32, UChar>& gbkEscapes() {
    static HashMap<UChar32, UChar> escapes;
    if (escapes.isEmpty()) {
        escapes.add(0x01F9, 0xE7C8);
        escapes.add(0x1E3F, 0xE7C7);
        escapes.add(0x22EF, 0x2026);
        escapes.add(0x301C, 0xFF5E);
    }
        
    return escapes;
}

static void gbkCallbackEscape(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length,
                              UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) 
{
    if (codePoint && gbkEscapes().contains(codePoint)) {
        UChar outChar = gbkEscapes().get(codePoint);
        const UChar* source = &outChar;
        *err = U_ZERO_ERROR;
        ucnv_cbFromUWriteUChars(fromUArgs, &source, source + 1, 0, err);
        return;
    }
    UCNV_FROM_U_CALLBACK_ESCAPE(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}

static void gbkCallbackSubstitute(const void* context, UConverterFromUnicodeArgs* fromUArgs, const UChar* codeUnits, int32_t length,
                                  UChar32 codePoint, UConverterCallbackReason reason, UErrorCode* err) 
{
    if (gbkEscapes().contains(codePoint)) {
        UChar outChar = gbkEscapes().get(codePoint);
        const UChar* source = &outChar;
        *err = U_ZERO_ERROR;
        ucnv_cbFromUWriteUChars(fromUArgs, &source, source + 1, 0, err);
        return;
    }
    UCNV_FROM_U_CALLBACK_SUBSTITUTE(context, fromUArgs, codeUnits, length, codePoint, reason, err);
}

CString TextCodecICU::encode(const UChar* characters, size_t length, bool allowEntities)
{
    if (!length)
        return "";

    if (!m_converterICU)
        createICUConverter();
    if (!m_converterICU)
        return CString();

    // FIXME: We should see if there is "force ASCII range" mode in ICU;
    // until then, we change the backslash into a yen sign.
    // Encoding will change the yen sign back into a backslash.
    String copy(characters, length);
    copy.replace('\\', m_encoding.backslashAsCurrencySymbol());

    const UChar* source = copy.characters();
    const UChar* sourceLimit = source + copy.length();
    
    UErrorCode err = U_ZERO_ERROR;

    if (allowEntities)
        ucnv_setFromUCallBack(m_converterICU, m_needsGBKFallbacks ? gbkCallbackEscape : UCNV_FROM_U_CALLBACK_ESCAPE, UCNV_ESCAPE_XML_DEC, 0, 0, &err);
    else {
        ucnv_setSubstChars(m_converterICU, "?", 1, &err);
        ucnv_setFromUCallBack(m_converterICU, m_needsGBKFallbacks ? gbkCallbackSubstitute : UCNV_FROM_U_CALLBACK_SUBSTITUTE, 0, 0, 0, &err);
    }

    ASSERT(U_SUCCESS(err));
    if (U_FAILURE(err))
        return CString();

    Vector<char> result;
    size_t size = 0;
    do {
        char buffer[ConversionBufferSize];
        char* target = buffer;
        char* targetLimit = target + ConversionBufferSize;
        err = U_ZERO_ERROR;
        ucnv_fromUnicode(m_converterICU, &target, targetLimit, &source, sourceLimit, 0, true, &err);
        size_t count = target - buffer;
        result.grow(size + count);
        memcpy(result.data() + size, buffer, count);
        size += count;
    } while (err == U_BUFFER_OVERFLOW_ERROR);

    return CString(result.data(), size);
}


} // namespace WebCore
