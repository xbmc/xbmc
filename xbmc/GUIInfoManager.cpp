/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "network/Network.h"
#include "system.h"
#include "CompileInfo.h"
#include "GUIInfoManager.h"
#include "windows/GUIMediaWindow.h"
#include "dialogs/GUIDialogProgress.h"
#include "Application.h"
#include "Util.h"
#include "utils/URIUtils.h"
#include "utils/Weather.h"
#include "PartyModeManager.h"
#include "addons/Visualisation.h"
#include "input/ButtonTranslator.h"
#include "utils/AlarmClock.h"
#include "LangInfo.h"
#include "utils/SystemInfo.h"
#include "guilib/GUITextBox.h"
#include "pictures/GUIWindowSlideShow.h"
#include "pictures/PictureInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "guilib/IGUIContainer.h"
#include "guilib/GUIWindowManager.h"
#include "playlists/PlayList.h"
#include "profiles/ProfilesManager.h"
#include "windowing/WindowingFactory.h"
#include "powermanagement/PowerManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/DisplaySettings.h"
#include "settings/MediaSettings.h"
#include "settings/Settings.h"
#include "settings/SkinSettings.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/StereoscopicsManager.h"
#include "utils/CharsetConverter.h"
#include "utils/CPUInfo.h"
#include "utils/StringUtils.h"
#include "utils/MathUtils.h"
#include "utils/SeekHandler.h"
#include "URL.h"
#include "addons/Skin.h"
#include <memory>
#include <functional>
#include "cores/DataCacheCore.h"

// stuff for current song
#include "music/MusicInfoLoader.h"

#include "GUIUserMessages.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "storage/MediaManager.h"
#include "utils/TimeUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "pvr/PVRManager.h"
#include "pvr/channels/PVRChannelGroupsContainer.h"
#include "epg/EpgContainer.h"
#include "pvr/timers/PVRTimers.h"
#include "pvr/recordings/PVRRecording.h"

#include "addons/AddonManager.h"
#include "interfaces/info/InfoBool.h"
#include "video/VideoThumbLoader.h"
#include "music/MusicThumbLoader.h"
#include "video/VideoDatabase.h"
#include "cores/IPlayer.h"
#include "cores/AudioEngine/Utils/AEUtil.h"
#include "cores/VideoRenderers/BaseRenderer.h"
#include "interfaces/info/InfoExpression.h"

#if defined(TARGET_DARWIN_OSX)
#include "osx/smc.h"
#include "linux/LinuxResourceCounter.h"
static CLinuxResourceCounter m_resourceCounter;
#endif

#define SYSHEATUPDATEINTERVAL 60000

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;
using namespace ADDON;
using namespace PVR;
using namespace INFO;
using namespace EPG;

CGUIInfoManager::CGUIInfoManager(void) :
    Observable()
{
  m_lastSysHeatInfoTime = -SYSHEATUPDATEINTERVAL;  // make sure we grab CPU temp on the first pass
  m_fanSpeed = 0;
  m_AfterSeekTimeout = 0;
  m_seekOffset = 0;
  m_nextWindowID = WINDOW_INVALID;
  m_prevWindowID = WINDOW_INVALID;
  m_stringParameters.push_back("__ZZZZ__");   // to offset the string parameters by 1 to assure that all entries are non-zero
  m_currentFile = new CFileItem;
  m_currentSlide = new CFileItem;
  m_frameCounter = 0;
  m_lastFPSTime = 0;
  m_playerShowTime = false;
  m_playerShowCodec = false;
  m_playerShowInfo = false;
  m_fps = 0.0f;
  ResetLibraryBools();
}

CGUIInfoManager::~CGUIInfoManager(void)
{
  delete m_currentFile;
  delete m_currentSlide;
}

bool CGUIInfoManager::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_NOTIFY_ALL)
  {
    if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetItem())
    {
      CFileItemPtr item = std::static_pointer_cast<CFileItem>(message.GetItem());
      if (m_currentFile->IsSamePath(item.get()))
      {
        m_currentFile->UpdateInfo(*item);
        return true;
      }
    }
  }
  return false;
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data. Can handle combined strings on the form
/// Player.Caching + VideoPlayer.IsFullscreen (Logical and)
/// Player.HasVideo | Player.HasAudio (Logical or)
int CGUIInfoManager::TranslateString(const std::string &condition)
{
  // translate $LOCALIZE as required
  std::string strCondition(CGUIInfoLabel::ReplaceLocalize(condition));
  return TranslateSingleString(strCondition);
}

typedef struct
{
  const char *str;
  int  val;
} infomap;

const infomap player_labels[] =  {{ "hasmedia",         PLAYER_HAS_MEDIA },           // bools from here
                                  { "hasaudio",         PLAYER_HAS_AUDIO },
                                  { "hasvideo",         PLAYER_HAS_VIDEO },
                                  { "playing",          PLAYER_PLAYING },
                                  { "paused",           PLAYER_PAUSED },
                                  { "rewinding",        PLAYER_REWINDING },
                                  { "forwarding",       PLAYER_FORWARDING },
                                  { "rewinding2x",      PLAYER_REWINDING_2x },
                                  { "rewinding4x",      PLAYER_REWINDING_4x },
                                  { "rewinding8x",      PLAYER_REWINDING_8x },
                                  { "rewinding16x",     PLAYER_REWINDING_16x },
                                  { "rewinding32x",     PLAYER_REWINDING_32x },
                                  { "forwarding2x",     PLAYER_FORWARDING_2x },
                                  { "forwarding4x",     PLAYER_FORWARDING_4x },
                                  { "forwarding8x",     PLAYER_FORWARDING_8x },
                                  { "forwarding16x",    PLAYER_FORWARDING_16x },
                                  { "forwarding32x",    PLAYER_FORWARDING_32x },
                                  { "canrecord",        PLAYER_CAN_RECORD },
                                  { "recording",        PLAYER_RECORDING },
                                  { "displayafterseek", PLAYER_DISPLAY_AFTER_SEEK },
                                  { "caching",          PLAYER_CACHING },
                                  { "seekbar",          PLAYER_SEEKBAR },
                                  { "seeking",          PLAYER_SEEKING },
                                  { "showtime",         PLAYER_SHOWTIME },
                                  { "showcodec",        PLAYER_SHOWCODEC },
                                  { "showinfo",         PLAYER_SHOWINFO },
                                  { "title",            PLAYER_TITLE },
                                  { "muted",            PLAYER_MUTED },
                                  { "hasduration",      PLAYER_HASDURATION },
                                  { "passthrough",      PLAYER_PASSTHROUGH },
                                  { "cachelevel",       PLAYER_CACHELEVEL },          // labels from here
                                  { "progress",         PLAYER_PROGRESS },
                                  { "progresscache",    PLAYER_PROGRESS_CACHE },
                                  { "volume",           PLAYER_VOLUME },
                                  { "subtitledelay",    PLAYER_SUBTITLE_DELAY },
                                  { "audiodelay",       PLAYER_AUDIO_DELAY },
                                  { "chapter",          PLAYER_CHAPTER },
                                  { "chaptercount",     PLAYER_CHAPTERCOUNT },
                                  { "chaptername",      PLAYER_CHAPTERNAME },
                                  { "starrating",       PLAYER_STAR_RATING },
                                  { "folderpath",       PLAYER_PATH },
                                  { "filenameandpath",  PLAYER_FILEPATH },
                                  { "filename",         PLAYER_FILENAME },
                                  { "isinternetstream", PLAYER_ISINTERNETSTREAM },
                                  { "pauseenabled",     PLAYER_CAN_PAUSE },
                                  { "seekenabled",      PLAYER_CAN_SEEK }};

const infomap player_param[] =   {{ "art",              PLAYER_ITEM_ART }};

const infomap player_times[] =   {{ "seektime",         PLAYER_SEEKTIME },
                                  { "seekoffset",       PLAYER_SEEKOFFSET },
                                  { "seekstepsize",     PLAYER_SEEKSTEPSIZE },
                                  { "timeremaining",    PLAYER_TIME_REMAINING },
                                  { "timespeed",        PLAYER_TIME_SPEED },
                                  { "time",             PLAYER_TIME },
                                  { "duration",         PLAYER_DURATION },
                                  { "finishtime",       PLAYER_FINISH_TIME },
                                  { "starttime",        PLAYER_START_TIME}};

const infomap weather[] =        {{ "isfetched",        WEATHER_IS_FETCHED },
                                  { "conditions",       WEATHER_CONDITIONS },         // labels from here
                                  { "temperature",      WEATHER_TEMPERATURE },
                                  { "location",         WEATHER_LOCATION },
                                  { "fanartcode",       WEATHER_FANART_CODE },
                                  { "plugin",           WEATHER_PLUGIN }};

const infomap system_labels[] =  {{ "hasnetwork",       SYSTEM_ETHERNET_LINK_ACTIVE },
                                  { "hasmediadvd",      SYSTEM_MEDIA_DVD },
                                  { "dvdready",         SYSTEM_DVDREADY },
                                  { "trayopen",         SYSTEM_TRAYOPEN },
                                  { "haslocks",         SYSTEM_HASLOCKS },
                                  { "hasloginscreen",   SYSTEM_HAS_LOGINSCREEN },
                                  { "ismaster",         SYSTEM_ISMASTER },
                                  { "isfullscreen",     SYSTEM_ISFULLSCREEN },
                                  { "isstandalone",     SYSTEM_ISSTANDALONE },
                                  { "loggedon",         SYSTEM_LOGGEDON },
                                  { "showexitbutton",   SYSTEM_SHOW_EXIT_BUTTON },
                                  { "canpowerdown",     SYSTEM_CAN_POWERDOWN },
                                  { "cansuspend",       SYSTEM_CAN_SUSPEND },
                                  { "canhibernate",     SYSTEM_CAN_HIBERNATE },
                                  { "canreboot",        SYSTEM_CAN_REBOOT },
                                  { "screensaveractive",SYSTEM_SCREENSAVER_ACTIVE },
                                  { "dpmsactive",       SYSTEM_DPMS_ACTIVE },
                                  { "cputemperature",   SYSTEM_CPU_TEMPERATURE },     // labels from here
                                  { "cpuusage",         SYSTEM_CPU_USAGE },
                                  { "gputemperature",   SYSTEM_GPU_TEMPERATURE },
                                  { "fanspeed",         SYSTEM_FAN_SPEED },
                                  { "freespace",        SYSTEM_FREE_SPACE },
                                  { "usedspace",        SYSTEM_USED_SPACE },
                                  { "totalspace",       SYSTEM_TOTAL_SPACE },
                                  { "usedspacepercent", SYSTEM_USED_SPACE_PERCENT },
                                  { "freespacepercent", SYSTEM_FREE_SPACE_PERCENT },
                                  { "buildversion",     SYSTEM_BUILD_VERSION },
                                  { "buildversionshort",SYSTEM_BUILD_VERSION_SHORT },
                                  { "builddate",        SYSTEM_BUILD_DATE },
                                  { "fps",              SYSTEM_FPS },
                                  { "dvdtraystate",     SYSTEM_DVD_TRAY_STATE },
                                  { "freememory",       SYSTEM_FREE_MEMORY },
                                  { "language",         SYSTEM_LANGUAGE },
                                  { "temperatureunits", SYSTEM_TEMPERATURE_UNITS },
                                  { "screenmode",       SYSTEM_SCREEN_MODE },
                                  { "screenwidth",      SYSTEM_SCREEN_WIDTH },
                                  { "screenheight",     SYSTEM_SCREEN_HEIGHT },
                                  { "currentwindow",    SYSTEM_CURRENT_WINDOW },
                                  { "currentcontrol",   SYSTEM_CURRENT_CONTROL },
                                  { "dvdlabel",         SYSTEM_DVD_LABEL },
                                  { "internetstate",    SYSTEM_INTERNET_STATE },
                                  { "osversioninfo",    SYSTEM_OS_VERSION_INFO },
                                  { "kernelversion",    SYSTEM_OS_VERSION_INFO }, // old, not correct name
                                  { "uptime",           SYSTEM_UPTIME },
                                  { "totaluptime",      SYSTEM_TOTALUPTIME },
                                  { "cpufrequency",     SYSTEM_CPUFREQUENCY },
                                  { "screenresolution", SYSTEM_SCREEN_RESOLUTION },
                                  { "videoencoderinfo", SYSTEM_VIDEO_ENCODER_INFO },
                                  { "profilename",      SYSTEM_PROFILENAME },
                                  { "profilethumb",     SYSTEM_PROFILETHUMB },
                                  { "profilecount",     SYSTEM_PROFILECOUNT },
                                  { "profileautologin", SYSTEM_PROFILEAUTOLOGIN },
                                  { "progressbar",      SYSTEM_PROGRESS_BAR },
                                  { "batterylevel",     SYSTEM_BATTERY_LEVEL },
                                  { "friendlyname",     SYSTEM_FRIENDLY_NAME },
                                  { "alarmpos",         SYSTEM_ALARM_POS },
                                  { "isinhibit",        SYSTEM_ISINHIBIT },
                                  { "hasshutdown",      SYSTEM_HAS_SHUTDOWN },
                                  { "haspvr",           SYSTEM_HAS_PVR },
                                  { "startupwindow",    SYSTEM_STARTUP_WINDOW },
                                  { "stereoscopicmode", SYSTEM_STEREOSCOPIC_MODE } };

const infomap system_param[] =   {{ "hasalarm",         SYSTEM_HAS_ALARM },
                                  { "hascoreid",        SYSTEM_HAS_CORE_ID },
                                  { "setting",          SYSTEM_SETTING },
                                  { "hasaddon",         SYSTEM_HAS_ADDON },
                                  { "coreusage",        SYSTEM_GET_CORE_USAGE }};

const infomap network_labels[] = {{ "isdhcp",            NETWORK_IS_DHCP },
                                  { "ipaddress",         NETWORK_IP_ADDRESS }, //labels from here
                                  { "linkstate",         NETWORK_LINK_STATE },
                                  { "macaddress",        NETWORK_MAC_ADDRESS },
                                  { "subnetmask",        NETWORK_SUBNET_MASK },
                                  { "gatewayaddress",    NETWORK_GATEWAY_ADDRESS },
                                  { "dns1address",       NETWORK_DNS1_ADDRESS },
                                  { "dns2address",       NETWORK_DNS2_ADDRESS },
                                  { "dhcpaddress",       NETWORK_DHCP_ADDRESS }};

const infomap musicpartymode[] = {{ "enabled",           MUSICPM_ENABLED },
                                  { "songsplayed",       MUSICPM_SONGSPLAYED },
                                  { "matchingsongs",     MUSICPM_MATCHINGSONGS },
                                  { "matchingsongspicked", MUSICPM_MATCHINGSONGSPICKED },
                                  { "matchingsongsleft", MUSICPM_MATCHINGSONGSLEFT },
                                  { "relaxedsongspicked",MUSICPM_RELAXEDSONGSPICKED },
                                  { "randomsongspicked", MUSICPM_RANDOMSONGSPICKED }};

const infomap musicplayer[] =    {{ "title",            MUSICPLAYER_TITLE },
                                  { "album",            MUSICPLAYER_ALBUM },
                                  { "artist",           MUSICPLAYER_ARTIST },
                                  { "albumartist",      MUSICPLAYER_ALBUM_ARTIST },
                                  { "year",             MUSICPLAYER_YEAR },
                                  { "genre",            MUSICPLAYER_GENRE },
                                  { "duration",         MUSICPLAYER_DURATION },
                                  { "tracknumber",      MUSICPLAYER_TRACK_NUMBER },
                                  { "cover",            MUSICPLAYER_COVER },
                                  { "bitrate",          MUSICPLAYER_BITRATE },
                                  { "playlistlength",   MUSICPLAYER_PLAYLISTLEN },
                                  { "playlistposition", MUSICPLAYER_PLAYLISTPOS },
                                  { "channels",         MUSICPLAYER_CHANNELS },
                                  { "bitspersample",    MUSICPLAYER_BITSPERSAMPLE },
                                  { "samplerate",       MUSICPLAYER_SAMPLERATE },
                                  { "codec",            MUSICPLAYER_CODEC },
                                  { "discnumber",       MUSICPLAYER_DISC_NUMBER },
                                  { "rating",           MUSICPLAYER_RATING },
                                  { "comment",          MUSICPLAYER_COMMENT },
                                  { "lyrics",           MUSICPLAYER_LYRICS },
                                  { "playlistplaying",  MUSICPLAYER_PLAYLISTPLAYING },
                                  { "exists",           MUSICPLAYER_EXISTS },
                                  { "hasprevious",      MUSICPLAYER_HASPREVIOUS },
                                  { "hasnext",          MUSICPLAYER_HASNEXT },
                                  { "playcount",        MUSICPLAYER_PLAYCOUNT },
                                  { "lastplayed",       MUSICPLAYER_LASTPLAYED },
                                  { "channelname",      MUSICPLAYER_CHANNEL_NAME },
                                  { "channelnumber",    MUSICPLAYER_CHANNEL_NUMBER },
                                  { "subchannelnumber", MUSICPLAYER_SUB_CHANNEL_NUMBER },
                                  { "channelnumberlabel", MUSICPLAYER_CHANNEL_NUMBER_LBL },
                                  { "channelgroup",     MUSICPLAYER_CHANNEL_GROUP }
};

const infomap videoplayer[] =    {{ "title",            VIDEOPLAYER_TITLE },
                                  { "genre",            VIDEOPLAYER_GENRE },
                                  { "country",          VIDEOPLAYER_COUNTRY },
                                  { "originaltitle",    VIDEOPLAYER_ORIGINALTITLE },
                                  { "director",         VIDEOPLAYER_DIRECTOR },
                                  { "year",             VIDEOPLAYER_YEAR },
                                  { "cover",            VIDEOPLAYER_COVER },
                                  { "usingoverlays",    VIDEOPLAYER_USING_OVERLAYS },
                                  { "isfullscreen",     VIDEOPLAYER_ISFULLSCREEN },
                                  { "hasmenu",          VIDEOPLAYER_HASMENU },
                                  { "playlistlength",   VIDEOPLAYER_PLAYLISTLEN },
                                  { "playlistposition", VIDEOPLAYER_PLAYLISTPOS },
                                  { "plot",             VIDEOPLAYER_PLOT },
                                  { "plotoutline",      VIDEOPLAYER_PLOT_OUTLINE },
                                  { "episode",          VIDEOPLAYER_EPISODE },
                                  { "season",           VIDEOPLAYER_SEASON },
                                  { "rating",           VIDEOPLAYER_RATING },
                                  { "ratingandvotes",   VIDEOPLAYER_RATING_AND_VOTES },
                                  { "votes",            VIDEOPLAYER_VOTES },
                                  { "tvshowtitle",      VIDEOPLAYER_TVSHOW },
                                  { "premiered",        VIDEOPLAYER_PREMIERED },
                                  { "studio",           VIDEOPLAYER_STUDIO },
                                  { "mpaa",             VIDEOPLAYER_MPAA },
                                  { "top250",           VIDEOPLAYER_TOP250 },
                                  { "cast",             VIDEOPLAYER_CAST },
                                  { "castandrole",      VIDEOPLAYER_CAST_AND_ROLE },
                                  { "artist",           VIDEOPLAYER_ARTIST },
                                  { "album",            VIDEOPLAYER_ALBUM },
                                  { "writer",           VIDEOPLAYER_WRITER },
                                  { "tagline",          VIDEOPLAYER_TAGLINE },
                                  { "hasinfo",          VIDEOPLAYER_HAS_INFO },
                                  { "trailer",          VIDEOPLAYER_TRAILER },
                                  { "videocodec",       VIDEOPLAYER_VIDEO_CODEC },
                                  { "videoresolution",  VIDEOPLAYER_VIDEO_RESOLUTION },
                                  { "videoaspect",      VIDEOPLAYER_VIDEO_ASPECT },
                                  { "audiocodec",       VIDEOPLAYER_AUDIO_CODEC },
                                  { "audiochannels",    VIDEOPLAYER_AUDIO_CHANNELS },
                                  { "audiolanguage",    VIDEOPLAYER_AUDIO_LANG },
                                  { "hasteletext",      VIDEOPLAYER_HASTELETEXT },
                                  { "lastplayed",       VIDEOPLAYER_LASTPLAYED },
                                  { "playcount",        VIDEOPLAYER_PLAYCOUNT },
                                  { "hassubtitles",     VIDEOPLAYER_HASSUBTITLES },
                                  { "subtitlesenabled", VIDEOPLAYER_SUBTITLESENABLED },
                                  { "subtitleslanguage",VIDEOPLAYER_SUBTITLES_LANG },
                                  { "endtime",          VIDEOPLAYER_ENDTIME },
                                  { "nexttitle",        VIDEOPLAYER_NEXT_TITLE },
                                  { "nextgenre",        VIDEOPLAYER_NEXT_GENRE },
                                  { "nextplot",         VIDEOPLAYER_NEXT_PLOT },
                                  { "nextplotoutline",  VIDEOPLAYER_NEXT_PLOT_OUTLINE },
                                  { "nextstarttime",    VIDEOPLAYER_NEXT_STARTTIME },
                                  { "nextendtime",      VIDEOPLAYER_NEXT_ENDTIME },
                                  { "nextduration",     VIDEOPLAYER_NEXT_DURATION },
                                  { "channelname",      VIDEOPLAYER_CHANNEL_NAME },
                                  { "channelnumber",    VIDEOPLAYER_CHANNEL_NUMBER },
                                  { "subchannelnumber", VIDEOPLAYER_SUB_CHANNEL_NUMBER },
                                  { "channelnumberlabel", VIDEOPLAYER_CHANNEL_NUMBER_LBL },
                                  { "channelgroup",     VIDEOPLAYER_CHANNEL_GROUP },
                                  { "hasepg",           VIDEOPLAYER_HAS_EPG },
                                  { "parentalrating",   VIDEOPLAYER_PARENTAL_RATING },
                                  { "isstereoscopic",   VIDEOPLAYER_IS_STEREOSCOPIC },
                                  { "stereoscopicmode", VIDEOPLAYER_STEREOSCOPIC_MODE },
                                  { "canresumelivetv",  VIDEOPLAYER_CAN_RESUME_LIVE_TV },
                                  { "imdbnumber",       VIDEOPLAYER_IMDBNUMBER },
                                  { "episodename",      VIDEOPLAYER_EPISODENAME }
};

const infomap mediacontainer[] = {{ "hasfiles",         CONTAINER_HASFILES },
                                  { "hasfolders",       CONTAINER_HASFOLDERS },
                                  { "isstacked",        CONTAINER_STACKED },
                                  { "folderpath",       CONTAINER_FOLDERPATH },
                                  { "foldername",       CONTAINER_FOLDERNAME },
                                  { "pluginname",       CONTAINER_PLUGINNAME },
                                  { "viewmode",         CONTAINER_VIEWMODE },
                                  { "totaltime",        CONTAINER_TOTALTIME },
                                  { "hasthumb",         CONTAINER_HAS_THUMB },
                                  { "sortmethod",       CONTAINER_SORT_METHOD },
                                  { "showplot",         CONTAINER_SHOWPLOT }};

const infomap container_bools[] ={{ "onnext",           CONTAINER_MOVE_NEXT },
                                  { "onprevious",       CONTAINER_MOVE_PREVIOUS },
                                  { "onscrollnext",     CONTAINER_SCROLL_NEXT },
                                  { "onscrollprevious", CONTAINER_SCROLL_PREVIOUS },
                                  { "numpages",         CONTAINER_NUM_PAGES },
                                  { "numitems",         CONTAINER_NUM_ITEMS },
                                  { "currentpage",      CONTAINER_CURRENT_PAGE },
                                  { "scrolling",        CONTAINER_SCROLLING },
                                  { "hasnext",          CONTAINER_HAS_NEXT },
                                  { "hasprevious",      CONTAINER_HAS_PREVIOUS },
                                  { "canfilter",        CONTAINER_CAN_FILTER },
                                  { "canfilteradvanced",CONTAINER_CAN_FILTERADVANCED },
                                  { "filtered",         CONTAINER_FILTERED },
                                  { "isupdating",       CONTAINER_ISUPDATING }};

const infomap container_ints[] = {{ "row",              CONTAINER_ROW },
                                  { "column",           CONTAINER_COLUMN },
                                  { "position",         CONTAINER_POSITION },
                                  { "currentitem",      CONTAINER_CURRENT_ITEM },
                                  { "subitem",          CONTAINER_SUBITEM },
                                  { "hasfocus",         CONTAINER_HAS_FOCUS }};

const infomap container_str[]  = {{ "property",         CONTAINER_PROPERTY },
                                  { "content",          CONTAINER_CONTENT },
                                  { "art",              CONTAINER_ART }};

