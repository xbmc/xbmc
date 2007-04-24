/*****************************************************************
|
|   Platinum - AV Media Playlist
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltMediaPlaylist.h"

/*----------------------------------------------------------------------
|   PLT_MediaPlaylist::PLT_MediaPlaylist
+---------------------------------------------------------------------*/
PLT_MediaPlaylist::PLT_MediaPlaylist()
{
    m_List = new PLT_MediaItemList();
}

/*----------------------------------------------------------------------
|   PLT_MediaPlaylist::~PLT_MediaPlaylist
+---------------------------------------------------------------------*/
PLT_MediaPlaylist::~PLT_MediaPlaylist(void)
{
}

/*----------------------------------------------------------------------
|   PLT_MediaPlaylist::Clear
+---------------------------------------------------------------------*/
NPT_Result
PLT_MediaPlaylist::Clear(void)
{
    m_List->Apply(NPT_ObjectDeleter<PLT_MediaItem>());
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaPlaylist::Clear
+---------------------------------------------------------------------*/
NPT_Result  
PLT_MediaPlaylist::Queue(PLT_MediaItem* item)
{
    PLT_MediaItem* new_item = new PLT_MediaItem(*item);
    m_List->Add(new_item);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_MediaPlaylist::Queue
+---------------------------------------------------------------------*/
NPT_Result  
PLT_MediaPlaylist::Queue(PLT_MediaItemList* list)
{
    list->Apply(PLT_MediaItemQueueIterator(this));
    return NPT_SUCCESS;
}
