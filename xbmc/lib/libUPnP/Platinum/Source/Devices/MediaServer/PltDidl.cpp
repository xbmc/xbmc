/*****************************************************************
|
|   Platinum - DIDL
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltDidl.h"
#include "PltXmlHelper.h"
#include "PltService.h"

NPT_SET_LOCAL_LOGGER("platinum.media.server.didl")

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/

const char* didl_header         = "<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\""
                                            " xmlns:dc=\"http://purl.org/dc/elements/1.1/\""
                                            " xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\""
                                            " xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0\">";
const char* didl_footer         = "\r\n</DIDL-Lite>";
const char* didl_namespace_dc   = "http://purl.org/dc/elements/1.1/";
const char* didl_namespace_upnp = "urn:schemas-upnp-org:metadata-1-0/upnp/";
const char* didl_namespace_dlna = "urn:schemas-dlna-org:metadata-1-0";

/*----------------------------------------------------------------------
|   PLT_MediaServer::ConvertFilterToMask
+---------------------------------------------------------------------*/
NPT_UInt32 
PLT_Didl::ConvertFilterToMask(NPT_String filter)
{
    // a filter string is a comma delimited set of fields identifying
    // a given DIDL property (or set of properties).  
    // These fields are or start with: upnp:, @, res@, res, dc:, container@

    NPT_UInt32  mask = 0;
    const char* s = filter;
    int         i = 0;

    while (s[i] != '\0') {
        int next_comma = filter.Find(',', i);
        int len = ((next_comma < 0)?filter.GetLength():next_comma)-i;

        if (NPT_String::CompareN(s+i, "*", 1) == 0) {
            // return now, there's no point in parsing the rest
            return PLT_FILTER_MASK_ALL;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_CREATOR, len) == 0) {
            mask |= PLT_FILTER_MASK_CREATOR;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_ARTIST, len) == 0) {
            mask |= PLT_FILTER_MASK_ARTIST;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_ALBUM, len) == 0) {
            mask |= PLT_FILTER_MASK_ALBUM;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_GENRE, len) == 0) {
            mask |= PLT_FILTER_MASK_GENRE;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_ALBUMARTURI, len) == 0) {
            mask |= PLT_FILTER_MASK_ALBUMARTURI;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_DESCRIPTION, len) == 0) {
            mask |= PLT_FILTER_MASK_DESCRIPTION;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_ORIGINALTRACK, len) == 0) {
            mask |= PLT_FILTER_MASK_ORIGINALTRACK;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_SEARCHABLE, len) == 0) {
            mask |= PLT_FILTER_MASK_SEARCHABLE;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_CONTAINER_SEARCHABLE, len) == 0) {
            mask |= PLT_FILTER_MASK_SEARCHABLE;       
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_CHILDCOUNT, len) == 0) {
            mask |= PLT_FILTER_MASK_CHILDCOUNT;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_CONTAINER_CHILDCOUNT, len) == 0) {
            mask |= PLT_FILTER_MASK_CHILDCOUNT;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_RES_DURATION, len) == 0) {
            mask |= PLT_FILTER_MASK_RES | PLT_FILTER_MASK_RES_DURATION;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_RES_SIZE, len) == 0) {
            mask |= PLT_FILTER_MASK_RES | PLT_FILTER_MASK_RES_SIZE;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_RES_PROTECTION, len) == 0) {
            mask |= PLT_FILTER_MASK_RES | PLT_FILTER_MASK_RES_PROTECTION;
        } else if (NPT_String::CompareN(s+i, PLT_FILTER_FIELD_RES, len) == 0) {
            mask |= PLT_FILTER_MASK_RES;
        } 

        if (next_comma < 0) {
            return mask;
        }

        i = next_comma + 1;
    }

    return mask;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::AppendXmlUnEscape