const infomap listitem_labels[]= {{ "thumb",            LISTITEM_THUMB },
                                  { "icon",             LISTITEM_ICON },
                                  { "actualicon",       LISTITEM_ACTUAL_ICON },
                                  { "overlay",          LISTITEM_OVERLAY },
                                  { "label",            LISTITEM_LABEL },
                                  { "label2",           LISTITEM_LABEL2 },
                                  { "title",            LISTITEM_TITLE },
                                  { "tracknumber",      LISTITEM_TRACKNUMBER },
                                  { "artist",           LISTITEM_ARTIST },
                                  { "album",            LISTITEM_ALBUM },
                                  { "albumartist",      LISTITEM_ALBUM_ARTIST },
                                  { "year",             LISTITEM_YEAR },
                                  { "genre",            LISTITEM_GENRE },
                                  { "director",         LISTITEM_DIRECTOR },
                                  { "filename",         LISTITEM_FILENAME },
                                  { "filenameandpath",  LISTITEM_FILENAME_AND_PATH },
                                  { "fileextension",    LISTITEM_FILE_EXTENSION },
                                  { "date",             LISTITEM_DATE },
                                  { "size",             LISTITEM_SIZE },
                                  { "rating",           LISTITEM_RATING },
                                  { "ratingandvotes",   LISTITEM_RATING_AND_VOTES },
                                  { "votes",            LISTITEM_VOTES },
                                  { "programcount",     LISTITEM_PROGRAM_COUNT },
                                  { "duration",         LISTITEM_DURATION },
                                  { "isselected",       LISTITEM_ISSELECTED },
                                  { "isplaying",        LISTITEM_ISPLAYING },
                                  { "plot",             LISTITEM_PLOT },
                                  { "plotoutline",      LISTITEM_PLOT_OUTLINE },
                                  { "episode",          LISTITEM_EPISODE },
                                  { "season",           LISTITEM_SEASON },
                                  { "tvshowtitle",      LISTITEM_TVSHOW },
                                  { "premiered",        LISTITEM_PREMIERED },
                                  { "comment",          LISTITEM_COMMENT },
                                  { "path",             LISTITEM_PATH },
                                  { "foldername",       LISTITEM_FOLDERNAME },
                                  { "folderpath",       LISTITEM_FOLDERPATH },
                                  { "picturepath",      LISTITEM_PICTURE_PATH },
                                  { "pictureresolution",LISTITEM_PICTURE_RESOLUTION },
                                  { "picturedatetime",  LISTITEM_PICTURE_DATETIME },
                                  { "picturedate",      LISTITEM_PICTURE_DATE },
                                  { "picturelongdatetime",LISTITEM_PICTURE_LONGDATETIME },
                                  { "picturelongdate",  LISTITEM_PICTURE_LONGDATE },
                                  { "picturecomment",   LISTITEM_PICTURE_COMMENT },
                                  { "picturecaption",   LISTITEM_PICTURE_CAPTION },
                                  { "picturedesc",      LISTITEM_PICTURE_DESC },
                                  { "picturekeywords",  LISTITEM_PICTURE_KEYWORDS },
                                  { "picturecammake",   LISTITEM_PICTURE_CAM_MAKE },
                                  { "picturecammodel",  LISTITEM_PICTURE_CAM_MODEL },
                                  { "pictureaperture",  LISTITEM_PICTURE_APERTURE },
                                  { "picturefocallen",  LISTITEM_PICTURE_FOCAL_LEN },
                                  { "picturefocusdist", LISTITEM_PICTURE_FOCUS_DIST },
                                  { "pictureexpmode",   LISTITEM_PICTURE_EXP_MODE },
                                  { "pictureexptime",   LISTITEM_PICTURE_EXP_TIME },
                                  { "pictureiso",       LISTITEM_PICTURE_ISO },
                                  { "pictureauthor",                 LISTITEM_PICTURE_AUTHOR },
                                  { "picturebyline",                 LISTITEM_PICTURE_BYLINE },
                                  { "picturebylinetitle",            LISTITEM_PICTURE_BYLINE_TITLE },
                                  { "picturecategory",               LISTITEM_PICTURE_CATEGORY },
                                  { "pictureccdwidth",               LISTITEM_PICTURE_CCD_WIDTH },
                                  { "picturecity",                   LISTITEM_PICTURE_CITY },
                                  { "pictureurgency",                LISTITEM_PICTURE_URGENCY },
                                  { "picturecopyrightnotice",        LISTITEM_PICTURE_COPYRIGHT_NOTICE },
                                  { "picturecountry",                LISTITEM_PICTURE_COUNTRY },
                                  { "picturecountrycode",            LISTITEM_PICTURE_COUNTRY_CODE },
                                  { "picturecredit",                 LISTITEM_PICTURE_CREDIT },
                                  { "pictureiptcdate",               LISTITEM_PICTURE_IPTCDATE },
                                  { "picturedigitalzoom",            LISTITEM_PICTURE_DIGITAL_ZOOM },
                                  { "pictureexposure",               LISTITEM_PICTURE_EXPOSURE },
                                  { "pictureexposurebias",           LISTITEM_PICTURE_EXPOSURE_BIAS },
                                  { "pictureflashused",              LISTITEM_PICTURE_FLASH_USED },
                                  { "pictureheadline",               LISTITEM_PICTURE_HEADLINE },
                                  { "picturecolour",                 LISTITEM_PICTURE_COLOUR },
                                  { "picturelightsource",            LISTITEM_PICTURE_LIGHT_SOURCE },
                                  { "picturemeteringmode",           LISTITEM_PICTURE_METERING_MODE },
                                  { "pictureobjectname",             LISTITEM_PICTURE_OBJECT_NAME },
                                  { "pictureorientation",            LISTITEM_PICTURE_ORIENTATION },
                                  { "pictureprocess",                LISTITEM_PICTURE_PROCESS },
                                  { "picturereferenceservice",       LISTITEM_PICTURE_REF_SERVICE },
                                  { "picturesource",                 LISTITEM_PICTURE_SOURCE },
                                  { "picturespecialinstructions",    LISTITEM_PICTURE_SPEC_INSTR },
                                  { "picturestate",                  LISTITEM_PICTURE_STATE },
                                  { "picturesupplementalcategories", LISTITEM_PICTURE_SUP_CATEGORIES },
                                  { "picturetransmissionreference",  LISTITEM_PICTURE_TX_REFERENCE },
                                  { "picturewhitebalance",           LISTITEM_PICTURE_WHITE_BALANCE },
                                  { "pictureimagetype",              LISTITEM_PICTURE_IMAGETYPE },
                                  { "picturesublocation",            LISTITEM_PICTURE_SUBLOCATION },
                                  { "pictureiptctime",               LISTITEM_PICTURE_TIMECREATED },
                                  { "picturegpslat",    LISTITEM_PICTURE_GPS_LAT },
                                  { "picturegpslon",    LISTITEM_PICTURE_GPS_LON },
                                  { "picturegpsalt",    LISTITEM_PICTURE_GPS_ALT },
                                  { "studio",           LISTITEM_STUDIO },
                                  { "country",          LISTITEM_COUNTRY },
                                  { "mpaa",             LISTITEM_MPAA },
                                  { "cast",             LISTITEM_CAST },
                                  { "castandrole",      LISTITEM_CAST_AND_ROLE },
                                  { "writer",           LISTITEM_WRITER },
                                  { "tagline",          LISTITEM_TAGLINE },
                                  { "top250",           LISTITEM_TOP250 },
                                  { "trailer",          LISTITEM_TRAILER },
                                  { "starrating",       LISTITEM_STAR_RATING },
                                  { "sortletter",       LISTITEM_SORT_LETTER },
                                  { "videocodec",       LISTITEM_VIDEO_CODEC },
                                  { "videoresolution",  LISTITEM_VIDEO_RESOLUTION },
                                  { "videoaspect",      LISTITEM_VIDEO_ASPECT },
                                  { "audiocodec",       LISTITEM_AUDIO_CODEC },
                                  { "audiochannels",    LISTITEM_AUDIO_CHANNELS },
                                  { "audiolanguage",    LISTITEM_AUDIO_LANGUAGE },
                                  { "subtitlelanguage", LISTITEM_SUBTITLE_LANGUAGE },
                                  { "isresumable",      LISTITEM_IS_RESUMABLE},
                                  { "percentplayed",    LISTITEM_PERCENT_PLAYED},
                                  { "isfolder",         LISTITEM_IS_FOLDER },
                                  { "iscollection",     LISTITEM_IS_COLLECTION },
                                  { "originaltitle",    LISTITEM_ORIGINALTITLE },
                                  { "lastplayed",       LISTITEM_LASTPLAYED },
                                  { "playcount",        LISTITEM_PLAYCOUNT },
                                  { "discnumber",       LISTITEM_DISC_NUMBER },
                                  { "starttime",        LISTITEM_STARTTIME },
                                  { "endtime",          LISTITEM_ENDTIME },
                                  { "startdate",        LISTITEM_STARTDATE },
                                  { "enddate",          LISTITEM_ENDDATE },
                                  { "nexttitle",        LISTITEM_NEXT_TITLE },
                                  { "nextgenre",        LISTITEM_NEXT_GENRE },
                                  { "nextplot",         LISTITEM_NEXT_PLOT },
                                  { "nextplotoutline",  LISTITEM_NEXT_PLOT_OUTLINE },
                                  { "nextstarttime",    LISTITEM_NEXT_STARTTIME },
                                  { "nextendtime",      LISTITEM_NEXT_ENDTIME },
                                  { "nextstartdate",    LISTITEM_NEXT_STARTDATE },
                                  { "nextenddate",      LISTITEM_NEXT_ENDDATE },
                                  { "channelname",      LISTITEM_CHANNEL_NAME },
                                  { "channelnumber",    LISTITEM_CHANNEL_NUMBER },
                                  { "subchannelnumber", LISTITEM_SUB_CHANNEL_NUMBER },
                                  { "channelnumberlabel", LISTITEM_CHANNEL_NUMBER_LBL },
                                  { "channelgroup",     LISTITEM_CHANNEL_GROUP },
                                  { "hasepg",           LISTITEM_HAS_EPG },
                                  { "hastimer",         LISTITEM_HASTIMER },
                                  { "hastimerschedule", LISTITEM_HASTIMERSCHEDULE },
                                  { "hasrecording",     LISTITEM_HASRECORDING },
                                  { "isrecording",      LISTITEM_ISRECORDING },
                                  { "inprogress",       LISTITEM_INPROGRESS },
                                  { "isencrypted",      LISTITEM_ISENCRYPTED },
                                  { "progress",         LISTITEM_PROGRESS },
                                  { "dateadded",        LISTITEM_DATE_ADDED },
                                  { "dbtype",           LISTITEM_DBTYPE },
                                  { "dbid",             LISTITEM_DBID },
                                  { "stereoscopicmode", LISTITEM_STEREOSCOPIC_MODE },
                                  { "isstereoscopic",   LISTITEM_IS_STEREOSCOPIC },
                                  { "imdbnumber",       LISTITEM_IMDBNUMBER },
                                  { "episodename",      LISTITEM_EPISODENAME },
                                  { "timertype",        LISTITEM_TIMERTYPE }};

const infomap visualisation[] =  {{ "locked",           VISUALISATION_LOCKED },
                                  { "preset",           VISUALISATION_PRESET },
                                  { "name",             VISUALISATION_NAME },
                                  { "enabled",          VISUALISATION_ENABLED }};

const infomap fanart_labels[] =  {{ "color1",           FANART_COLOR1 },
                                  { "color2",           FANART_COLOR2 },
                                  { "color3",           FANART_COLOR3 },
                                  { "image",            FANART_IMAGE }};

const infomap skin_labels[] =    {{ "currenttheme",     SKIN_THEME },
                                  { "currentcolourtheme",SKIN_COLOUR_THEME },
                                  {"hasvideooverlay",   SKIN_HAS_VIDEO_OVERLAY},
                                  {"hasmusicoverlay",   SKIN_HAS_MUSIC_OVERLAY},
                                  {"aspectratio",       SKIN_ASPECT_RATIO}};

const infomap window_bools[] =   {{ "ismedia",          WINDOW_IS_MEDIA },
                                  { "isactive",         WINDOW_IS_ACTIVE },
                                  { "istopmost",        WINDOW_IS_TOPMOST },
                                  { "isvisible",        WINDOW_IS_VISIBLE },
                                  { "previous",         WINDOW_PREVIOUS },
                                  { "next",             WINDOW_NEXT }};

const infomap control_labels[] = {{ "hasfocus",         CONTROL_HAS_FOCUS },
                                  { "isvisible",        CONTROL_IS_VISIBLE },
                                  { "isenabled",        CONTROL_IS_ENABLED },
                                  { "getlabel",         CONTROL_GET_LABEL }};

const infomap playlist[] =       {{ "length",           PLAYLIST_LENGTH },
                                  { "position",         PLAYLIST_POSITION },
                                  { "random",           PLAYLIST_RANDOM },
                                  { "repeat",           PLAYLIST_REPEAT },
                                  { "israndom",         PLAYLIST_ISRANDOM },
                                  { "isrepeat",         PLAYLIST_ISREPEAT },
                                  { "isrepeatone",      PLAYLIST_ISREPEATONE }};

const infomap pvr[] =            {{ "isrecording",              PVR_IS_RECORDING },
                                  { "hastimer",                 PVR_HAS_TIMER },
                                  { "hastvchannels",            PVR_HAS_TV_CHANNELS },
                                  { "hasradiochannels",         PVR_HAS_RADIO_CHANNELS },
                                  { "hasnonrecordingtimer",     PVR_HAS_NONRECORDING_TIMER },
                                  { "nowrecordingtitle",        PVR_NOW_RECORDING_TITLE },
                                  { "nowrecordingdatetime",     PVR_NOW_RECORDING_DATETIME },
                                  { "nowrecordingchannel",      PVR_NOW_RECORDING_CHANNEL },
                                  { "nowrecordingchannelicon",  PVR_NOW_RECORDING_CHAN_ICO },
                                  { "nextrecordingtitle",       PVR_NEXT_RECORDING_TITLE },
                                  { "nextrecordingdatetime",    PVR_NEXT_RECORDING_DATETIME },
                                  { "nextrecordingchannel",     PVR_NEXT_RECORDING_CHANNEL },
                                  { "nextrecordingchannelicon", PVR_NEXT_RECORDING_CHAN_ICO },
                                  { "backendname",              PVR_BACKEND_NAME },
                                  { "backendversion",           PVR_BACKEND_VERSION },
                                  { "backendhost",              PVR_BACKEND_HOST },
                                  { "backenddiskspace",         PVR_BACKEND_DISKSPACE },
                                  { "backenddiskspaceprogr",    PVR_BACKEND_DISKSPACE_PROGR },
                                  { "backendchannels",          PVR_BACKEND_CHANNELS },
                                  { "backendtimers",            PVR_BACKEND_TIMERS },
                                  { "backendrecordings",        PVR_BACKEND_RECORDINGS },
                                  { "backenddeletedrecordings", PVR_BACKEND_DELETED_RECORDINGS },
                                  { "backendnumber",            PVR_BACKEND_NUMBER },
                                  { "hasepg",                   PVR_HAS_EPG },
                                  { "hastxt",                   PVR_HAS_TXT },
                                  { "hasdirector",              PVR_HAS_DIRECTOR },
                                  { "totaldiscspace",           PVR_TOTAL_DISKSPACE },
                                  { "nexttimer",                PVR_NEXT_TIMER },
                                  { "isplayingtv",              PVR_IS_PLAYING_TV },
                                  { "isplayingradio",           PVR_IS_PLAYING_RADIO },
                                  { "isplayingrecording",       PVR_IS_PLAYING_RECORDING },
                                  { "duration",                 PVR_PLAYING_DURATION },
                                  { "time",                     PVR_PLAYING_TIME },
                                  { "progress",                 PVR_PLAYING_PROGRESS },
                                  { "actstreamclient",          PVR_ACTUAL_STREAM_CLIENT },
                                  { "actstreamdevice",          PVR_ACTUAL_STREAM_DEVICE },
                                  { "actstreamstatus",          PVR_ACTUAL_STREAM_STATUS },
                                  { "actstreamsignal",          PVR_ACTUAL_STREAM_SIG },
                                  { "actstreamsnr",             PVR_ACTUAL_STREAM_SNR },
                                  { "actstreamber",             PVR_ACTUAL_STREAM_BER },
                                  { "actstreamunc",             PVR_ACTUAL_STREAM_UNC },
                                  { "actstreamvideobitrate",    PVR_ACTUAL_STREAM_VIDEO_BR },
                                  { "actstreamaudiobitrate",    PVR_ACTUAL_STREAM_AUDIO_BR },
                                  { "actstreamdolbybitrate",    PVR_ACTUAL_STREAM_DOLBY_BR },
                                  { "actstreamprogrsignal",     PVR_ACTUAL_STREAM_SIG_PROGR },
                                  { "actstreamprogrsnr",        PVR_ACTUAL_STREAM_SNR_PROGR },
                                  { "actstreamisencrypted",     PVR_ACTUAL_STREAM_ENCRYPTED },
                                  { "actstreamencryptionname",  PVR_ACTUAL_STREAM_CRYPTION },
                                  { "actstreamservicename",     PVR_ACTUAL_STREAM_SERVICE },
                                  { "actstreammux",             PVR_ACTUAL_STREAM_MUX },
                                  { "actstreamprovidername",    PVR_ACTUAL_STREAM_PROVIDER }};

const infomap slideshow[] =      {{ "ispaused",         SLIDESHOW_ISPAUSED },
                                  { "isactive",         SLIDESHOW_ISACTIVE },
                                  { "isvideo",          SLIDESHOW_ISVIDEO },
                                  { "israndom",         SLIDESHOW_ISRANDOM }};

const int picture_slide_map[]  = {/* LISTITEM_PICTURE_RESOLUTION => */ SLIDE_RESOLUTION,
                                  /* LISTITEM_PICTURE_LONGDATE   => */ SLIDE_EXIF_LONG_DATE,
                                  /* LISTITEM_PICTURE_LONGDATETIME => */ SLIDE_EXIF_LONG_DATE_TIME,
                                  /* LISTITEM_PICTURE_DATE       => */ SLIDE_EXIF_DATE,
                                  /* LISTITEM_PICTURE_DATETIME   => */ SLIDE_EXIF_DATE_TIME,
                                  /* LISTITEM_PICTURE_COMMENT    => */ SLIDE_COMMENT,
                                  /* LISTITEM_PICTURE_CAPTION    => */ SLIDE_IPTC_CAPTION,
                                  /* LISTITEM_PICTURE_DESC       => */ SLIDE_EXIF_DESCRIPTION,
                                  /* LISTITEM_PICTURE_KEYWORDS   => */ SLIDE_IPTC_KEYWORDS,
                                  /* LISTITEM_PICTURE_CAM_MAKE   => */ SLIDE_EXIF_CAMERA_MAKE,
                                  /* LISTITEM_PICTURE_CAM_MODEL  => */ SLIDE_EXIF_CAMERA_MODEL,
                                  /* LISTITEM_PICTURE_APERTURE   => */ SLIDE_EXIF_APERTURE,
                                  /* LISTITEM_PICTURE_FOCAL_LEN  => */ SLIDE_EXIF_FOCAL_LENGTH,
                                  /* LISTITEM_PICTURE_FOCUS_DIST => */ SLIDE_EXIF_FOCUS_DIST,
                                  /* LISTITEM_PICTURE_EXP_MODE   => */ SLIDE_EXIF_EXPOSURE_MODE,
                                  /* LISTITEM_PICTURE_EXP_TIME   => */ SLIDE_EXIF_EXPOSURE_TIME,
                                  /* LISTITEM_PICTURE_ISO        => */ SLIDE_EXIF_ISO_EQUIV,
                                  /* LISTITEM_PICTURE_AUTHOR           => */ SLIDE_IPTC_AUTHOR,
                                  /* LISTITEM_PICTURE_BYLINE           => */ SLIDE_IPTC_BYLINE,
                                  /* LISTITEM_PICTURE_BYLINE_TITLE     => */ SLIDE_IPTC_BYLINE_TITLE,
                                  /* LISTITEM_PICTURE_CATEGORY         => */ SLIDE_IPTC_CATEGORY,
                                  /* LISTITEM_PICTURE_CCD_WIDTH        => */ SLIDE_EXIF_CCD_WIDTH,
                                  /* LISTITEM_PICTURE_CITY             => */ SLIDE_IPTC_CITY,
                                  /* LISTITEM_PICTURE_URGENCY          => */ SLIDE_IPTC_URGENCY,
                                  /* LISTITEM_PICTURE_COPYRIGHT_NOTICE => */ SLIDE_IPTC_COPYRIGHT_NOTICE,
                                  /* LISTITEM_PICTURE_COUNTRY          => */ SLIDE_IPTC_COUNTRY,
                                  /* LISTITEM_PICTURE_COUNTRY_CODE     => */ SLIDE_IPTC_COUNTRY_CODE,
                                  /* LISTITEM_PICTURE_CREDIT           => */ SLIDE_IPTC_CREDIT,
                                  /* LISTITEM_PICTURE_IPTCDATE         => */ SLIDE_IPTC_DATE,
                                  /* LISTITEM_PICTURE_DIGITAL_ZOOM     => */ SLIDE_EXIF_DIGITAL_ZOOM,
                                  /* LISTITEM_PICTURE_EXPOSURE         => */ SLIDE_EXIF_EXPOSURE,
                                  /* LISTITEM_PICTURE_EXPOSURE_BIAS    => */ SLIDE_EXIF_EXPOSURE_BIAS,
                                  /* LISTITEM_PICTURE_FLASH_USED       => */ SLIDE_EXIF_FLASH_USED,
                                  /* LISTITEM_PICTURE_HEADLINE         => */ SLIDE_IPTC_HEADLINE,
                                  /* LISTITEM_PICTURE_COLOUR           => */ SLIDE_COLOUR,
                                  /* LISTITEM_PICTURE_LIGHT_SOURCE     => */ SLIDE_EXIF_LIGHT_SOURCE,
                                  /* LISTITEM_PICTURE_METERING_MODE    => */ SLIDE_EXIF_METERING_MODE,
                                  /* LISTITEM_PICTURE_OBJECT_NAME      => */ SLIDE_IPTC_OBJECT_NAME,
                                  /* LISTITEM_PICTURE_ORIENTATION      => */ SLIDE_EXIF_ORIENTATION,
                                  /* LISTITEM_PICTURE_PROCESS          => */ SLIDE_PROCESS,
                                  /* LISTITEM_PICTURE_REF_SERVICE      => */ SLIDE_IPTC_REF_SERVICE,
                                  /* LISTITEM_PICTURE_SOURCE           => */ SLIDE_IPTC_SOURCE,
                                  /* LISTITEM_PICTURE_SPEC_INSTR       => */ SLIDE_IPTC_SPEC_INSTR,
                                  /* LISTITEM_PICTURE_STATE            => */ SLIDE_IPTC_STATE,
                                  /* LISTITEM_PICTURE_SUP_CATEGORIES   => */ SLIDE_IPTC_SUP_CATEGORIES,
                                  /* LISTITEM_PICTURE_TX_REFERENCE     => */ SLIDE_IPTC_TX_REFERENCE,
                                  /* LISTITEM_PICTURE_WHITE_BALANCE    => */ SLIDE_EXIF_WHITE_BALANCE,
                                  /* LISTITEM_PICTURE_IMAGETYPE        => */ SLIDE_IPTC_IMAGETYPE,
                                  /* LISTITEM_PICTURE_SUBLOCATION      => */ SLIDE_IPTC_SUBLOCATION,
                                  /* LISTITEM_PICTURE_TIMECREATED      => */ SLIDE_IPTC_TIMECREATED,
                                  /* LISTITEM_PICTURE_GPS_LAT    => */ SLIDE_EXIF_GPS_LATITUDE,
                                  /* LISTITEM_PICTURE_GPS_LON    => */ SLIDE_EXIF_GPS_LONGITUDE,
                                  /* LISTITEM_PICTURE_GPS_ALT    => */ SLIDE_EXIF_GPS_ALTITUDE };

CGUIInfoManager::Property::Property(const std::string &property, const std::string &parameters)
: name(property)
{
  CUtil::SplitParams(parameters, params);
}

const std::string &CGUIInfoManager::Property::param(unsigned int n /* = 0 */) const
{
  if (n < params.size())
    return params[n];
  return StringUtils::Empty;
}

unsigned int CGUIInfoManager::Property::num_params() const
{
  return params.size();
}

