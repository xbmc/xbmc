/*****************************************************************
|
|   Platinum - AV Media Item
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#ifndef _PLT_MEDIA_ITEM_H_
#define _PLT_MEDIA_ITEM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

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
    NPT_String allowed_use; // (CSV)
    NPT_String validity_start;
    NPT_String validity_end;
    NPT_String remaining_time;
    NPT_String usage_info;
    NPT_String rights_info_uri;
    NPT_String content_info_uri;
} PLT_Constraint;

typedef struct {
    NPT_String artist;
    NPT_String artist_role;
    NPT_String actor;
    NPT_String actor_role;
    NPT_String author;
    NPT_String author_role;
    NPT_String producer;
    NPT_String director;
    NPT_String publisher;
    NPT_String contributor; // should match m_Creator (dc:creator)
} PLT_PeopleInfo;

typedef struct {
    NPT_String genre;
    NPT_String genre_id;
    NPT_List<NPT_String> genre_extended;
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
    NPT_Integer total;
    NPT_Integer used;
    NPT_Integer free;
    NPT_Integer max_partition;
    NPT_String  medium;
} PLT_StorageInfo;

/*----------------------------------------------------------------------
|   PLT_MediaItemResource class
+---------------------------------------------------------------------*/
class PLT_MediaItemResource
{
public:
    PLT_MediaItemResource();
    ~PLT_MediaItemResource() {}

    NPT_String  m_Uri;
    NPT_String  m_ProtocolInfo;
    NPT_Integer m_Duration; /* seconds */
    NPT_Integer m_Size;
    NPT_String  m_Protection;
    NPT_UInt32  m_Bitrate; /* bytes/seconds */
    NPT_UInt32  m_BitsPerSample;
    NPT_UInt32  m_SampleFrequency;
    NPT_UInt32  m_NbAudioChannels;
    NPT_String  m_Resolution;
    NPT_UInt32  m_ColorDepth;
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

    static const char* GetExtFromFilePath(const NPT_String filepath, const char* dir_delimiter);
    static const char* GetProtInfoFromExt(const char* ext);
    static const char* GetUPnPClassFromExt(const char* ext);

    virtual NPT_Result Reset();
    virtual NPT_Result ToDidl(NPT_UInt32 mask, NPT_String& didl);
    virtual NPT_Result FromDidl(NPT_XmlElementNode* entry);

public:
    /* common properties */
    PLT_ObjectClass     m_ObjectClass;
    NPT_String          m_ObjectID;
    NPT_String          m_ParentID;

    /* metadata */
    NPT_String          m_Title;
    NPT_String          m_Creator;
    PLT_PeopleInfo      m_People;
    PLT_AffiliationInfo m_Affiliation;
    PLT_Description     m_Description;

    /* properties */
    bool m_Restricted;

    /* extras */
    PLT_ExtraInfo m_ExtraInfo;

    /* miscelaneous info */
    PLT_MiscInfo m_MiscInfo;

    /* resources related */
    NPT_Array<PLT_MediaItemResource> m_Resources;

    /* original DIDL for Control Points to pass to a renderer when invoking SetAVTransportURI */
    NPT_String      m_Didl;    
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
