/*****************************************************************
|
|   Platinum - DIDL
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#ifndef _PLT_DIDL_H_
#define _PLT_DIDL_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltMediaItem.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define PLT_FILTER_MASK_ALL                         0xFFFFFFFF

#define PLT_FILTER_MASK_CREATOR                     0x00000001
#define PLT_FILTER_MASK_ARTIST                      0x00000002
#define PLT_FILTER_MASK_ALBUM                       0x00000004
#define PLT_FILTER_MASK_GENRE                       0x00000008
#define PLT_FILTER_MASK_ALBUMARTURI                 0x00000010
#define PLT_FILTER_MASK_DESCRIPTION                 0x00000020
#define PLT_FILTER_MASK_SEARCHABLE                  0x00000040
#define PLT_FILTER_MASK_CHILDCOUNT                  0x00000080
#define PLT_FILTER_MASK_ORIGINALTRACK               0x00000100
#define PLT_FILTER_MASK_ACTOR                       0x00000200
#define PLT_FILTER_MASK_AUTHOR                      0x00000400

#define PLT_FILTER_MASK_RES                         0x00010000
#define PLT_FILTER_MASK_RES_DURATION                0x00020000
#define PLT_FILTER_MASK_RES_SIZE                    0x00040000
#define PLT_FILTER_MASK_RES_PROTECTION              0x00080000

#define PLT_FILTER_FIELD_CREATOR                    "dc:creator"
#define PLT_FILTER_FIELD_ARTIST                     "upnp:artist"
#define PLT_FILTER_FIELD_ACTOR                      "upnp:actor"
#define PLT_FILTER_FIELD_AUTHOR                     "upnp:actor"
#define PLT_FILTER_FIELD_ALBUM                      "upnp:author"
#define PLT_FILTER_FIELD_GENRE                      "upnp:genre"
#define PLT_FILTER_FIELD_ALBUMARTURI                "upnp:albumArtURI"
#define PLT_FILTER_FIELD_DESCRIPTION                "upnp:longDescription"
#define PLT_FILTER_FIELD_ORIGINALTRACK              "upnp:originalTrackNumber"
#define PLT_FILTER_FIELD_SEARCHABLE                 "@searchable"
#define PLT_FILTER_FIELD_CHILDCOUNT                 "@childcount"
#define PLT_FILTER_FIELD_CONTAINER_CHILDCOUNT       "container@childCount"
#define PLT_FILTER_FIELD_CONTAINER_SEARCHABLE       "container@searchable"

#define PLT_FILTER_FIELD_RES                        "res"
#define PLT_FILTER_FIELD_RES_DURATION               "res@duration"
#define PLT_FILTER_FIELD_RES_SIZE                   "res@size"
#define PLT_FILTER_FIELD_RES_PROTECTION             "res@protection"

extern const char* didl_header;
extern const char* didl_footer;
extern const char* didl_namespace_dc;
extern const char* didl_namespace_upnp;

/*----------------------------------------------------------------------
|   PLT_Didl class
+---------------------------------------------------------------------*/
class PLT_Didl
{
public:
    static NPT_Result  ToDidl(PLT_MediaObject& object, NPT_String filter, NPT_String& didl);
    static NPT_Result  FromDidl(const char* didl, PLT_MediaObjectListReference& objects);

    static const char* AppendXmlEscape(NPT_String& out, NPT_String& in);
    static const char* AppendXmlUnEscape(NPT_String& out, NPT_String& in);
    static NPT_Result  ParseTimeStamp(NPT_String timestamp, NPT_UInt32& seconds);
    static const char* FormatTimeStamp(NPT_String& out, NPT_UInt32 seconds);

    static NPT_Result  ParseTimeStamp(NPT_String in, NPT_TimeStamp& timestamp) {
        NPT_UInt32 seconds;
        NPT_Result res = ParseTimeStamp(in, seconds);
        timestamp = NPT_TimeStamp(seconds, 0);
        return res;
    }

private:
    static NPT_UInt32  ConvertFilterToMask(NPT_String filter);
};

#endif /* _PLT_DIDL_H_ */