void CGUIInfoManager::SplitInfoString(const std::string &infoString, vector<Property> &info)
{
  // our string is of the form:
  // category[(params)][.info(params).info2(params)] ...
  // so we need to split on . while taking into account of () pairs
  unsigned int parentheses = 0;
  std::string property;
  std::string param;
  for (size_t i = 0; i < infoString.size(); ++i)
  {
    if (infoString[i] == '(')
    {
      if (!parentheses++)
        continue;
    }
    else if (infoString[i] == ')')
    {
      if (!parentheses)
        CLog::Log(LOGERROR, "unmatched parentheses in %s", infoString.c_str());
      else if (!--parentheses)
        continue;
    }
    else if (infoString[i] == '.' && !parentheses)
    {
      if (!property.empty()) // add our property and parameters
      {
        StringUtils::ToLower(property);
        info.push_back(Property(property, param));
      }
      property.clear();
      param.clear();
      continue;
    }
    if (parentheses)
      param += infoString[i];
    else
      property += infoString[i];
  }
  if (parentheses)
    CLog::Log(LOGERROR, "unmatched parentheses in %s", infoString.c_str());
  if (!property.empty())
  {
    StringUtils::ToLower(property);
    info.push_back(Property(property, param));
  }
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data.
int CGUIInfoManager::TranslateSingleString(const std::string &strCondition)
{
  bool listItemDependent;
  return TranslateSingleString(strCondition, listItemDependent);
}

int CGUIInfoManager::TranslateSingleString(const std::string &strCondition, bool &listItemDependent)
{
  /* We need to disable caching in INFO::InfoBool::Get if either of the following are true:
   *  1. if condition is between LISTITEM_START and LISTITEM_END
   *  2. if condition is STRING_IS_EMPTY, STRING_COMPARE, STRING_STR, INTEGER_GREATER_THAN and the
   *     corresponding label is between LISTITEM_START and LISTITEM_END
   *  This is achieved by setting the bool pointed at by listItemDependent, either here or in a recursive call
   */
  // trim whitespaces
  std::string strTest = strCondition;
  StringUtils::Trim(strTest);

  vector< Property> info;
  SplitInfoString(strTest, info);

  if (info.empty())
    return 0;

  const Property &cat = info[0];
  if (info.size() == 1)
  { // single category
    if (cat.name == "false" || cat.name == "no" || cat.name == "off")
      return SYSTEM_ALWAYS_FALSE;
    else if (cat.name == "true" || cat.name == "yes" || cat.name == "on")
      return SYSTEM_ALWAYS_TRUE;
    if (cat.name == "isempty" && cat.num_params() == 1)
      return AddMultiInfo(GUIInfo(STRING_IS_EMPTY, TranslateSingleString(cat.param(), listItemDependent)));
    else if (cat.name == "stringcompare" && cat.num_params() == 2)
    {
      int info = TranslateSingleString(cat.param(0), listItemDependent);
      int info2 = TranslateSingleString(cat.param(1), listItemDependent);
      if (info2 > 0)
        return AddMultiInfo(GUIInfo(STRING_COMPARE, info, -info2));
      // pipe our original string through the localize parsing then make it lowercase (picks up $LBRACKET etc.)
      std::string label = CGUIInfoLabel::GetLabel(cat.param(1));
      StringUtils::ToLower(label);
      int compareString = ConditionalStringParameter(label);
      return AddMultiInfo(GUIInfo(STRING_COMPARE, info, compareString));
    }
    else if (cat.name == "integergreaterthan" && cat.num_params() == 2)
    {
      int info = TranslateSingleString(cat.param(0), listItemDependent);
      int compareInt = atoi(cat.param(1).c_str());
      return AddMultiInfo(GUIInfo(INTEGER_GREATER_THAN, info, compareInt));
    }
    else if (cat.name == "substring" && cat.num_params() >= 2)
    {
      int info = TranslateSingleString(cat.param(0), listItemDependent);
      std::string label = CGUIInfoLabel::GetLabel(cat.param(1));
      StringUtils::ToLower(label);
      int compareString = ConditionalStringParameter(label);
      if (cat.num_params() > 2)
      {
        if (StringUtils::EqualsNoCase(cat.param(2), "left"))
          return AddMultiInfo(GUIInfo(STRING_STR_LEFT, info, compareString));
        else if (StringUtils::EqualsNoCase(cat.param(2), "right"))
          return AddMultiInfo(GUIInfo(STRING_STR_RIGHT, info, compareString));
      }
      return AddMultiInfo(GUIInfo(STRING_STR, info, compareString));
    }
  }
  else if (info.size() == 2)
  {
    const Property &prop = info[1];
    if (cat.name == "player")
    {
      for (size_t i = 0; i < sizeof(player_labels) / sizeof(infomap); i++)
      {
        if (prop.name == player_labels[i].str)
          return player_labels[i].val;
      }
      for (size_t i = 0; i < sizeof(player_times) / sizeof(infomap); i++)
      {
        if (prop.name == player_times[i].str)
          return AddMultiInfo(GUIInfo(player_times[i].val, TranslateTimeFormat(prop.param())));
      }
      if (prop.num_params() == 1)
      {
        for (size_t i = 0; i < sizeof(player_param) / sizeof(infomap); i++)
        {
          if (prop.name == player_param[i].str)
            return AddMultiInfo(GUIInfo(player_param[i].val, ConditionalStringParameter(prop.param())));
        }
      }
    }
    else if (cat.name == "weather")
    {
      for (size_t i = 0; i < sizeof(weather) / sizeof(infomap); i++)
      {
        if (prop.name == weather[i].str)
          return weather[i].val;
      }
    }
    else if (cat.name == "network")
    {
      for (size_t i = 0; i < sizeof(network_labels) / sizeof(infomap); i++)
      {
        if (prop.name == network_labels[i].str)
          return network_labels[i].val;
      }
    }
    else if (cat.name == "musicpartymode")
    {
      for (size_t i = 0; i < sizeof(musicpartymode) / sizeof(infomap); i++)
      {
        if (prop.name == musicpartymode[i].str)
          return musicpartymode[i].val;
      }
    }
    else if (cat.name == "system")
    {
      for (size_t i = 0; i < sizeof(system_labels) / sizeof(infomap); i++)
      {
        if (prop.name == system_labels[i].str)
          return system_labels[i].val;
      }
      if (prop.num_params() == 1)
      {
        const std::string &param = prop.param();
        if (prop.name == "getbool")
        {
          std::string paramCopy = param;
          StringUtils::ToLower(paramCopy);
          return AddMultiInfo(GUIInfo(SYSTEM_GET_BOOL, ConditionalStringParameter(paramCopy, true)));
        }
        for (size_t i = 0; i < sizeof(system_param) / sizeof(infomap); i++)
        {
          if (prop.name == system_param[i].str)
            return AddMultiInfo(GUIInfo(system_param[i].val, ConditionalStringParameter(param)));
        }
        if (prop.name == "memory")
        {
          if (param == "free") return SYSTEM_FREE_MEMORY;
          else if (param == "free.percent") return SYSTEM_FREE_MEMORY_PERCENT;
          else if (param == "used") return SYSTEM_USED_MEMORY;
          else if (param == "used.percent") return SYSTEM_USED_MEMORY_PERCENT;
          else if (param == "total") return SYSTEM_TOTAL_MEMORY;
        }
        else if (prop.name == "addontitle")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(GUIInfo(SYSTEM_ADDON_TITLE, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(GUIInfo(SYSTEM_ADDON_TITLE, ConditionalStringParameter(label), 1));
        }
        else if (prop.name == "addonicon")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(GUIInfo(SYSTEM_ADDON_ICON, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(GUIInfo(SYSTEM_ADDON_ICON, ConditionalStringParameter(label), 1));
        }
        else if (prop.name == "addonversion")
        {
          int infoLabel = TranslateSingleString(param, listItemDependent);
          if (infoLabel > 0)
            return AddMultiInfo(GUIInfo(SYSTEM_ADDON_VERSION, infoLabel, 0));
          std::string label = CGUIInfoLabel::GetLabel(param);
          StringUtils::ToLower(label);
          return AddMultiInfo(GUIInfo(SYSTEM_ADDON_VERSION, ConditionalStringParameter(label), 1));
        }
        else if (prop.name == "idletime")
          return AddMultiInfo(GUIInfo(SYSTEM_IDLE_TIME, atoi(param.c_str())));
      }
      if (prop.name == "alarmlessorequal" && prop.num_params() == 2)
        return AddMultiInfo(GUIInfo(SYSTEM_ALARM_LESS_OR_EQUAL, ConditionalStringParameter(prop.param(0)), ConditionalStringParameter(prop.param(1))));
      else if (prop.name == "date")
      {
        if (prop.num_params() == 2)
          return AddMultiInfo(GUIInfo(SYSTEM_DATE, StringUtils::DateStringToYYYYMMDD(prop.param(0)) % 10000, StringUtils::DateStringToYYYYMMDD(prop.param(1)) % 10000));
        else if (prop.num_params() == 1)
        {
          int dateformat = StringUtils::DateStringToYYYYMMDD(prop.param(0));
          if (dateformat <= 0) // not concrete date
            return AddMultiInfo(GUIInfo(SYSTEM_DATE, ConditionalStringParameter(prop.param(0), true), -1));
          else
            return AddMultiInfo(GUIInfo(SYSTEM_DATE, dateformat % 10000));
        }
        return SYSTEM_DATE;
      }
      else if (prop.name == "time")
      {
        if (prop.num_params() == 0)
          return AddMultiInfo(GUIInfo(SYSTEM_TIME, TIME_FORMAT_GUESS));
        if (prop.num_params() == 1)
        {
          TIME_FORMAT timeFormat = TranslateTimeFormat(prop.param(0));
          if (timeFormat == TIME_FORMAT_GUESS)
            return AddMultiInfo(GUIInfo(SYSTEM_TIME, StringUtils::TimeStringToSeconds(prop.param(0))));
          return AddMultiInfo(GUIInfo(SYSTEM_TIME, timeFormat));
        }
        else
          return AddMultiInfo(GUIInfo(SYSTEM_TIME, StringUtils::TimeStringToSeconds(prop.param(0)), StringUtils::TimeStringToSeconds(prop.param(1))));
      }
    }
    else if (cat.name == "library")
    {
      if (prop.name == "isscanning") return LIBRARY_IS_SCANNING;
      else if (prop.name == "isscanningvideo") return LIBRARY_IS_SCANNING_VIDEO; // TODO: change to IsScanning(Video)
      else if (prop.name == "isscanningmusic") return LIBRARY_IS_SCANNING_MUSIC;
      else if (prop.name == "hascontent" && prop.num_params())
      {
        std::string cat = prop.param(0);
        StringUtils::ToLower(cat);
        if (cat == "music") return LIBRARY_HAS_MUSIC;
        else if (cat == "video") return LIBRARY_HAS_VIDEO;
        else if (cat == "movies") return LIBRARY_HAS_MOVIES;
        else if (cat == "tvshows") return LIBRARY_HAS_TVSHOWS;
        else if (cat == "musicvideos") return LIBRARY_HAS_MUSICVIDEOS;
        else if (cat == "moviesets") return LIBRARY_HAS_MOVIE_SETS;
        else if (cat == "singles") return LIBRARY_HAS_SINGLES;
        else if (cat == "compilations") return LIBRARY_HAS_COMPILATIONS;
      }
    }
    else if (cat.name == "musicplayer")
    {
      for (size_t i = 0; i < sizeof(player_times) / sizeof(infomap); i++) // TODO: remove these, they're repeats
      {
        if (prop.name == player_times[i].str)
          return AddMultiInfo(GUIInfo(player_times[i].val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "content" && prop.num_params())
        return AddMultiInfo(GUIInfo(MUSICPLAYER_CONTENT, ConditionalStringParameter(prop.param()), 0));
      else if (prop.name == "property")
      {
        // properties are stored case sensitive in m_listItemProperties, but lookup is insensitive in CGUIListItem::GetProperty
        if (StringUtils::EqualsNoCase(prop.param(), "fanart_image"))
          return AddMultiInfo(GUIInfo(PLAYER_ITEM_ART, ConditionalStringParameter("fanart")));
        return AddListItemProp(prop.param(), MUSICPLAYER_PROPERTY_OFFSET);
      }
      return TranslateMusicPlayerString(prop.name);
    }
    else if (cat.name == "videoplayer")
    {
      for (size_t i = 0; i < sizeof(player_times) / sizeof(infomap); i++) // TODO: remove these, they're repeats
      {
        if (prop.name == player_times[i].str)
          return AddMultiInfo(GUIInfo(player_times[i].val, TranslateTimeFormat(prop.param())));
      }
      if (prop.name == "content" && prop.num_params())
        return AddMultiInfo(GUIInfo(VIDEOPLAYER_CONTENT, ConditionalStringParameter(prop.param()), 0));
      for (size_t i = 0; i < sizeof(videoplayer) / sizeof(infomap); i++)
      {
        if (prop.name == videoplayer[i].str)
          return videoplayer[i].val;
      }
    }
    else if (cat.name == "slideshow")
    {
      for (size_t i = 0; i < sizeof(slideshow) / sizeof(infomap); i++)
      {
        if (prop.name == slideshow[i].str)
          return slideshow[i].val;
      }
      return CPictureInfoTag::TranslateString(prop.name);
    }
    else if (cat.name == "container")
    {
      for (size_t i = 0; i < sizeof(mediacontainer) / sizeof(infomap); i++) // these ones don't have or need an id
      {
        if (prop.name == mediacontainer[i].str)
          return mediacontainer[i].val;
      }
      int id = atoi(cat.param().c_str());
      for (size_t i = 0; i < sizeof(container_bools) / sizeof(infomap); i++) // these ones can have an id (but don't need to?)
      {
        if (prop.name == container_bools[i].str)
          return id ? AddMultiInfo(GUIInfo(container_bools[i].val, id)) : container_bools[i].val;
      }
      for (size_t i = 0; i < sizeof(container_ints) / sizeof(infomap); i++) // these ones can have an int param on the property
      {
        if (prop.name == container_ints[i].str)
          return AddMultiInfo(GUIInfo(container_ints[i].val, id, atoi(prop.param().c_str())));
      }
      for (size_t i = 0; i < sizeof(container_str) / sizeof(infomap); i++) // these ones have a string param on the property
      {
        if (prop.name == container_str[i].str)
          return AddMultiInfo(GUIInfo(container_str[i].val, id, ConditionalStringParameter(prop.param())));
      }
      if (prop.name == "sortdirection")
      {
        SortOrder order = SortOrderNone;
        if (StringUtils::EqualsNoCase(prop.param(), "ascending"))
          order = SortOrderAscending;
        else if (StringUtils::EqualsNoCase(prop.param(), "descending"))
          order = SortOrderDescending;
        return AddMultiInfo(GUIInfo(CONTAINER_SORT_DIRECTION, order));
      }
      else if (prop.name == "sort")
      {
        if (StringUtils::EqualsNoCase(prop.param(), "songrating"))
          return AddMultiInfo(GUIInfo(CONTAINER_SORT_METHOD, SortByRating));
      }
    }
    else if (cat.name == "listitem")
    {
      int offset = atoi(cat.param().c_str());
      int ret = TranslateListItem(prop);
      if (ret)
        listItemDependent = true;
      if (offset)
        return AddMultiInfo(GUIInfo(ret, 0, offset, INFOFLAG_LISTITEM_WRAP));
      return ret;
    }
    else if (cat.name == "listitemposition")
    {
      int offset = atoi(cat.param().c_str());
      int ret = TranslateListItem(prop);
      if (ret)
        listItemDependent = true;
      if (offset)
        return AddMultiInfo(GUIInfo(ret, 0, offset, INFOFLAG_LISTITEM_POSITION));
      return ret;
    }
    else if (cat.name == "listitemnowrap")
    {
      int offset = atoi(cat.param().c_str());
      int ret = TranslateListItem(prop);
      if (ret)
        listItemDependent = true;
      if (offset)
        return AddMultiInfo(GUIInfo(ret, 0, offset));
      return ret;
    }
    else if (cat.name == "visualisation")
    {
      for (size_t i = 0; i < sizeof(visualisation) / sizeof(infomap); i++)
      {
        if (prop.name == visualisation[i].str)
          return visualisation[i].val;
      }
    }
    else if (cat.name == "fanart")
    {
      for (size_t i = 0; i < sizeof(fanart_labels) / sizeof(infomap); i++)
      {
        if (prop.name == fanart_labels[i].str)
          return fanart_labels[i].val;
      }
    }
    else if (cat.name == "skin")
    {
      for (size_t i = 0; i < sizeof(skin_labels) / sizeof(infomap); i++)
      {
        if (prop.name == skin_labels[i].str)
          return skin_labels[i].val;
      }
      if (prop.num_params())
      {
        if (prop.name == "string")
        {
          if (prop.num_params() == 2)
            return AddMultiInfo(GUIInfo(SKIN_STRING, CSkinSettings::Get().TranslateString(prop.param(0)), ConditionalStringParameter(prop.param(1))));
          else
            return AddMultiInfo(GUIInfo(SKIN_STRING, CSkinSettings::Get().TranslateString(prop.param(0))));
        }
        if (prop.name == "hassetting")
          return AddMultiInfo(GUIInfo(SKIN_BOOL, CSkinSettings::Get().TranslateBool(prop.param(0))));
        else if (prop.name == "hastheme")
          return AddMultiInfo(GUIInfo(SKIN_HAS_THEME, ConditionalStringParameter(prop.param(0))));
      }
    }
    else if (cat.name == "window")
    {
      if (prop.name == "property" && prop.num_params() == 1)
      { // TODO: this doesn't support foo.xml
        int winID = cat.param().empty() ? 0 : CButtonTranslator::TranslateWindow(cat.param());
        if (winID != WINDOW_INVALID)
          return AddMultiInfo(GUIInfo(WINDOW_PROPERTY, winID, ConditionalStringParameter(prop.param())));
      }
      for (size_t i = 0; i < sizeof(window_bools) / sizeof(infomap); i++)
      {
        if (prop.name == window_bools[i].str)
        { // TODO: The parameter for these should really be on the first not the second property
          if (prop.param().find("xml") != std::string::npos)
            return AddMultiInfo(GUIInfo(window_bools[i].val, 0, ConditionalStringParameter(prop.param())));
          int winID = prop.param().empty() ? 0 : CButtonTranslator::TranslateWindow(prop.param());
          if (winID != WINDOW_INVALID)
            return AddMultiInfo(GUIInfo(window_bools[i].val, winID, 0));
          return 0;
        }
      }
    }
    else if (cat.name == "control")
    {
      for (size_t i = 0; i < sizeof(control_labels) / sizeof(infomap); i++)
      {
        if (prop.name == control_labels[i].str)
        { // TODO: The parameter for these should really be on the first not the second property
          int controlID = atoi(prop.param().c_str());
          if (controlID)
            return AddMultiInfo(GUIInfo(control_labels[i].val, controlID, 0));
          return 0;
        }
      }
    }
    else if (cat.name == "controlgroup" && prop.name == "hasfocus")
    {
      int groupID = atoi(cat.param().c_str());
      if (groupID)
        return AddMultiInfo(GUIInfo(CONTROL_GROUP_HAS_FOCUS, groupID, atoi(prop.param(0).c_str())));
    }
    else if (cat.name == "playlist")
    {
      int ret = -1;
      for (size_t i = 0; i < sizeof(playlist) / sizeof(infomap); i++)
      {
        if (prop.name == playlist[i].str)
        {
          ret = playlist[i].val;
          break;
        }
      }
      if (ret >= 0)
      {
        if (prop.num_params() <= 0)
          return ret;
        else
        {
          int playlistid = PLAYLIST_NONE;
          if (StringUtils::EqualsNoCase(prop.param(), "video"))
            playlistid = PLAYLIST_VIDEO;
          else if (StringUtils::EqualsNoCase(prop.param(), "music"))
            playlistid = PLAYLIST_MUSIC;

          if (playlistid > PLAYLIST_NONE)
            return AddMultiInfo(GUIInfo(ret, playlistid));
        }
      }
    }
    else if (cat.name == "pvr")
    {
      for (size_t i = 0; i < sizeof(pvr) / sizeof(infomap); i++)
      {
        if (prop.name == pvr[i].str)
          return pvr[i].val;
      }
    }
  }
  else if (info.size() == 3 || info.size() == 4)
  {
    if (info[0].name == "system" && info[1].name == "platform")
    { // TODO: replace with a single system.platform
      std::string platform = info[2].name;
      if (platform == "linux")
      {
        if (info.size() == 4)
        {
          std::string device = info[3].name;
          if (device == "raspberrypi") return SYSTEM_PLATFORM_LINUX_RASPBERRY_PI;
        }
        else return SYSTEM_PLATFORM_LINUX;
      }
      else if (platform == "windows") return SYSTEM_PLATFORM_WINDOWS;
      else if (platform == "darwin")  return SYSTEM_PLATFORM_DARWIN;
      else if (platform == "osx")  return SYSTEM_PLATFORM_DARWIN_OSX;
      else if (platform == "ios")  return SYSTEM_PLATFORM_DARWIN_IOS;
      else if (platform == "atv2") return SYSTEM_PLATFORM_DARWIN_ATV2;
      else if (platform == "android") return SYSTEM_PLATFORM_ANDROID;
    }
    if (info[0].name == "musicplayer")
    { // TODO: these two don't allow duration(foo) and also don't allow more than this number of levels...
      if (info[1].name == "position")
      {
        int position = atoi(info[1].param().c_str());
        int value = TranslateMusicPlayerString(info[2].name); // musicplayer.position(foo).bar
        return AddMultiInfo(GUIInfo(value, 0, position));
      }
      else if (info[1].name == "offset")
      {
        int position = atoi(info[1].param().c_str());
        int value = TranslateMusicPlayerString(info[2].name); // musicplayer.offset(foo).bar
        return AddMultiInfo(GUIInfo(value, 1, position));
      }
    }
    else if (info[0].name == "container")
    {
      int id = atoi(info[0].param().c_str());
      int offset = atoi(info[1].param().c_str());
      if (info[1].name == "listitemnowrap")
      {
        listItemDependent = true;
        return AddMultiInfo(GUIInfo(TranslateListItem(info[2]), id, offset));
      }
      else if (info[1].name == "listitemposition")
      {
        listItemDependent = true;
        return AddMultiInfo(GUIInfo(TranslateListItem(info[2]), id, offset, INFOFLAG_LISTITEM_POSITION));
      }
      else if (info[1].name == "listitem")
      {
        listItemDependent = true;
        return AddMultiInfo(GUIInfo(TranslateListItem(info[2]), id, offset, INFOFLAG_LISTITEM_WRAP));
      }
    }
    else if (info[0].name == "control")
    {
      const Property &prop = info[1];
      for (size_t i = 0; i < sizeof(control_labels) / sizeof(infomap); i++)
      {
        if (prop.name == control_labels[i].str)
        { // TODO: The parameter for these should really be on the first not the second property
          int controlID = atoi(prop.param().c_str());
          if (controlID)
            return AddMultiInfo(GUIInfo(control_labels[i].val, controlID, atoi(info[2].param(0).c_str())));
          return 0;
        }
      }
    }
  }

  return 0;
}

int CGUIInfoManager::TranslateListItem(const Property &info)
{
  for (size_t i = 0; i < sizeof(listitem_labels) / sizeof(infomap); i++) // these ones don't have or need an id
  {
    if (info.name == listitem_labels[i].str)
      return listitem_labels[i].val;
  }
  if (info.name == "property" && info.num_params() == 1)
  {
    // properties are stored case sensitive in m_listItemProperties, but lookup is insensitive in CGUIListItem::GetProperty
    if (StringUtils::EqualsNoCase(info.param(), "fanart_image"))
      return AddListItemProp("fanart", LISTITEM_ART_OFFSET);
    return AddListItemProp(info.param());
  }
  if (info.name == "art" && info.num_params() == 1)
    return AddListItemProp(info.param(), LISTITEM_ART_OFFSET);
  return 0;
}

int CGUIInfoManager::TranslateMusicPlayerString(const std::string &info) const
{
  for (size_t i = 0; i < sizeof(musicplayer) / sizeof(infomap); i++)
  {
    if (info == musicplayer[i].str)
      return musicplayer[i].val;
  }
  return 0;
}

TIME_FORMAT CGUIInfoManager::TranslateTimeFormat(const std::string &format)
{
  if (format.empty()) return TIME_FORMAT_GUESS;
  else if (StringUtils::EqualsNoCase(format, "hh")) return TIME_FORMAT_HH;
  else if (StringUtils::EqualsNoCase(format, "mm")) return TIME_FORMAT_MM;
  else if (StringUtils::EqualsNoCase(format, "ss")) return TIME_FORMAT_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm")) return TIME_FORMAT_HH_MM;
  else if (StringUtils::EqualsNoCase(format, "mm:ss")) return TIME_FORMAT_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm:ss")) return TIME_FORMAT_HH_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "hh:mm:ss xx")) return TIME_FORMAT_HH_MM_SS_XX;
  else if (StringUtils::EqualsNoCase(format, "h")) return TIME_FORMAT_H;
  else if (StringUtils::EqualsNoCase(format, "h:mm:ss")) return TIME_FORMAT_H_MM_SS;
  else if (StringUtils::EqualsNoCase(format, "h:mm:ss xx")) return TIME_FORMAT_H_MM_SS_XX;
  else if (StringUtils::EqualsNoCase(format, "xx")) return TIME_FORMAT_XX;
  return TIME_FORMAT_GUESS;
}

std::string CGUIInfoManager::GetLabel(int info, int contextWindow, std::string *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, false);

  std::string strLabel;
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow);

  if (info >= SLIDE_INFO_START && info <= SLIDE_INFO_END)
    return GetPictureLabel(info);

  if (info >= LISTITEM_PROPERTY_START+MUSICPLAYER_PROPERTY_OFFSET &&
      info - (LISTITEM_PROPERTY_START+MUSICPLAYER_PROPERTY_OFFSET) < (int)m_listitemProperties.size())
  { // grab the property
    if (!m_currentFile)
      return "";

    std::string property = m_listitemProperties[info - LISTITEM_PROPERTY_START-MUSICPLAYER_PROPERTY_OFFSET];
    return m_currentFile->GetProperty(property).asString();
  }

  if (info >= LISTITEM_START && info <= LISTITEM_END)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS); // true for has list items
    if (window)
    {
      CFileItemPtr item = window->GetCurrentListItem();
      strLabel = GetItemLabel(item.get(), info, fallback);
    }

    return strLabel;
  }

  switch (info)
  {
  case PVR_NEXT_RECORDING_CHANNEL:
  case PVR_NEXT_RECORDING_CHAN_ICO:
  case PVR_NEXT_RECORDING_DATETIME:
  case PVR_NEXT_RECORDING_TITLE:
  case PVR_NOW_RECORDING_CHANNEL:
  case PVR_NOW_RECORDING_CHAN_ICO:
  case PVR_NOW_RECORDING_DATETIME:
  case PVR_NOW_RECORDING_TITLE:
  case PVR_BACKEND_NAME:
  case PVR_BACKEND_VERSION:
  case PVR_BACKEND_HOST:
  case PVR_BACKEND_DISKSPACE:
  case PVR_BACKEND_CHANNELS:
  case PVR_BACKEND_TIMERS:
  case PVR_BACKEND_RECORDINGS:
  case PVR_BACKEND_DELETED_RECORDINGS:
  case PVR_BACKEND_NUMBER:
  case PVR_TOTAL_DISKSPACE:
  case PVR_NEXT_TIMER:
  case PVR_PLAYING_DURATION:
  case PVR_PLAYING_TIME:
  case PVR_PLAYING_PROGRESS:
  case PVR_ACTUAL_STREAM_CLIENT:
  case PVR_ACTUAL_STREAM_DEVICE:
  case PVR_ACTUAL_STREAM_STATUS:
  case PVR_ACTUAL_STREAM_SIG:
  case PVR_ACTUAL_STREAM_SNR:
  case PVR_ACTUAL_STREAM_SIG_PROGR:
  case PVR_ACTUAL_STREAM_SNR_PROGR:
  case PVR_ACTUAL_STREAM_BER:
  case PVR_ACTUAL_STREAM_UNC:
  case PVR_ACTUAL_STREAM_VIDEO_BR:
  case PVR_ACTUAL_STREAM_AUDIO_BR:
  case PVR_ACTUAL_STREAM_DOLBY_BR:
  case PVR_ACTUAL_STREAM_CRYPTION:
  case PVR_ACTUAL_STREAM_SERVICE:
  case PVR_ACTUAL_STREAM_MUX:
  case PVR_ACTUAL_STREAM_PROVIDER:
    g_PVRManager.TranslateCharInfo(info, strLabel);
    break;
  case WEATHER_CONDITIONS:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_COND);
    StringUtils::Trim(strLabel);
    break;
  case WEATHER_TEMPERATURE:
    strLabel = StringUtils::Format("%s%s",
                                   g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_TEMP).c_str(),
                                   g_langInfo.GetTemperatureUnitString().c_str());
    break;
  case WEATHER_LOCATION:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_LOCATION);
    break;
  case WEATHER_FANART_CODE:
    strLabel = URIUtils::GetFileName(g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));
    URIUtils::RemoveExtension(strLabel);
    break;
  case WEATHER_PLUGIN:
    strLabel = CSettings::Get().GetString("weather.addon");
    break;
  case SYSTEM_DATE:
    strLabel = GetDate();
    break;
  case SYSTEM_FPS:
    strLabel = StringUtils::Format("%02.2f", m_fps);
    break;
  case PLAYER_VOLUME:
    strLabel = StringUtils::Format("%2.1f dB", CAEUtil::PercentToGain(g_application.GetVolume(false)));
    break;
  case PLAYER_SUBTITLE_DELAY:
    strLabel = StringUtils::Format("%2.3f s", CMediaSettings::Get().GetCurrentVideoSettings().m_SubtitleDelay);
    break;
  case PLAYER_AUDIO_DELAY:
    strLabel = StringUtils::Format("%2.3f s", CMediaSettings::Get().GetCurrentVideoSettings().m_AudioDelay);
    break;
  case PLAYER_CHAPTER:
    if(g_application.m_pPlayer->IsPlaying())
      strLabel = StringUtils::Format("%02d", g_application.m_pPlayer->GetChapter());
    break;
  case PLAYER_CHAPTERCOUNT:
    if(g_application.m_pPlayer->IsPlaying())
      strLabel = StringUtils::Format("%02d", g_application.m_pPlayer->GetChapterCount());
    break;
  case PLAYER_CHAPTERNAME:
    if(g_application.m_pPlayer->IsPlaying())
      g_application.m_pPlayer->GetChapterName(strLabel);
    break;
  case PLAYER_CACHELEVEL:
    {
      int iLevel = 0;
      if(g_application.m_pPlayer->IsPlaying() && GetInt(iLevel, PLAYER_CACHELEVEL) && iLevel >= 0)
        strLabel = StringUtils::Format("%i", iLevel);
    }
    break;
  case PLAYER_TIME:
    if(g_application.m_pPlayer->IsPlaying())
      strLabel = GetCurrentPlayTime(TIME_FORMAT_HH_MM);
    break;
  case PLAYER_DURATION:
    if(g_application.m_pPlayer->IsPlaying())
      strLabel = GetDuration(TIME_FORMAT_HH_MM);
    break;
  case PLAYER_PATH:
  case PLAYER_FILENAME:
  case PLAYER_FILEPATH:
    if (m_currentFile)
    {
      if (m_currentFile->HasMusicInfoTag())
        strLabel = m_currentFile->GetMusicInfoTag()->GetURL();
      else if (m_currentFile->HasVideoInfoTag())
        strLabel = m_currentFile->GetVideoInfoTag()->m_strFileNameAndPath;
      if (strLabel.empty())
        strLabel = m_currentFile->GetPath();
    }
    if (info == PLAYER_PATH)
    {
      // do this twice since we want the path outside the archive if this
      // is to be of use.
      if (URIUtils::IsInArchive(strLabel))
        strLabel = URIUtils::GetParentPath(strLabel);
      strLabel = URIUtils::GetParentPath(strLabel);
    }
    else if (info == PLAYER_FILENAME)
      strLabel = URIUtils::GetFileName(strLabel);
    break;
  case PLAYER_TITLE:
    {
      if(m_currentFile)
      {
        if (m_currentFile->HasPVRChannelInfoTag())
        {
          CEpgInfoTagPtr tag(m_currentFile->GetPVRChannelInfoTag()->GetEPGNow());
          return tag ?
                   tag->Title() :
                   CSettings::Get().GetBool("epg.hidenoinfoavailable") ?
                            "" : g_localizeStrings.Get(19055); // no information available
        }
        if (m_currentFile->HasPVRRecordingInfoTag() && !m_currentFile->GetPVRRecordingInfoTag()->m_strTitle.empty())
          return m_currentFile->GetPVRRecordingInfoTag()->m_strTitle;
        if (m_currentFile->HasVideoInfoTag() && !m_currentFile->GetVideoInfoTag()->m_strTitle.empty())
          return m_currentFile->GetVideoInfoTag()->m_strTitle;
        if (m_currentFile->HasMusicInfoTag() && !m_currentFile->GetMusicInfoTag()->GetTitle().empty())
          return m_currentFile->GetMusicInfoTag()->GetTitle();
        // don't have the title, so use dvdplayer, label, or drop down to title from path
        if (!g_application.m_pPlayer->GetPlayingTitle().empty())
          return g_application.m_pPlayer->GetPlayingTitle();
        if (!m_currentFile->GetLabel().empty())
          return m_currentFile->GetLabel();
        return CUtil::GetTitleFromPath(m_currentFile->GetPath());
      }
      else
      {
        if (!g_application.m_pPlayer->GetPlayingTitle().empty())
          return g_application.m_pPlayer->GetPlayingTitle();
      }
    }
    break;
  case MUSICPLAYER_TITLE:
  case MUSICPLAYER_ALBUM:
  case MUSICPLAYER_ARTIST:
  case MUSICPLAYER_ALBUM_ARTIST:
  case MUSICPLAYER_GENRE:
  case MUSICPLAYER_YEAR:
  case MUSICPLAYER_TRACK_NUMBER:
  case MUSICPLAYER_BITRATE:
  case MUSICPLAYER_PLAYLISTLEN:
  case MUSICPLAYER_PLAYLISTPOS:
  case MUSICPLAYER_CHANNELS:
  case MUSICPLAYER_BITSPERSAMPLE:
  case MUSICPLAYER_SAMPLERATE:
  case MUSICPLAYER_CODEC:
  case MUSICPLAYER_DISC_NUMBER:
  case MUSICPLAYER_RATING:
  case MUSICPLAYER_COMMENT:
  case MUSICPLAYER_LYRICS:
  case MUSICPLAYER_CHANNEL_NAME:
  case MUSICPLAYER_CHANNEL_NUMBER:
  case MUSICPLAYER_SUB_CHANNEL_NUMBER:
  case MUSICPLAYER_CHANNEL_NUMBER_LBL:
  case MUSICPLAYER_CHANNEL_GROUP:
  case MUSICPLAYER_PLAYCOUNT:
  case MUSICPLAYER_LASTPLAYED:
    strLabel = GetMusicLabel(info);
  break;
  case VIDEOPLAYER_TITLE:
  case VIDEOPLAYER_ORIGINALTITLE:
  case VIDEOPLAYER_GENRE:
  case VIDEOPLAYER_DIRECTOR:
  case VIDEOPLAYER_YEAR:
  case VIDEOPLAYER_PLAYLISTLEN:
  case VIDEOPLAYER_PLAYLISTPOS:
  case VIDEOPLAYER_PLOT:
  case VIDEOPLAYER_PLOT_OUTLINE:
  case VIDEOPLAYER_EPISODE:
  case VIDEOPLAYER_SEASON:
  case VIDEOPLAYER_RATING:
  case VIDEOPLAYER_RATING_AND_VOTES:
  case VIDEOPLAYER_TVSHOW:
  case VIDEOPLAYER_PREMIERED:
  case VIDEOPLAYER_STUDIO:
  case VIDEOPLAYER_COUNTRY:
  case VIDEOPLAYER_MPAA:
  case VIDEOPLAYER_TOP250:
  case VIDEOPLAYER_CAST:
  case VIDEOPLAYER_CAST_AND_ROLE:
  case VIDEOPLAYER_ARTIST:
  case VIDEOPLAYER_ALBUM:
  case VIDEOPLAYER_WRITER:
  case VIDEOPLAYER_TAGLINE:
  case VIDEOPLAYER_TRAILER:
  case VIDEOPLAYER_STARTTIME:
  case VIDEOPLAYER_ENDTIME:
  case VIDEOPLAYER_NEXT_TITLE:
  case VIDEOPLAYER_NEXT_GENRE:
  case VIDEOPLAYER_NEXT_PLOT:
  case VIDEOPLAYER_NEXT_PLOT_OUTLINE:
  case VIDEOPLAYER_NEXT_STARTTIME:
  case VIDEOPLAYER_NEXT_ENDTIME:
  case VIDEOPLAYER_NEXT_DURATION:
  case VIDEOPLAYER_CHANNEL_NAME:
  case VIDEOPLAYER_CHANNEL_NUMBER:
  case VIDEOPLAYER_SUB_CHANNEL_NUMBER:
  case VIDEOPLAYER_CHANNEL_NUMBER_LBL:
  case VIDEOPLAYER_CHANNEL_GROUP:
  case VIDEOPLAYER_PARENTAL_RATING:
  case VIDEOPLAYER_PLAYCOUNT:
  case VIDEOPLAYER_LASTPLAYED:
  case VIDEOPLAYER_IMDBNUMBER:
  case VIDEOPLAYER_EPISODENAME:
    strLabel = GetVideoLabel(info);
  break;
  case VIDEOPLAYER_VIDEO_CODEC:
    if(g_application.m_pPlayer->IsPlaying())
    {
      strLabel = m_videoInfo.videoCodecName;
    }
    break;
  case VIDEOPLAYER_VIDEO_RESOLUTION:
    if(g_application.m_pPlayer->IsPlaying())
    {
      return CStreamDetails::VideoDimsToResolutionDescription(m_videoInfo.width, m_videoInfo.height);
    }
    break;
  case VIDEOPLAYER_AUDIO_CODEC:
    if(g_application.m_pPlayer->IsPlaying())
    {
      strLabel = m_audioInfo.audioCodecName;
    }
    break;
  case VIDEOPLAYER_VIDEO_ASPECT:
    if (g_application.m_pPlayer->IsPlaying())
    {
      strLabel = CStreamDetails::VideoAspectToAspectDescription(m_videoInfo.videoAspectRatio);
    }
    break;
  case VIDEOPLAYER_AUDIO_CHANNELS:
    if(g_application.m_pPlayer->IsPlaying())
    {
      strLabel = StringUtils::Format("%i", m_audioInfo.channels);
    }
    break;
  case VIDEOPLAYER_AUDIO_LANG:
    if(g_application.m_pPlayer->IsPlaying())
    {
      SPlayerAudioStreamInfo info;
      g_application.m_pPlayer->GetAudioStreamInfo(CURRENT_STREAM, info);
      strLabel = info.language;
    }
    break;
  case VIDEOPLAYER_STEREOSCOPIC_MODE:
    if(g_application.m_pPlayer->IsPlaying())
    {
      strLabel = m_videoInfo.stereoMode;
    }
    break;
  case VIDEOPLAYER_SUBTITLES_LANG:
    if(g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying() && g_application.m_pPlayer->GetSubtitleVisible())
    {
      SPlayerSubtitleStreamInfo info;
      g_application.m_pPlayer->GetSubtitleStreamInfo(g_application.m_pPlayer->GetSubtitle(), info);
      strLabel = info.language;
    }
    break;
  case PLAYLIST_LENGTH:
  case PLAYLIST_POSITION:
  case PLAYLIST_RANDOM:
  case PLAYLIST_REPEAT:
    strLabel = GetPlaylistLabel(info);
  break;
  case MUSICPM_SONGSPLAYED:
  case MUSICPM_MATCHINGSONGS:
  case MUSICPM_MATCHINGSONGSPICKED:
  case MUSICPM_MATCHINGSONGSLEFT:
  case MUSICPM_RELAXEDSONGSPICKED:
  case MUSICPM_RANDOMSONGSPICKED:
    strLabel = GetMusicPartyModeLabel(info);
  break;

  case SYSTEM_FREE_SPACE:
  case SYSTEM_USED_SPACE:
  case SYSTEM_TOTAL_SPACE:
  case SYSTEM_FREE_SPACE_PERCENT:
  case SYSTEM_USED_SPACE_PERCENT:
    return g_sysinfo.GetHddSpaceInfo(info);
  break;

  case SYSTEM_CPU_TEMPERATURE:
  case SYSTEM_GPU_TEMPERATURE:
  case SYSTEM_FAN_SPEED:
  case SYSTEM_CPU_USAGE:
    return GetSystemHeatInfo(info);
    break;

  case SYSTEM_VIDEO_ENCODER_INFO:
  case NETWORK_MAC_ADDRESS:
  case SYSTEM_OS_VERSION_INFO:
  case SYSTEM_CPUFREQUENCY:
  case SYSTEM_INTERNET_STATE:
  case SYSTEM_UPTIME:
  case SYSTEM_TOTALUPTIME:
  case SYSTEM_BATTERY_LEVEL:
    return g_sysinfo.GetInfo(info);
    break;

  case SYSTEM_SCREEN_RESOLUTION:
    if(g_Windowing.IsFullScreen())
      strLabel = StringUtils::Format("%ix%i@%.2fHz - %s (%02.2f fps)",
        CDisplaySettings::Get().GetCurrentResolutionInfo().iScreenWidth,
        CDisplaySettings::Get().GetCurrentResolutionInfo().iScreenHeight,
        CDisplaySettings::Get().GetCurrentResolutionInfo().fRefreshRate,
        g_localizeStrings.Get(244).c_str(),
        GetFPS());
    else
      strLabel = StringUtils::Format("%ix%i - %s (%02.2f fps)",
        CDisplaySettings::Get().GetCurrentResolutionInfo().iScreenWidth,
        CDisplaySettings::Get().GetCurrentResolutionInfo().iScreenHeight,
        g_localizeStrings.Get(242).c_str(),
        GetFPS());
    return strLabel;
    break;

  case CONTAINER_FOLDERPATH:
  case CONTAINER_FOLDERNAME:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        if (info==CONTAINER_FOLDERNAME)
          strLabel = ((CGUIMediaWindow*)window)->CurrentDirectory().GetLabel();
        else
          strLabel = CURL(((CGUIMediaWindow*)window)->CurrentDirectory().GetPath()).GetWithoutUserDetails();
      }
      break;
    }
  case CONTAINER_PLUGINNAME:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        CURL url(((CGUIMediaWindow*)window)->CurrentDirectory().GetPath());
        if (url.IsProtocol("plugin"))
          strLabel = URIUtils::GetFileName(url.GetHostName());
      }
      break;
    }
  case CONTAINER_VIEWMODE:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CGUIControl *control = window->GetControl(window->GetViewContainerID());
        if (control && control->IsContainer())
          strLabel = ((IGUIContainer *)control)->GetLabel();
      }
      break;
    }
  case CONTAINER_SORT_METHOD:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CGUIViewState *viewState = ((CGUIMediaWindow*)window)->GetViewState();
        if (viewState)
          strLabel = g_localizeStrings.Get(viewState->GetSortMethodLabel());
    }
    }
    break;
  case CONTAINER_NUM_PAGES:
  case CONTAINER_NUM_ITEMS:
  case CONTAINER_CURRENT_ITEM:
  case CONTAINER_CURRENT_PAGE:
    return GetMultiInfoLabel(GUIInfo(info), contextWindow);
    break;
  case CONTAINER_SHOWPLOT:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("showplot").asString();
    }
    break;
  case CONTAINER_TOTALTIME:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        const CFileItemList& items=((CGUIMediaWindow *)window)->CurrentDirectory();
        int duration=0;
        for (int i=0;i<items.Size();++i)
        {
          CFileItemPtr item=items.Get(i);
          if (item->HasMusicInfoTag())
            duration += item->GetMusicInfoTag()->GetDuration();
          else if (item->HasVideoInfoTag())
            duration += item->GetVideoInfoTag()->m_streamDetails.GetVideoDuration();
        }
        if (duration > 0)
          return StringUtils::SecondsToTimeString(duration);
      }
    }
    break;
  case SYSTEM_BUILD_VERSION_SHORT:
    strLabel = GetVersionShort();
    break;
  case SYSTEM_BUILD_VERSION:
    strLabel = GetVersion();
    break;
  case SYSTEM_BUILD_DATE:
    strLabel = GetBuild();
    break;
  case SYSTEM_FREE_MEMORY:
  case SYSTEM_FREE_MEMORY_PERCENT:
  case SYSTEM_USED_MEMORY:
  case SYSTEM_USED_MEMORY_PERCENT:
  case SYSTEM_TOTAL_MEMORY:
    {
      MEMORYSTATUSEX stat;
      stat.dwLength = sizeof(MEMORYSTATUSEX);
      GlobalMemoryStatusEx(&stat);
      int iMemPercentFree = 100 - ((int)( 100.0f* (stat.ullTotalPhys - stat.ullAvailPhys)/stat.ullTotalPhys + 0.5f ));
      int iMemPercentUsed = 100 - iMemPercentFree;

      if (info == SYSTEM_FREE_MEMORY)
        strLabel = StringUtils::Format("%luMB", (ULONG)(stat.ullAvailPhys/MB));
      else if (info == SYSTEM_FREE_MEMORY_PERCENT)
        strLabel = StringUtils::Format("%i%%", iMemPercentFree);
      else if (info == SYSTEM_USED_MEMORY)
        strLabel = StringUtils::Format("%luMB", (ULONG)((stat.ullTotalPhys - stat.ullAvailPhys)/MB));
      else if (info == SYSTEM_USED_MEMORY_PERCENT)
        strLabel = StringUtils::Format("%i%%", iMemPercentUsed);
      else if (info == SYSTEM_TOTAL_MEMORY)
        strLabel = StringUtils::Format("%luMB", (ULONG)(stat.ullTotalPhys/MB));
    }
    break;
  case SYSTEM_SCREEN_MODE:
    strLabel = g_graphicsContext.GetResInfo().strMode;
    break;
  case SYSTEM_SCREEN_WIDTH:
    strLabel = StringUtils::Format("%i", g_graphicsContext.GetResInfo().iScreenWidth);
    break;
  case SYSTEM_SCREEN_HEIGHT:
    strLabel = StringUtils::Format("%i", g_graphicsContext.GetResInfo().iScreenHeight);
    break;
  case SYSTEM_CURRENT_WINDOW:
    return g_localizeStrings.Get(g_windowManager.GetFocusedWindow());
    break;
  case SYSTEM_STARTUP_WINDOW:
    strLabel = StringUtils::Format("%i", CSettings::Get().GetInt("lookandfeel.startupwindow"));
    break;
  case SYSTEM_CURRENT_CONTROL:
    {
      CGUIWindow *window = g_windowManager.GetWindow(g_windowManager.GetFocusedWindow());
      if (window)
      {
        CGUIControl *control = window->GetFocusedControl();
        if (control)
          strLabel = control->GetDescription();
      }
    }
    break;
