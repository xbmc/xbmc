/*****************************************************************
|
|   Platinum - DIDL handling
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
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
#define PLT_FILTER_MASK_DATE                        0x00000800
#define PLT_FILTER_MASK_PROGRAMTITLE                0x00001000
#define PLT_FILTER_MASK_SERIESTITLE                 0x00002000
#define PLT_FILTER_MASK_EPISODE                     0x00004000

#define PLT_FILTER_MASK_RES                         0x00010000
#define PLT_FILTER_MASK_RES_DURATION                0x00020000
#define PLT_FILTER_MASK_RES_SIZE                    0x00040000
#define PLT_FILTER_MASK_RES_PROTECTION              0x00080000
#define PLT_FILTER_MASK_RES_RESOLUTION              0x00100000
#define PLT_FILTER_MASK_RES_BITRATE                 0x00200000
#define PLT_FILTER_MASK_RES_BITSPERSAMPLE           0x00400000
#define PLT_FILTER_MASK_RES_NRAUDIOCHANNELS			0x00800000
#define PLT_FILTER_MASK_RES_SAMPLEFREQUENCY			0x01000000

#define PLT_FILTER_MASK_TOC							0x02000000

#define PLT_FILTER_FIELD_CREATOR                    "dc:creator"
#define PLT_FILTER_FIELD_DATE                       "dc:date"
#define PLT_FILTER_FIELD_ARTIST                     "upnp:artist"
#define PLT_FILTER_FIELD_ACTOR                      "upnp:actor"
#define PLT_FILTER_FIELD_AUTHOR                     "upnp:author"
#define PLT_FILTER_FIELD_ALBUM                      "upnp:album"
#define PLT_FILTER_FIELD_GENRE                      "upnp:genre"
#define PLT_FILTER_FIELD_ALBUMARTURI                "upnp:albumArtURI"
#define PLT_FILTER_FIELD_DESCRIPTION                "upnp:longDescription"
#define PLT_FILTER_FIELD_ORIGINALTRACK              "upnp:originalTrackNumber"
#define PLT_FILTER_FIELD_PROGRAMTITLE               "upnp:programTitle"
#define PLT_FILTER_FIELD_SERIESTITLE                "upnp:seriesTitle"
#define PLT_FILTER_FIELD_EPISODE                    "upnp:episodeNumber"
#define PLT_FILTER_FIELD_SEARCHABLE                 "@searchable"
#define PLT_FILTER_FIELD_CHILDCOUNT                 "@childcount"
#define PLT_FILTER_FIELD_CONTAINER_CHILDCOUNT       "container@childCount"
#define PLT_FILTER_FIELD_CONTAINER_SEARCHABLE       "container@searchable"

#define PLT_FILTER_FIELD_RES                        "res"
#define PLT_FILTER_FIELD_RES_DURATION               "res@duration"
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

/*----------------------------------------------------------------------
|   PLT_Didl class
+---------------------------------------------------------------------*/
class PLT_Didl
{
public:
    static NPT_Result  ToDidl(PLT_MediaObject& object, 
                              NPT_String       filter, 
                              NPT_String&      didl);
    static NPT_Result  FromDidl(const char* didl, 
                                PLT_MediaObjectListReference& objects);

    static void        AppendXmlEscape(NPT_String& out, const char* in);
    static void        AppendXmlUnEscape(NPT_String& out, const char* in);
    static NPT_Result  ParseTimeStamp(NPT_String timestamp, NPT_UInt32& seconds);
    static void        FormatTimeStamp(NPT_String& out, NPT_UInt32 seconds);

    static NPT_Result  ParseTimeStamp(NPT_String in, NPT_TimeStamp& timestamp) {
        NPT_UInt32 seconds;
        NPT_Result res = ParseTimeStamp(in, seconds);
        timestamp = NPT_TimeStamp(seconds, 0);
        return res;
    }

    static NPT_UInt32  ConvertFilterToMask(NPT_String filter);
};

#endif /* _PLT_DIDL_H_ */
