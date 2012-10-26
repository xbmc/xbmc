/*****************************************************************
|
|   Platinum - DIDL handling
|
| Copyright (c) 2004-2010, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| licensing@plutinosoft.com
|  
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

/** @file
 UPnP AV Didl
 */

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
#define PLT_FILTER_MASK_ALL                         NPT_UINT64_C(0xFFFFFFFFFFFFFFFF)

#define PLT_FILTER_MASK_CREATOR                     NPT_UINT64_C(0x0000000000000001)
#define PLT_FILTER_MASK_ARTIST                      NPT_UINT64_C(0x0000000000000002)
#define PLT_FILTER_MASK_ALBUM                       NPT_UINT64_C(0x0000000000000004)
#define PLT_FILTER_MASK_GENRE                       NPT_UINT64_C(0x0000000000000008)
#define PLT_FILTER_MASK_ALBUMARTURI                 NPT_UINT64_C(0x0000000000000010)
#define PLT_FILTER_MASK_DESCRIPTION                 NPT_UINT64_C(0x0000000000000020)
#define PLT_FILTER_MASK_SEARCHABLE                  NPT_UINT64_C(0x0000000000000040)
#define PLT_FILTER_MASK_CHILDCOUNT                  NPT_UINT64_C(0x0000000000000080)
#define PLT_FILTER_MASK_ORIGINALTRACK               NPT_UINT64_C(0x0000000000000100)
#define PLT_FILTER_MASK_ACTOR                       NPT_UINT64_C(0x0000000000000200)
#define PLT_FILTER_MASK_AUTHOR                      NPT_UINT64_C(0x0000000000000400)
#define PLT_FILTER_MASK_DIRECTOR                    NPT_UINT64_C(0x0000000000000800)
#define PLT_FILTER_MASK_DATE                        NPT_UINT64_C(0x0000000000001000)
#define PLT_FILTER_MASK_PROGRAMTITLE                NPT_UINT64_C(0x0000000000002000)
#define PLT_FILTER_MASK_SERIESTITLE                 NPT_UINT64_C(0x0000000000004000)
#define PLT_FILTER_MASK_EPISODE                     NPT_UINT64_C(0x0000000000008000)
#define PLT_FILTER_MASK_TITLE                       NPT_UINT64_C(0x0000000000010000)

#define PLT_FILTER_MASK_RES                         NPT_UINT64_C(0x0000000000020000)
#define PLT_FILTER_MASK_RES_DURATION                NPT_UINT64_C(0x0000000000040000)
#define PLT_FILTER_MASK_RES_SIZE                    NPT_UINT64_C(0x0000000000080000)
#define PLT_FILTER_MASK_RES_PROTECTION              NPT_UINT64_C(0x0000000000100000)
#define PLT_FILTER_MASK_RES_RESOLUTION              NPT_UINT64_C(0x0000000000200000)
#define PLT_FILTER_MASK_RES_BITRATE                 NPT_UINT64_C(0x0000000000400000)
#define PLT_FILTER_MASK_RES_BITSPERSAMPLE           NPT_UINT64_C(0x0000000000800000)
#define PLT_FILTER_MASK_RES_NRAUDIOCHANNELS         NPT_UINT64_C(0x0000000001000000)
#define PLT_FILTER_MASK_RES_SAMPLEFREQUENCY         NPT_UINT64_C(0x0000000002000000)

#define PLT_FILTER_MASK_LONGDESCRIPTION             NPT_UINT64_C(0x0000000004000000)
#define PLT_FILTER_MASK_ICON                        NPT_UINT64_C(0x0000000008000000)
#define PLT_FILTER_MASK_RATING                      NPT_UINT64_C(0x0000000010000000)

#define PLT_FILTER_MASK_TOC                         NPT_UINT64_C(0x0000000020000000)
#define PLT_FILTER_MASK_SEARCHCLASS                 NPT_UINT64_C(0x0000000040000000)
#define PLT_FILTER_MASK_REFID                       NPT_UINT64_C(0x0000000080000000)

#define PLT_FILTER_MASK_LASTPOSITION                NPT_UINT64_C(0x0000000100000000)
#define PLT_FILTER_MASK_LASTPLAYBACK                NPT_UINT64_C(0x0000000200000000)
#define PLT_FILTER_MASK_PLAYCOUNT                   NPT_UINT64_C(0x0000000400000000)