#ifdef HAS_DVD_DRIVE
  case SYSTEM_DVD_LABEL:
    strLabel = g_mediaManager.GetDiskLabel();
    break;
#endif
  case SYSTEM_ALARM_POS:
    if (g_alarmClock.GetRemaining("shutdowntimer") == 0.f)
      strLabel = "";
    else
    {
      double fTime = g_alarmClock.GetRemaining("shutdowntimer");
      if (fTime > 60.f)
        strLabel = StringUtils::Format(g_localizeStrings.Get(13213).c_str(), g_alarmClock.GetRemaining("shutdowntimer")/60.f);
      else
        strLabel = StringUtils::Format(g_localizeStrings.Get(13214).c_str(), g_alarmClock.GetRemaining("shutdowntimer"));
    }
    break;
  case SYSTEM_PROFILENAME:
    strLabel = CProfilesManager::Get().GetCurrentProfile().getName();
    break;
  case SYSTEM_PROFILECOUNT:
    strLabel = StringUtils::Format("%" PRIuS, CProfilesManager::Get().GetNumberOfProfiles());
    break;
  case SYSTEM_PROFILEAUTOLOGIN:
    {
      int profileId = CProfilesManager::Get().GetAutoLoginProfileId();
      if ((profileId < 0) || (!CProfilesManager::Get().GetProfileName(profileId, strLabel)))
        strLabel = g_localizeStrings.Get(37014); // Last used profile
    }
    break;
  case SYSTEM_LANGUAGE:
    strLabel = g_langInfo.GetEnglishLanguageName();
    break;
  case SYSTEM_TEMPERATURE_UNITS:
    strLabel = g_langInfo.GetTemperatureUnitString();
    break;
  case SYSTEM_PROGRESS_BAR:
    {
      int percent;
      if (GetInt(percent, SYSTEM_PROGRESS_BAR) && percent > 0)
        strLabel = StringUtils::Format("%i", percent);
    }
    break;
  case SYSTEM_FRIENDLY_NAME:
    {
      std::string friendlyName = CSettings::Get().GetString("services.devicename");
      if (StringUtils::EqualsNoCase(friendlyName, CCompileInfo::GetAppName()))
      {
        std::string hostname("[unknown]");
        g_application.getNetwork().GetHostName(hostname);
        strLabel = StringUtils::Format("%s (%s)", friendlyName.c_str(), hostname.c_str());
      }
      else
        strLabel = friendlyName;
    }
    break;
  case SYSTEM_STEREOSCOPIC_MODE:
    {
      int stereoMode = CSettings::Get().GetInt("videoscreen.stereoscopicmode");
      strLabel = StringUtils::Format("%i", stereoMode);
    }
    break;

  case SKIN_THEME:
    strLabel = CSettings::Get().GetString("lookandfeel.skintheme");
    break;
  case SKIN_COLOUR_THEME:
    strLabel = CSettings::Get().GetString("lookandfeel.skincolors");
    break;
  case SKIN_ASPECT_RATIO:
    if (g_SkinInfo)
      strLabel = g_SkinInfo->GetCurrentAspect();
    break;
  case NETWORK_IP_ADDRESS:
    {
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface)
        return iface->GetCurrentIPAddress();
    }
    break;
  case NETWORK_SUBNET_MASK:
    {
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface)
        return iface->GetCurrentNetmask();
    }
    break;
  case NETWORK_GATEWAY_ADDRESS:
    {
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface)
        return iface->GetCurrentDefaultGateway();
    }
    break;
  case NETWORK_DNS1_ADDRESS:
    {
      vector<std::string> nss = g_application.getNetwork().GetNameServers();
      if (nss.size() >= 1)
        return nss[0];
    }
    break;
  case NETWORK_DNS2_ADDRESS:
    {
      vector<std::string> nss = g_application.getNetwork().GetNameServers();
      if (nss.size() >= 2)
        return nss[1];
    }
    break;
  case NETWORK_DHCP_ADDRESS:
    {
      std::string dhcpserver;
      return dhcpserver;
    }
    break;
  case NETWORK_LINK_STATE:
    {
      std::string linkStatus = g_localizeStrings.Get(151);
      linkStatus += " ";
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface && iface->IsConnected())
        linkStatus += g_localizeStrings.Get(15207);
      else
        linkStatus += g_localizeStrings.Get(15208);
      return linkStatus;
    }
    break;

  case VISUALISATION_PRESET:
    {
      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      g_windowManager.SendMessage(msg);
      if (msg.GetPointer())
      {
        CVisualisation* viz = NULL;
        viz = (CVisualisation*)msg.GetPointer();
        if (viz)
        {
          strLabel = viz->GetPresetName();
          URIUtils::RemoveExtension(strLabel);
        }
      }
    }
    break;
  case VISUALISATION_NAME:
    {
      AddonPtr addon;
      strLabel = CSettings::Get().GetString("musicplayer.visualisation");
      if (CAddonMgr::Get().GetAddon(strLabel,addon) && addon)
        strLabel = addon->Name();
    }
    break;
  case FANART_COLOR1:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color1").asString();
    }
    break;
  case FANART_COLOR2:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color2").asString();
    }
    break;
  case FANART_COLOR3:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color3").asString();
    }
    break;
  case FANART_IMAGE:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetArt("fanart");
    }
    break;
  case SYSTEM_RENDER_VENDOR:
    strLabel = g_Windowing.GetRenderVendor();
    break;
  case SYSTEM_RENDER_RENDERER:
    strLabel = g_Windowing.GetRenderRenderer();
    break;
  case SYSTEM_RENDER_VERSION:
    strLabel = g_Windowing.GetRenderVersionString();
    break;
  }

  return strLabel;
}

// tries to get a integer value for use in progressbars/sliders and such
bool CGUIInfoManager::GetInt(int &value, int info, int contextWindow, const CGUIListItem *item /* = NULL */) const
{
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
    return GetMultiInfoInt(value, m_multiInfo[info - MULTI_INFO_START], contextWindow);

  if (info >= LISTITEM_START && info <= LISTITEM_END)
  {
    if (item == NULL)
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS); // true for has list items
      if (window)
        item = window->GetCurrentListItem().get();
    }

    return GetItemInt(value, item, info);
  }

  value = 0;
  switch( info )
  {
    case PLAYER_VOLUME:
      value = (int)g_application.GetVolume();
      return true;
    case PLAYER_SUBTITLE_DELAY:
      value = g_application.GetSubtitleDelay();
      return true;
    case PLAYER_AUDIO_DELAY:
      value = g_application.GetAudioDelay();
      return true;
    case PLAYER_PROGRESS:
    case PLAYER_PROGRESS_CACHE:
    case PLAYER_SEEKBAR:
    case PLAYER_CACHELEVEL:
    case PLAYER_CHAPTER:
    case PLAYER_CHAPTERCOUNT:
      {
        if( g_application.m_pPlayer->IsPlaying())
        {
          switch( info )
          {
          case PLAYER_PROGRESS:
            value = (int)(g_application.GetPercentage());
            break;
          case PLAYER_PROGRESS_CACHE:
            value = (int)(g_application.GetCachePercentage());
            break;
          case PLAYER_SEEKBAR:
            value = (int)GetSeekPercent();
            break;
          case PLAYER_CACHELEVEL:
            value = (int)(g_application.m_pPlayer->GetCacheLevel());
            break;
          case PLAYER_CHAPTER:
            value = g_application.m_pPlayer->GetChapter();
            break;
          case PLAYER_CHAPTERCOUNT:
            value = g_application.m_pPlayer->GetChapterCount();
            break;
          }
        }
      }
      return true;
    case SYSTEM_FREE_MEMORY:
    case SYSTEM_USED_MEMORY:
      {
        MEMORYSTATUSEX stat;
        stat.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&stat);
        int memPercentUsed = (int)( 100.0f* (stat.ullTotalPhys - stat.ullAvailPhys)/stat.ullTotalPhys + 0.5f );
        if (info == SYSTEM_FREE_MEMORY)
          value = 100 - memPercentUsed;
        else
          value = memPercentUsed;
        return true;
      }
    case SYSTEM_PROGRESS_BAR:
      {
        CGUIDialogProgress *bar = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
        if (bar && bar->IsDialogRunning())
          value = bar->GetPercentage();
        return true;
      }
    case SYSTEM_FREE_SPACE:
    case SYSTEM_USED_SPACE:
      {
        g_sysinfo.GetHddSpaceInfo(value, info, true);
        return true;
      }
    case SYSTEM_CPU_USAGE:
      value = g_cpuInfo.getUsedPercentage();
      return true;
    case PVR_PLAYING_PROGRESS:
    case PVR_ACTUAL_STREAM_SIG_PROGR:
    case PVR_ACTUAL_STREAM_SNR_PROGR:
    case PVR_BACKEND_DISKSPACE_PROGR:
      value = g_PVRManager.TranslateIntInfo(info);
      return true;
    case SYSTEM_BATTERY_LEVEL:
      value = g_powerManager.BatteryLevel();
      return true;
  }
  return false;
}

// functor for comparison InfoPtr's
struct InfoBoolFinder
{
  InfoBoolFinder(const std::string &expression, int context) : m_bool(expression, context) {};
  bool operator() (const InfoPtr &right) const { return m_bool == *right; };
  InfoBool m_bool;
};

INFO::InfoPtr CGUIInfoManager::Register(const std::string &expression, int context)
{
  std::string condition(CGUIInfoLabel::ReplaceLocalize(expression));
  StringUtils::Trim(condition);

  if (condition.empty())
    return INFO::InfoPtr();

  CSingleLock lock(m_critInfo);
  // do we have the boolean expression already registered?
  vector<InfoPtr>::const_iterator i = find_if(m_bools.begin(), m_bools.end(), InfoBoolFinder(condition, context));
  if (i != m_bools.end())
    return *i;

  if (condition.find_first_of("|+[]!") != condition.npos)
    m_bools.push_back(std::make_shared<InfoExpression>(condition, context));
  else
    m_bools.push_back(std::make_shared<InfoSingle>(condition, context));

  return m_bools.back();
}

bool CGUIInfoManager::EvaluateBool(const std::string &expression, int contextWindow)
{
  bool result = false;
  INFO::InfoPtr info = Register(expression, contextWindow);
  if (info)
    result = info->Get();
  return result;
}

// checks the condition and returns it as necessary.  Currently used
// for toggle button controls and visibility of images.
bool CGUIInfoManager::GetBool(int condition1, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;
  int condition = abs(condition1);

  if (condition >= LISTITEM_START && condition < LISTITEM_END)
  {
    if (item)
      bReturn = GetItemBool(item, condition);
    else
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS); // true for has list items
      if (window)
      {
        CFileItemPtr item = window->GetCurrentListItem();
        bReturn = GetItemBool(item.get(), condition);
      }
    }
  }
  // Ethernet Link state checking
  // Will check if system has a Ethernet Link connection! [Cable in!]
  // This can used for the skinner to switch off Network or Inter required functions
  else if ( condition == SYSTEM_ALWAYS_TRUE)
    bReturn = true;
  else if (condition == SYSTEM_ALWAYS_FALSE)
    bReturn = false;
  else if (condition == SYSTEM_ETHERNET_LINK_ACTIVE)
    bReturn = true;
  else if (condition == WINDOW_IS_MEDIA)
  { // note: This doesn't return true for dialogs (content, favourites, login, videoinfo)
    CGUIWindow *pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
    bReturn = (pWindow && pWindow->IsMediaWindow());
  }
  else if (condition == PLAYER_MUTED)
    bReturn = g_application.IsMuted();
  else if (condition >= LIBRARY_HAS_MUSIC && condition <= LIBRARY_HAS_COMPILATIONS)
    bReturn = GetLibraryBool(condition);
  else if (condition == LIBRARY_IS_SCANNING)
  {
    if (g_application.IsMusicScanning() || g_application.IsVideoScanning())
      bReturn = true;
    else
      bReturn = false;
  }
  else if (condition == LIBRARY_IS_SCANNING_VIDEO)
  {
    bReturn = g_application.IsVideoScanning();
  }
  else if (condition == LIBRARY_IS_SCANNING_MUSIC)
  {
    bReturn = g_application.IsMusicScanning();
  }
  else if (condition == SYSTEM_PLATFORM_LINUX)
#if defined(TARGET_LINUX) || defined(TARGET_FREEBSD)
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_WINDOWS)
#ifdef TARGET_WINDOWS
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_DARWIN)
#ifdef TARGET_DARWIN
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_DARWIN_OSX)
#ifdef TARGET_DARWIN_OSX
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_DARWIN_IOS)
#ifdef TARGET_DARWIN_IOS
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_DARWIN_ATV2)
#ifdef TARGET_DARWIN_IOS_ATV2
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_ANDROID)
#if defined(TARGET_ANDROID)
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_LINUX_RASPBERRY_PI)
#if defined(TARGET_RASPBERRY_PI)
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_MEDIA_DVD)
    bReturn = g_mediaManager.IsDiscInDrive();
#ifdef HAS_DVD_DRIVE
  else if (condition == SYSTEM_DVDREADY)
    bReturn = g_mediaManager.GetDriveStatus() != DRIVE_NOT_READY;
  else if (condition == SYSTEM_TRAYOPEN)
    bReturn = g_mediaManager.GetDriveStatus() == DRIVE_OPEN;
