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
#include "NptStrings.h"
#include "NptList.h"
#include "NptArray.h"
#include "NptReferences.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_MediaServer;

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
typedef enum {
    PLT_PROPERTIES_SEARCHABLE = 0x00000001,
    PLT_PROPERTIES_RESTRICTED = 0x00000002
} PLT_PropertiesType;

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
};

/*----------------------------------------------------------------------
|   PLT_MediaItem class
+---------------------------------------------------------------------*/
class PLT_MediaItem
{
public:
    PLT_MediaItem();
    ~PLT_MediaItem(void);

    void        Reset();
    bool        IsContainer() { return m_ObjectClass.StartsWith("object.container"); }

    static const char* GetExtFromFilePath(const NPT_String filepath, const char* dir_delimiter);
    static const char* GetProtInfoFromExt(const char* ext);
    static const char* GetUPnPClassFromExt(const char* ext);

    /* common properties */
    NPT_String m_ObjectClass;
    NPT_String m_ObjectID;
    NPT_String m_ParentID;

    /* metadata */
    NPT_String m_Title;
    NPT_String m_Creator;
    NPT_String m_Artist;
    NPT_String m_Album;
    NPT_String m_Genre;

    /* extras */
    NPT_String m_Description;
    NPT_String m_AlbumArtURI;

    /* resources related */
    NPT_Array<PLT_MediaItemResource> m_Resources;

    /* type */
    NPT_UInt8 m_Flags;

    /* container info related */
    NPT_Int32 m_ChildrenCount;

    /* original DIDL for Control Points to pass to a renderer when invoking SetAVTransportURI */
    NPT_String m_Didl;

private:
    friend class PLT_MediaServer;
};

/*----------------------------------------------------------------------
|   PLT_MediaItemList class
+---------------------------------------------------------------------*/
class PLT_MediaItemList : public NPT_List<PLT_MediaItem*>
{
public:
    PLT_MediaItemList();

protected:
    virtual ~PLT_MediaItemList(void);
    friend class NPT_Reference<PLT_MediaItemList>;
};

typedef NPT_Reference<PLT_MediaItemList> PLT_MediaItemListReference;


#endif /* _PLT_MEDIA_ITEM_H_ */
