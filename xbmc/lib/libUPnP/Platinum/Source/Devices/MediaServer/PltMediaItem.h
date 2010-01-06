/*****************************************************************
|
|   Platinum - AV Media Item
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

#ifndef _PLT_MEDIA_ITEM_H_
#define _PLT_MEDIA_ITEM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltHttp.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_MediaServer;

/*----------------------------------------------------------------------
|   typedefs
+---------------------------------------------------------------------*/
typedef struct { 
    NPT_String type;
    NPT_String friendly_name;
} PLT_ObjectClass;

typedef struct { 
    NPT_String type;
    NPT_String friendly_name;
    bool       include_derived;
} PLT_SearchClass;

typedef struct {
    NPT_String name;
    NPT_String role;
} PLT_PersonRole;

class PLT_PersonRoles  : public NPT_List<PLT_PersonRole>
{
public:
    NPT_Result Add(const NPT_String& name, const NPT_String& role = "");
    NPT_Result ToDidl(NPT_String& didl, const NPT_String& tag);
    NPT_Result FromDidl(const NPT_Array<NPT_XmlElementNode*>& nodes);
};

typedef struct {
    NPT_String allowed_use; // (CSV)
    NPT_String validity_start;
    NPT_String validity_end;
    NPT_String remaining_time;
    NPT_String usage_info;
    NPT_String rights_info_uri;
    NPT_String content_info_uri;
} PLT_Constraint;

typedef struct {
    PLT_PersonRoles artists;
    PLT_PersonRoles actors;
    PLT_PersonRoles authors;
    NPT_String      producer;
    NPT_String      director;
    NPT_String      publisher;
    NPT_String      contributor; // should match m_Creator (dc:creator)
} PLT_PeopleInfo;

typedef struct {
    NPT_List<NPT_String> genre;
    NPT_String album;
    NPT_String playlist; // dc:title of the playlist item the content belongs too
} PLT_AffiliationInfo;

typedef struct {
    NPT_String description;
    NPT_String long_description;
    NPT_String icon_uri;
    NPT_String region;
    NPT_String rating;
    NPT_String rights;
    NPT_String date;
    NPT_String language;
} PLT_Description;

typedef struct {
    NPT_String album_art_uri;
    NPT_String album_art_uri_dlna_profile;
    NPT_String artist_discography_uri;
    NPT_String lyrics_uri;
    NPT_List<NPT_String> relation; // dc:relation
} PLT_ExtraInfo;

typedef struct {
    NPT_UInt32 dvdregioncode;
    NPT_UInt32 original_track_number;
    NPT_String toc;
    NPT_String user_annotation;
} PLT_MiscInfo;

typedef struct {
    int         total;
    int         used;
    int         free;
    int         max_partition;
    NPT_String  medium;
} PLT_StorageInfo;

typedef struct {
    NPT_String program_title;
    NPT_String series_title;
    int        episode_number;
} PLT_RecordedInfo;

/*----------------------------------------------------------------------
|   PLT_ProtocolInfo class
+---------------------------------------------------------------------*/
class PLT_ProtocolInfo
{
public:
    class FieldEntry {
    public:
        FieldEntry(const char* key, const char* value) :
          m_Key(key), m_Value(value) {}
        NPT_String m_Key;
        NPT_String m_Value;
    };

    PLT_ProtocolInfo();
    PLT_ProtocolInfo(const char* protocol_info);
    PLT_ProtocolInfo(const char* protocol,
                     const char* mask,
                     const char* content_type,
                     const char* extra);
    const NPT_String& GetProtocol()     const { return m_Protocol;  }
    const NPT_String& GetMask()         const { return m_Mask; }
    const NPT_String& GetContentType()  const { return m_ContentType;  }
    const NPT_String& GetExtra()        const { return m_Extra; }

    const NPT_String& GetDLNA_PN()      const { return m_DLNA_PN; }

    bool IsValid() { return m_Valid; }
    NPT_String ToString() const;

    bool Match(const PLT_ProtocolInfo& other) const;

private:
    NPT_Result ValidateField(const char*  val, 
                        const char*  valid_chars, 
                        NPT_Cardinal num_chars = 0); // 0 means variable number of chars
    NPT_Result ParseExtra(NPT_List<FieldEntry>& entries);
    NPT_Result ValidateExtra();

private:
    NPT_String m_Protocol;
    NPT_String m_Mask;
    NPT_String m_ContentType;
    NPT_String m_Extra;

    NPT_String m_DLNA_PN;
    NPT_String m_DLNA_OP;
    NPT_String m_DLNA_PS;
    NPT_String m_DLNA_CI;
    NPT_String m_DLNA_FLAGS;
    NPT_String m_DLNA_MAXSP;

    NPT_List<FieldEntry> m_DLNA_OTHER;
    bool       m_Valid;
};