#endif
  else if (condition == SYSTEM_CAN_POWERDOWN)
    bReturn = g_powerManager.CanPowerdown();
  else if (condition == SYSTEM_CAN_SUSPEND)
    bReturn = g_powerManager.CanSuspend();
  else if (condition == SYSTEM_CAN_HIBERNATE)
    bReturn = g_powerManager.CanHibernate();
  else if (condition == SYSTEM_CAN_REBOOT)
    bReturn = g_powerManager.CanReboot();
  else if (condition == SYSTEM_SCREENSAVER_ACTIVE)
    bReturn = g_application.IsInScreenSaver();
  else if (condition == SYSTEM_DPMS_ACTIVE)
    bReturn = g_application.IsDPMSActive();

  else if (condition == PLAYER_SHOWINFO)
    bReturn = m_playerShowInfo;
  else if (condition == PLAYER_SHOWCODEC)
    bReturn = m_playerShowCodec;
  else if (condition >= MULTI_INFO_START && condition <= MULTI_INFO_END)
  {
    return GetMultiInfoBool(m_multiInfo[condition - MULTI_INFO_START], contextWindow, item);
  }
  else if (condition == SYSTEM_HASLOCKS)
    bReturn = CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE;
  else if (condition == SYSTEM_HAS_PVR)
    bReturn = true;
  else if (condition == SYSTEM_ISMASTER)
    bReturn = CProfilesManager::Get().GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && g_passwordManager.bMasterUser;
  else if (condition == SYSTEM_ISFULLSCREEN)
    bReturn = g_Windowing.IsFullScreen();
  else if (condition == SYSTEM_ISSTANDALONE)
    bReturn = g_application.IsStandAlone();
  else if (condition == SYSTEM_ISINHIBIT)
    bReturn = g_application.IsIdleShutdownInhibited();
  else if (condition == SYSTEM_HAS_SHUTDOWN)
    bReturn = (CSettings::Get().GetInt("powermanagement.shutdowntime") > 0);
  else if (condition == SYSTEM_LOGGEDON)
    bReturn = !(g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN);
  else if (condition == SYSTEM_SHOW_EXIT_BUTTON)
    bReturn = g_advancedSettings.m_showExitButton;
  else if (condition == SYSTEM_HAS_LOGINSCREEN)
    bReturn = CProfilesManager::Get().UsingLoginScreen();
  else if (condition == WEATHER_IS_FETCHED)
    bReturn = g_weatherManager.IsFetched();
  else if (condition >= PVR_CONDITIONS_START && condition <= PVR_CONDITIONS_END)
    bReturn = g_PVRManager.TranslateBoolInfo(condition);

  else if (condition == SYSTEM_INTERNET_STATE)
  {
    g_sysinfo.GetInfo(condition);
    bReturn = g_sysinfo.HasInternet();
  }
  else if (condition == SKIN_HAS_VIDEO_OVERLAY)
  {
    bReturn = g_windowManager.IsOverlayAllowed() && g_application.m_pPlayer->IsPlayingVideo();
  }
  else if (condition == SKIN_HAS_MUSIC_OVERLAY)
  {
    bReturn = g_windowManager.IsOverlayAllowed() && g_application.m_pPlayer->IsPlayingAudio();
  }
  else if (condition == CONTAINER_HASFILES || condition == CONTAINER_HASFOLDERS)
  {
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
    {
      const CFileItemList& items=((CGUIMediaWindow*)pWindow)->CurrentDirectory();
      for (int i=0;i<items.Size();++i)
      {
        CFileItemPtr item=items.Get(i);
        if (!item->m_bIsFolder && condition == CONTAINER_HASFILES)
        {
          bReturn=true;
          break;
        }
        else if (item->m_bIsFolder && !item->IsParentFolder() && condition == CONTAINER_HASFOLDERS)
        {
          bReturn=true;
          break;
        }
      }
    }
  }
  else if (condition == CONTAINER_STACKED)
  {
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
      bReturn = ((CGUIMediaWindow*)pWindow)->CurrentDirectory().GetProperty("isstacked").asBoolean();
  }
  else if (condition == CONTAINER_HAS_THUMB)
  {
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
      bReturn = ((CGUIMediaWindow*)pWindow)->CurrentDirectory().HasArt("thumb");
  }
  else if (condition == CONTAINER_HAS_NEXT || condition == CONTAINER_HAS_PREVIOUS
           || condition == CONTAINER_SCROLLING || condition == CONTAINER_ISUPDATING)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
    {
      const CGUIControl* control = window->GetControl(window->GetViewContainerID());
      if (control)
        bReturn = control->GetCondition(condition, 0);
    }
  }
  else if (condition == CONTAINER_CAN_FILTER)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      bReturn = !((CGUIMediaWindow*)window)->CanFilterAdvanced();
  }
  else if (condition == CONTAINER_CAN_FILTERADVANCED)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      bReturn = ((CGUIMediaWindow*)window)->CanFilterAdvanced();
  }
  else if (condition == CONTAINER_FILTERED)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      bReturn = ((CGUIMediaWindow*)window)->IsFiltered();
  }
  else if (condition == VIDEOPLAYER_HAS_INFO)
    bReturn = ((m_currentFile->HasVideoInfoTag() && !m_currentFile->GetVideoInfoTag()->IsEmpty()) ||
               (m_currentFile->HasPVRChannelInfoTag()  && !m_currentFile->GetPVRChannelInfoTag()->IsEmpty()));
  else if (condition >= CONTAINER_SCROLL_PREVIOUS && condition <= CONTAINER_SCROLL_NEXT)
  {
    // no parameters, so we assume it's just requested for a media window.  It therefore
    // can only happen if the list has focus.
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
    {
      map<int,int>::const_iterator it = m_containerMoves.find(pWindow->GetViewContainerID());
      if (it != m_containerMoves.end())
      {
        if (condition > CONTAINER_STATIC) // moving up
          bReturn = it->second >= std::max(condition - CONTAINER_STATIC, 1);
        else
          bReturn = it->second <= std::min(condition - CONTAINER_STATIC, -1);
      }
    }
  }
  else if (condition == SLIDESHOW_ISPAUSED)
  {
    CGUIWindowSlideShow *slideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    bReturn = (slideShow && slideShow->IsPaused());
  }
  else if (condition == SLIDESHOW_ISRANDOM)
  {
    CGUIWindowSlideShow *slideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    bReturn = (slideShow && slideShow->IsShuffled());
  }
  else if (condition == SLIDESHOW_ISACTIVE)
  {
    CGUIWindowSlideShow *slideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    bReturn = (slideShow && slideShow->InSlideShow());
  }
  else if (condition == SLIDESHOW_ISVIDEO)
  {
    CGUIWindowSlideShow *slideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    bReturn = (slideShow && slideShow->GetCurrentSlide() && slideShow->GetCurrentSlide()->IsVideo());
  }
  else if (g_application.m_pPlayer->IsPlaying())
  {
    switch (condition)
    {
    case PLAYER_HAS_MEDIA:
      bReturn = true;
      break;
    case PLAYER_HAS_AUDIO:
      bReturn = g_application.m_pPlayer->IsPlayingAudio();
      break;
    case PLAYER_HAS_VIDEO:
      bReturn = g_application.m_pPlayer->IsPlayingVideo();
      break;
    case PLAYER_PLAYING:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && (g_application.m_pPlayer->GetPlaySpeed() == 1);
      break;
    case PLAYER_PAUSED:
      bReturn = g_application.m_pPlayer->IsPausedPlayback();
      break;
    case PLAYER_REWINDING:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() < 1;
      break;
    case PLAYER_FORWARDING:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() > 1;
      break;
    case PLAYER_REWINDING_2x:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() == -2;
      break;
    case PLAYER_REWINDING_4x:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() == -4;
      break;
    case PLAYER_REWINDING_8x:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() == -8;
      break;
    case PLAYER_REWINDING_16x:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() == -16;
      break;
    case PLAYER_REWINDING_32x:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() == -32;
      break;
    case PLAYER_FORWARDING_2x:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() == 2;
      break;
    case PLAYER_FORWARDING_4x:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() == 4;
      break;
    case PLAYER_FORWARDING_8x:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() == 8;
      break;
    case PLAYER_FORWARDING_16x:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() == 16;
      break;
    case PLAYER_FORWARDING_32x:
      bReturn = !g_application.m_pPlayer->IsPausedPlayback() && g_application.m_pPlayer->GetPlaySpeed() == 32;
      break;
    case PLAYER_CAN_RECORD:
      bReturn = g_application.m_pPlayer->CanRecord();
      break;
    case PLAYER_CAN_PAUSE:
      bReturn = g_application.m_pPlayer->CanPause();
      break;
    case PLAYER_CAN_SEEK:
      bReturn = g_application.m_pPlayer->CanSeek();
      break;
    case PLAYER_RECORDING:
      bReturn = g_application.m_pPlayer->IsRecording();
    break;
    case PLAYER_DISPLAY_AFTER_SEEK:
      bReturn = GetDisplayAfterSeek();
    break;
    case PLAYER_CACHING:
      bReturn = g_application.m_pPlayer->IsCaching();
    break;
    case PLAYER_SEEKBAR:
      {
        CGUIDialog *seekBar = (CGUIDialog*)g_windowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR);
        bReturn = seekBar ? seekBar->IsDialogRunning() : false;
      }
    break;
    case PLAYER_SEEKING:
      bReturn = CSeekHandler::Get().InProgress();
    break;
    case PLAYER_SHOWTIME:
      bReturn = m_playerShowTime;
    break;
    case PLAYER_PASSTHROUGH:
      bReturn = g_application.m_pPlayer->IsPassthrough();
      break;
    case PLAYER_ISINTERNETSTREAM:
      bReturn = m_currentFile && URIUtils::IsInternetStream(m_currentFile->GetPath());
      break;
    case MUSICPM_ENABLED:
      bReturn = g_partyModeManager.IsEnabled();
    break;
    case MUSICPLAYER_HASPREVIOUS:
      {
        // requires current playlist be PLAYLIST_MUSIC
        bReturn = false;
        if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
          bReturn = (g_playlistPlayer.GetCurrentSong() > 0); // not first song
      }
      break;
    case MUSICPLAYER_HASNEXT:
      {
        // requires current playlist be PLAYLIST_MUSIC
        bReturn = false;
        if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
          bReturn = (g_playlistPlayer.GetCurrentSong() < (g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size() - 1)); // not last song
      }
      break;
    case MUSICPLAYER_PLAYLISTPLAYING:
      {
        bReturn = false;
        if (g_application.m_pPlayer->IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
          bReturn = true;
      }
      break;
    case VIDEOPLAYER_USING_OVERLAYS:
      bReturn = (CSettings::Get().GetInt("videoplayer.rendermethod") == RENDER_OVERLAYS);
    break;
    case VIDEOPLAYER_ISFULLSCREEN:
      bReturn = g_windowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO;
    break;
    case VIDEOPLAYER_HASMENU:
      bReturn = g_application.m_pPlayer->HasMenu();
    break;
    case PLAYLIST_ISRANDOM:
      bReturn = g_playlistPlayer.IsShuffled(g_playlistPlayer.GetCurrentPlaylist());
    break;
    case PLAYLIST_ISREPEAT:
      bReturn = g_playlistPlayer.GetRepeat(g_playlistPlayer.GetCurrentPlaylist()) == PLAYLIST::REPEAT_ALL;
    break;
    case PLAYLIST_ISREPEATONE:
      bReturn = g_playlistPlayer.GetRepeat(g_playlistPlayer.GetCurrentPlaylist()) == PLAYLIST::REPEAT_ONE;
    break;
    case PLAYER_HASDURATION:
      bReturn = g_application.GetTotalTime() > 0;
      break;
    case VIDEOPLAYER_HASTELETEXT:
      if (g_application.m_pPlayer->GetTeletextCache())
        bReturn = true;
      break;
    case VIDEOPLAYER_HASSUBTITLES:
      bReturn = g_application.m_pPlayer->GetSubtitleCount() > 0;
      break;
    case VIDEOPLAYER_SUBTITLESENABLED:
      bReturn = g_application.m_pPlayer->GetSubtitleVisible();
      break;
    case VISUALISATION_LOCKED:
      {
        CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
        g_windowManager.SendMessage(msg);
        if (msg.GetPointer())
        {
          CVisualisation *pVis = (CVisualisation *)msg.GetPointer();
          bReturn = pVis->IsLocked();
        }
      }
    break;
    case VISUALISATION_ENABLED:
      bReturn = !CSettings::Get().GetString("musicplayer.visualisation").empty();
    break;
    case VIDEOPLAYER_HAS_EPG:
      if (m_currentFile->HasPVRChannelInfoTag())
        bReturn = (m_currentFile->GetPVRChannelInfoTag()->GetEPGNow().get() != NULL);
    break;
    case VIDEOPLAYER_IS_STEREOSCOPIC:
      if(g_application.m_pPlayer->IsPlaying())
      {
        bReturn = !m_videoInfo.stereoMode.empty();
      }
      break;
    case VIDEOPLAYER_CAN_RESUME_LIVE_TV:
      if (m_currentFile->HasPVRRecordingInfoTag())
      {
        EPG::CEpgInfoTagPtr epgTag = EPG::CEpgContainer::Get().GetTagById(m_currentFile->GetPVRRecordingInfoTag()->EpgEvent());
        bReturn = (epgTag && epgTag->IsActive() && epgTag->ChannelTag());
      }
      break;
    default: // default, use integer value different from 0 as true
      {
        int val;
        bReturn = GetInt(val, condition) && val != 0;
      }
    }
  }
  if (condition1 < 0)
    bReturn = !bReturn;
  return bReturn;
}

/// \brief Examines the multi information sent and returns true or false accordingly.
bool CGUIInfoManager::GetMultiInfoBool(const GUIInfo &info, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;
  int condition = abs(info.m_info);

  if (condition >= LISTITEM_START && condition <= LISTITEM_END)
  {
    if (!item)
    {
      CGUIWindow *window = NULL;
      int data1 = info.GetData1();
      if (!data1) // No container specified, so we lookup the current view container
      {
        window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
        if (window && window->IsMediaWindow())
          data1 = ((CGUIMediaWindow*)(window))->GetViewContainerID();
      }

      if (!window) // If we don't have a window already (from lookup above), get one
        window = GetWindowWithCondition(contextWindow, 0);

      if (window)
      {
        const CGUIControl *control = window->GetControl(data1);
        if (control && control->IsContainer())
          item = ((IGUIContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag()).get();
      }
    }
    if (item) // If we got a valid item, do the lookup
      bReturn = GetItemBool(item, condition); // Image prioritizes images over labels (in the case of music item ratings for instance)
  }
  else
  {
    switch (condition)
    {
      case SKIN_BOOL:
        {
          bReturn = CSkinSettings::Get().GetBool(info.GetData1());
        }
        break;
      case SKIN_STRING:
        {
          if (info.GetData2())
            bReturn = StringUtils::EqualsNoCase(CSkinSettings::Get().GetString(info.GetData1()), m_stringParameters[info.GetData2()]);
          else
            bReturn = !CSkinSettings::Get().GetString(info.GetData1()).empty();
        }
        break;
      case SKIN_HAS_THEME:
        {
          std::string theme = CSettings::Get().GetString("lookandfeel.skintheme");
          URIUtils::RemoveExtension(theme);
          bReturn = StringUtils::EqualsNoCase(theme, m_stringParameters[info.GetData1()]);
        }
        break;
      case STRING_IS_EMPTY:
        // note: Get*Image() falls back to Get*Label(), so this should cover all of them
        if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
          bReturn = GetItemImage((const CFileItem *)item, info.GetData1()).empty();
        else
          bReturn = GetImage(info.GetData1(), contextWindow).empty();
        break;
      case STRING_COMPARE:
        {
          std::string compare;
          if (info.GetData2() < 0) // info labels are stored with negative numbers
          {
            int info2 = -info.GetData2();
            if (item && item->IsFileItem() && info2 >= LISTITEM_START && info2 < LISTITEM_END)
              compare = GetItemImage((const CFileItem *)item, info2);
            else
              compare = GetImage(info2, contextWindow);
          }
          else if (info.GetData2() < (int)m_stringParameters.size())
          { // conditional string
            compare = m_stringParameters[info.GetData2()];
          }
          if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
            bReturn = StringUtils::EqualsNoCase(GetItemImage((const CFileItem *)item, info.GetData1()), compare);
          else
            bReturn = StringUtils::EqualsNoCase(GetImage(info.GetData1(), contextWindow), compare);
        }
        break;
      case INTEGER_GREATER_THAN:
        {
          int integer;
          if (GetInt(integer, info.GetData1(), contextWindow, item))
            bReturn = integer > info.GetData2();
          else
          {
            std::string value;

            if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
              value = GetItemImage((const CFileItem *)item, info.GetData1());
            else
              value = GetImage(info.GetData1(), contextWindow);

            // Handle the case when a value contains time separator (:). This makes IntegerGreaterThan
            // useful for Player.Time* members without adding a separate set of members returning time in seconds
            if ( value.find_first_of( ':' ) != value.npos )
              bReturn = StringUtils::TimeStringToSeconds( value ) > info.GetData2();
            else
              bReturn = atoi( value.c_str() ) > info.GetData2();
          }
        }
        break;
      case STRING_STR:
      case STRING_STR_LEFT:
      case STRING_STR_RIGHT:
        {
          std::string compare = m_stringParameters[info.GetData2()];
          // our compare string is already in lowercase, so lower case our label as well
          // as std::string::Find() is case sensitive
          std::string label;
          if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
          {
            label = GetItemImage((const CFileItem *)item, info.GetData1());
            StringUtils::ToLower(label);
          }
          else
          {
            label = GetImage(info.GetData1(), contextWindow);
            StringUtils::ToLower(label);
          }
          if (condition == STRING_STR_LEFT)
            bReturn = StringUtils::StartsWith(label, compare);
          else if (condition == STRING_STR_RIGHT)
            bReturn = StringUtils::EndsWith(label, compare);
          else
            bReturn = label.find(compare) != std::string::npos;
        }
        break;
      case SYSTEM_ALARM_LESS_OR_EQUAL:
        {
          int time = lrint(g_alarmClock.GetRemaining(m_stringParameters[info.GetData1()]));
          int timeCompare = atoi(m_stringParameters[info.GetData2()].c_str());
          if (time > 0)
            bReturn = timeCompare >= time;
          else
            bReturn = false;
        }
        break;
      case SYSTEM_IDLE_TIME:
        bReturn = g_application.GlobalIdleTime() >= (int)info.GetData1();
        break;
      case CONTROL_GROUP_HAS_FOCUS:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
            bReturn = window->ControlGroupHasFocus(info.GetData1(), info.GetData2());
        }
        break;
      case CONTROL_IS_VISIBLE:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            // Note: This'll only work for unique id's
            const CGUIControl *control = window->GetControl(info.GetData1());
            if (control)
              bReturn = control->IsVisible();
          }
        }
        break;
      case CONTROL_IS_ENABLED:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            // Note: This'll only work for unique id's
            const CGUIControl *control = window->GetControl(info.GetData1());
            if (control)
              bReturn = !control->IsDisabled();
          }
        }
        break;
      case CONTROL_HAS_FOCUS:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
            bReturn = (window->GetFocusedControlID() == (int)info.GetData1());
        }
        break;
      case WINDOW_NEXT:
        if (info.GetData1())
          bReturn = ((int)info.GetData1() == m_nextWindowID);
        else
        {
          CGUIWindow *window = g_windowManager.GetWindow(m_nextWindowID);
          if (window && StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), m_stringParameters[info.GetData2()]))
            bReturn = true;
        }
        break;
      case WINDOW_PREVIOUS:
        if (info.GetData1())
          bReturn = ((int)info.GetData1() == m_prevWindowID);
        else
        {
          CGUIWindow *window = g_windowManager.GetWindow(m_prevWindowID);
          if (window && StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), m_stringParameters[info.GetData2()]))
            bReturn = true;
        }
        break;
      case WINDOW_IS_VISIBLE:
        if (info.GetData1())
          bReturn = g_windowManager.IsWindowVisible(info.GetData1());
        else
          bReturn = g_windowManager.IsWindowVisible(m_stringParameters[info.GetData2()]);
        break;
      case WINDOW_IS_TOPMOST:
        if (info.GetData1())
          bReturn = g_windowManager.IsWindowTopMost(info.GetData1());
        else
          bReturn = g_windowManager.IsWindowTopMost(m_stringParameters[info.GetData2()]);
        break;
      case WINDOW_IS_ACTIVE:
        if (info.GetData1())
          bReturn = g_windowManager.IsWindowActive(info.GetData1());
        else
          bReturn = g_windowManager.IsWindowActive(m_stringParameters[info.GetData2()]);
        break;
      case SYSTEM_HAS_ALARM:
        bReturn = g_alarmClock.HasAlarm(m_stringParameters[info.GetData1()]);
        break;
      case SYSTEM_GET_BOOL:
        bReturn = CSettings::Get().GetBool(m_stringParameters[info.GetData1()]);
        break;
      case SYSTEM_HAS_CORE_ID:
        bReturn = g_cpuInfo.HasCoreId(info.GetData1());
        break;
      case SYSTEM_SETTING:
        {
          if ( StringUtils::EqualsNoCase(m_stringParameters[info.GetData1()], "hidewatched") )
          {
            CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              bReturn = CMediaSettings::Get().GetWatchedMode(((CGUIMediaWindow *)window)->CurrentDirectory().GetContent()) == WatchedModeUnwatched;
          }
        }
        break;
      case SYSTEM_HAS_ADDON:
      {
        AddonPtr addon;
        bReturn = CAddonMgr::Get().GetAddon(m_stringParameters[info.GetData1()],addon) && addon;
        break;
      }
      case CONTAINER_SCROLL_PREVIOUS:
      case CONTAINER_MOVE_PREVIOUS:
      case CONTAINER_MOVE_NEXT:
      case CONTAINER_SCROLL_NEXT:
        {
          map<int,int>::const_iterator it = m_containerMoves.find(info.GetData1());
          if (it != m_containerMoves.end())
          {
            if (condition > CONTAINER_STATIC) // moving up
              bReturn = it->second >= std::max(condition - CONTAINER_STATIC, 1);
            else
              bReturn = it->second <= std::min(condition - CONTAINER_STATIC, -1);
          }
        }
        break;
      case CONTAINER_CONTENT:
        {
          std::string content;
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            if (window->GetID() == WINDOW_DIALOG_MUSIC_INFO)
              content = ((CGUIDialogMusicInfo *)window)->CurrentDirectory().GetContent();
            else if (window->GetID() == WINDOW_DIALOG_VIDEO_INFO)
              content = ((CGUIDialogVideoInfo *)window)->CurrentDirectory().GetContent();
          }
          if (content.empty())
          {
            window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              content = ((CGUIMediaWindow *)window)->CurrentDirectory().GetContent();
          }
          bReturn = StringUtils::EqualsNoCase(m_stringParameters[info.GetData2()], content);
        }
        break;
      case CONTAINER_ROW:
      case CONTAINER_COLUMN:
      case CONTAINER_POSITION:
      case CONTAINER_HAS_NEXT:
      case CONTAINER_HAS_PREVIOUS:
      case CONTAINER_SCROLLING:
      case CONTAINER_SUBITEM:
      case CONTAINER_ISUPDATING:
        {
          const CGUIControl *control = NULL;
          if (info.GetData1())
          { // container specified
            CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
            if (window)
              control = window->GetControl(info.GetData1());
          }
          else
          { // no container specified - assume a mediawindow
            CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              control = window->GetControl(window->GetViewContainerID());
          }
          if (control)
            bReturn = control->GetCondition(condition, info.GetData2());
        }
        break;
      case CONTAINER_HAS_FOCUS:
        { // grab our container
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            const CGUIControl *control = window->GetControl(info.GetData1());
            if (control && control->IsContainer())
            {
              CFileItemPtr item = std::static_pointer_cast<CFileItem>(((IGUIContainer *)control)->GetListItem(0));
              if (item && item->m_iprogramCount == info.GetData2())  // programcount used to store item id
                bReturn = true;
            }
          }
          break;
        }
      case MUSICPLAYER_CONTENT:
        {
          std::string strContent = "files";
          if (m_currentFile->HasPVRChannelInfoTag())
            strContent = "livetv";
          bReturn = StringUtils::EqualsNoCase(m_stringParameters[info.GetData1()], strContent);
          break;
        }
      case VIDEOPLAYER_CONTENT:
        {
          std::string strContent="files";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_type == MediaTypeMovie)
            strContent = "movies";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_type == MediaTypeEpisode)
            strContent = "episodes";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_type == MediaTypeMusicVideo)
            strContent = "musicvideos";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_strStatus == "livetv")
            strContent = "livetv";
          if (m_currentFile->HasPVRChannelInfoTag())
            strContent = "livetv";
          bReturn = StringUtils::EqualsNoCase(m_stringParameters[info.GetData1()], strContent);
        }
        break;
      case CONTAINER_SORT_METHOD:
      {
        CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
        if (window)
        {
          const CGUIViewState *viewState = ((CGUIMediaWindow*)window)->GetViewState();
          if (viewState)
            bReturn = ((unsigned int)viewState->GetSortMethod().sortBy == info.GetData1());
        }
        break;
      }
      case CONTAINER_SORT_DIRECTION:
      {
        CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
        if (window)
        {
          const CGUIViewState *viewState = ((CGUIMediaWindow*)window)->GetViewState();
          if (viewState)
            bReturn = ((unsigned int)viewState->GetSortOrder() == info.GetData1());
        }
        break;
      }
      case SYSTEM_DATE:
        {
          if (info.GetData2() == -1) // info doesn't contain valid startDate
            return false;
          CDateTime date = CDateTime::GetCurrentDateTime();
          int currentDate = date.GetMonth()*100+date.GetDay();
          int startDate = info.GetData1();
          int stopDate = info.GetData2();

          if (stopDate < startDate)
            bReturn = currentDate >= startDate || currentDate < stopDate;
          else
            bReturn = currentDate >= startDate && currentDate < stopDate;
        }
        break;
      case SYSTEM_TIME:
        {
          CDateTime time=CDateTime::GetCurrentDateTime();
          int currentTime = time.GetMinuteOfDay();
          int startTime = info.GetData1();
          int stopTime = info.GetData2();

          if (stopTime < startTime)
            bReturn = currentTime >= startTime || currentTime < stopTime;
          else
            bReturn = currentTime >= startTime && currentTime < stopTime;
        }
        break;
      case MUSICPLAYER_EXISTS:
        {
          int index = info.GetData2();
          if (info.GetData1() == 1)
          { // relative index
            if (g_playlistPlayer.GetCurrentPlaylist() != PLAYLIST_MUSIC)
            {
              bReturn = false;
              break;
            }
            index += g_playlistPlayer.GetCurrentSong();
          }
          bReturn = (index >= 0 && index < g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size());
        }
        break;

      case PLAYLIST_ISRANDOM:
        {
          int playlistid = info.GetData1();
          if (playlistid > PLAYLIST_NONE)
            bReturn = g_playlistPlayer.IsShuffled(playlistid);
        }
        break;

      case PLAYLIST_ISREPEAT:
        {
          int playlistid = info.GetData1();
          if (playlistid > PLAYLIST_NONE)
            bReturn = g_playlistPlayer.GetRepeat(playlistid) == PLAYLIST::REPEAT_ALL;
        }
        break;

      case PLAYLIST_ISREPEATONE:
        {
          int playlistid = info.GetData1();
          if (playlistid > PLAYLIST_NONE)
            bReturn = g_playlistPlayer.GetRepeat(playlistid) == PLAYLIST::REPEAT_ONE;
        }
        break;
    }
  }
  return (info.m_info < 0) ? !bReturn : bReturn;
}

bool CGUIInfoManager::GetMultiInfoInt(int &value, const GUIInfo &info, int contextWindow) const
{
  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    CFileItemPtr item;
    CGUIWindow *window = NULL;

    int data1 = info.GetData1();
    if (!data1) // No container specified, so we lookup the current view container
    {
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
      if (window && window->IsMediaWindow())
        data1 = ((CGUIMediaWindow*)(window))->GetViewContainerID();
    }

    if (!window) // If we don't have a window already (from lookup above), get one
      window = GetWindowWithCondition(contextWindow, 0);

    if (window)
    {
      const CGUIControl *control = window->GetControl(data1);
      if (control && control->IsContainer())
        item = std::static_pointer_cast<CFileItem>(((IGUIContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag()));
    }

    if (item) // If we got a valid item, do the lookup
      return GetItemInt(value, item.get(), info.m_info);
  }

  return 0;
}

