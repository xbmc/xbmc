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

    // class methods
    static NPT_String Encode(const char* uri);
    static NPT_String Decode(const char* uri);
    static SchemeId   ParseScheme(const NPT_String& scheme);

    // methods
    NPT_Uri(const char* uri);
    virtual ~NPT_Uri() {}
    const NPT_String& AsString() const { 
        return m_Uri;  
    }
    const NPT_String& GetScheme() const {
        return m_Scheme;
    }
    SchemeId GetSchemeId() const {
        return m_SchemeId;
    }
    const NPT_String& GetSpecific() { return m_Specific; }

protected:
    // members
    NPT_String m_Uri;
    NPT_String m_Scheme;
    SchemeId   m_SchemeId;
    NPT_String m_Specific;
};

#endif // _NPT_URI_H_
