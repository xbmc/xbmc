//	GUI messages outside GuiLib
//
#pragma once
#include "guimessage.h"

//	DVD Drive Messages
#define GUI_MSG_DVDDRIVE_EJECTED_CD			GUI_MSG_USER + 1
#define GUI_MSG_DVDDRIVE_CHANGED_CD			GUI_MSG_USER + 2

//	General playlist items changed
#define GUI_MSG_PLAYLIST_CHANGED				GUI_MSG_USER + 3

//	Start Slideshow in my pictures lpVoid = CStdString
//	Param lpVoid: CStdString* that points to the Directory 
//	to start the slideshow in.
#define GUI_MSG_START_SLIDESHOW					GUI_MSG_USER + 4

#define GUI_MSG_PLAYBACK_ENDED					GUI_MSG_USER + 5

//	Message is send by playlistplayer when an item starts playing or next or previous item is started
//	Parameter:
//	dwParam1 = Current Playlist, can be PLAYLIST_MUSIC, PLAYLIST_TEMP_MUSIC etc.
//	lpVoid = Current Playlistitem
#define GUI_MSG_PLAYLIST_PLAY_NEXT_PREV	GUI_MSG_USER + 6