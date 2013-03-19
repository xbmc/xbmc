#ifndef __PLEX_TYPES_H__
#define __PLEX_TYPES_H__

// Windows.
#define WINDOW_NOW_PLAYING                10050
#define WINDOW_PLEX_SEARCH                10051
#define WINDOW_PLUGIN_SETTINGS            10052
#define WINDOW_SHARED_CONTENT             10053

#define WINDOW_PLEX_PREPLAY_VIDEO         10090
#define WINDOW_PLEX_PREPLAY_MUSIC         10091

// Dialogs.
#define WINDOW_DIALOG_RATING              10200
#define WINDOW_DIALOG_TIMER               10201
#define WINDOW_DIALOG_FILTER_SORT         10202
#define WINDOW_DIALOG_MYPLEX_PIN          10203
#define WINDOW_DIALOG_PLEX_AUDIO_SUBTITLE_PICKER 10204

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

#define GUI_MSG_UPDATE_FILTERS GUI_MSG_USER + 48

#define GUI_MSG_MYPLEX_GOT_PIN GUI_MSG_USER + 60
#define GUI_MSG_MYPLEX_GOT_TOKEN GUI_MSG_USER + 61

#define GUI_MSG_LIST_REMOVE_ITEM GUI_MSG_USER + 70

#define GUI_MSG_PLEX_SECTION_LOADED + 71
#define GUI_MSG_PLEX_SERVER_NOTIFICATION + 72

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

// GUIInfoManager defines
class CMusicThumbLoader;
typedef boost::shared_ptr < CFileItem > CFileItemPtr;

#define SYSTEM_SEARCH_IN_PROGRESS 180
#define MUSICPLAYER_HAS_NEW_COVER_NEXT 227
#define MUSICPLAYER_NEXT_NEW_COVER  228
#define MUSICPLAYER_NOW_PLAYING_FLIPPED 229
#define MUSICPLAYER_FANART          240
#define CONTAINER_FIRST_TITLE       5000
#define CONTAINER_SECOND_TITLE      5001

#define SYSTEM_SELECTED_PLEX_MEDIA_SERVER      5002
#define SLIDESHOW_SHOW_DESCRIPTION  990

#define LISTITEM_STAR_DIFFUSE       (LISTITEM_START + 110)
#define LISTITEM_BANNER             (LISTITEM_START + 111)
#define LISTITEM_FIRST_GENRE        (LISTITEM_START + 112)

#define LISTITEM_TYPE               (LISTITEM_START + 150)
#define LISTITEM_GRANDPARENT_THUMB  (LISTITEM_START + 151)
#define LISTITEM_STATUS             (LISTITEM_START + 152)

#define LISTITEM_THUMB0             (LISTITEM_START + 170)
#define LISTITEM_THUMB1             (LISTITEM_START + 171)
#define LISTITEM_THUMB2             (LISTITEM_START + 172)
#define LISTITEM_THUMB3             (LISTITEM_START + 173)
#define LISTITEM_THUMB4             (LISTITEM_START + 174)
#define LISTITEM_DURATION_STRING    (LISTITEM_START + 175)

#define PLAYER_HAS_MUSIC_PLAYLIST   90

// Message Ids for ApplicationMessenger
#define TMSG_MEDIA_RESTART_WITH_NEW_PLAYER 208
#define TMSG_HIDE                 911

#define CONF_FLAGS_RGB           0x20

#define PLEX_ART_THUMB "thumb"
#define PLEX_ART_FANART "fanart"
#define PLEX_ART_BANNER "banner"
#define PLEX_ART_POSTER "poster"
#define PLEX_ART_BIG_POSTER "bigPoster"

#define PLEX_ART_TVSHOW_BANNER "tvshow.banner"
#define PLEX_ART_TVSHOW_POSTER "tvshow.poster"
#define PLEX_ART_TVSHOW_THUMB "tvshow.thumb"

#define PLEX_ART_SEASON_POSTER "season.poster"
#define PLEX_ART_SEASON_BANNER "season.banner"
#define PLEX_ART_SEASON_FANART "season.fanart"

#define PLEX_ART_FANART_FALLBACK "plex_fanart_fallback"

#endif