/// \brief Examines the multi information sent and returns the string as appropriate
std::string CGUIInfoManager::GetMultiInfoLabel(const GUIInfo &info, int contextWindow, std::string *fallback)
{
  if (info.m_info == SKIN_STRING)
  {
    return CSkinSettings::Get().GetString(info.GetData1());
  }
  else if (info.m_info == SKIN_BOOL)
  {
    bool bInfo = CSkinSettings::Get().GetBool(info.GetData1());
    if (bInfo)
      return g_localizeStrings.Get(20122);
  }
  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    CFileItemPtr item;
    CGUIWindow *window = NULL;

    int data1 = info.GetData1();
    if (!data1) // No container specified, so we lookup the current view container
    {
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
      if (window && window->IsMediaWindow())
        data1 = ((CGUIMediaWindow*)(window))->GetViewContainerID();
    }

    if (!window) // If we don't have a window already (from lookup above), get one
      window = GetWindowWithCondition(contextWindow, 0);

    if (window)
    {
      const CGUIControl *control = window->GetControl(data1);
      if (control && control->IsContainer())
        item = std::static_pointer_cast<CFileItem>(((IGUIContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag()));
    }

    if (item) // If we got a valid item, do the lookup
      return GetItemImage(item.get(), info.m_info, fallback); // Image prioritizes images over labels (in the case of music item ratings for instance)
  }
  else if (info.m_info == PLAYER_TIME)
  {
    return GetCurrentPlayTime((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_TIME_REMAINING)
  {
    return GetCurrentPlayTimeRemaining((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_FINISH_TIME)
  {
    CDateTime time;
    CEpgInfoTagPtr currentTag(GetEpgInfoTag());
    if (currentTag)
      time = currentTag->EndAsLocalTime();
    else
    {
      time = CDateTime::GetCurrentDateTime();
      time += CDateTimeSpan(0, 0, 0, GetPlayTimeRemaining());
    }
    return LocalizeTime(time, (TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_START_TIME)
  {
    CDateTime time;
    CEpgInfoTagPtr currentTag(GetEpgInfoTag());
    if (currentTag)
      time = currentTag->StartAsLocalTime();
    else
    {
      time = CDateTime::GetCurrentDateTime();
      time -= CDateTimeSpan(0, 0, 0, (int)GetPlayTime());
    }
    return LocalizeTime(time, (TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_TIME_SPEED)
  {
    std::string strTime;
    if (g_application.m_pPlayer->GetPlaySpeed() != 1)
      strTime = StringUtils::Format("%s (%ix)", GetCurrentPlayTime((TIME_FORMAT)info.GetData1()).c_str(), g_application.m_pPlayer->GetPlaySpeed());
    else
      strTime = GetCurrentPlayTime();
    return strTime;
  }
  else if (info.m_info == PLAYER_DURATION)
  {
    return GetDuration((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_SEEKTIME)
  {
    return GetCurrentSeekTime((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_SEEKOFFSET)
  {
    std::string seekOffset = StringUtils::SecondsToTimeString(abs(m_seekOffset / 1000), (TIME_FORMAT)info.GetData1());
    if (m_seekOffset < 0)
      return "-" + seekOffset;
    if (m_seekOffset > 0)
      return "+" + seekOffset;
  }
  else if (info.m_info == PLAYER_SEEKSTEPSIZE)
  {
    int seekSize = CSeekHandler::Get().GetSeekSize();
    std::string strSeekSize = StringUtils::SecondsToTimeString(abs(seekSize), (TIME_FORMAT)info.GetData1());
    if (seekSize < 0)
      return "-" + strSeekSize;
    if (seekSize > 0)
      return "+" + strSeekSize;
  }
  else if (info.m_info == PLAYER_ITEM_ART)
  {
    return m_currentFile->GetArt(m_stringParameters[info.GetData1()]);
  }
  else if (info.m_info == SYSTEM_TIME)
  {
    return GetTime((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == SYSTEM_DATE)
  {
    CDateTime time=CDateTime::GetCurrentDateTime();
    return time.GetAsLocalizedDate(m_stringParameters[info.GetData1()],false);
  }
  else if (info.m_info == CONTAINER_NUM_PAGES || info.m_info == CONTAINER_CURRENT_PAGE ||
           info.m_info == CONTAINER_NUM_ITEMS || info.m_info == CONTAINER_POSITION ||
           info.m_info == CONTAINER_CURRENT_ITEM)
  {
    const CGUIControl *control = NULL;
    if (info.GetData1())
    { // container specified
      CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
      if (window)
        control = window->GetControl(info.GetData1());
    }
    else
    { // no container specified - assume a mediawindow
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        control = window->GetControl(window->GetViewContainerID());
    }
    if (control)
    {
      if (control->IsContainer())
        return ((IGUIContainer *)control)->GetLabel(info.m_info);
      else if (control->GetControlType() == CGUIControl::GUICONTROL_TEXTBOX)
        return ((CGUITextBox *)control)->GetLabel(info.m_info);
    }
  }
  else if (info.m_info == SYSTEM_GET_CORE_USAGE)
  {
    std::string strCpu = StringUtils::Format("%4.2f", g_cpuInfo.GetCoreInfo(atoi(m_stringParameters[info.GetData1()].c_str())).m_fPct);
    return strCpu;
  }
  else if (info.m_info >= MUSICPLAYER_TITLE && info.m_info <= MUSICPLAYER_ALBUM_ARTIST)
    return GetMusicPlaylistInfo(info);
  else if (info.m_info == CONTAINER_PROPERTY)
  {
    CGUIWindow *window = NULL;
    if (info.GetData1())
    { // container specified
      window = GetWindowWithCondition(contextWindow, 0);
    }
    else
    { // no container specified - assume a mediawindow
      window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    }
    if (window)
      return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty(m_stringParameters[info.GetData2()]).asString();
  }
  else if (info.m_info == CONTAINER_ART)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      return ((CGUIMediaWindow *)window)->CurrentDirectory().GetArt(m_stringParameters[info.GetData2()]);
  }
  else if (info.m_info == CONTROL_GET_LABEL)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
    if (window)
    {
      const CGUIControl *control = window->GetControl(info.GetData1());
      if (control)
      {
        int data2 = info.GetData2();
        if (data2)
          return control->GetDescriptionByIndex(data2);
        else
          return control->GetDescription();
      }
    }
  }
  else if (info.m_info == WINDOW_PROPERTY)
  {
    CGUIWindow *window = NULL;
    if (info.GetData1())
    { // window specified
      window = g_windowManager.GetWindow(info.GetData1());//GetWindowWithCondition(contextWindow, 0);
    }
    else
    { // no window specified - assume active
      window = GetWindowWithCondition(contextWindow, 0);
    }

    if (window)
      return window->GetProperty(m_stringParameters[info.GetData2()]).asString();
  }
  else if (info.m_info == SYSTEM_ADDON_TITLE ||
           info.m_info == SYSTEM_ADDON_ICON ||
           info.m_info == SYSTEM_ADDON_VERSION)
  {
    // This logic does not check/care whether an addon has been disabled/marked as broken,
    // it simply retrieves it's name or icon that means if an addon is placed on the home screen it
    // will stay there even if it's disabled/marked as broken. This might need to be changed/fixed
    // in the future.
    AddonPtr addon;
    if (info.GetData2() == 0)
      CAddonMgr::Get().GetAddon(const_cast<CGUIInfoManager*>(this)->GetLabel(info.GetData1(), contextWindow),addon,ADDON_UNKNOWN,false);
    else
      CAddonMgr::Get().GetAddon(m_stringParameters[info.GetData1()],addon,ADDON_UNKNOWN,false);
    if (addon && info.m_info == SYSTEM_ADDON_TITLE)
      return addon->Name();
    if (addon && info.m_info == SYSTEM_ADDON_ICON)
      return addon->Icon();
    if (addon && info.m_info == SYSTEM_ADDON_VERSION)
      return addon->Version().asString();
  }
  else if (info.m_info == PLAYLIST_LENGTH ||
           info.m_info == PLAYLIST_POSITION ||
           info.m_info == PLAYLIST_RANDOM ||
           info.m_info == PLAYLIST_REPEAT)
  {
    int playlistid = info.GetData1();
    if (playlistid > PLAYLIST_NONE)
      return GetPlaylistLabel(info.m_info, playlistid);
  }

  return "";
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
std::string CGUIInfoManager::GetImage(int info, int contextWindow, std::string *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, true);

  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow, fallback);
  }
  else if (info == WEATHER_CONDITIONS)
    return g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON);
  else if (info == SYSTEM_PROFILETHUMB)
  {
    std::string thumb = CProfilesManager::Get().GetCurrentProfile().getThumb();
    if (thumb.empty())
      thumb = "unknown-user.png";
    return thumb;
  }
  else if (info == MUSICPLAYER_COVER)
  {
    if (!g_application.m_pPlayer->IsPlayingAudio()) return "";
    if (fallback)
      *fallback = "DefaultAlbumCover.png";
    return m_currentFile->HasArt("thumb") ? m_currentFile->GetArt("thumb") : "DefaultAlbumCover.png";
  }
  else if (info == MUSICPLAYER_RATING)
  {
    if (!g_application.m_pPlayer->IsPlayingAudio()) return "";
    return GetItemImage(m_currentFile, LISTITEM_RATING);
  }
  else if (info == PLAYER_STAR_RATING)
  {
    if (!g_application.m_pPlayer->IsPlaying()) return "";
    return GetItemImage(m_currentFile, LISTITEM_STAR_RATING);
  }
  else if (info == VIDEOPLAYER_COVER)
  {
    if (!g_application.m_pPlayer->IsPlayingVideo()) return "";
    if (fallback)
      *fallback = "DefaultVideoCover.png";
    if(m_currentMovieThumb.empty())
      return m_currentFile->HasArt("thumb") ? m_currentFile->GetArt("thumb") : "DefaultVideoCover.png";
    else return m_currentMovieThumb;
  }
  else if (info == LISTITEM_THUMB || info == LISTITEM_ICON || info == LISTITEM_ACTUAL_ICON ||
          info == LISTITEM_OVERLAY || info == LISTITEM_RATING || info == LISTITEM_STAR_RATING)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
    if (window)
    {
      CFileItemPtr item = window->GetCurrentListItem();
      if (item)
        return GetItemImage(item.get(), info, fallback);
    }
  }
  return GetLabel(info, contextWindow, fallback);
}

std::string CGUIInfoManager::GetDate(bool bNumbersOnly)
{
  CDateTime time=CDateTime::GetCurrentDateTime();
  return time.GetAsLocalizedDate(!bNumbersOnly);
}

std::string CGUIInfoManager::GetTime(TIME_FORMAT format) const
{
  CDateTime time=CDateTime::GetCurrentDateTime();
  return LocalizeTime(time, format);
}

std::string CGUIInfoManager::LocalizeTime(const CDateTime &time, TIME_FORMAT format) const
{
  const std::string timeFormat = g_langInfo.GetTimeFormat();
  bool use12hourclock = timeFormat.find('h') != std::string::npos;
  switch (format)
  {
  case TIME_FORMAT_GUESS:
    return time.GetAsLocalizedTime("", false);
  case TIME_FORMAT_SS:
    return time.GetAsLocalizedTime("ss", true);
  case TIME_FORMAT_MM:
    return time.GetAsLocalizedTime("mm", true);
  case TIME_FORMAT_MM_SS:
    return time.GetAsLocalizedTime("mm:ss", true);
  case TIME_FORMAT_HH:  // this forces it to a 12 hour clock
    return time.GetAsLocalizedTime(use12hourclock ? "h" : "HH", false);
  case TIME_FORMAT_HH_MM:
    return time.GetAsLocalizedTime(use12hourclock ? "h:mm" : "HH:mm", false);
  case TIME_FORMAT_HH_MM_XX:
      return time.GetAsLocalizedTime(use12hourclock ? "h:mm xx" : "HH:mm", false);
  case TIME_FORMAT_HH_MM_SS:
    return time.GetAsLocalizedTime(use12hourclock ? "hh:mm:ss" : "HH:mm:ss", true);
  case TIME_FORMAT_HH_MM_SS_XX:
    return time.GetAsLocalizedTime(use12hourclock ? "hh:mm:ss xx" : "HH:mm:ss", true);
  case TIME_FORMAT_H:
    return time.GetAsLocalizedTime("h", false);
  case TIME_FORMAT_H_MM_SS:
    return time.GetAsLocalizedTime("h:mm:ss", true);
  case TIME_FORMAT_H_MM_SS_XX:
    return time.GetAsLocalizedTime("h:mm:ss xx", true);
  case TIME_FORMAT_XX:
    return use12hourclock ? time.GetAsLocalizedTime("xx", false) : "";
  default:
    break;
  }
  return time.GetAsLocalizedTime("", false);
}

std::string CGUIInfoManager::GetDuration(TIME_FORMAT format) const
{
  if (g_application.m_pPlayer->IsPlayingAudio() && m_currentFile->HasMusicInfoTag())
  {
    const CMusicInfoTag& tag = *m_currentFile->GetMusicInfoTag();
    if (tag.GetDuration() > 0)
      return StringUtils::SecondsToTimeString(tag.GetDuration(), format);
  }
  if (g_application.m_pPlayer->IsPlayingVideo() && !m_currentMovieDuration.empty())
    return m_currentMovieDuration;
  unsigned int iTotal = (unsigned int)g_application.GetTotalTime();
  if (iTotal > 0)
    return StringUtils::SecondsToTimeString(iTotal, format);
  return "";
}

std::string CGUIInfoManager::GetMusicPartyModeLabel(int item)
{
  // get song counts
  if (item >= MUSICPM_SONGSPLAYED && item <= MUSICPM_RANDOMSONGSPICKED)
  {
    int iSongs = -1;
    switch (item)
    {
    case MUSICPM_SONGSPLAYED:
      {
        iSongs = g_partyModeManager.GetSongsPlayed();
        break;
      }
    case MUSICPM_MATCHINGSONGS:
      {
        iSongs = g_partyModeManager.GetMatchingSongs();
        break;
      }
    case MUSICPM_MATCHINGSONGSPICKED:
      {
        iSongs = g_partyModeManager.GetMatchingSongsPicked();
        break;
      }
    case MUSICPM_MATCHINGSONGSLEFT:
      {
        iSongs = g_partyModeManager.GetMatchingSongsLeft();
        break;
      }
    case MUSICPM_RELAXEDSONGSPICKED:
      {
        iSongs = g_partyModeManager.GetRelaxedSongs();
        break;
      }
    case MUSICPM_RANDOMSONGSPICKED:
      {
        iSongs = g_partyModeManager.GetRandomSongs();
        break;
      }
    }
    if (iSongs < 0)
      return "";
    std::string strLabel = StringUtils::Format("%i", iSongs);
    return strLabel;
  }
  return "";
}

const std::string CGUIInfoManager::GetMusicPlaylistInfo(const GUIInfo& info)
{
  PLAYLIST::CPlayList& playlist = g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC);
  if (playlist.size() < 1)
    return "";
  int index = info.GetData2();
  if (info.GetData1() == 1)
  { // relative index (requires current playlist is PLAYLIST_MUSIC)
    if (g_playlistPlayer.GetCurrentPlaylist() != PLAYLIST_MUSIC)
      return "";
    index = g_playlistPlayer.GetNextSong(index);
  }
  if (index < 0 || index >= playlist.size())
    return "";
  CFileItemPtr playlistItem = playlist[index];
  if (!playlistItem->GetMusicInfoTag()->Loaded())
  {
    playlistItem->LoadMusicTag();
    playlistItem->GetMusicInfoTag()->SetLoaded();
  }
  // try to set a thumbnail
  if (!playlistItem->HasArt("thumb"))
  {
    CMusicThumbLoader loader;
    loader.LoadItem(playlistItem.get());
    // still no thumb? then just the set the default cover
    if (!playlistItem->HasArt("thumb"))
      playlistItem->SetArt("thumb", "DefaultAlbumCover.png");
  }
  if (info.m_info == MUSICPLAYER_PLAYLISTPOS)
  {
    std::string strPosition = StringUtils::Format("%i", index + 1);
    return strPosition;
  }
  else if (info.m_info == MUSICPLAYER_COVER)
    return playlistItem->GetArt("thumb");
  return GetMusicTagLabel(info.m_info, playlistItem.get());
}

std::string CGUIInfoManager::GetPlaylistLabel(int item, int playlistid /* = PLAYLIST_NONE */) const
{
  if (playlistid <= PLAYLIST_NONE && !g_application.m_pPlayer->IsPlaying())
    return "";

  int iPlaylist = playlistid == PLAYLIST_NONE ? g_playlistPlayer.GetCurrentPlaylist() : playlistid;
  switch (item)
  {
  case PLAYLIST_LENGTH:
    {
      return StringUtils::Format("%i", g_playlistPlayer.GetPlaylist(iPlaylist).size());;
    }
  case PLAYLIST_POSITION:
    {
      return StringUtils::Format("%i", g_playlistPlayer.GetCurrentSong() + 1);
    }
  case PLAYLIST_RANDOM:
    {
      if (g_playlistPlayer.IsShuffled(iPlaylist))
        return g_localizeStrings.Get(590); // 590: Random
      else
        return g_localizeStrings.Get(591); // 591: Off
    }
  case PLAYLIST_REPEAT:
    {
      PLAYLIST::REPEAT_STATE state = g_playlistPlayer.GetRepeat(iPlaylist);
      if (state == PLAYLIST::REPEAT_ONE)
        return g_localizeStrings.Get(592); // 592: One
      else if (state == PLAYLIST::REPEAT_ALL)
        return g_localizeStrings.Get(593); // 593: All
      else
        return g_localizeStrings.Get(594); // 594: Off
    }
  }
  return "";
}

std::string CGUIInfoManager::GetMusicLabel(int item)
{
  if (!g_application.m_pPlayer->IsPlaying() || !m_currentFile->HasMusicInfoTag()) return "";

  switch (item)
  {
  case MUSICPLAYER_PLAYLISTLEN:
    {
      if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
        return GetPlaylistLabel(PLAYLIST_LENGTH);
    }
    break;
  case MUSICPLAYER_PLAYLISTPOS:
    {
      if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
        return GetPlaylistLabel(PLAYLIST_POSITION);
    }
    break;
  case MUSICPLAYER_BITRATE:
    {
      std::string strBitrate = "";
      if (m_audioInfo.bitrate > 0)
        strBitrate = StringUtils::Format("%i", MathUtils::round_int((double)m_audioInfo.bitrate / 1000.0));
      return strBitrate;
    }
    break;
  case MUSICPLAYER_CHANNELS:
    {
      std::string strChannels = "";
      if (m_audioInfo.channels > 0)
      {
        strChannels = StringUtils::Format("%i", m_audioInfo.channels);
      }
      return strChannels;
    }
    break;
  case MUSICPLAYER_BITSPERSAMPLE:
    {
      std::string strBitsPerSample = "";
      if (m_audioInfo.bitspersample > 0)
        strBitsPerSample = StringUtils::Format("%i", m_audioInfo.bitspersample);
      return strBitsPerSample;
    }
    break;
  case MUSICPLAYER_SAMPLERATE:
    {
      std::string strSampleRate = "";
      if (m_audioInfo.samplerate > 0)
        strSampleRate = StringUtils::Format("%.5g", ((double)m_audioInfo.samplerate / 1000.0));
      return strSampleRate;
    }
    break;
  case MUSICPLAYER_CODEC:
    {
      return StringUtils::Format("%s", m_audioInfo.audioCodecName.c_str());
    }
    break;
  case MUSICPLAYER_LYRICS:
    return GetItemLabel(m_currentFile, AddListItemProp("lyrics"));
  }
  return GetMusicTagLabel(item, m_currentFile);
}

std::string CGUIInfoManager::GetMusicTagLabel(int info, const CFileItem *item)
{
  if (!item->HasMusicInfoTag()) return "";
  const CMusicInfoTag &tag = *item->GetMusicInfoTag();
  switch (info)
  {
  case MUSICPLAYER_TITLE:
    if (tag.GetTitle().size()) { return tag.GetTitle(); }
    break;
  case MUSICPLAYER_ALBUM:
    if (tag.GetAlbum().size()) { return tag.GetAlbum(); }
    break;
  case MUSICPLAYER_ARTIST:
    if (tag.GetArtist().size()) { return StringUtils::Join(tag.GetArtist(), g_advancedSettings.m_musicItemSeparator); }
    break;
  case MUSICPLAYER_ALBUM_ARTIST:
    if (tag.GetAlbumArtist().size()) { return StringUtils::Join(tag.GetAlbumArtist(), g_advancedSettings.m_musicItemSeparator); }
    break;
  case MUSICPLAYER_YEAR:
    if (tag.GetYear()) { return tag.GetYearString(); }
    break;
  case MUSICPLAYER_GENRE:
    if (tag.GetGenre().size()) { return StringUtils::Join(tag.GetGenre(), g_advancedSettings.m_musicItemSeparator); }
    break;
  case MUSICPLAYER_LYRICS:
    if (tag.GetLyrics().size()) { return tag.GetLyrics(); }
  break;
  case MUSICPLAYER_TRACK_NUMBER:
    {
      std::string strTrack;
      if (tag.Loaded() && tag.GetTrackNumber() > 0)
      {
        return StringUtils::Format("%02i", tag.GetTrackNumber());
      }
    }
    break;
  case MUSICPLAYER_DISC_NUMBER:
    return GetItemLabel(item, LISTITEM_DISC_NUMBER);
  case MUSICPLAYER_RATING:
    return GetItemLabel(item, LISTITEM_RATING);
  case MUSICPLAYER_COMMENT:
    return GetItemLabel(item, LISTITEM_COMMENT);
  case MUSICPLAYER_DURATION:
    return GetItemLabel(item, LISTITEM_DURATION);
  case MUSICPLAYER_CHANNEL_NAME:
    {
      if (m_currentFile->HasPVRChannelInfoTag())
        return m_currentFile->GetPVRChannelInfoTag()->ChannelName();
    }
    break;
  case MUSICPLAYER_CHANNEL_NUMBER:
    {
      if (m_currentFile->HasPVRChannelInfoTag())
        return StringUtils::Format("%i", m_currentFile->GetPVRChannelInfoTag()->ChannelNumber());
    }
    break;
  case MUSICPLAYER_SUB_CHANNEL_NUMBER:
    {
      if (m_currentFile->HasPVRChannelInfoTag())
        return StringUtils::Format("%i", m_currentFile->GetPVRChannelInfoTag()->SubChannelNumber());
    }
    break;
  case MUSICPLAYER_CHANNEL_NUMBER_LBL:
    {
      if (m_currentFile->HasPVRChannelInfoTag())
        return m_currentFile->GetPVRChannelInfoTag()->FormattedChannelNumber();
    }
    break;
  case MUSICPLAYER_CHANNEL_GROUP:
    {
      if (m_currentFile->HasPVRChannelInfoTag() && m_currentFile->GetPVRChannelInfoTag()->IsRadio())
        return g_PVRManager.GetPlayingGroup(true)->GroupName();
    }
    break;
  case MUSICPLAYER_PLAYCOUNT:
    return GetItemLabel(item, LISTITEM_PLAYCOUNT);
  case MUSICPLAYER_LASTPLAYED:
    return GetItemLabel(item, LISTITEM_LASTPLAYED);
  }
  return "";
}

std::string CGUIInfoManager::GetVideoLabel(int item)
{
  if (!g_application.m_pPlayer->IsPlaying())
    return "";

  if (item == VIDEOPLAYER_TITLE)
  {
    if(g_application.m_pPlayer->IsPlayingVideo())
       return GetLabel(PLAYER_TITLE);
  }
  else if (item == VIDEOPLAYER_PLAYLISTLEN)
  {
    if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
      return GetPlaylistLabel(PLAYLIST_LENGTH);
  }
  else if (item == VIDEOPLAYER_PLAYLISTPOS)
  {
    if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
      return GetPlaylistLabel(PLAYLIST_POSITION);
  }
  else if (m_currentFile->HasPVRChannelInfoTag())
  {
    CPVRChannelPtr tag(m_currentFile->GetPVRChannelInfoTag());
    CEpgInfoTagPtr epgTag;

    switch (item)
    {
    /* Now playing infos */
    case VIDEOPLAYER_TITLE:
      epgTag = tag->GetEPGNow();
      return epgTag ?
          epgTag->Title() :
          CSettings::Get().GetBool("epg.hidenoinfoavailable") ?
                            "" : g_localizeStrings.Get(19055); // no information available
    case VIDEOPLAYER_GENRE:
      epgTag = tag->GetEPGNow();
      return epgTag ? StringUtils::Join(epgTag->Genre(), g_advancedSettings.m_videoItemSeparator) : "";
    case VIDEOPLAYER_PLOT:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->Plot() : "";
    case VIDEOPLAYER_PLOT_OUTLINE:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->PlotOutline() : "";
    case VIDEOPLAYER_STARTTIME:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->StartAsLocalTime().GetAsLocalizedTime("", false) : CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
    case VIDEOPLAYER_ENDTIME:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->EndAsLocalTime().GetAsLocalizedTime("", false) : CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
    case VIDEOPLAYER_IMDBNUMBER:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->IMDBNumber() : "";
    case VIDEOPLAYER_ORIGINALTITLE:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->OriginalTitle() : "";
    case VIDEOPLAYER_YEAR:
      epgTag = tag->GetEPGNow();
      if (epgTag && epgTag->Year() > 0)
        return StringUtils::Format("%i", epgTag->Year());
      break;
    case VIDEOPLAYER_EPISODE:
      epgTag = tag->GetEPGNow();
      if (epgTag && epgTag->EpisodeNumber() > 0)
      {
        if (epgTag->SeriesNumber() == 0) // prefix episode with 'S'
          return StringUtils::Format("S%i", epgTag->EpisodeNumber());
        else
          return StringUtils::Format("%i", epgTag->EpisodeNumber());
      }
      break;
    case VIDEOPLAYER_SEASON:
      epgTag = tag->GetEPGNow();
      if (epgTag && epgTag->SeriesNumber() > 0)
        return StringUtils::Format("%i", epgTag->SeriesNumber());
      break;
    case VIDEOPLAYER_EPISODENAME:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->EpisodeName() : "";
    case VIDEOPLAYER_CAST:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->Cast() : "";
    case VIDEOPLAYER_DIRECTOR:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->Director() : "";
    case VIDEOPLAYER_WRITER:
      epgTag = tag->GetEPGNow();
      return epgTag ? epgTag->Writer() : "";

    /* Next playing infos */
    case VIDEOPLAYER_NEXT_TITLE:
      epgTag = tag->GetEPGNext();
      return epgTag ?
          epgTag->Title() :
          CSettings::Get().GetBool("epg.hidenoinfoavailable") ?
                            "" : g_localizeStrings.Get(19055); // no information available
    case VIDEOPLAYER_NEXT_GENRE:
      epgTag = tag->GetEPGNext();
      return epgTag ? StringUtils::Join(epgTag->Genre(), g_advancedSettings.m_videoItemSeparator) : "";
    case VIDEOPLAYER_NEXT_PLOT:
      epgTag = tag->GetEPGNext();
      return epgTag ? epgTag->Plot() : "";
    case VIDEOPLAYER_NEXT_PLOT_OUTLINE:
      epgTag = tag->GetEPGNext();
      return epgTag ? epgTag->PlotOutline() : "";
    case VIDEOPLAYER_NEXT_STARTTIME:
      epgTag = tag->GetEPGNext();
      return epgTag ? epgTag->StartAsLocalTime().GetAsLocalizedTime("", false) : CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
    case VIDEOPLAYER_NEXT_ENDTIME:
      epgTag = tag->GetEPGNext();
      return epgTag ? epgTag->EndAsLocalTime().GetAsLocalizedTime("", false) : CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
    case VIDEOPLAYER_NEXT_DURATION:
      {
        std::string duration;
        epgTag = tag->GetEPGNext();
        if (epgTag && epgTag->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(epgTag->GetDuration());
        return duration;
      }

    case VIDEOPLAYER_PARENTAL_RATING:
      {
        std::string rating;
        epgTag = tag->GetEPGNow();
        if (epgTag && epgTag->ParentalRating() > 0)
          rating = StringUtils::Format("%i", epgTag->ParentalRating());
        return rating;
      }
      break;

    /* General channel infos */
    case VIDEOPLAYER_CHANNEL_NAME:
      return tag->ChannelName();

    case VIDEOPLAYER_CHANNEL_NUMBER:
      return StringUtils::Format("%i", tag->ChannelNumber());

    case VIDEOPLAYER_SUB_CHANNEL_NUMBER:
      return StringUtils::Format("%i", tag->SubChannelNumber());

    case VIDEOPLAYER_CHANNEL_NUMBER_LBL:
      return tag->FormattedChannelNumber();

    case VIDEOPLAYER_CHANNEL_GROUP:
      {
        if (tag && !tag->IsRadio())
          return g_PVRManager.GetPlayingTVGroupName();
      }
    }
  }
  else if (m_currentFile->HasVideoInfoTag())
  {
    switch (item)
    {
    case VIDEOPLAYER_ORIGINALTITLE:
      return m_currentFile->GetVideoInfoTag()->m_strOriginalTitle;
      break;
    case VIDEOPLAYER_GENRE:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_genre, g_advancedSettings.m_videoItemSeparator);
      break;
    case VIDEOPLAYER_DIRECTOR:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_director, g_advancedSettings.m_videoItemSeparator);
      break;
    case VIDEOPLAYER_IMDBNUMBER:
      return m_currentFile->GetVideoInfoTag()->m_strIMDBNumber;
    case VIDEOPLAYER_RATING:
      {
        std::string strRating;
        if (m_currentFile->GetVideoInfoTag()->m_fRating > 0.f)
          strRating = StringUtils::Format("%.1f", m_currentFile->GetVideoInfoTag()->m_fRating);
        return strRating;
      }
      break;
    case VIDEOPLAYER_RATING_AND_VOTES:
      {
        std::string strRatingAndVotes;
        if (m_currentFile->GetVideoInfoTag()->m_fRating > 0.f)
        {
          if (m_currentFile->GetVideoInfoTag()->m_strVotes.empty())
            strRatingAndVotes = StringUtils::Format("%.1f",
                                                    m_currentFile->GetVideoInfoTag()->m_fRating);
          else
            strRatingAndVotes = StringUtils::Format("%.1f (%s %s)",
                                                    m_currentFile->GetVideoInfoTag()->m_fRating,
                                                    m_currentFile->GetVideoInfoTag()->m_strVotes.c_str(),
                                                    g_localizeStrings.Get(20350).c_str());
        }
        return strRatingAndVotes;
      }
      break;
    case VIDEOPLAYER_VOTES:
      return m_currentFile->GetVideoInfoTag()->m_strVotes;
    case VIDEOPLAYER_YEAR:
      {
        std::string strYear;
        if (m_currentFile->GetVideoInfoTag()->m_iYear > 0)
          strYear = StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->m_iYear);
        return strYear;
      }
      break;
    case VIDEOPLAYER_PREMIERED:
      {
        CDateTime dateTime;
        if (m_currentFile->GetVideoInfoTag()->m_firstAired.IsValid())
          dateTime = m_currentFile->GetVideoInfoTag()->m_firstAired;
        else if (m_currentFile->GetVideoInfoTag()->m_premiered.IsValid())
          dateTime = m_currentFile->GetVideoInfoTag()->m_premiered;

        if (dateTime.IsValid())
          return dateTime.GetAsLocalizedDate();
        break;
      }
      break;
    case VIDEOPLAYER_PLOT:
      return m_currentFile->GetVideoInfoTag()->m_strPlot;
    case VIDEOPLAYER_TRAILER:
      return m_currentFile->GetVideoInfoTag()->m_strTrailer;
    case VIDEOPLAYER_PLOT_OUTLINE:
      return m_currentFile->GetVideoInfoTag()->m_strPlotOutline;
    case VIDEOPLAYER_EPISODE:
      if (m_currentFile->GetVideoInfoTag()->m_iEpisode > 0)
      {
        std::string strEpisode;
        if (m_currentFile->GetVideoInfoTag()->m_iSeason == 0) // prefix episode with 'S'
          strEpisode = StringUtils::Format("S%i", m_currentFile->GetVideoInfoTag()->m_iEpisode);
        else 
          strEpisode = StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->m_iEpisode);
        return strEpisode;
      }
      break;
    case VIDEOPLAYER_SEASON:
      if (m_currentFile->GetVideoInfoTag()->m_iSeason > 0)
      {
        return StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->m_iSeason);
      }
      break;
    case VIDEOPLAYER_TVSHOW:
      return m_currentFile->GetVideoInfoTag()->m_strShowTitle;

    case VIDEOPLAYER_STUDIO:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_studio, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_COUNTRY:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_country, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_MPAA:
      return m_currentFile->GetVideoInfoTag()->m_strMPAARating;
    case VIDEOPLAYER_TOP250:
      {
        std::string strTop250;
        if (m_currentFile->GetVideoInfoTag()->m_iTop250 > 0)
          strTop250 = StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->m_iTop250);
        return strTop250;
      }
      break;
    case VIDEOPLAYER_CAST:
      return m_currentFile->GetVideoInfoTag()->GetCast();
    case VIDEOPLAYER_CAST_AND_ROLE:
      return m_currentFile->GetVideoInfoTag()->GetCast(true);
    case VIDEOPLAYER_ARTIST:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_ALBUM:
      return m_currentFile->GetVideoInfoTag()->m_strAlbum;
    case VIDEOPLAYER_WRITER:
      return StringUtils::Join(m_currentFile->GetVideoInfoTag()->m_writingCredits, g_advancedSettings.m_videoItemSeparator);
    case VIDEOPLAYER_TAGLINE:
      return m_currentFile->GetVideoInfoTag()->m_strTagLine;
    case VIDEOPLAYER_LASTPLAYED:
      {
        if (m_currentFile->GetVideoInfoTag()->m_lastPlayed.IsValid())
          return m_currentFile->GetVideoInfoTag()->m_lastPlayed.GetAsLocalizedDateTime();
        break;
      }
    case VIDEOPLAYER_PLAYCOUNT:
      {
        std::string strPlayCount;
        if (m_currentFile->GetVideoInfoTag()->m_playCount > 0)
          strPlayCount = StringUtils::Format("%i", m_currentFile->GetVideoInfoTag()->m_playCount);
        return strPlayCount;
      }
    }
  }
  return "";
}

int64_t CGUIInfoManager::GetPlayTime() const
{
  if (g_application.m_pPlayer->IsPlaying())
  {
    int64_t lPTS = (int64_t)(g_application.GetTime() * 1000);
    if (lPTS < 0) lPTS = 0;
    return lPTS;
  }
  return 0;
}

std::string CGUIInfoManager::GetCurrentPlayTime(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  if (g_application.m_pPlayer->IsPlaying())
    return StringUtils::SecondsToTimeString((int)(GetPlayTime()/1000), format);
  return "";
}

std::string CGUIInfoManager::GetCurrentSeekTime(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  return StringUtils::SecondsToTimeString(g_application.GetTime() + CSeekHandler::Get().GetSeekSize(), format);
}

int CGUIInfoManager::GetTotalPlayTime() const
{
  int iTotalTime = (int)g_application.GetTotalTime();
  return iTotalTime > 0 ? iTotalTime : 0;
}

int CGUIInfoManager::GetPlayTimeRemaining() const
{
  int iReverse = GetTotalPlayTime() - (int)g_application.GetTime();
  return iReverse > 0 ? iReverse : 0;
}

float CGUIInfoManager::GetSeekPercent() const
{
  if (g_infoManager.GetTotalPlayTime() == 0)
    return 0.0f;

  float percentPlayTime = static_cast<float>(GetPlayTime()) / GetTotalPlayTime() * 0.1f;
  float percentPerSecond = 100.0f / static_cast<float>(GetTotalPlayTime());
  float percent = percentPlayTime + percentPerSecond * CSeekHandler::Get().GetSeekSize();

  if (percent > 100.0f)
    percent = 100.0f;
  if (percent < 0.0f)
    percent = 0.0f;

  return percent;
}

std::string CGUIInfoManager::GetCurrentPlayTimeRemaining(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  int timeRemaining = GetPlayTimeRemaining();
  if (timeRemaining && g_application.m_pPlayer->IsPlaying())
    return StringUtils::SecondsToTimeString(timeRemaining, format);
  return "";
}

void CGUIInfoManager::ResetCurrentItem()
{
  m_currentFile->Reset();
  m_currentMovieThumb = "";
  m_currentMovieDuration = "";
}

void CGUIInfoManager::SetCurrentItem(CFileItem &item)
{
  ResetCurrentItem();

  if (item.IsAudio())
    SetCurrentSong(item);
  else
    SetCurrentMovie(item);

  if (item.HasEPGInfoTag())
    m_currentFile->SetEPGInfoTag(item.GetEPGInfoTag());
  else if (item.HasPVRChannelInfoTag())
  {
    CEpgInfoTagPtr tag(item.GetPVRChannelInfoTag()->GetEPGNow());
    if (tag)
      m_currentFile->SetEPGInfoTag(tag);
  }

  SetChanged();
  NotifyObservers(ObservableMessageCurrentItem);
}

void CGUIInfoManager::SetCurrentAlbumThumb(const std::string &thumbFileName)
{
  if (CFile::Exists(thumbFileName))
    m_currentFile->SetArt("thumb", thumbFileName);
  else
  {
    m_currentFile->SetArt("thumb", "");
    m_currentFile->FillInDefaultIcon();
  }
}

void CGUIInfoManager::SetCurrentSong(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentSong(%s)",item.GetPath().c_str());
  *m_currentFile = item;

  m_currentFile->LoadMusicTag();
  if (m_currentFile->GetMusicInfoTag()->GetTitle().empty())
  {
    // No title in tag, show filename only
    m_currentFile->GetMusicInfoTag()->SetTitle(CUtil::GetTitleFromPath(m_currentFile->GetPath()));
  }
  m_currentFile->GetMusicInfoTag()->SetLoaded(true);

  // find a thumb for this file.
  if (m_currentFile->IsInternetStream())
  {
    if (!g_application.m_strPlayListFile.empty())
    {
      CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
      CFileItem streamingItem(g_application.m_strPlayListFile,false);

      CMusicThumbLoader loader;
      loader.FillThumb(streamingItem);
      if (streamingItem.HasArt("thumb"))
        m_currentFile->SetArt("thumb", streamingItem.GetArt("thumb"));
    }
  }
  else
  {
    CMusicThumbLoader loader;
    loader.LoadItem(m_currentFile);
  }
  m_currentFile->FillInDefaultIcon();

  CMusicInfoLoader::LoadAdditionalTagInfo(m_currentFile);
}

void CGUIInfoManager::SetCurrentMovie(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentMovie(%s)", CURL::GetRedacted(item.GetPath()).c_str());
  *m_currentFile = item;

  /* also call GetMovieInfo() when a VideoInfoTag is already present or additional info won't be present in the tag */
  if (!m_currentFile->HasPVRChannelInfoTag())
  {
    CVideoDatabase dbs;
    if (dbs.Open())
    {
      std::string path = item.GetPath();
      std::string videoInfoTagPath(item.GetVideoInfoTag()->m_strFileNameAndPath);
      if (videoInfoTagPath.find("removable://") == 0)
        path = videoInfoTagPath;
      dbs.LoadVideoInfo(path, *m_currentFile->GetVideoInfoTag());
      dbs.Close();
    }
  }

  // Find a thumb for this file.
  if (!item.HasArt("thumb"))
  {
    CVideoThumbLoader loader;
    loader.LoadItem(m_currentFile);
  }

  // find a thumb for this stream
  if (item.IsInternetStream())
  {
    // case where .strm is used to start an audio stream
    if (g_application.m_pPlayer->IsPlayingAudio())
    {
      SetCurrentSong(item);
      return;
    }

    // else its a video
    if (!g_application.m_strPlayListFile.empty())
    {
      CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
      CFileItem thumbItem(g_application.m_strPlayListFile,false);

      CVideoThumbLoader loader;
      if (loader.FillThumb(thumbItem))
        item.SetArt("thumb", thumbItem.GetArt("thumb"));
    }
  }

  item.FillInDefaultIcon();
  m_currentMovieThumb = item.GetArt("thumb");
}

string CGUIInfoManager::GetSystemHeatInfo(int info)
{
  if (CTimeUtils::GetFrameTime() - m_lastSysHeatInfoTime >= SYSHEATUPDATEINTERVAL)
  { // update our variables
    m_lastSysHeatInfoTime = CTimeUtils::GetFrameTime();
#if defined(TARGET_POSIX)
    g_cpuInfo.getTemperature(m_cpuTemp);
    m_gpuTemp = GetGPUTemperature();
#endif
  }

  std::string text;
  switch(info)
  {
    case SYSTEM_CPU_TEMPERATURE:
      return m_cpuTemp.IsValid() ? g_langInfo.GetTemperatureAsString(m_cpuTemp) : "?";
      break;
    case SYSTEM_GPU_TEMPERATURE:
      return m_gpuTemp.IsValid() ? g_langInfo.GetTemperatureAsString(m_gpuTemp) : "?";
      break;
    case SYSTEM_FAN_SPEED:
      text = StringUtils::Format("%i%%", m_fanSpeed * 2);
      break;
    case SYSTEM_CPU_USAGE:
#if defined(TARGET_DARWIN_OSX)
      text = StringUtils::Format("%4.2f%%", m_resourceCounter.GetCPUUsage());
#elif defined(TARGET_DARWIN) || defined(TARGET_WINDOWS)
      text = StringUtils::Format("%d%%", g_cpuInfo.getUsedPercentage());
#else
      text = StringUtils::Format("%s", g_cpuInfo.GetCoresUsageString().c_str());
#endif
      break;
  }
  return text;
}

CTemperature CGUIInfoManager::GetGPUTemperature()
{
  int  value = 0;
  char scale = 0;

#if defined(TARGET_DARWIN_OSX)
  value = SMCGetTemperature(SMC_KEY_GPU_TEMP);
  return CTemperature::CreateFromCelsius(value);
#else
  std::string  cmd   = g_advancedSettings.m_gpuTempCmd;
  int         ret   = 0;
  FILE        *p    = NULL;

  if (cmd.empty() || !(p = popen(cmd.c_str(), "r")))
    return CTemperature();

  ret = fscanf(p, "%d %c", &value, &scale);
  pclose(p);

  if (ret != 2)
    return CTemperature();
#endif

  if (scale == 'C' || scale == 'c')
    return CTemperature::CreateFromCelsius(value);
  if (scale == 'F' || scale == 'f')
    return CTemperature::CreateFromFahrenheit(value);
  return CTemperature();
}

// Version string MUST NOT contain spaces.  It is used
// in the HTTP request user agent.
std::string CGUIInfoManager::GetVersionShort(void)
{
  if (strlen(CCompileInfo::GetSuffix()) == 0)
    return StringUtils::Format("%d.%d", CCompileInfo::GetMajor(), CCompileInfo::GetMinor());
  else
    return StringUtils::Format("%d.%d-%s", CCompileInfo::GetMajor(), CCompileInfo::GetMinor(), CCompileInfo::GetSuffix());
}

std::string CGUIInfoManager::GetVersion()
{
  return GetVersionShort() + " Git:" + CCompileInfo::GetSCMID();
}

std::string CGUIInfoManager::GetBuild()
{
  return StringUtils::Format("%s", __DATE__);
}

std::string CGUIInfoManager::GetAppName()
{
  return CCompileInfo::GetAppName();
}

void CGUIInfoManager::SetDisplayAfterSeek(unsigned int timeOut, int seekOffset)
{
  if (timeOut>0)
  {
    m_AfterSeekTimeout = CTimeUtils::GetFrameTime() +  timeOut;
    if (seekOffset)
      m_seekOffset = seekOffset;
  }
  else
    m_AfterSeekTimeout = 0;
}

bool CGUIInfoManager::GetDisplayAfterSeek()
{
  if (CTimeUtils::GetFrameTime() < m_AfterSeekTimeout)
    return true;
  m_seekOffset = 0;
  return false;
}

void CGUIInfoManager::Clear()
{
  CSingleLock lock(m_critInfo);
  m_skinVariableStrings.clear();

  /*
    Erase any info bools that are unused. We do this repeatedly as each run
    will remove those bools that are no longer dependencies of other bools
    in the vector.
   */
  vector<InfoPtr>::iterator i = remove_if(m_bools.begin(), m_bools.end(), std::mem_fun_ref(&InfoPtr::unique));
  while (i != m_bools.end())
  {
    m_bools.erase(i, m_bools.end());
    i = remove_if(m_bools.begin(), m_bools.end(), std::mem_fun_ref(&InfoPtr::unique));
  }
  // log which ones are used - they should all be gone by now
  for (vector<InfoPtr>::const_iterator i = m_bools.begin(); i != m_bools.end(); ++i)
    CLog::Log(LOGDEBUG, "Infobool '%s' still used by %u instances", (*i)->GetExpression().c_str(), (unsigned int) i->use_count());
}

void CGUIInfoManager::UpdateFPS()
{
  m_frameCounter++;
  unsigned int curTime = CTimeUtils::GetFrameTime();

  float fTimeSpan = (float)(curTime - m_lastFPSTime);
  if (fTimeSpan >= 1000.0f)
  {
    fTimeSpan /= 1000.0f;
    m_fps = m_frameCounter / fTimeSpan;
    m_lastFPSTime = curTime;
    m_frameCounter = 0;
  }
}

void CGUIInfoManager::UpdateAVInfo()
{
  if(g_application.m_pPlayer->IsPlaying())
  {
    if (g_dataCacheCore.HasAVInfoChanges())
    {
      SPlayerVideoStreamInfo video;
      SPlayerAudioStreamInfo audio;

      g_application.m_pPlayer->GetVideoStreamInfo(video);
      g_application.m_pPlayer->GetAudioStreamInfo(CURRENT_STREAM, audio);

      m_videoInfo = video;
      m_audioInfo = audio;
    }
  }
}

int CGUIInfoManager::AddListItemProp(const std::string &str, int offset)
{
  for (int i=0; i < (int)m_listitemProperties.size(); i++)
    if (m_listitemProperties[i] == str)
      return (LISTITEM_PROPERTY_START+offset + i);

  if (m_listitemProperties.size() < LISTITEM_PROPERTY_END - LISTITEM_PROPERTY_START)
  {
    m_listitemProperties.push_back(str);
    return LISTITEM_PROPERTY_START + offset + m_listitemProperties.size() - 1;
  }

  CLog::Log(LOGERROR,"%s - not enough listitem property space!", __FUNCTION__);
  return 0;
}

int CGUIInfoManager::AddMultiInfo(const GUIInfo &info)
{
  // check to see if we have this info already
  for (unsigned int i = 0; i < m_multiInfo.size(); i++)
    if (m_multiInfo[i] == info)
      return (int)i + MULTI_INFO_START;
  // return the new offset
  m_multiInfo.push_back(info);
  int id = (int)m_multiInfo.size() + MULTI_INFO_START - 1;
  if (id > MULTI_INFO_END)
    CLog::Log(LOGERROR, "%s - too many multiinfo bool/labels in this skin", __FUNCTION__);
  return id;
}

int CGUIInfoManager::ConditionalStringParameter(const std::string &parameter, bool caseSensitive /*= false*/)
{
  // check to see if we have this parameter already
  if (caseSensitive)
  {
    vector<string>::const_iterator i = find(m_stringParameters.begin(), m_stringParameters.end(), parameter);
    if (i != m_stringParameters.end())
      return (int)distance<vector<string>::const_iterator>(m_stringParameters.begin(), i);
  }
  else
  {
    for (unsigned int i = 0; i < m_stringParameters.size(); i++)
      if (StringUtils::EqualsNoCase(parameter, m_stringParameters[i]))
        return (int)i;
  }

  // return the new offset
  m_stringParameters.push_back(parameter);
  return (int)m_stringParameters.size() - 1;
}

bool CGUIInfoManager::GetItemInt(int &value, const CGUIListItem *item, int info) const
{
  if (!item)
  {
    value = 0;
    return false;
  }

  if (info >= LISTITEM_PROPERTY_START && info - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { // grab the property
    std::string property = m_listitemProperties[info - LISTITEM_PROPERTY_START];
    std::string val = item->GetProperty(property).asString();
    value = atoi(val.c_str());
    return true;
  }

  switch (info)
  {
    case LISTITEM_PROGRESS:
    {
      value = 0;
      if (item->IsFileItem())
      {
        const CFileItem *pItem = (const CFileItem *)item;
        if (pItem && pItem->HasPVRChannelInfoTag())
        {
          CEpgInfoTagPtr epgNow(pItem->GetPVRChannelInfoTag()->GetEPGNow());
          if (epgNow)
            value = (int) epgNow->ProgressPercentage();
        }
        else if (pItem && pItem->HasEPGInfoTag())
        {
          value = (int) pItem->GetEPGInfoTag()->ProgressPercentage();
        }
      }

      return true;
    }
    break;
  case LISTITEM_PERCENT_PLAYED:
    if (item->IsFileItem() && ((const CFileItem *)item)->HasVideoInfoTag() && ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.IsPartWay())
      value = (int)(100 * ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.timeInSeconds / ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.totalTimeInSeconds);
    else if (item->IsFileItem() && ((const CFileItem *)item)->HasPVRRecordingInfoTag() && ((const CFileItem *)item)->GetPVRRecordingInfoTag()->m_resumePoint.IsPartWay())
      value = (int)(100 * ((const CFileItem *)item)->GetPVRRecordingInfoTag()->m_resumePoint.timeInSeconds / ((const CFileItem *)item)->GetPVRRecordingInfoTag()->m_resumePoint.totalTimeInSeconds);
    else
      value = 0;
    return true;
  }

  value = 0;
  return false;
}

std::string CGUIInfoManager::GetItemLabel(const CFileItem *item, int info, std::string *fallback)
{
  if (!item) return "";

  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, false, item);

  if (info >= LISTITEM_PROPERTY_START + LISTITEM_ART_OFFSET && info - (LISTITEM_PROPERTY_START + LISTITEM_ART_OFFSET) < (int)m_listitemProperties.size())
  { // grab the art
    std::string art = m_listitemProperties[info - (LISTITEM_PROPERTY_START + LISTITEM_ART_OFFSET)];
    return item->GetArt(art);
  }

  if (info >= LISTITEM_PROPERTY_START && info - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { // grab the property
    std::string property = m_listitemProperties[info - LISTITEM_PROPERTY_START];
    return item->GetProperty(property).asString();
  }

  if (info >= LISTITEM_PICTURE_START && info <= LISTITEM_PICTURE_END && item->HasPictureInfoTag())
    return item->GetPictureInfoTag()->GetInfo(picture_slide_map[info - LISTITEM_PICTURE_START]);

  switch (info)
  {
  case LISTITEM_LABEL:
    return item->GetLabel();
  case LISTITEM_LABEL2:
    return item->GetLabel2();
  case LISTITEM_TITLE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr epgTag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return epgTag ?
          epgTag->Title() :
          CSettings::Get().GetBool("epg.hidenoinfoavailable") ?
                            "" : g_localizeStrings.Get(19055); // no information available
    }
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->m_strTitle;
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Title();
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->Title();
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strTitle;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetTitle();
    break;
  case LISTITEM_ORIGINALTITLE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      if (tag)
        return tag->OriginalTitle();
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->OriginalTitle();
    if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->HasEpgInfoTag())
      return item->GetPVRTimerInfoTag()->GetEpgInfoTag()->OriginalTitle();
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strOriginalTitle;
    break;
  case LISTITEM_PLAYCOUNT:
    {
      std::string strPlayCount;
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_playCount > 0)
        strPlayCount = StringUtils::Format("%i", item->GetVideoInfoTag()->m_playCount);
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetPlayCount() > 0)
        strPlayCount = StringUtils::Format("%i", item->GetMusicInfoTag()->GetPlayCount());
      return strPlayCount;
    }
  case LISTITEM_LASTPLAYED:
    {
      CDateTime dateTime;
      if (item->HasVideoInfoTag())
        dateTime = item->GetVideoInfoTag()->m_lastPlayed;
      else if (item->HasMusicInfoTag())
        dateTime = item->GetMusicInfoTag()->GetLastPlayed();

      if (dateTime.IsValid())
        return dateTime.GetAsLocalizedDate();
      break;
    }
  case LISTITEM_TRACKNUMBER:
    {
      std::string track;
      if (item->HasMusicInfoTag())
        track = StringUtils::Format("%i", item->GetMusicInfoTag()->GetTrackNumber());
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iTrack > -1 )
        track = StringUtils::Format("%i", item->GetVideoInfoTag()->m_iTrack);
      return track;
    }
  case LISTITEM_DISC_NUMBER:
    {
      std::string disc;
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDiscNumber() > 0)
        disc = StringUtils::Format("%i", item->GetMusicInfoTag()->GetDiscNumber());
      return disc;
    }
  case LISTITEM_ARTIST:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_artist, g_advancedSettings.m_videoItemSeparator);
    if (item->HasMusicInfoTag())
      return StringUtils::Join(item->GetMusicInfoTag()->GetArtist(), g_advancedSettings.m_musicItemSeparator);
    break;
  case LISTITEM_ALBUM_ARTIST:
    if (item->HasMusicInfoTag())
      return StringUtils::Join(item->GetMusicInfoTag()->GetAlbumArtist(), g_advancedSettings.m_musicItemSeparator);
    break;
  case LISTITEM_DIRECTOR:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      if (tag)
        return tag->Director();
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Director();
    if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->HasEpgInfoTag())
      return item->GetPVRTimerInfoTag()->GetEpgInfoTag()->Director();
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_director, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_ALBUM:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strAlbum;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetAlbum();
    break;
  case LISTITEM_YEAR:
    {
      std::string year;
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iYear > 0)
        year = StringUtils::Format("%i", item->GetVideoInfoTag()->m_iYear);
      if (item->HasMusicInfoTag())
        year = item->GetMusicInfoTag()->GetYearString();
      if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->Year() > 0)
        year = StringUtils::Format("%i", item->GetEPGInfoTag()->Year());
      if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->HasEpgInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
        if (tag->Year() > 0)
          year = StringUtils::Format("%i", tag->Year());
      }
      return year;
    }
  case LISTITEM_PREMIERED:
    if (item->HasVideoInfoTag())
    {
      CDateTime dateTime;
      if (item->GetVideoInfoTag()->m_firstAired.IsValid())
        dateTime = item->GetVideoInfoTag()->m_firstAired;
      else if (item->GetVideoInfoTag()->m_premiered.IsValid())
        dateTime = item->GetVideoInfoTag()->m_premiered;

      if (dateTime.IsValid())
        return dateTime.GetAsLocalizedDate();
    }
    else if (item->HasEPGInfoTag())
    {
      if (item->GetEPGInfoTag()->FirstAiredAsLocalTime().IsValid())
        return item->GetEPGInfoTag()->FirstAiredAsLocalTime().GetAsLocalizedDate(true);
    }
    else if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->HasEpgInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
      if (tag->FirstAiredAsLocalTime().IsValid())
        return tag->FirstAiredAsLocalTime().GetAsLocalizedDate(true);
    }
    break;
  case LISTITEM_GENRE:
    if (item->HasPVRRecordingInfoTag())
      return StringUtils::Join(item->GetPVRRecordingInfoTag()->m_genre, g_advancedSettings.m_videoItemSeparator);
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr epgTag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return epgTag ? StringUtils::Join(epgTag->Genre(), g_advancedSettings.m_videoItemSeparator) : "";
    }
    if (item->HasEPGInfoTag())
      return StringUtils::Join(item->GetEPGInfoTag()->Genre(), g_advancedSettings.m_videoItemSeparator);
    if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->HasEpgInfoTag())
      return StringUtils::Join(item->GetPVRTimerInfoTag()->GetEpgInfoTag()->Genre(), g_advancedSettings.m_videoItemSeparator);
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_genre, g_advancedSettings.m_videoItemSeparator);
    if (item->HasMusicInfoTag())
      return StringUtils::Join(item->GetMusicInfoTag()->GetGenre(), g_advancedSettings.m_musicItemSeparator);
    break;
  case LISTITEM_FILENAME:
  case LISTITEM_FILE_EXTENSION:
    {
      std::string strFile;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        strFile = URIUtils::GetFileName(item->GetMusicInfoTag()->GetURL());
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
        strFile = URIUtils::GetFileName(item->GetVideoInfoTag()->m_strFileNameAndPath);
      else
        strFile = URIUtils::GetFileName(item->GetPath());

      if (info==LISTITEM_FILE_EXTENSION)
      {
        std::string strExtension = URIUtils::GetExtension(strFile);
        return StringUtils::TrimLeft(strExtension, ".");
      }
      return strFile;
    }
    break;
  case LISTITEM_DATE:
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->StartAsLocalTime().GetAsLocalizedDateTime(false, false);
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr epgTag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return epgTag ? epgTag->StartAsLocalTime().GetAsLocalizedDateTime(false, false) : CDateTime::GetCurrentDateTime().GetAsLocalizedDateTime(false, false);
    }
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->RecordingTimeAsLocalTime().GetAsLocalizedDateTime(false, false);
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->Summary();
    if (item->m_dateTime.IsValid())
      return item->m_dateTime.GetAsLocalizedDate();
    break;
  case LISTITEM_SIZE:
    if (!item->m_bIsFolder || item->m_dwSize)
      return StringUtils::SizeToString(item->m_dwSize);
    break;
  case LISTITEM_RATING:
    {
      std::string rating;
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_fRating > 0.f) // movie rating
        rating = StringUtils::Format("%.1f", item->GetVideoInfoTag()->m_fRating);
      else if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetRating() > '0')
      { // song rating.  Images will probably be better than numbers for this in the long run
        rating.assign(1, item->GetMusicInfoTag()->GetRating());
      }
      return rating;
    }
  case LISTITEM_RATING_AND_VOTES:
    {
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_fRating > 0.f) // movie rating
      {
        std::string strRatingAndVotes;
        if (item->GetVideoInfoTag()->m_strVotes.empty())
          strRatingAndVotes = StringUtils::Format("%.1f",
                                                  item->GetVideoInfoTag()->m_fRating);
        else
          strRatingAndVotes = StringUtils::Format("%.1f (%s %s)",
                                                  item->GetVideoInfoTag()->m_fRating,
                                                  item->GetVideoInfoTag()->m_strVotes.c_str(),
                                                  g_localizeStrings.Get(20350).c_str());
        return strRatingAndVotes;
      }
    }
    break;
  case LISTITEM_VOTES:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strVotes;
    break;
  case LISTITEM_PROGRAM_COUNT:
    {
      return StringUtils::Format("%i", item->m_iprogramCount);;
    }
  case LISTITEM_DURATION:
    {
      std::string duration;
      if (item->HasPVRChannelInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
        return tag ? StringUtils::SecondsToTimeString(tag->GetDuration()) : "";
      }
      else if (item->HasPVRRecordingInfoTag())
      {
        if (item->GetPVRRecordingInfoTag()->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(item->GetPVRRecordingInfoTag()->GetDuration());
      }
      else if (item->HasEPGInfoTag())
      {
        if (item->GetEPGInfoTag()->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(item->GetEPGInfoTag()->GetDuration());
      }
      else if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->HasEpgInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
        if (tag->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(tag->GetDuration());
      }
      else if (item->HasVideoInfoTag())
      {
        if (item->GetVideoInfoTag()->GetDuration() > 0)
          duration = StringUtils::Format("%d", item->GetVideoInfoTag()->GetDuration() / 60);
      }
      else if (item->HasMusicInfoTag())
      {
        if (item->GetMusicInfoTag()->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(item->GetMusicInfoTag()->GetDuration());
      }
      return duration;
    }
  case LISTITEM_PLOT:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return tag ? tag->Plot() : "";
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Plot();
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->m_strPlot;
    if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->HasEpgInfoTag())
      return item->GetPVRTimerInfoTag()->GetEpgInfoTag()->Plot();
    if (item->HasVideoInfoTag())
    {
      if (item->GetVideoInfoTag()->m_type != MediaTypeTvShow)
        if (item->GetVideoInfoTag()->m_playCount == 0 && !CSettings::Get().GetBool("videolibrary.showunwatchedplots"))
          return g_localizeStrings.Get(20370);

      return item->GetVideoInfoTag()->m_strPlot;
    }
    break;
  case LISTITEM_PLOT_OUTLINE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return tag ? tag->PlotOutline() : "";
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->PlotOutline();
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->m_strPlotOutline;
    if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->HasEpgInfoTag())
      return item->GetPVRTimerInfoTag()->GetEpgInfoTag()->PlotOutline();
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strPlotOutline;
    break;
  case LISTITEM_EPISODE:
    {
      int iSeason = -1, iEpisode = -1;
      if (item->HasPVRChannelInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
        if (tag)
        {
          if (tag->SeriesNumber() > 0)
            iSeason = tag->SeriesNumber();
          if (tag->EpisodeNumber() > 0)
            iEpisode = tag->EpisodeNumber();
        }
      }
      else if (item->HasEPGInfoTag())
      {
        if (item->GetEPGInfoTag()->SeriesNumber() > 0)
          iSeason = item->GetEPGInfoTag()->SeriesNumber();
        if (item->GetEPGInfoTag()->EpisodeNumber() > 0)
          iEpisode = item->GetEPGInfoTag()->EpisodeNumber();
      }
      else if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->HasEpgInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRTimerInfoTag()->GetEpgInfoTag());
        if (tag->SeriesNumber() > 0)
          iSeason = tag->SeriesNumber();
        if (tag->EpisodeNumber() > 0)
          iEpisode = tag->EpisodeNumber();
      }
      else if (item->HasVideoInfoTag())
      {
        iSeason = item->GetVideoInfoTag()->m_iSeason;
        iEpisode = item->GetVideoInfoTag()->m_iEpisode;
      }

      if (iEpisode >= 0)
      {
        if (iSeason == 0) // prefix episode with 'S'
          return StringUtils::Format("S%d", iEpisode);
        else
          return StringUtils::Format("%d", iEpisode);
      }
    }
    break;
  case LISTITEM_SEASON:
    {
      int iSeason = -1;
      if (item->HasPVRChannelInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
        if (tag && tag->SeriesNumber() > 0)
          iSeason = tag->SeriesNumber();
      }
      else if (item->HasEPGInfoTag() &&
               item->GetEPGInfoTag()->SeriesNumber() > 0)
        iSeason = item->GetEPGInfoTag()->SeriesNumber();
      else if (item->HasPVRTimerInfoTag() &&
               item->GetPVRTimerInfoTag()->HasEpgInfoTag() &&
               item->GetPVRTimerInfoTag()->GetEpgInfoTag()->SeriesNumber() > 0)
        iSeason = item->GetPVRTimerInfoTag()->GetEpgInfoTag()->SeriesNumber();
      else if (item->HasVideoInfoTag())
        iSeason = item->GetVideoInfoTag()->m_iSeason;

      if (iSeason >= 0)
        return StringUtils::Format("%d", iSeason);
    }
    break;
  case LISTITEM_TVSHOW:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strShowTitle;
    break;
  case LISTITEM_COMMENT:
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->GetStatus();
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetComment();
    break;
  case LISTITEM_ACTUAL_ICON:
    return item->GetIconImage();
  case LISTITEM_ICON:
    {
      std::string strThumb = item->GetArt("thumb");
      if (strThumb.empty())
        strThumb = item->GetIconImage();
      if (fallback)
        *fallback = item->GetIconImage();
      return strThumb;
    }
  case LISTITEM_OVERLAY:
    return item->GetOverlayImage();
  case LISTITEM_THUMB:
    return item->GetArt("thumb");
  case LISTITEM_FOLDERPATH:
    return CURL(item->GetPath()).GetWithoutUserDetails();
  case LISTITEM_FOLDERNAME:
  case LISTITEM_PATH:
    {
      std::string path;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        path = URIUtils::GetDirectory(item->GetMusicInfoTag()->GetURL());
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
      {
        if( item->m_bIsFolder )
          path = item->GetVideoInfoTag()->m_strPath;
        else
          URIUtils::GetParentPath(item->GetVideoInfoTag()->m_strFileNameAndPath, path);
      }
      else
        URIUtils::GetParentPath(item->GetPath(), path);
      path = CURL(path).GetWithoutUserDetails();
      if (info==LISTITEM_FOLDERNAME)
      {
        URIUtils::RemoveSlashAtEnd(path);
        path=URIUtils::GetFileName(path);
      }
      return path;
    }
  case LISTITEM_FILENAME_AND_PATH:
    {
      std::string path;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        path = item->GetMusicInfoTag()->GetURL();
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
        path = item->GetVideoInfoTag()->m_strFileNameAndPath;
      else
        path = item->GetPath();
      path = CURL(path).GetWithoutUserDetails();
      return path;
    }
  case LISTITEM_PICTURE_PATH:
    if (item->IsPicture() && (!item->IsZIP() || item->IsRAR() || item->IsCBZ() || item->IsCBR()))
      return item->GetPath();
    break;
  case LISTITEM_STUDIO:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_studio, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_COUNTRY:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_country, g_advancedSettings.m_videoItemSeparator);
    break;
  case LISTITEM_MPAA:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strMPAARating;
    break;
  case LISTITEM_CAST:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->GetCast();
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Cast();
    break;
  case LISTITEM_CAST_AND_ROLE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->GetCast(true);
    break;
  case LISTITEM_WRITER:
    if (item->HasVideoInfoTag())
      return StringUtils::Join(item->GetVideoInfoTag()->m_writingCredits, g_advancedSettings.m_videoItemSeparator);
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->Writer();
    break;
  case LISTITEM_TAGLINE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strTagLine;
    break;
  case LISTITEM_TRAILER:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strTrailer;
    break;
  case LISTITEM_TOP250:
    if (item->HasVideoInfoTag())
    {
      std::string strResult;
      if (item->GetVideoInfoTag()->m_iTop250 > 0)
        strResult = StringUtils::Format("%i",item->GetVideoInfoTag()->m_iTop250);
      return strResult;
    }
    break;
  case LISTITEM_SORT_LETTER:
    {
      std::string letter;
      std::wstring character(1, item->GetSortLabel()[0]);
      StringUtils::ToUpper(character);
      g_charsetConverter.wToUTF8(character, letter);
      return letter;
    }
    break;
  case LISTITEM_VIDEO_CODEC:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_streamDetails.GetVideoCodec();
    break;
  case LISTITEM_VIDEO_RESOLUTION:
    if (item->HasVideoInfoTag())
      return CStreamDetails::VideoDimsToResolutionDescription(item->GetVideoInfoTag()->m_streamDetails.GetVideoWidth(), item->GetVideoInfoTag()->m_streamDetails.GetVideoHeight());
    break;
  case LISTITEM_VIDEO_ASPECT:
    if (item->HasVideoInfoTag())
      return CStreamDetails::VideoAspectToAspectDescription(item->GetVideoInfoTag()->m_streamDetails.GetVideoAspect());
    break;
  case LISTITEM_AUDIO_CODEC:
    if (item->HasVideoInfoTag())
    {
      return item->GetVideoInfoTag()->m_streamDetails.GetAudioCodec();
    }
    break;
  case LISTITEM_AUDIO_CHANNELS:
    if (item->HasVideoInfoTag())
    {
      std::string strResult;
      int iChannels = item->GetVideoInfoTag()->m_streamDetails.GetAudioChannels();
      if (iChannels > -1)
        strResult = StringUtils::Format("%i", iChannels);
      return strResult;
    }
    break;
  case LISTITEM_AUDIO_LANGUAGE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_streamDetails.GetAudioLanguage();
    break;
  case LISTITEM_SUBTITLE_LANGUAGE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_streamDetails.GetSubtitleLanguage();
    break;
  case LISTITEM_STARTTIME:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return tag ? tag->StartAsLocalTime().GetAsLocalizedTime("", false) : CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->StartAsLocalTime().GetAsLocalizedTime("", false);
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsLocalizedTime("", false);
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->RecordingTimeAsLocalTime().GetAsLocalizedTime("", false);
    if (item->m_dateTime.IsValid())
      return item->m_dateTime.GetAsLocalizedTime("", false);
    break;
  case LISTITEM_ENDTIME:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return tag ? tag->EndAsLocalTime().GetAsLocalizedTime("", false) : CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
    }
    else if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->EndAsLocalTime().GetAsLocalizedTime("", false);
    else if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsLocalizedTime("", false);
    else if (item->HasVideoInfoTag())
    {
      CDateTimeSpan duration(0, 0, 0, item->GetVideoInfoTag()->GetDuration());
      return (CDateTime::GetCurrentDateTime() + duration).GetAsLocalizedTime("", false);
    }
    break;
  case LISTITEM_STARTDATE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return tag ? tag->StartAsLocalTime().GetAsLocalizedDate(true) : CDateTime::GetCurrentDateTime().GetAsLocalizedDate(true);
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->StartAsLocalTime().GetAsLocalizedDate(true);
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->StartAsLocalTime().GetAsLocalizedDate(true);
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->RecordingTimeAsLocalTime().GetAsLocalizedDate(true);
    if (item->m_dateTime.IsValid())
      return item->m_dateTime.GetAsLocalizedDate(true);
    break;
  case LISTITEM_ENDDATE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
      return tag ? tag->EndAsLocalTime().GetAsLocalizedDate(true) : CDateTime::GetCurrentDateTime().GetAsLocalizedDate(true);
    }
    if (item->HasEPGInfoTag())
      return item->GetEPGInfoTag()->EndAsLocalTime().GetAsLocalizedDate(true);
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->EndAsLocalTime().GetAsLocalizedDate(true);
    break;
  case LISTITEM_CHANNEL_NUMBER:
    {
      std::string number;
      if (item->HasPVRChannelInfoTag())
        number = StringUtils::Format("%i", item->GetPVRChannelInfoTag()->ChannelNumber());
      if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->HasPVRChannel())
        number = StringUtils::Format("%i", item->GetEPGInfoTag()->PVRChannelNumber());
      if (item->HasPVRTimerInfoTag())
        number = StringUtils::Format("%i", item->GetPVRTimerInfoTag()->ChannelNumber());

      return number;
    }
    break;
  case LISTITEM_SUB_CHANNEL_NUMBER:
    {
      std::string number;
      if (item->HasPVRChannelInfoTag())
        number = StringUtils::Format("%i", item->GetPVRChannelInfoTag()->SubChannelNumber());
      if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->HasPVRChannel())
        number = StringUtils::Format("%i", item->GetEPGInfoTag()->ChannelTag()->SubChannelNumber());
      if (item->HasPVRTimerInfoTag())
        number = StringUtils::Format("%i", item->GetPVRTimerInfoTag()->ChannelTag()->SubChannelNumber());

      return number;
    }
    break;
  case LISTITEM_CHANNEL_NUMBER_LBL:
    {
      CPVRChannelPtr channel;
      if (item->HasPVRChannelInfoTag())
        channel = item->GetPVRChannelInfoTag();
      else if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->HasPVRChannel())
        channel = item->GetEPGInfoTag()->ChannelTag();
      else if (item->HasPVRTimerInfoTag())
        channel = item->GetPVRTimerInfoTag()->ChannelTag();

      return channel ?
          channel->FormattedChannelNumber() :
          "";
    }
    break;
  case LISTITEM_CHANNEL_NAME:
    if (item->HasPVRChannelInfoTag())
      return item->GetPVRChannelInfoTag()->ChannelName();
    if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->HasPVRChannel())
      return item->GetEPGInfoTag()->PVRChannelName();
    if (item->HasPVRRecordingInfoTag())
      return item->GetPVRRecordingInfoTag()->m_strChannelName;
    if (item->HasPVRTimerInfoTag())
      return item->GetPVRTimerInfoTag()->ChannelName();
    break;
  case LISTITEM_NEXT_STARTTIME:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->StartAsLocalTime().GetAsLocalizedTime("", false);
    }
    return CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
  case LISTITEM_NEXT_ENDTIME:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->EndAsLocalTime().GetAsLocalizedTime("", false);
    }
    return CDateTime::GetCurrentDateTime().GetAsLocalizedTime("", false);
  case LISTITEM_NEXT_STARTDATE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->StartAsLocalTime().GetAsLocalizedDate(true);
    }
    return CDateTime::GetCurrentDateTime().GetAsLocalizedDate(true);
  case LISTITEM_NEXT_ENDDATE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->EndAsLocalTime().GetAsLocalizedDate(true);
    }
    return CDateTime::GetCurrentDateTime().GetAsLocalizedDate(true);
  case LISTITEM_NEXT_PLOT:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->Plot();
    }
    return "";
  case LISTITEM_NEXT_PLOT_OUTLINE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->PlotOutline();
    }
    return "";
  case LISTITEM_NEXT_DURATION:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return StringUtils::SecondsToTimeString(tag->GetDuration());
    }
    return "";
  case LISTITEM_NEXT_GENRE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return StringUtils::Join(tag->Genre(), g_advancedSettings.m_videoItemSeparator);
    }
    return "";
  case LISTITEM_NEXT_TITLE:
    if (item->HasPVRChannelInfoTag())
    {
      CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNext());
      if (tag)
        return tag->Title();
    }
    return "";
  case LISTITEM_PARENTALRATING:
    {
      std::string rating;
      if (item->HasEPGInfoTag() && item->GetEPGInfoTag()->ParentalRating() > 0)
        rating = StringUtils::Format("%i", item->GetEPGInfoTag()->ParentalRating());
      return rating;
    }
    break;
  case LISTITEM_PERCENT_PLAYED:
    {
      int val;
      if (GetItemInt(val, item, info))
      {
        return StringUtils::Format("%d", val);;
      }
      break;
    }
  case LISTITEM_DATE_ADDED:
    if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_dateAdded.IsValid())
      return item->GetVideoInfoTag()->m_dateAdded.GetAsLocalizedDate();
    break;
  case LISTITEM_DBTYPE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_type;
    break;
  case LISTITEM_DBID:
    if (item->HasVideoInfoTag())
      {
        return StringUtils::Format("%i", item->GetVideoInfoTag()->m_iDbId);;
      }
    if (item->HasMusicInfoTag())
      {
        return StringUtils::Format("%i", item->GetMusicInfoTag()->GetDatabaseId());;
      }
    break;
  case LISTITEM_STEREOSCOPIC_MODE:
    {
      std::string stereoMode = item->GetProperty("stereomode").asString();
      if (stereoMode.empty() && item->HasVideoInfoTag())
        stereoMode = CStereoscopicsManager::Get().NormalizeStereoMode(item->GetVideoInfoTag()->m_streamDetails.GetStereoMode());
      return stereoMode;
    }
  case LISTITEM_IMDBNUMBER:
    {
      if (item->HasPVRChannelInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
        if (tag)
          return tag->IMDBNumber();
      }
      if (item->HasEPGInfoTag())
        return item->GetEPGInfoTag()->IMDBNumber();
      if (item->HasVideoInfoTag())
        return item->GetVideoInfoTag()->m_strIMDBNumber;
      break;
    }
  case LISTITEM_EPISODENAME:
    {
      if (item->HasPVRChannelInfoTag())
      {
        CEpgInfoTagPtr tag(item->GetPVRChannelInfoTag()->GetEPGNow());
        if (tag)
          return tag->EpisodeName();
      }
      if (item->HasEPGInfoTag())
        return item->GetEPGInfoTag()->EpisodeName();
      if (item->HasPVRTimerInfoTag() && item->GetPVRTimerInfoTag()->HasEpgInfoTag())
        return item->GetPVRTimerInfoTag()->GetEpgInfoTag()->EpisodeName();
      break;
    }
  case LISTITEM_TIMERTYPE:
    {
      if (item->HasPVRTimerInfoTag())
        return item->GetPVRTimerInfoTag()->GetTypeAsString();
    }
    break;
  }
  return "";
}

