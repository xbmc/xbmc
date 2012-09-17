#ifndef __PLEX_TYPES_H__
#define __PLEX_TYPES_H__

// Windows.
#define WINDOW_NOW_PLAYING                10050
#define WINDOW_PLEX_SEARCH                10051
#define WINDOW_PLUGIN_SETTINGS            10140

// Dialogs.
#define WINDOW_DIALOG_RATING              10200
#define WINDOW_DIALOG_TIMER               10201

// Sent when the set of remote sources has changed
#define GUI_MSG_UPDATE_REMOTE_SOURCES GUI_MSG_USER + 40

// Send when the main menu needs updating.
#define GUI_MSG_UPDATE_MAIN_MENU      GUI_MSG_USER + 42

// Send when the application is activated (moving to the forground)
#define GUI_MSG_APP_ACTIVATED         GUI_MSG_USER + 43

// Send when the application is deactivating (moving to the background)
#define GUI_MSG_APP_DEACTIVATED       GUI_MSG_USER + 44

// Sent when the background music settings have changed.
#define GUI_MSG_BG_MUSIC_SETTINGS_UPDATED  GUI_MSG_USER + 45

// Sent when the current background music them is updated.
#define GUI_MSG_BG_MUSIC_THEME_UPDATED	GUI_MSG_USER + 46

// Send when a search helper has finished.
#define GUI_MSG_SEARCH_HELPER_COMPLETE GUI_MSG_USER + 47

typedef boost::shared_ptr<CFileItemList> CFileItemListPtr;

#define PLEX_STREAM_VIDEO    1
#define PLEX_STREAM_AUDIO    2
#define PLEX_STREAM_SUBTITLE 3

// Media quality preference.
#define MEDIA_QUALITY_ALWAYS_ASK  0
#define MEDIA_QUALITY_1080P       1080
#define MEDIA_QUALITY_720P        720
#define MEDIA_QUALITY_480P        480
#define MEDIA_QUALITY_SD          400

#endif
