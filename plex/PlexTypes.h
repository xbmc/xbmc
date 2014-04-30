#ifndef __PLEX_TYPES_H__
#define __PLEX_TYPES_H__

#include <string>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/algorithm/string.hpp>
#include "Variant.h"
#include "StdString.h"

enum EPlexDirectoryType
{
  PLEX_DIR_TYPE_UNKNOWN,
  PLEX_DIR_TYPE_MOVIE,
  PLEX_DIR_TYPE_SHOW,
  PLEX_DIR_TYPE_SEASON,
  PLEX_DIR_TYPE_EPISODE,
  PLEX_DIR_TYPE_ARTIST,
  PLEX_DIR_TYPE_ALBUM,
  PLEX_DIR_TYPE_TRACK,
  PLEX_DIR_TYPE_PHOTOALBUM,
  PLEX_DIR_TYPE_PHOTO,
  PLEX_DIR_TYPE_VIDEO,
  PLEX_DIR_TYPE_DIRECTORY,
  PLEX_DIR_TYPE_SECTION,
  PLEX_DIR_TYPE_SERVER,
  PLEX_DIR_TYPE_DEVICE,
  PLEX_DIR_TYPE_SYNCITEM,
  PLEX_DIR_TYPE_MEDIASETTINGS,
  PLEX_DIR_TYPE_POLICY,
  PLEX_DIR_TYPE_LOCATION,
  PLEX_DIR_TYPE_MEDIA,
  PLEX_DIR_TYPE_PART,
  PLEX_DIR_TYPE_SYNCITEMS,
  PLEX_DIR_TYPE_STREAM,
  PLEX_DIR_TYPE_STATUS,
  PLEX_DIR_TYPE_TRANSCODEJOB,
  PLEX_DIR_TYPE_TRANSCODESESSION,
  PLEX_DIR_TYPE_PROVIDER,
  PLEX_DIR_TYPE_CLIP,
  PLEX_DIR_TYPE_PLAYLIST,
  PLEX_DIR_TYPE_CHANNEL,
  PLEX_DIR_TYPE_SECONDARY,
  PLEX_DIR_TYPE_ROLE,
  PLEX_DIR_TYPE_GENRE,
  PLEX_DIR_TYPE_WRITER,
  PLEX_DIR_TYPE_COUNTRY,
  PLEX_DIR_TYPE_PRODUCER,
  PLEX_DIR_TYPE_DIRECTOR,
  PLEX_DIR_TYPE_THUMB,
  PLEX_DIR_TYPE_IMAGE,
  PLEX_DIR_TYPE_CHANNELS,
  PLEX_DIR_TYPE_MESSAGE,
  PLEX_DIR_TYPE_USER,
  PLEX_DIR_TYPE_RELEASE,
  PLEX_DIR_TYPE_PACKAGE,
  PLEX_DIR_TYPE_HOME_MOVIES
};

enum ePlexMediaType {
  PLEX_MEDIA_TYPE_MUSIC,
  PLEX_MEDIA_TYPE_PHOTO,
  PLEX_MEDIA_TYPE_VIDEO,
  PLEX_MEDIA_TYPE_UNKNOWN
};

enum ePlexMediaState {
  PLEX_MEDIA_STATE_STOPPED,
  PLEX_MEDIA_STATE_PLAYING,
  PLEX_MEDIA_STATE_BUFFERING,
  PLEX_MEDIA_STATE_PAUSED
};

// This is used when we filter stuff in the media window
// it's seperate because the numbers below actually map
// to what the server expects. This should all be merged
// with the enums above. The reason why it's separate here
// is because of legacy reasons.
enum ePlexMediaFilterTypes
{
  PLEX_MEDIA_FILTER_TYPE_NONE = 0,
  PLEX_MEDIA_FILTER_TYPE_MOVIE = 1,
  PLEX_MEDIA_FILTER_TYPE_SHOW,
  PLEX_MEDIA_FILTER_TYPE_SEASON,
  PLEX_MEDIA_FILTER_TYPE_EPISODE,
  PLEX_MEDIA_FILTER_TYPE_TRAILER,
  PLEX_MEDIA_FILTER_TYPE_COMIC,
  PLEX_MEDIA_FILTER_TYPE_PERSON,
  PLEX_MEDIA_FILTER_TYPE_ARTIST,
  PLEX_MEDIA_FILTER_TYPE_ALBUM,
  PLEX_MEDIA_FILTER_TYPE_TRACK,
  PLEX_MEDIA_FILTER_TYPE_PHOTOALBUM,
  PLEX_MEDIA_FILTER_TYPE_PICTURE,
  PLEX_MEDIA_FILTER_TYPE_PHOTO,
  PLEX_MEDIA_FILTER_TYPE_CLIP,
  PLEX_MEDIA_FILTER_TYPE_PLAYLISTITEM
};


// Windows.
#define WINDOW_NOW_PLAYING                10050
#define WINDOW_PLEX_SEARCH                10051
#define WINDOW_PLUGIN_SETTINGS            10052
#define WINDOW_SHARED_CONTENT             10053

#define WINDOW_PLEX_PREPLAY_VIDEO         10090
#define WINDOW_PLEX_PREPLAY_MUSIC         10091
#define WINDOW_PLEX_MYCHANNELS            10092
#define WINDOW_PLEX_STARTUP_HELPER        10093
#define WINDOW_PLEX_PLAY_QUEUE            10094
#define WINDOW_MYPLEX_LOGIN                 10203

