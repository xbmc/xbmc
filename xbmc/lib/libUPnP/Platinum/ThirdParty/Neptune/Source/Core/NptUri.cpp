/*****************************************************************
|
|   Neptune - URI
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptUri.h"
#include "NptUtils.h"
#include "NptResults.h"

/*----------------------------------------------------------------------
|   NPT_Uri::ParseScheme
+---------------------------------------------------------------------*/
NPT_Uri::SchemeId
NPT_Uri::ParseScheme(const NPT_String& scheme)
{
    if (scheme == "http") {
        return SCHEME_ID_HTTP;
    } else {
        return SCHEME_ID_UNKNOWN;
    }
}

/*----------------------------------------------------------------------
|   NPT_Uri::SetScheme
+---------------------------------------------------------------------*/
void
NPT_Uri::SetScheme(const char* scheme)
{
    m_Scheme = scheme;
    m_Scheme.MakeLowercase();
    m_SchemeId = ParseScheme(m_Scheme);
}

/*----------------------------------------------------------------------
|   NPT_Uri::SetSchemeFromUri
+---------------------------------------------------------------------*/
NPT_Result
NPT_Uri::SetSchemeFromUri(const char* uri)
{
    const char* start = uri;
    char c;
    while ((c =*uri++)) {
        if (c == ':') {
            m_Scheme.Assign(start, (NPT_Size)(uri-start-1));
            m_Scheme.MakeLowercase();
            m_SchemeId = ParseScheme(m_Scheme);
            return NPT_SUCCESS;
        } else if ((c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') ||
                   (c == '+')             ||
                   (c == '.')             ||
                   (c == '-')) {
            continue;
        } else {
            break;
        }
    }
    return NPT_ERROR_INVALID_SYNTAX;
}

/*----------------------------------------------------------------------
|   NPT_Uri::PathCharsToEncode
+---------------------------------------------------------------------*/
const char* const
NPT_Uri::PathCharsToEncode = " !\"#$&'()*+,:;<=>?@[\\]^`{|}~";

/*----------------------------------------------------------------------
|   NPT_Uri::QueryCharsToEncode
+---------------------------------------------------------------------*/
const char* const
NPT_Uri::QueryCharsToEncode = " !\"#$&'()*+,/:;<=>?@[\\]^`{|}~";

/*----------------------------------------------------------------------
|   NPT_Uri::UnsafeCharsToEncode
+---------------------------------------------------------------------*/
const char* const
NPT_Uri::UnsafeCharsToEncode = " \"#<>[\\]^`{|}~";

/*----------------------------------------------------------------------
|   NPT_Uri::Encode
+---------------------------------------------------------------------*/
NPT_String
NPT_Uri::Encode(const char* uri, const char* chars, bool encode_percents)
{
    NPT_String encoded;

    // check args
    if (uri == NULL) return encoded;

    // reserve at least the size of the current uri
    encoded.Reserve(NPT_StringLength(uri));

    // process each character
    char escaped[3];
    escaped[0] = '%';
    while (unsigned char c = *uri++) {
        bool encode = false;
        if (encode_percents && c == '%') {
            encode = true;
        } else {
            const char* match = chars;
            while (*match) {
                if (c == *match) {
                    encode = true;
                    break;
                }
                ++match;
            }
        }
        if (encode) {
            // encode
            NPT_ByteToHex(c, &escaped[1]);
            encoded.Append(escaped, 3);
        } else {
            // no encoding required
            encoded += c;
        }
    }
    
    return encoded;
}

/*----------------------------------------------------------------------
|   NPT_Uri::Decode
+---------------------------------------------------------------------*/
NPT_String
NPT_Uri::Decode(const char* uri)
{
    NPT_String decoded;

    // check args
    if (uri == NULL) return decoded;

    // reserve at least the size of the current uri
    decoded.Reserve(NPT_StringLength(uri));

    // process each character
    while (unsigned char c = *uri++) {
        if (c == '%') {
            // needs to be unescaped
            unsigned char unescaped;
            if (NPT_SUCCEEDED(NPT_HexToByte(uri, unescaped))) {
                decoded += unescaped;
                uri += 2;
            } else {
                // not a valid escape sequence, just keep the %
                decoded += c;
            }
        } else {
            // no unescaping required
            decoded += c;
        }
    }

    return decoded;
}

