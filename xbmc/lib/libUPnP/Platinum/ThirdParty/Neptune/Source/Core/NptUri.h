/*****************************************************************
|
|   Neptune - URI
|
| Copyright (c) 2002-2008, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
 ****************************************************************/

#ifndef _NPT_URI_H_
#define _NPT_URI_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptStrings.h"
#include "NptList.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const NPT_UInt16 NPT_URL_INVALID_PORT = 0;

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
    static const char* const FragmentCharsToEncode;
    static const char* const UnsafeCharsToEncode;

    // class methods
    static NPT_String PercentEncode(const char* str, const char* chars, bool encode_percents=true);
    static NPT_String PercentDecode(const char* str);
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

/*----------------------------------------------------------------------
|   NPT_UrlQuery
+---------------------------------------------------------------------*/
class NPT_UrlQuery
{
public:
    // class methods
    static NPT_String UrlEncode(const char* str, bool encode_percents=true);
    static NPT_String UrlDecode(const char* str);
    
    // types
    struct Field {
        Field(const char* name, const char* value) :
            m_Name(name), m_Value(value) {}
        NPT_String m_Name;
        NPT_String m_Value;
    };

    // constructor
    NPT_UrlQuery() {}
    NPT_UrlQuery(const char* query);

    // accessors
    NPT_List<Field>& GetFields() { return m_Fields; }

    // methods
    NPT_Result  Parse(const char* query);
    NPT_Result  SetField(const char* name, const char* value);
    NPT_Result  AddField(const char* name, const char* value);
    const char* GetField(const char* name);
    NPT_String  ToString();

private:
    // members
    NPT_List<Field> m_Fields;
};

/*----------------------------------------------------------------------
|   NPT_Url
+---------------------------------------------------------------------*/
class NPT_Url : public NPT_Uri {
public:
    // constructors and destructor
    NPT_Url();
    NPT_Url(const char* url, 
            SchemeId    expected_scheme = SCHEME_ID_UNKNOWN, 
            NPT_UInt16  default_port = NPT_URL_INVALID_PORT);
    NPT_Url(const char* scheme,
            const char* host, 
            NPT_UInt16  port, 
            const char* path,
            const char* query = NULL,
            const char* fragment = NULL);

    // methods
    const NPT_String&  GetHost() const     { return m_Host;     }
    NPT_UInt16         GetPort() const     { return m_Port;     }
    const NPT_String&  GetPath() const     { return m_Path;     }
    const NPT_String&  GetQuery() const    { return m_Query;    }
    const NPT_String&  GetFragment() const { return m_Fragment; }
    virtual bool       IsValid() const;
    bool               HasQuery()    const { return m_HasQuery;    } 
    bool               HasFragment() const { return m_HasFragment; }
    NPT_Result         SetHost(const char*  host);
    NPT_Result         SetPort(NPT_UInt16 port);
    NPT_Result         SetPath(const char* path);
    NPT_Result         SetPathPlus(const char* path_plus);
    NPT_Result         SetQuery(const char* query);
    NPT_Result         SetFragment(const char* fragment);
    virtual NPT_String ToRequestString(bool with_fragment = false) const;
    virtual NPT_String ToStringWithDefaultPort(NPT_UInt16 default_port, bool with_fragment = true) const;
    virtual NPT_String ToString(bool with_fragment = true) const;

protected:
    // members
    NPT_String m_Host;
    NPT_UInt16 m_Port;
    NPT_String m_Path;
    bool       m_HasQuery;
    NPT_String m_Query;
    bool       m_HasFragment;
    NPT_String m_Fragment;
};

#endif // _NPT_URI_H_