// Dialogs.
#define WINDOW_DIALOG_RATING                10200
#define WINDOW_DIALOG_TIMER                 10201
#define WINDOW_DIALOG_FILTER_SORT           10202
#define WINDOW_DIALOG_PLEX_SUBTITLE_PICKER  10204
#define WINDOW_DIALOG_PLEX_AUDIO_PICKER     10205
#define WINDOW_DIALOG_PLEX_SS_PHOTOS        10206
#define WINDOW_DIALOG_PLEX_PLAYQUEUE        10207

// Sent when the set of remote sources has changed
#define GUI_MSG_UPDATE_REMOTE_SOURCES GUI_MSG_USER + 40

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

#define GUI_MSG_FILTER_SELECTED GUI_MSG_USER + 50
#define GUI_MSG_FILTER_LOADED GUI_MSG_USER + 51
#define GUI_MSG_FILTER_VALUES_LOADED GUI_MSG_USER + 52

#define GUI_MSG_LIST_REMOVE_ITEM GUI_MSG_USER + 70

#define GUI_MSG_PLEX_SECTION_LOADED GUI_MSG_USER + 71
#define GUI_MSG_PLEX_SERVER_NOTIFICATION GUI_MSG_USER + 72
#define GUI_MSG_PLEX_SERVER_DATA_LOADED GUI_MSG_USER + 73
#define GUI_MSG_PLEX_BEST_SERVER_UPDATED GUI_MSG_USER + 74
#define GUI_MSG_MYPLEX_STATE_CHANGE GUI_MSG_USER + 75
#define GUI_MSG_PLEX_SERVER_DATA_UNLOADED GUI_MSG_USER + 76
#define GUI_MSG_PLEX_PAGE_LOADED GUI_MSG_USER + 77
#define GUI_MSG_PLEX_PLAYQUEUE_UPDATED GUI_MSG_USER + 78

#define PLEX_DATA_LOADER 99990
#define PLEX_SERVER_MANAGER 99991
#define PLEX_MYPLEX_MANAGER 99992
#define PLEX_AUTO_UPDATER 99993
#define PLEX_FILTER_MANAGER 99994
#define PLEX_PLAYQUEUE_MANAGER 99995

#define PLEX_STREAM_VIDEO    1
#define PLEX_STREAM_AUDIO    2
#define PLEX_STREAM_SUBTITLE 3

// GUIInfoManager defines
class CMusicThumbLoader;

#define SYSTEM_SEARCH_IN_PROGRESS 180
#define MUSICPLAYER_HAS_NEW_COVER_NEXT 227
#define MUSICPLAYER_NEXT_NEW_COVER  228
#define MUSICPLAYER_NOW_PLAYING_FLIPPED 229
#define MUSICPLAYER_FANART          240
#define CONTAINER_FIRST_TITLE       5000
#define CONTAINER_SECOND_TITLE      5001

#define SYSTEM_SELECTED_PLEX_MEDIA_SERVER      5002
#define SYSTEM_UPDATE_IS_AVAILABLE  5003
#define SYSTEM_NO_PLEX_SERVERS      5004
#define SYSTEM_ISRASPLEX            5005
#define SYSTEM_ISOPENELEC           5006
#define CONTAINER_PLEXCONTENT       5007
#define CONTAINER_PLEXFILTER        5008
#define SLIDESHOW_SHOW_DESCRIPTION  990

#define LISTITEM_STAR_DIFFUSE       (LISTITEM_START + 110)
#define LISTITEM_BANNER             (LISTITEM_START + 111)
#define LISTITEM_FIRST_GENRE        (LISTITEM_START + 112)

#define LISTITEM_TYPE               (LISTITEM_START + 150)
#define LISTITEM_GRANDPARENT_THUMB  (LISTITEM_START + 151)
#define LISTITEM_STATUS             (LISTITEM_START + 152)

#define LISTITEM_DURATION_STRING    (LISTITEM_START + 175)
#define LISTITEM_COMPOSITE_IMAGE    (LISTITEM_START + 176)

#define PLAYER_HAS_MUSIC_PLAYLIST   90

// Message Ids for ApplicationMessenger
#define TMSG_MEDIA_RESTART_WITH_NEW_PLAYER 2000
#define TMSG_HIDE                          2001
#define TMSG_PLEX_PLAY_QUEUE_UPDATED       2002

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

#define PLEX_IDENTIFIER_LIBRARY "com.plexapp.plugins.library"
#define PLEX_IDENTIFIER_MYPLEX "com.plexapp.plugins.myplex"
#define PLEX_IDENTIFIER_SYSTEM "com.plexapp.system"

typedef std::pair<std::string, std::string> PlexStringPair;
typedef std::pair<int64_t, std::string> PlexIntStringPair;
typedef std::vector<PlexStringPair> PlexStringPairVector;
typedef std::map<std::string, std::string> PlexStringMap;
typedef std::map<int64_t, std::string> PlexIntStringMap;
typedef std::vector<std::string> PlexStringVector;
typedef std::vector<int64_t> PlexIntVector;

class CPlexServer;
typedef boost::shared_ptr<CPlexServer> CPlexServerPtr;

typedef std::vector<CPlexServerPtr> PlexServerList;
typedef std::map<std::string, CPlexServerPtr> PlexServerMap;
typedef std::pair<std::string, CPlexServerPtr> PlexServerPair;

#define PLEX_DEFAULT_PAGE_SIZE 50

/* Property map definition */
typedef boost::unordered_map<CStdString, CVariant> PropertyMap;

#endif
