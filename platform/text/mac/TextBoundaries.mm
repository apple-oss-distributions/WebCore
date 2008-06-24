/*
 * Copyright (C) 2004, 2006 Apple Computer, Inc.  All rights reserved.
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

#import "config.h"
#import "TextBoundaries.h"

#import <Foundation/Foundation.h>
#import <unicode/uchar.h>
#import <unicode/ustring.h>
#import <unicode/utypes.h>
#import <unicode/ubrk.h>

namespace WebCore {


static bool isSkipCharacter(UChar c)
{
    return c == 0xA0 || 
        c == '\n' || 
        c == '.' || 
        c == ',' || 
        c == '!'  || 
        c == '?' || 
        c == ';' || 
        c == ':' || 
        u_isspace(c);
}

static bool isWhitespaceCharacter(UChar c)
{
    return c == 0xA0 || 
        c == '\n' || 
        u_isspace(c);
}

static bool isWordDelimitingCharacter(UChar c)
{
    CFCharacterSetRef set = CFCharacterSetGetPredefined(kCFCharacterSetAlphaNumeric);
    return !CFCharacterSetIsCharacterMember(set, c) && c != '\'' && c != '&';
}


void findWordBoundary(const UChar* chars, int len, int position, int* start, int* end)
{
    // Much simpler notion on Purple. A word is a stream of characters
    // delimited by a special set of word-delimiting characters.
    int startPos = position;
    if (startPos > 0 && isWordDelimitingCharacter(chars[startPos-1]))
        startPos--;
    while (startPos > 0 && !isWordDelimitingCharacter(chars[startPos-1]))
        startPos--;
        
    int endPos = position;
    while (endPos < len && !isWordDelimitingCharacter(chars[endPos]))
        endPos++;

    *start = startPos;
    *end = endPos;
}

int findNextWordFromIndex(const UChar* chars, int len, int position, bool forward)
{   
    // This very likely won't behave exactly like the non-purple version, but it works
    // for the contexts in which it is used on purple, and in the future will be
    // tuned to improve the purple-specific behavior for the keyboard and text editing.
    int pos = position;
    UErrorCode status = U_ZERO_ERROR;
    UBreakIterator *boundary = ubrk_open(UBRK_WORD, currentTextBreakLocaleID(), const_cast<unichar *>(reinterpret_cast<const unichar *>(chars)), len, &status);

    if (boundary && U_SUCCESS(status)) {
        if (forward) {
            do {
                pos = ubrk_following(boundary, pos);    
                if (pos == UBRK_DONE) {
                    pos = len;
                }
            } while (pos <= len && (pos == 0 || !isSkipCharacter(chars[pos-1])) && isSkipCharacter(chars[pos]));
        }
        else {
            do {
                pos = ubrk_preceding(boundary, pos);
                if (pos == UBRK_DONE) {
                    pos = 0;
                }
            } while (pos > 0 && isSkipCharacter(chars[pos]) && !isWhitespaceCharacter(chars[pos-1]));
        }
        ubrk_close(boundary);
    }
    return pos;
}

}