+---------------------------------------------------------------------*/
const char*
PLT_Didl::AppendXmlUnEscape(NPT_String& out, NPT_String& in)
{
    const char* input = (const char*) in;
    unsigned int i=0;
    while (i<in.GetLength()) {
        if (NPT_String::CompareN(input+i, "&lt;", 4) == 0) {
            out += '<';
            i   +=4;
        } else if (NPT_String::CompareN(input+i, "&gt;", 4) == 0) {
            out += '>';
            i   += 4;
        } else if (NPT_String::CompareN(input+i, "&amp;", 5) == 0) {
            out += '&';
            i   += 5;
        } else if (NPT_String::CompareN(input+i, "&quot;", 6) == 0) {
            out += '"';
            i   += 6;
        } else if (NPT_String::CompareN(input+i, "&apos;", 6) == 0) {
            out += '\'';
            i   += 6;
        } else {
            out += *(input+i);
            i++;
        }
    }

    return out;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::AppendXmlEscape
+---------------------------------------------------------------------*/
const char*
PLT_Didl::AppendXmlEscape(NPT_String& out, NPT_String& in)
{
    for (int i=0; i<(int)in.GetLength(); i++) {
        if (in[i] == '<') {
            out += "&lt;";
        } else if (in[i] == '>') {
            out += "&gt;";
        } else if (in[i] == '&') {
            out += "&amp;";
        } else if (in[i] == '"') {
            out += "&quot;";
        }  else if (in[i] == '\'') {
            out += "&apos;";
        } else {
            out += in[i];
        }
    }

    return out;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::FormatTimeStamp
+---------------------------------------------------------------------*/
const char*
PLT_Didl::FormatTimeStamp(NPT_String& out, NPT_UInt32 seconds)
{
    NPT_Integer hours = seconds/3600;
    if (hours == 0) {
        out += "00:";
    } else {
        if (hours < 10) {
            out += '0';
        }
        out += NPT_String::FromInteger(hours) + ":";
    }

    NPT_Integer minutes = (seconds/60)%60;
    if (minutes == 0) {
        out += "00:";
    } else {
        if (minutes < 10) {
            out += '0';
        }
        out += NPT_String::FromInteger(minutes) + ":";
    }

    NPT_Integer secs = seconds%60;
    if (secs == 0) {
        out += "00:";
    } else {
        if (secs < 10) {
            out += '0';
        }
        out += NPT_String::FromInteger(secs);
    }

    return out;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::ParseTimeStamp
+---------------------------------------------------------------------*/
NPT_Result
PLT_Didl::ParseTimeStamp(NPT_String timestamp, NPT_UInt32& seconds)
{
    // assume a timestamp in the format HH:MM:SS
    int colon;
    NPT_String str = timestamp;
    NPT_Int32 num;

    // extract millisecondsfirst
    if ((colon = timestamp.ReverseFind('.')) != -1) {
        str = timestamp.SubString(colon + 1);
        timestamp = timestamp.Left(colon);
    }

    // extract seconds first
    str = timestamp;
    if ((colon = timestamp.ReverseFind(':')) != -1) {
        str = timestamp.SubString(colon + 1);
        timestamp = timestamp.Left(colon);
    }

    if (NPT_FAILED(str.ToInteger((long&)num))) {
        return NPT_FAILURE;
    }

    seconds = num;

    // extract minutes
    str = timestamp;
    if (timestamp.GetLength()) {
        if ((colon = timestamp.ReverseFind(':')) != -1) {
            str = timestamp.SubString(colon + 1);
            timestamp = timestamp.Left(colon);
        }

        if (NPT_FAILED(str.ToInteger((long&)num))) {
            return NPT_FAILURE;
        }

        seconds += 60*num;
    }

    // extract hours
    str = timestamp;
    if (timestamp.GetLength()) {
        if ((colon = timestamp.ReverseFind(':')) != -1) {
            str = timestamp.SubString(colon + 1);
            timestamp = timestamp.Left(colon);
        }

        if (NPT_FAILED(str.ToInteger((long&)num))) {
            return NPT_FAILURE;
        }

        seconds += 3600*num;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::ToDidl
+---------------------------------------------------------------------*/
NPT_Result
PLT_Didl::ToDidl(PLT_MediaObject& object, NPT_String filter, NPT_String& didl)
{
    NPT_UInt32 mask = ConvertFilterToMask(filter);

    // Allocate enough space for the didl
    didl.Reserve(2048);

    return object.ToDidl(mask, didl);
}

/*----------------------------------------------------------------------
|   PLT_MediaServer::FromDidl
+---------------------------------------------------------------------*/
NPT_Result  
PLT_Didl::FromDidl(const char* xml, PLT_MediaObjectListReference& objects)
{
    NPT_String          str;
    PLT_MediaObject*    object = NULL;
    NPT_XmlNode*        node = NULL;
    NPT_XmlElementNode* didl = NULL;

    NPT_LOG_FINE("Parsing Didl...");

    NPT_XmlParser parser;
    if (NPT_FAILED(parser.Parse(xml, node)) || !node || !node->AsElementNode()) {
        goto cleanup;
    }

    didl = node->AsElementNode();

    NPT_LOG_FINE("Processing Didl xml...");
    if (didl->GetTag().Compare("DIDL-Lite", true)) {
        goto cleanup;
    }

    // create entry list
    objects = new PLT_MediaObjectList();

    // for each child, find out if it's a container or not
    // and then invoke the FromDidl on it
    for (NPT_List<NPT_XmlNode*>::Iterator children = didl->GetChildren().GetFirstItem(); children; children++) {
        NPT_XmlElementNode* child = (*children)->AsElementNode();
        if (!child) continue;

        if (child->GetTag().Compare("Container", true) == 0) {
            object = new PLT_MediaContainer();
        } else if (child->GetTag().Compare("item", true) == 0) {
            object = new PLT_MediaItem();
        } else {
            goto cleanup;
        }

        if (NPT_FAILED(object->FromDidl(child))) {
            goto cleanup;
        }
        objects->Add(object);
    }

    delete node;
    return NPT_SUCCESS;

cleanup:
    objects = NULL;
    delete node;
    delete object;
    return NPT_FAILURE;
}