std::string CGUIInfoManager::GetItemImage(const CFileItem *item, int info, std::string *fallback)
{
  if (info >= CONDITIONAL_LABEL_START && info <= CONDITIONAL_LABEL_END)
    return GetSkinVariableString(info, true, item);

  switch (info)
  {
  case LISTITEM_RATING:  // old song rating format
    {
      if (item->HasMusicInfoTag())
      {
        return StringUtils::Format("songrating%c.png", item->GetMusicInfoTag()->GetRating());
      }
    }
    break;
  case LISTITEM_STAR_RATING:
    {
      std::string rating;
      if (item->HasVideoInfoTag())
      { // rating for videos is assumed 0..10, so convert to 0..5
        rating = StringUtils::Format("rating%ld.png", (long)((item->GetVideoInfoTag()->m_fRating * 0.5f) + 0.5f));
      }
      else if (item->HasMusicInfoTag())
      { // song rating.
        rating = StringUtils::Format("rating%c.png", item->GetMusicInfoTag()->GetRating());
      }
      return rating;
    }
    break;
  }  /* switch (info) */

  return GetItemLabel(item, info, fallback);
}

bool CGUIInfoManager::GetItemBool(const CGUIListItem *item, int condition) const
{
  if (!item) return false;
  if (condition >= LISTITEM_PROPERTY_START && condition - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { // grab the property
    std::string property = m_listitemProperties[condition - LISTITEM_PROPERTY_START];
    return item->GetProperty(property).asBoolean();
  }
  else if (condition == LISTITEM_ISPLAYING)
  {
    if (item->HasProperty("playlistposition"))
      return (int)item->GetProperty("playlisttype").asInteger() == g_playlistPlayer.GetCurrentPlaylist() && (int)item->GetProperty("playlistposition").asInteger() == g_playlistPlayer.GetCurrentSong();
    else if (item->IsFileItem() && !m_currentFile->GetPath().empty())
    {
      if (!g_application.m_strPlayListFile.empty())
      {
        //playlist file that is currently playing or the playlistitem that is currently playing.
        return ((const CFileItem *)item)->IsPath(g_application.m_strPlayListFile) || m_currentFile->IsSamePath((const CFileItem *)item);
      }
      return m_currentFile->IsSamePath((const CFileItem *)item);
    }
  }
  else if (condition == LISTITEM_ISSELECTED)
    return item->IsSelected();
  else if (condition == LISTITEM_IS_FOLDER)
    return item->m_bIsFolder;
  else if (condition == LISTITEM_IS_RESUMABLE)
  {
    if (item->IsFileItem())
    {
      if (((const CFileItem *)item)->HasVideoInfoTag())
        return ((const CFileItem *)item)->GetVideoInfoTag()->m_resumePoint.timeInSeconds > 0;
      else if (((const CFileItem *)item)->HasPVRRecordingInfoTag())
        return ((const CFileItem *)item)->GetPVRRecordingInfoTag()->m_resumePoint.timeInSeconds > 0;
    }
  }
  else if (item->IsFileItem())
  {
    const CFileItem *pItem = (const CFileItem *)item;
    if (condition == LISTITEM_ISRECORDING)
    {
      if (!g_PVRManager.IsStarted())
        return false;

      if (pItem->HasPVRChannelInfoTag())
      {
        return pItem->GetPVRChannelInfoTag()->IsRecording();
      }
      else if (pItem->HasPVRTimerInfoTag())
      {
        const CPVRTimerInfoTagPtr timer = pItem->GetPVRTimerInfoTag();
        if (timer)
          return timer->IsRecording();
      }
      else if (pItem->HasEPGInfoTag())
      {
        CEpgInfoTagPtr epgTag = pItem->GetEPGInfoTag();

        // Check if the tag has a currently active recording associated
        if (epgTag->HasRecording() && epgTag->IsActive())
          return true;

        // Search all timers for something that matches the tag
        CFileItemPtr timer = g_PVRTimers->GetTimerForEpgTag(pItem);
        if (timer && timer->HasPVRTimerInfoTag())
          return timer->GetPVRTimerInfoTag()->IsRecording();
      }
    }
    else if (condition == LISTITEM_INPROGRESS)
    {
      if (!g_PVRManager.IsStarted())
        return false;

      if (pItem->HasEPGInfoTag())
        return pItem->GetEPGInfoTag()->IsActive();
    }
    else if (condition == LISTITEM_HASTIMER)
    {
      if (pItem->HasEPGInfoTag())
      {
        CFileItemPtr timer = g_PVRTimers->GetTimerForEpgTag(pItem);
        if (timer && timer->HasPVRTimerInfoTag())
          return timer->GetPVRTimerInfoTag()->IsActive();
      }
    }
    else if (condition == LISTITEM_HASTIMERSCHEDULE)
    {
      if (pItem->HasEPGInfoTag())
      {
        CFileItemPtr timer = g_PVRTimers->GetTimerForEpgTag(pItem);
        if (timer && timer->HasPVRTimerInfoTag())
          return timer->GetPVRTimerInfoTag()->IsRepeating();
      }
    }
    else if (condition == LISTITEM_HASRECORDING)
    {
      return pItem->HasEPGInfoTag() && pItem->GetEPGInfoTag()->HasRecording();
    }
    else if (condition == LISTITEM_HAS_EPG)
    {
      if (pItem->HasPVRChannelInfoTag())
      {
        return (pItem->GetPVRChannelInfoTag()->GetEPGNow().get() != NULL);
      }
      else
      {
        return pItem->HasEPGInfoTag();
      }
    }
    else if (condition == LISTITEM_ISENCRYPTED)
    {
      if (pItem->HasPVRChannelInfoTag())
      {
        return pItem->GetPVRChannelInfoTag()->IsEncrypted();
      }
      else if (pItem->HasEPGInfoTag() && pItem->GetEPGInfoTag()->HasPVRChannel())
      {
        return pItem->GetEPGInfoTag()->ChannelTag()->IsEncrypted();
      }
    }
    else if (condition == LISTITEM_IS_STEREOSCOPIC)
    {
      std::string stereoMode = pItem->GetProperty("stereomode").asString();
      if (stereoMode.empty() && pItem->HasVideoInfoTag())
          stereoMode = CStereoscopicsManager::Get().NormalizeStereoMode(pItem->GetVideoInfoTag()->m_streamDetails.GetStereoMode());
      if (!stereoMode.empty() && stereoMode != "mono")
        return true;
    }
    else if (condition == LISTITEM_IS_COLLECTION)
    {
      if (pItem->HasVideoInfoTag())
        return (pItem->GetVideoInfoTag()->m_type == MediaTypeVideoCollection);
    }
  }

  return false;
}

void CGUIInfoManager::ResetCache()
{
  // reset any animation triggers as well
  m_containerMoves.clear();
  // mark our infobools as dirty
  CSingleLock lock(m_critInfo);
  for (vector<InfoPtr>::iterator i = m_bools.begin(); i != m_bools.end(); ++i)
    (*i)->SetDirty();
}

std::string CGUIInfoManager::GetPictureLabel(int info)
{
  if (info == SLIDE_FILE_NAME)
    return GetItemLabel(m_currentSlide, LISTITEM_FILENAME);
  else if (info == SLIDE_FILE_PATH)
  {
    std::string path = URIUtils::GetDirectory(m_currentSlide->GetPath());
    return CURL(path).GetWithoutUserDetails();
  }
  else if (info == SLIDE_FILE_SIZE)
    return GetItemLabel(m_currentSlide, LISTITEM_SIZE);
  else if (info == SLIDE_FILE_DATE)
    return GetItemLabel(m_currentSlide, LISTITEM_DATE);
  else if (info == SLIDE_INDEX)
  {
    CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    if (slideshow && slideshow->NumSlides())
    {
      return StringUtils::Format("%d/%d", slideshow->CurrentSlide(), slideshow->NumSlides());
    }
  }
  if (m_currentSlide->HasPictureInfoTag())
    return m_currentSlide->GetPictureInfoTag()->GetInfo(info);
  return "";
}

void CGUIInfoManager::SetCurrentSlide(CFileItem &item)
{
  if (m_currentSlide->GetPath() != item.GetPath())
  {
    if (!item.GetPictureInfoTag()->Loaded()) // If picture metadata has not been loaded yet, load it now
      item.GetPictureInfoTag()->Load(item.GetPath());
    *m_currentSlide = item;
  }
}

void CGUIInfoManager::ResetCurrentSlide()
{
  m_currentSlide->Reset();
}

bool CGUIInfoManager::CheckWindowCondition(CGUIWindow *window, int condition) const
{
  // check if it satisfies our condition
  if (!window) return false;
  if ((condition & WINDOW_CONDITION_HAS_LIST_ITEMS) && !window->HasListItems())
    return false;
  if ((condition & WINDOW_CONDITION_IS_MEDIA_WINDOW) && !window->IsMediaWindow())
    return false;
  return true;
}

CGUIWindow *CGUIInfoManager::GetWindowWithCondition(int contextWindow, int condition) const
{
  CGUIWindow *window = g_windowManager.GetWindow(contextWindow);
  if (CheckWindowCondition(window, condition))
    return window;

  // try topmost dialog
  window = g_windowManager.GetWindow(g_windowManager.GetTopMostModalDialogID());
  if (CheckWindowCondition(window, condition))
    return window;

  // try active window
  window = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (CheckWindowCondition(window, condition))
    return window;

  return NULL;
}

void CGUIInfoManager::SetCurrentVideoTag(const CVideoInfoTag &tag)
{
  *m_currentFile->GetVideoInfoTag() = tag;
  m_currentFile->m_lStartOffset = 0;
}

void CGUIInfoManager::SetCurrentSongTag(const MUSIC_INFO::CMusicInfoTag &tag)
{
  //CLog::Log(LOGDEBUG, "Asked to SetCurrentTag");
  *m_currentFile->GetMusicInfoTag() = tag;
  m_currentFile->m_lStartOffset = 0;
}

const CFileItem& CGUIInfoManager::GetCurrentSlide() const
{
  return *m_currentSlide;
}

const MUSIC_INFO::CMusicInfoTag* CGUIInfoManager::GetCurrentSongTag() const
{
  if (m_currentFile->HasMusicInfoTag())
    return m_currentFile->GetMusicInfoTag();

  return NULL;
}

const CVideoInfoTag* CGUIInfoManager::GetCurrentMovieTag() const
{
  if (m_currentFile->HasVideoInfoTag())
    return m_currentFile->GetVideoInfoTag();

  return NULL;
}

void GUIInfo::SetInfoFlag(uint32_t flag)
{
  assert(flag >= (1 << 24));
  m_data1 |= flag;
}

uint32_t GUIInfo::GetInfoFlag() const
{
  // we strip out the bottom 24 bits, where we keep data
  // and return the flag only
  return m_data1 & 0xff000000;
}

uint32_t GUIInfo::GetData1() const
{
  // we strip out the top 8 bits, where we keep flags
  // and return the unflagged data
  return m_data1 & ((1 << 24) -1);
}

int GUIInfo::GetData2() const
{
  return m_data2;
}

void CGUIInfoManager::SetLibraryBool(int condition, bool value)
{
  switch (condition)
  {
    case LIBRARY_HAS_MUSIC:
      m_libraryHasMusic = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MOVIES:
      m_libraryHasMovies = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MOVIE_SETS:
      m_libraryHasMovieSets = value ? 1 : 0;
      break;
    case LIBRARY_HAS_TVSHOWS:
      m_libraryHasTVShows = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MUSICVIDEOS:
      m_libraryHasMusicVideos = value ? 1 : 0;
      break;
    case LIBRARY_HAS_SINGLES:
      m_libraryHasSingles = value ? 1 : 0;
      break;
    case LIBRARY_HAS_COMPILATIONS:
      m_libraryHasCompilations = value ? 1 : 0;
      break;
    default:
      break;
  }
}

void CGUIInfoManager::ResetLibraryBools()
{
  m_libraryHasMusic = -1;
  m_libraryHasMovies = -1;
  m_libraryHasTVShows = -1;
  m_libraryHasMusicVideos = -1;
  m_libraryHasMovieSets = -1;
  m_libraryHasSingles = -1;
  m_libraryHasCompilations = -1;
}

bool CGUIInfoManager::GetLibraryBool(int condition)
{
  if (condition == LIBRARY_HAS_MUSIC)
  {
    if (m_libraryHasMusic < 0)
    { // query
      CMusicDatabase db;
      if (db.Open())
      {
        m_libraryHasMusic = (db.GetSongsCount() > 0) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMusic > 0;
  }
  else if (condition == LIBRARY_HAS_MOVIES)
  {
    if (m_libraryHasMovies < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasMovies = db.HasContent(VIDEODB_CONTENT_MOVIES) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMovies > 0;
  }
  else if (condition == LIBRARY_HAS_MOVIE_SETS)
  {
    if (m_libraryHasMovieSets < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasMovieSets = db.HasSets() ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMovieSets > 0;
  }
  else if (condition == LIBRARY_HAS_TVSHOWS)
  {
    if (m_libraryHasTVShows < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasTVShows = db.HasContent(VIDEODB_CONTENT_TVSHOWS) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasTVShows > 0;
  }
  else if (condition == LIBRARY_HAS_MUSICVIDEOS)
  {
    if (m_libraryHasMusicVideos < 0)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        m_libraryHasMusicVideos = db.HasContent(VIDEODB_CONTENT_MUSICVIDEOS) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasMusicVideos > 0;
  }
  else if (condition == LIBRARY_HAS_SINGLES)
  {
    if (m_libraryHasSingles < 0)
    {
      CMusicDatabase db;
      if (db.Open())
      {
        m_libraryHasSingles = (db.GetSinglesCount() > 0) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasSingles > 0;
  }
  else if (condition == LIBRARY_HAS_COMPILATIONS)
  {
    if (m_libraryHasCompilations < 0)
    {
      CMusicDatabase db;
      if (db.Open())
      {
        m_libraryHasCompilations = (db.GetCompilationAlbumsCount() > 0) ? 1 : 0;
        db.Close();
      }
    }
    return m_libraryHasCompilations > 0;
  }
  else if (condition == LIBRARY_HAS_VIDEO)
  {
    return (GetLibraryBool(LIBRARY_HAS_MOVIES) ||
            GetLibraryBool(LIBRARY_HAS_TVSHOWS) ||
            GetLibraryBool(LIBRARY_HAS_MUSICVIDEOS));
  }
  return false;
}

int CGUIInfoManager::RegisterSkinVariableString(const CSkinVariableString* info)
{
  if (!info)
    return 0;

  CSingleLock lock(m_critInfo);
  m_skinVariableStrings.push_back(*info);
  delete info;
  return CONDITIONAL_LABEL_START + m_skinVariableStrings.size() - 1;
}

int CGUIInfoManager::TranslateSkinVariableString(const std::string& name, int context)
{
  for (vector<CSkinVariableString>::const_iterator it = m_skinVariableStrings.begin();
       it != m_skinVariableStrings.end(); ++it)
  {
    if (StringUtils::EqualsNoCase(it->GetName(), name) && it->GetContext() == context)
      return it - m_skinVariableStrings.begin() + CONDITIONAL_LABEL_START;
  }
  return 0;
}

std::string CGUIInfoManager::GetSkinVariableString(int info,
                                                  bool preferImage /*= false*/,
                                                  const CGUIListItem *item /*= NULL*/)
{
  info -= CONDITIONAL_LABEL_START;
  if (info >= 0 && info < (int)m_skinVariableStrings.size())
    return m_skinVariableStrings[info].GetValue(preferImage, item);

  return "";
}

bool CGUIInfoManager::ConditionsChangedValues(const std::map<INFO::InfoPtr, bool>& map)
{
  for (std::map<INFO::InfoPtr, bool>::const_iterator it = map.begin() ; it != map.end() ; ++it)
  {
    if (it->first->Get() != it->second)
      return true;
  }
  return false;
}

CEpgInfoTagPtr CGUIInfoManager::GetEpgInfoTag() const
{
  CEpgInfoTagPtr currentTag;
  if (m_currentFile->HasEPGInfoTag())
  {
    currentTag = m_currentFile->GetEPGInfoTag();
    while (currentTag && !currentTag->IsActive())
      currentTag = currentTag->GetNextEvent();
  }
  return currentTag;
}