/*----------------------------------------------------------------------
|   PLT_MediaItemResource class
+---------------------------------------------------------------------*/
class PLT_MediaItemResource
{
public:
    PLT_MediaItemResource();
    ~PLT_MediaItemResource() {}

    NPT_String       m_Uri;
    PLT_ProtocolInfo m_ProtocolInfo;
    NPT_UInt32       m_Duration; /* seconds */
    NPT_LargeSize    m_Size;
    NPT_String       m_Protection;
    NPT_UInt32       m_Bitrate; /* bytes/seconds */
    NPT_UInt32       m_BitsPerSample;
    NPT_UInt32       m_SampleFrequency;
    NPT_UInt32       m_NbAudioChannels;
    NPT_String       m_Resolution;
    NPT_UInt32       m_ColorDepth;
};

/*----------------------------------------------------------------------
|   PLT_MediaObject class
+---------------------------------------------------------------------*/
class PLT_MediaObject
{
public:
    PLT_MediaObject() {}
    virtual ~PLT_MediaObject() {}

    bool IsContainer() { return m_ObjectClass.type.StartsWith("object.container"); }

    static const char* GetMimeType(const NPT_String& filename, 
                                   const PLT_HttpRequestContext* context = NULL);
    static const char* GetMimeTypeFromExtension(const NPT_String& extension, 
                                                const PLT_HttpRequestContext* context = NULL);
    static NPT_String  GetProtocolInfo(const char* filename, 
                                       bool with_dlna_extension = true, 
                                       const PLT_HttpRequestContext* context = NULL);
	static NPT_String  GetMimeTypeFromProtocolInfo(const char* protocol_info);
    static const char* GetUPnPClass(const char* filename, 
                                    const PLT_HttpRequestContext* context = NULL);
    static const char* GetDlnaExtension(const char* mime_type, 
                                        const PLT_HttpRequestContext* context = NULL);

    virtual NPT_Result Reset();
    virtual NPT_Result ToDidl(NPT_UInt32 mask, NPT_String& didl);
    virtual NPT_Result FromDidl(NPT_XmlElementNode* entry);

public:
    /* common properties */
    PLT_ObjectClass     m_ObjectClass;
    NPT_String          m_ObjectID;
    NPT_String          m_ParentID;
    NPT_String          m_ReferenceID;

    /* metadata */
    NPT_String          m_Title;
    NPT_String          m_Creator;
    NPT_String          m_Date;
    PLT_PeopleInfo      m_People;
    PLT_AffiliationInfo m_Affiliation;
    PLT_Description     m_Description;
    PLT_RecordedInfo    m_Recorded;

    /* properties */
    bool m_Restricted;

    /* extras */
    PLT_ExtraInfo m_ExtraInfo;

    /* miscellaneous info */
    PLT_MiscInfo m_MiscInfo;

    /* resources related */
    NPT_Array<PLT_MediaItemResource> m_Resources;

    /* original DIDL for Control Points to pass to a renderer when invoking SetAVTransportURI */
    NPT_String m_Didl;    
};

/*----------------------------------------------------------------------
|   PLT_MediaItem class
+---------------------------------------------------------------------*/
class PLT_MediaItem : public PLT_MediaObject
{
public:
    PLT_MediaItem();
    virtual ~PLT_MediaItem();

    // PLT_MediaObject methods
    NPT_Result ToDidl(NPT_UInt32 mask, NPT_String& didl);
    NPT_Result FromDidl(NPT_XmlElementNode* entry);
};

/*----------------------------------------------------------------------
|   PLT_MediaContainer class
+---------------------------------------------------------------------*/
class PLT_MediaContainer : public PLT_MediaObject
{
public:
    PLT_MediaContainer();
    virtual ~PLT_MediaContainer();

    // PLT_MediaObject methods
    NPT_Result Reset();
    NPT_Result ToDidl(NPT_UInt32 mask, NPT_String& didl);
    NPT_Result FromDidl(NPT_XmlElementNode* entry);

public:
    NPT_List<PLT_SearchClass> m_SearchClasses;

    /* properties */
    bool m_Searchable;

    /* container info related */
    NPT_Int32 m_ChildrenCount;    
};

/*----------------------------------------------------------------------
|   PLT_MediaObjectList class
+---------------------------------------------------------------------*/
class PLT_MediaObjectList : public NPT_List<PLT_MediaObject*>
{
public:
    PLT_MediaObjectList();

protected:
    virtual ~PLT_MediaObjectList(void);
    friend class NPT_Reference<PLT_MediaObjectList>;
};

typedef NPT_Reference<PLT_MediaObjectList> PLT_MediaObjectListReference;
typedef NPT_Reference<PLT_MediaObject> PLT_MediaObjectReference;


#endif /* _PLT_MEDIA_ITEM_H_ */