#define PLT_FILTER_FIELD_TITLE                      "dc:title"
#define PLT_FILTER_FIELD_CREATOR                    "dc:creator"
#define PLT_FILTER_FIELD_DATE                       "dc:date"
#define PLT_FILTER_FIELD_ARTIST                     "upnp:artist"
#define PLT_FILTER_FIELD_ACTOR                      "upnp:actor"
#define PLT_FILTER_FIELD_AUTHOR                     "upnp:author"
#define PLT_FILTER_FIELD_DIRECTOR                   "upnp:director"
#define PLT_FILTER_FIELD_ALBUM                      "upnp:album"
#define PLT_FILTER_FIELD_GENRE                      "upnp:genre"
#define PLT_FILTER_FIELD_ALBUMARTURI                "upnp:albumArtURI"
#define PLT_FILTER_FIELD_ALBUMARTURI_DLNAPROFILEID  "upnp:albumArtURI@dlna:profileID"
#define PLT_FILTER_FIELD_DESCRIPTION                "dc:description"
#define PLT_FILTER_FIELD_LONGDESCRIPTION            "upnp:longDescription"
#define PLT_FILTER_FIELD_ICON                       "upnp:icon"
#define PLT_FILTER_FIELD_RATING                     "upnp:rating"
#define PLT_FILTER_FIELD_ORIGINALTRACK              "upnp:originalTrackNumber"
#define PLT_FILTER_FIELD_PROGRAMTITLE               "upnp:programTitle"
#define PLT_FILTER_FIELD_SERIESTITLE                "upnp:seriesTitle"
#define PLT_FILTER_FIELD_EPISODE                    "upnp:episodeNumber"
#define PLT_FILTER_FIELD_LASTPOSITION               "upnp:lastPlaybackPosition"
#define PLT_FILTER_FIELD_LASTPLAYBACK               "upnp:lastPlaybackTime"
#define PLT_FILTER_FIELD_PLAYCOUNT                  "upnp:playbackCount"
#define PLT_FILTER_FIELD_SEARCHCLASS				"upnp:searchClass"
#define PLT_FILTER_FIELD_SEARCHABLE                 "@searchable"
#define PLT_FILTER_FIELD_CHILDCOUNT                 "@childcount"
#define PLT_FILTER_FIELD_CONTAINER_CHILDCOUNT       "container@childCount"
#define PLT_FILTER_FIELD_CONTAINER_SEARCHABLE       "container@searchable"
#define PLT_FILTER_FIELD_REFID                      "@refID"

#define PLT_FILTER_FIELD_RES                        "res"
#define PLT_FILTER_FIELD_RES_DURATION               "res@duration"
#define PLT_FILTER_FIELD_RES_DURATION_SHORT         "@duration"
#define PLT_FILTER_FIELD_RES_SIZE                   "res@size"
#define PLT_FILTER_FIELD_RES_PROTECTION             "res@protection"
#define PLT_FILTER_FIELD_RES_RESOLUTION             "res@resolution"
#define PLT_FILTER_FIELD_RES_BITRATE                "res@bitrate"
#define PLT_FILTER_FIELD_RES_BITSPERSAMPLE          "res@bitsPerSample"
#define PLT_FILTER_FIELD_RES_NRAUDIOCHANNELS        "res@nrAudioChannels"
#define PLT_FILTER_FIELD_RES_SAMPLEFREQUENCY        "res@sampleFrequency"

extern const char* didl_header;
extern const char* didl_footer;
extern const char* didl_namespace_dc;
extern const char* didl_namespace_upnp;
extern const char* didl_namespace_dlna;

/*----------------------------------------------------------------------
|   PLT_Didl
+---------------------------------------------------------------------*/
/**
 DIDL manipulation.
 The PLT_Didl class provides a mechanism to (de)serialize a PLT_MediaObject or
 list of PLT_MediaObject (PLT_MediaObjectList).
 */
class PLT_Didl
{
public:
    static NPT_Result  ToDidl(PLT_MediaObject&  object, 
                              const NPT_String& filter, 
                              NPT_String&       didl);
    static NPT_Result  FromDidl(const char* didl, 
                                PLT_MediaObjectListReference& objects);
    static void        AppendXmlEscape(NPT_String& out, const char* in);
    static void        AppendXmlUnEscape(NPT_String& out, const char* in);
    static NPT_Result  ParseTimeStamp(const NPT_String& timestamp, NPT_UInt32& seconds);
    static NPT_String  FormatTimeStamp(NPT_UInt32 seconds);
    static NPT_Result  ParseTimeStamp(const NPT_String& in, NPT_TimeStamp& timestamp) {
        NPT_UInt32 seconds;
        NPT_Result res = ParseTimeStamp(in, seconds);
        timestamp = NPT_TimeStamp((double)seconds);
        return res;
    }

    static NPT_UInt64  ConvertFilterToMask(const NPT_String& filter);
};

#endif /* _PLT_DIDL_H_ */
