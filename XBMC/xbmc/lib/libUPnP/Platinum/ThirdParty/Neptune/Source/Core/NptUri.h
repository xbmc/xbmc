/*****************************************************************
|
|   Neptune - URI
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_URI_H_
#define _NPT_URI_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptStrings.h"

/*----------------------------------------------------------------------
|   NPT_Uri
+---------------------------------------------------------------------*/
class NPT_Uri {
public:
    // types
    typedef enum {
        SCHEME_ID_UNKNOWN,
        SCHEME_ID_HTTP
    } SchemeId;

    // constants. use as a parameter to Encode()
    static const char* const PathCharsToEncode;
    static const char* const QueryCharsToEncode;
    static const char* const UnsafeCharsToEncode;

    // class methods
    static NPT_String Encode(const char* uri, const char* chars, bool encode_percents=true);
    static NPT_String Decode(const char* uri);
    static SchemeId   ParseScheme(const NPT_String& scheme);

    // methods
    NPT_Uri() : m_SchemeId(SCHEME_ID_UNKNOWN) {}
    virtual ~NPT_Uri() {}
    const NPT_String& GetScheme() const {
        return m_Scheme;
    }
    void SetScheme(const char* scheme);
    NPT_Result SetSchemeFromUri(const char* uri);
    SchemeId GetSchemeId() const {
        return m_SchemeId;
    }

protected:
    // members
    NPT_String m_Scheme;
    SchemeId   m_SchemeId;
};

#endif // _NPT_URI_H_
