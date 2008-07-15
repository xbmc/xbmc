/*****************************************************************
|
|   Platinum - AV Media Playlist
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#ifndef _PLT_MEDIA_PLAYLIST_H_
#define _PLT_MEDIA_PLAYLIST_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltMediaItem.h"

/*----------------------------------------------------------------------
|   typedefs
+---------------------------------------------------------------------*/
typedef NPT_List<PLT_MediaItem*> PLT_MediaItemList;
typedef NPT_Reference<PLT_MediaItemList> PLT_MediaItemListReference;

/*----------------------------------------------------------------------
|   PLT_MediaPlaylist class
+---------------------------------------------------------------------*/
class PLT_MediaPlaylist
{
public:
    PLT_MediaPlaylist();
    ~PLT_MediaPlaylist(void);

    NPT_Result  Clear();
    NPT_Result  Queue(PLT_MediaItem* item);
    NPT_Result  Queue(PLT_MediaItemList* list);
    template <typename X> 
    NPT_Result  Apply(const X& function) {
        return m_List->Apply(function);
    }

private:
    PLT_MediaItemListReference m_List;
};

/*----------------------------------------------------------------------
|   PLT_MediaItemQueueIterator class
+---------------------------------------------------------------------*/
class PLT_MediaItemQueueIterator
{
public:
    PLT_MediaItemQueueIterator(PLT_MediaPlaylist* playlist) : m_Playlist(playlist) {}
    NPT_Result operator()(PLT_MediaItem*& item) const {
        return m_Playlist->Queue(item);
    }

private:
    PLT_MediaPlaylist* m_Playlist;
};

#endif /* _PLT_MEDIA_PLAYLIST_H_ */
