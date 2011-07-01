/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "dialogs/GUIDialogSeekBar.h"
#include "windows/GUIMediaWindow.h"
#include "dialogs/GUIDialogFileBrowser.h"
#include "settings/GUIDialogContentSettings.h"
#include "dialogs/GUIDialogProgress.h"
#include "Application.h"
#include "Util.h"
#include "network/libscrobbler/lastfmscrobbler.h"
#include "utils/URIUtils.h"
#include "utils/Weather.h"
#include "PartyModeManager.h"
#include "addons/Visualisation.h"
#include "input/ButtonTranslator.h"
#include "utils/AlarmClock.h"
#ifdef HAS_LCD
#include "utils/LCD.h"
#endif
#include "GUIPassword.h"
#include "LangInfo.h"
#include "utils/SystemInfo.h"
#include "guilib/GUIButtonScroller.h"
#include "guilib/GUITextBox.h"
#include "GUIInfoManager.h"
#include "pictures/GUIWindowSlideShow.h"
#include "music/LastFmManager.h"
#include "pictures/PictureInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "music/dialogs/GUIDialogMusicScan.h"
#include "video/dialogs/GUIDialogVideoScan.h"
#include "guilib/GUIWindowManager.h"
#include "filesystem/File.h"
#include "playlists/PlayList.h"
#include "utils/TuxBoxUtil.h"
#include "windowing/WindowingFactory.h"
#include "powermanagement/PowerManager.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "guilib/LocalizeStrings.h"
#include "utils/CPUInfo.h"
#include "utils/StringUtils.h"
#include "utils/MathUtils.h"

// stuff for current song
#include "music/tags/MusicInfoTagLoaderFactory.h"
#include "music/MusicInfoLoader.h"
#include "utils/LabelFormatter.h"

#include "GUIUserMessages.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "storage/MediaManager.h"
#include "utils/TimeUtils.h"
#include "threads/SingleLock.h"
#include "utils/log.h"

#include "addons/AddonManager.h"

#define SYSHEATUPDATEINTERVAL 60000

using namespace std;
using namespace XFILE;
using namespace MUSIC_INFO;
using namespace ADDON;

CGUIInfoManager::CCombinedValue& CGUIInfoManager::CCombinedValue::operator =(const CGUIInfoManager::CCombinedValue& mSrc)
{
  this->m_info = mSrc.m_info;
  this->m_id = mSrc.m_id;
  this->m_postfix = mSrc.m_postfix;
  return *this;
}

CGUIInfoManager::CGUIInfoManager(void)
{
  m_lastSysHeatInfoTime = -SYSHEATUPDATEINTERVAL;  // make sure we grab CPU temp on the first pass
  m_lastMusicBitrateTime = 0;
  m_fanSpeed = 0;
  m_AfterSeekTimeout = 0;
  m_seekOffset = 0;
  m_playerSeeking = false;
  m_performingSeek = false;
  m_nextWindowID = WINDOW_INVALID;
  m_prevWindowID = WINDOW_INVALID;
  m_stringParameters.push_back("__ZZZZ__");   // to offset the string parameters by 1 to assure that all entries are non-zero
  m_currentFile = new CFileItem;
  m_currentSlide = new CFileItem;
  m_frameCounter = 0;
  m_lastFPSTime = 0;
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
      CFileItemPtr item = boost::static_pointer_cast<CFileItem>(message.GetItem());
      if (item && m_currentFile->m_strPath.Equals(item->m_strPath))
        *m_currentFile = *item;
      return true;
    }
  }
  return false;
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data. Can handle combined strings on the form
/// Player.Caching + VideoPlayer.IsFullscreen (Logical and)
/// Player.HasVideo | Player.HasAudio (Logical or)
int CGUIInfoManager::TranslateString(const CStdString &condition)
{
  // translate $LOCALIZE as required
  CStdString strCondition(CGUIInfoLabel::ReplaceLocalize(condition));
  if (strCondition.find_first_of("|") != strCondition.npos ||
      strCondition.find_first_of("+") != strCondition.npos ||
      strCondition.find_first_of("[") != strCondition.npos ||
      strCondition.find_first_of("]") != strCondition.npos)
  {
    // Have a boolean expression
    // Check if this was added before
    vector<CCombinedValue>::iterator it;
    for(it = m_CombinedValues.begin(); it != m_CombinedValues.end(); it++)
    {
      if(strCondition.CompareNoCase(it->m_info) == 0)
        return it->m_id;
    }
    return TranslateBooleanExpression(strCondition);
  }
  //Just single command.
  return TranslateSingleString(strCondition);
}


/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data.
int CGUIInfoManager::TranslateSingleString(const CStdString &strCondition)
{
  // trim whitespace, and convert to lowercase
  CStdString strTest = strCondition;
  strTest.TrimLeft(" \t\r\n");
  strTest.TrimRight(" \t\r\n");
  if (strTest.IsEmpty()) return 0;

  bool bNegate = strTest[0] == '!';
  int ret = 0;

  if(bNegate)
    strTest.Delete(0, 1);
  CStdString original(strTest);
  strTest.ToLower();

  CStdString strCategory = strTest.Left(strTest.Find("."));

  // translate conditions...
  if (strTest.Equals("false") || strTest.Equals("no") || strTest.Equals("off")) ret = SYSTEM_ALWAYS_FALSE;
  else if (strTest.Equals("true") || strTest.Equals("yes") || strTest.Equals("on")) ret = SYSTEM_ALWAYS_TRUE;
  if (strCategory.Equals("player"))
  {
    if (strTest.Equals("player.hasmedia")) ret = PLAYER_HAS_MEDIA;
    else if (strTest.Equals("player.hasaudio")) ret = PLAYER_HAS_AUDIO;
    else if (strTest.Equals("player.hasvideo")) ret = PLAYER_HAS_VIDEO;
    else if (strTest.Equals("player.playing")) ret = PLAYER_PLAYING;
    else if (strTest.Equals("player.paused")) ret = PLAYER_PAUSED;
    else if (strTest.Equals("player.rewinding")) ret = PLAYER_REWINDING;
    else if (strTest.Equals("player.forwarding")) ret = PLAYER_FORWARDING;
    else if (strTest.Equals("player.rewinding2x")) ret = PLAYER_REWINDING_2x;
    else if (strTest.Equals("player.rewinding4x")) ret = PLAYER_REWINDING_4x;
    else if (strTest.Equals("player.rewinding8x")) ret = PLAYER_REWINDING_8x;
    else if (strTest.Equals("player.rewinding16x")) ret = PLAYER_REWINDING_16x;
    else if (strTest.Equals("player.rewinding32x")) ret = PLAYER_REWINDING_32x;
    else if (strTest.Equals("player.forwarding2x")) ret = PLAYER_FORWARDING_2x;
    else if (strTest.Equals("player.forwarding4x")) ret = PLAYER_FORWARDING_4x;
    else if (strTest.Equals("player.forwarding8x")) ret = PLAYER_FORWARDING_8x;
    else if (strTest.Equals("player.forwarding16x")) ret = PLAYER_FORWARDING_16x;
    else if (strTest.Equals("player.forwarding32x")) ret = PLAYER_FORWARDING_32x;
    else if (strTest.Equals("player.canrecord")) ret = PLAYER_CAN_RECORD;
    else if (strTest.Equals("player.recording")) ret = PLAYER_RECORDING;
    else if (strTest.Equals("player.displayafterseek")) ret = PLAYER_DISPLAY_AFTER_SEEK;
    else if (strTest.Equals("player.caching")) ret = PLAYER_CACHING;
    else if (strTest.Equals("player.cachelevel")) ret = PLAYER_CACHELEVEL;
    else if (strTest.Equals("player.seekbar")) ret = PLAYER_SEEKBAR;
    else if (strTest.Equals("player.progress")) ret = PLAYER_PROGRESS;
    else if (strTest.Equals("player.progresscache")) ret = PLAYER_PROGRESS_CACHE;
    else if (strTest.Equals("player.seeking")) ret = PLAYER_SEEKING;
    else if (strTest.Equals("player.showtime")) ret = PLAYER_SHOWTIME;
    else if (strTest.Equals("player.showcodec")) ret = PLAYER_SHOWCODEC;
    else if (strTest.Equals("player.showinfo")) ret = PLAYER_SHOWINFO;
    else if (strTest.Left(15).Equals("player.seektime")) return AddMultiInfo(GUIInfo(PLAYER_SEEKTIME, TranslateTimeFormat(strTest.Mid(15))));
    else if (strTest.Left(17).Equals("player.seekoffset")) return AddMultiInfo(GUIInfo(PLAYER_SEEKOFFSET, TranslateTimeFormat(strTest.Mid(17))));
    else if (strTest.Left(20).Equals("player.timeremaining")) return AddMultiInfo(GUIInfo(PLAYER_TIME_REMAINING, TranslateTimeFormat(strTest.Mid(20))));
    else if (strTest.Left(16).Equals("player.timespeed")) return AddMultiInfo(GUIInfo(PLAYER_TIME_SPEED, TranslateTimeFormat(strTest.Mid(16))));
    else if (strTest.Left(11).Equals("player.time")) return AddMultiInfo(GUIInfo(PLAYER_TIME, TranslateTimeFormat(strTest.Mid(11))));
    else if (strTest.Left(15).Equals("player.duration")) return AddMultiInfo(GUIInfo(PLAYER_DURATION, TranslateTimeFormat(strTest.Mid(15))));
    else if (strTest.Left(17).Equals("player.finishtime")) return AddMultiInfo(GUIInfo(PLAYER_FINISH_TIME, TranslateTimeFormat(strTest.Mid(17))));
    else if (strTest.Equals("player.volume")) ret = PLAYER_VOLUME;
    else if (strTest.Equals("player.subtitledelay")) ret = PLAYER_SUBTITLE_DELAY;
    else if (strTest.Equals("player.audiodelay")) ret = PLAYER_AUDIO_DELAY;
    else if (strTest.Equals("player.muted")) ret = PLAYER_MUTED;
    else if (strTest.Equals("player.hasduration")) ret = PLAYER_HASDURATION;
    else if (strTest.Equals("player.chapter")) ret = PLAYER_CHAPTER;
    else if (strTest.Equals("player.chaptercount")) ret = PLAYER_CHAPTERCOUNT;
    else if (strTest.Equals("player.chaptername")) ret = PLAYER_CHAPTERNAME;
    else if (strTest.Equals("player.starrating")) ret = PLAYER_STAR_RATING;
    else if (strTest.Equals("player.passthrough")) ret = PLAYER_PASSTHROUGH;
    else if (strTest.Equals("player.folderpath")) ret = PLAYER_PATH;
    else if (strTest.Equals("player.filenameandpath")) ret = PLAYER_FILEPATH;
  }
  else if (strCategory.Equals("weather"))
  {
    if (strTest.Equals("weather.conditions")) ret = WEATHER_CONDITIONS;
    else if (strTest.Equals("weather.temperature")) ret = WEATHER_TEMPERATURE;
    else if (strTest.Equals("weather.location")) ret = WEATHER_LOCATION;
    else if (strTest.Equals("weather.isfetched")) ret = WEATHER_IS_FETCHED;
    else if (strTest.Equals("weather.fanartcode")) ret = WEATHER_FANART_CODE;
    else if (strTest.Equals("weather.plugin")) ret = WEATHER_PLUGIN;
  }
  else if (strCategory.Equals("bar"))
  {
    if (strTest.Equals("bar.gputemperature")) ret = SYSTEM_GPU_TEMPERATURE;
    else if (strTest.Equals("bar.cputemperature")) ret = SYSTEM_CPU_TEMPERATURE;
    else if (strTest.Equals("bar.cpuusage")) ret = SYSTEM_CPU_USAGE;
    else if (strTest.Equals("bar.freememory")) ret = SYSTEM_FREE_MEMORY;
    else if (strTest.Equals("bar.usedmemory")) ret = SYSTEM_USED_MEMORY;
    else if (strTest.Equals("bar.fanspeed")) ret = SYSTEM_FAN_SPEED;
    else if (strTest.Equals("bar.usedspace")) ret = SYSTEM_USED_SPACE;
    else if (strTest.Equals("bar.freespace")) ret = SYSTEM_FREE_SPACE;
    else if (strTest.Equals("bar.usedspace(c)")) ret = SYSTEM_USED_SPACE_C;
    else if (strTest.Equals("bar.freespace(c)")) ret = SYSTEM_FREE_SPACE_C;
    else if (strTest.Equals("bar.usedspace(e)")) ret = SYSTEM_USED_SPACE_E;
    else if (strTest.Equals("bar.freespace(e)")) ret = SYSTEM_FREE_SPACE_E;
    else if (strTest.Equals("bar.usedspace(f)")) ret = SYSTEM_USED_SPACE_F;
    else if (strTest.Equals("bar.freespace(f)")) ret = SYSTEM_FREE_SPACE_F;
    else if (strTest.Equals("bar.usedspace(g)")) ret = SYSTEM_USED_SPACE_G;
    else if (strTest.Equals("bar.freespace(g)")) ret = SYSTEM_FREE_SPACE_G;
    else if (strTest.Equals("bar.usedspace(x)")) ret = SYSTEM_USED_SPACE_X;
    else if (strTest.Equals("bar.freespace(x)")) ret = SYSTEM_FREE_SPACE_X;
    else if (strTest.Equals("bar.usedspace(y)")) ret = SYSTEM_USED_SPACE_Y;
    else if (strTest.Equals("bar.freespace(y)")) ret = SYSTEM_FREE_SPACE_Y;
    else if (strTest.Equals("bar.usedspace(z)")) ret = SYSTEM_USED_SPACE_Z;
    else if (strTest.Equals("bar.freespace(z)")) ret = SYSTEM_FREE_SPACE_Z;
    else if (strTest.Equals("bar.hddtemperature")) ret = SYSTEM_HDD_TEMPERATURE;
  }
  else if (strCategory.Equals("system"))
  {
    if (strTest.Equals("system.date")) ret = SYSTEM_DATE;
    else if (strTest.Left(12).Equals("system.date("))
    {
      // the skin must submit the date in the format MM-DD
      // This InfoBool is designed for generic range checking, so year is NOT used.  Only Month-Day.
      CStdString param = strTest.Mid(12, strTest.length() - 13);
      CStdStringArray params;
      StringUtils::SplitString(param, ",", params);
      if (params.size() == 2)
        return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_DATE : SYSTEM_DATE, StringUtils::DateStringToYYYYMMDD(params[0]) % 10000, StringUtils::DateStringToYYYYMMDD(params[1]) % 10000));
      else if (params.size() == 1)
        return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_DATE : SYSTEM_DATE, StringUtils::DateStringToYYYYMMDD(params[0]) % 10000));
    }
    else if (strTest.Left(11).Equals("system.time"))
    {
      // determine if this is a System.Time(TIME_FORMAT) infolabel or a System.Time(13:00,14:00) boolean based on the contents of the param
      // essentially if it isn't a valid TIME_FORMAT then its considered to be the latter.
      CStdString param = strTest.Mid(11);
      TIME_FORMAT timeFormat = TranslateTimeFormat(param);
      if ((timeFormat == TIME_FORMAT_GUESS) && (!param.IsEmpty()))
      {
        param = strTest.Mid(12, strTest.length() - 13);
        CStdStringArray params;
        StringUtils::SplitString(param, ",", params);
        if (params.size() == 2)
          return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_TIME : SYSTEM_TIME, StringUtils::TimeStringToSeconds(params[0]), StringUtils::TimeStringToSeconds(params[1])));
        else if (params.size() == 1)
          return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_TIME : SYSTEM_TIME, StringUtils::TimeStringToSeconds(params[0])));
      }
      else
        return AddMultiInfo(GUIInfo(SYSTEM_TIME, timeFormat));
    }
    else if (strTest.Equals("system.cputemperature")) ret = SYSTEM_CPU_TEMPERATURE;
    else if (strTest.Equals("system.cpuusage")) ret = SYSTEM_CPU_USAGE;
    else if (strTest.Equals("system.gputemperature")) ret = SYSTEM_GPU_TEMPERATURE;
    else if (strTest.Equals("system.fanspeed")) ret = SYSTEM_FAN_SPEED;
    else if (strTest.Equals("system.freespace")) ret = SYSTEM_FREE_SPACE;
    else if (strTest.Equals("system.usedspace")) ret = SYSTEM_USED_SPACE;
    else if (strTest.Equals("system.totalspace")) ret = SYSTEM_TOTAL_SPACE;
    else if (strTest.Equals("system.usedspacepercent")) ret = SYSTEM_USED_SPACE_PERCENT;
    else if (strTest.Equals("system.freespacepercent")) ret = SYSTEM_FREE_SPACE_PERCENT;
    else if (strTest.Equals("system.freespace(c)")) ret = SYSTEM_FREE_SPACE_C;
    else if (strTest.Equals("system.usedspace(c)")) ret = SYSTEM_USED_SPACE_C;
    else if (strTest.Equals("system.totalspace(c)")) ret = SYSTEM_TOTAL_SPACE_C;
    else if (strTest.Equals("system.usedspacepercent(c)")) ret = SYSTEM_USED_SPACE_PERCENT_C;
    else if (strTest.Equals("system.freespacepercent(c)")) ret = SYSTEM_FREE_SPACE_PERCENT_C;
    else if (strTest.Equals("system.freespace(e)")) ret = SYSTEM_FREE_SPACE_E;
    else if (strTest.Equals("system.usedspace(e)")) ret = SYSTEM_USED_SPACE_E;
    else if (strTest.Equals("system.totalspace(e)")) ret = SYSTEM_TOTAL_SPACE_E;
    else if (strTest.Equals("system.usedspacepercent(e)")) ret = SYSTEM_USED_SPACE_PERCENT_E;
    else if (strTest.Equals("system.freespacepercent(e)")) ret = SYSTEM_FREE_SPACE_PERCENT_E;
    else if (strTest.Equals("system.freespace(f)")) ret = SYSTEM_FREE_SPACE_F;
    else if (strTest.Equals("system.usedspace(f)")) ret = SYSTEM_USED_SPACE_F;
    else if (strTest.Equals("system.totalspace(f)")) ret = SYSTEM_TOTAL_SPACE_F;
    else if (strTest.Equals("system.usedspacepercent(f)")) ret = SYSTEM_USED_SPACE_PERCENT_F;
    else if (strTest.Equals("system.freespacepercent(f)")) ret = SYSTEM_FREE_SPACE_PERCENT_F;
    else if (strTest.Equals("system.freespace(g)")) ret = SYSTEM_FREE_SPACE_G;
    else if (strTest.Equals("system.usedspace(g)")) ret = SYSTEM_USED_SPACE_G;
    else if (strTest.Equals("system.totalspace(g)")) ret = SYSTEM_TOTAL_SPACE_G;
    else if (strTest.Equals("system.usedspacepercent(g)")) ret = SYSTEM_USED_SPACE_PERCENT_G;
    else if (strTest.Equals("system.freespacepercent(g)")) ret = SYSTEM_FREE_SPACE_PERCENT_G;
    else if (strTest.Equals("system.usedspace(x)")) ret = SYSTEM_USED_SPACE_X;
    else if (strTest.Equals("system.freespace(x)")) ret = SYSTEM_FREE_SPACE_X;
    else if (strTest.Equals("system.totalspace(x)")) ret = SYSTEM_TOTAL_SPACE_X;
    else if (strTest.Equals("system.usedspace(y)")) ret = SYSTEM_USED_SPACE_Y;
    else if (strTest.Equals("system.freespace(y)")) ret = SYSTEM_FREE_SPACE_Y;
    else if (strTest.Equals("system.totalspace(y)")) ret = SYSTEM_TOTAL_SPACE_Y;
    else if (strTest.Equals("system.usedspace(z)")) ret = SYSTEM_USED_SPACE_Z;
    else if (strTest.Equals("system.freespace(z)")) ret = SYSTEM_FREE_SPACE_Z;
    else if (strTest.Equals("system.totalspace(z)")) ret = SYSTEM_TOTAL_SPACE_Z;
    else if (strTest.Equals("system.buildversion")) ret = SYSTEM_BUILD_VERSION;
    else if (strTest.Equals("system.builddate")) ret = SYSTEM_BUILD_DATE;
    else if (strTest.Equals("system.hasnetwork")) ret = SYSTEM_ETHERNET_LINK_ACTIVE;
    else if (strTest.Equals("system.fps")) ret = SYSTEM_FPS;
    else if (strTest.Equals("system.hasmediadvd")) ret = SYSTEM_MEDIA_DVD;
    else if (strTest.Equals("system.dvdready")) ret = SYSTEM_DVDREADY;
    else if (strTest.Equals("system.trayopen")) ret = SYSTEM_TRAYOPEN;
    else if (strTest.Equals("system.dvdtraystate")) ret = SYSTEM_DVD_TRAY_STATE;

    else if (strTest.Equals("system.memory(free)") || strTest.Equals("system.freememory")) ret = SYSTEM_FREE_MEMORY;
    else if (strTest.Equals("system.memory(free.percent)")) ret = SYSTEM_FREE_MEMORY_PERCENT;
    else if (strTest.Equals("system.memory(used)")) ret = SYSTEM_USED_MEMORY;
    else if (strTest.Equals("system.memory(used.percent)")) ret = SYSTEM_USED_MEMORY_PERCENT;
    else if (strTest.Equals("system.memory(total)")) ret = SYSTEM_TOTAL_MEMORY;

    else if (strTest.Equals("system.language")) ret = SYSTEM_LANGUAGE;
    else if (strTest.Equals("system.temperatureunits")) ret = SYSTEM_TEMPERATURE_UNITS;
    else if (strTest.Equals("system.screenmode")) ret = SYSTEM_SCREEN_MODE;
    else if (strTest.Equals("system.screenwidth")) ret = SYSTEM_SCREEN_WIDTH;
    else if (strTest.Equals("system.screenheight")) ret = SYSTEM_SCREEN_HEIGHT;
    else if (strTest.Equals("system.currentwindow")) ret = SYSTEM_CURRENT_WINDOW;
    else if (strTest.Equals("system.currentcontrol")) ret = SYSTEM_CURRENT_CONTROL;
    else if (strTest.Equals("system.dvdlabel")) ret = SYSTEM_DVD_LABEL;
    else if (strTest.Equals("system.haslocks")) ret = SYSTEM_HASLOCKS;
    else if (strTest.Equals("system.hasloginscreen")) ret = SYSTEM_HAS_LOGINSCREEN;
    else if (strTest.Equals("system.ismaster")) ret = SYSTEM_ISMASTER;
    else if (strTest.Equals("system.internetstate")) ret = SYSTEM_INTERNET_STATE;
    else if (strTest.Equals("system.loggedon")) ret = SYSTEM_LOGGEDON;
    else if (strTest.Equals("system.showexitbutton")) ret = SYSTEM_SHOW_EXIT_BUTTON;
    else if (strTest.Equals("system.hasdrivef")) ret = SYSTEM_HAS_DRIVE_F;
    else if (strTest.Equals("system.hasdriveg")) ret = SYSTEM_HAS_DRIVE_G;
    else if (strTest.Equals("system.kernelversion")) ret = SYSTEM_KERNEL_VERSION;
    else if (strTest.Equals("system.uptime")) ret = SYSTEM_UPTIME;
    else if (strTest.Equals("system.totaluptime")) ret = SYSTEM_TOTALUPTIME;
    else if (strTest.Equals("system.cpufrequency")) ret = SYSTEM_CPUFREQUENCY;
    else if (strTest.Equals("system.screenresolution")) ret = SYSTEM_SCREEN_RESOLUTION;
    else if (strTest.Equals("system.videoencoderinfo")) ret = SYSTEM_VIDEO_ENCODER_INFO;
    else if (strTest.Left(16).Equals("system.idletime("))
    {
      int time = atoi((strTest.Mid(16, strTest.GetLength() - 17).c_str()));
      if (time > SYSTEM_IDLE_TIME_FINISH - SYSTEM_IDLE_TIME_START)
        time = SYSTEM_IDLE_TIME_FINISH - SYSTEM_IDLE_TIME_START;
      if (time > 0)
        ret = SYSTEM_IDLE_TIME_START + time;
    }
    else if (strTest.Left(16).Equals("system.hasalarm("))
      return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_HAS_ALARM : SYSTEM_HAS_ALARM, ConditionalStringParameter(strTest.Mid(16,strTest.size()-17)), 0));
    else if (strTest.Equals("system.alarmpos")) ret = SYSTEM_ALARM_POS;
    else if (strTest.Left(24).Equals("system.alarmlessorequal("))
    {
      int pos = strTest.Find(",");
      int skinOffset = ConditionalStringParameter(strTest.Mid(24, pos-24));
      int compareString = ConditionalStringParameter(strTest.Mid(pos + 1, strTest.GetLength() - (pos + 2)));
      return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_ALARM_LESS_OR_EQUAL: SYSTEM_ALARM_LESS_OR_EQUAL, skinOffset, compareString));
    }
    else if (strTest.Equals("system.profilename")) ret = SYSTEM_PROFILENAME;
    else if (strTest.Equals("system.profilethumb")) ret = SYSTEM_PROFILETHUMB;
    else if (strTest.Equals("system.profilecount")) ret = SYSTEM_PROFILECOUNT;
    else if (strTest.Equals("system.progressbar")) ret = SYSTEM_PROGRESS_BAR;
    else if (strTest.Equals("system.platform.linux")) ret = SYSTEM_PLATFORM_LINUX;
    else if (strTest.Equals("system.platform.windows")) ret = SYSTEM_PLATFORM_WINDOWS;
    else if (strTest.Equals("system.platform.osx")) ret = SYSTEM_PLATFORM_OSX;
    else if (strTest.Left(15).Equals("system.getbool("))
      return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_GET_BOOL : SYSTEM_GET_BOOL, ConditionalStringParameter(strTest.Mid(15,strTest.size()-16)), 0));
    else if (strTest.Left(17).Equals("system.coreusage("))
      return AddMultiInfo(GUIInfo(SYSTEM_GET_CORE_USAGE, atoi(strTest.Mid(17,strTest.size()-18)), 0));
    else if (strTest.Left(17).Equals("system.hascoreid("))
      return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_HAS_CORE_ID : SYSTEM_HAS_CORE_ID, ConditionalStringParameter(strTest.Mid(17,strTest.size()-18)), 0));
    else if (strTest.Left(15).Equals("system.setting("))
      return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_SETTING : SYSTEM_SETTING, ConditionalStringParameter(strTest.Mid(15,strTest.size()-16)), 0));
    else if (strTest.Equals("system.canpowerdown")) ret = SYSTEM_CAN_POWERDOWN;
    else if (strTest.Equals("system.cansuspend"))   ret = SYSTEM_CAN_SUSPEND;
    else if (strTest.Equals("system.canhibernate")) ret = SYSTEM_CAN_HIBERNATE;
    else if (strTest.Equals("system.canreboot"))    ret = SYSTEM_CAN_REBOOT;
    else if (strTest.Left(16).Equals("system.hasaddon("))
      return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_HAS_ADDON: SYSTEM_HAS_ADDON, ConditionalStringParameter(strTest.Mid(16,strTest.size()-17)), 0));
    else if (strTest.Left(18).Equals("system.addontitle("))
    {
      CStdString param = strTest.Mid(18,strTest.size()-19);
      int info = TranslateString(param);
      if (info > 0)
        return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_ADDON_TITLE: SYSTEM_ADDON_TITLE, info, 0));
    // pipe our original string through the localize parsing then make it lowercase (picks up $LBRACKET etc.)
      CStdString label = CGUIInfoLabel::GetLabel(param).ToLower();
      int compareString = ConditionalStringParameter(label);
      return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_ADDON_TITLE: SYSTEM_ADDON_TITLE, compareString, 1));
    }
    else if (strTest.Left(17).Equals("system.addonicon("))
    {
      CStdString param = strTest.Mid(17,strTest.size()-18);
      int info = TranslateString(param);
      if (info > 0)
        return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_ADDON_ICON: SYSTEM_ADDON_ICON, info, 0));
    // pipe our original string through the localize parsing then make it lowercase (picks up $LBRACKET etc.)
      CStdString label = CGUIInfoLabel::GetLabel(param).ToLower();
      int compareString = ConditionalStringParameter(label);
      return AddMultiInfo(GUIInfo(bNegate ? -SYSTEM_ADDON_ICON : SYSTEM_ADDON_ICON, compareString, 1));
    }
  }
  // library test conditions
  else if (strTest.Left(7).Equals("library"))
  {
    if (strTest.Equals("library.hascontent(music)")) ret = LIBRARY_HAS_MUSIC;
    else if (strTest.Equals("library.hascontent(video)")) ret = LIBRARY_HAS_VIDEO;
    else if (strTest.Equals("library.hascontent(movies)")) ret = LIBRARY_HAS_MOVIES;
    else if (strTest.Equals("library.hascontent(tvshows)")) ret = LIBRARY_HAS_TVSHOWS;
    else if (strTest.Equals("library.hascontent(musicvideos)")) ret = LIBRARY_HAS_MUSICVIDEOS;
    else if (strTest.Equals("library.isscanning")) ret = LIBRARY_IS_SCANNING;
  }
  else if (strTest.Left(8).Equals("isempty("))
  {
    CStdString str = strTest.Mid(8, strTest.GetLength() - 9);
    return AddMultiInfo(GUIInfo(bNegate ? -STRING_IS_EMPTY : STRING_IS_EMPTY, TranslateSingleString(str)));
  }
  else if (strTest.Left(14).Equals("stringcompare("))
  {
    int pos = strTest.Find(",");
    int info = TranslateString(strTest.Mid(14, pos-14));
    int info2 = TranslateString(strTest.Mid(pos + 1, strTest.GetLength() - (pos + 2)));
    if (info2 > 0)
      return AddMultiInfo(GUIInfo(bNegate ? -STRING_COMPARE: STRING_COMPARE, info, -info2));
    // pipe our original string through the localize parsing then make it lowercase (picks up $LBRACKET etc.)
    CStdString label = CGUIInfoLabel::GetLabel(original.Mid(pos + 1, original.GetLength() - (pos + 2))).ToLower();
    int compareString = ConditionalStringParameter(label);
    return AddMultiInfo(GUIInfo(bNegate ? -STRING_COMPARE: STRING_COMPARE, info, compareString));
  }
  else if (strTest.Left(19).Equals("integergreaterthan("))
  {
    int pos = strTest.Find(",");
    int info = TranslateString(strTest.Mid(19, pos-19));
    int compareInt = atoi(strTest.Mid(pos + 1, strTest.GetLength() - (pos + 2)).c_str());
    return AddMultiInfo(GUIInfo(bNegate ? -INTEGER_GREATER_THAN: INTEGER_GREATER_THAN, info, compareInt));
  }
  else if (strTest.Left(10).Equals("substring("))
  {
    int pos = strTest.Find(",");
    int info = TranslateString(strTest.Mid(10, pos-10));
    // pipe our original string through the localize parsing then make it lowercase (picks up $LBRACKET etc.)
    CStdString label = CGUIInfoLabel::GetLabel(original.Mid(pos + 1, original.GetLength() - (pos + 2))).ToLower();
    int compareString = ConditionalStringParameter(label);
    return AddMultiInfo(GUIInfo(bNegate ? -STRING_STR: STRING_STR, info, compareString));
  }
  else if (strCategory.Equals("lcd"))
  {
    if (strTest.Equals("lcd.playicon")) ret = LCD_PLAY_ICON;
    else if (strTest.Equals("lcd.progressbar")) ret = LCD_PROGRESS_BAR;
    else if (strTest.Equals("lcd.cputemperature")) ret = LCD_CPU_TEMPERATURE;
    else if (strTest.Equals("lcd.gputemperature")) ret = LCD_GPU_TEMPERATURE;
    else if (strTest.Equals("lcd.hddtemperature")) ret = LCD_HDD_TEMPERATURE;
    else if (strTest.Equals("lcd.fanspeed")) ret = LCD_FAN_SPEED;
    else if (strTest.Equals("lcd.date")) ret = LCD_DATE;
    else if (strTest.Equals("lcd.freespace(c)")) ret = LCD_FREE_SPACE_C;
    else if (strTest.Equals("lcd.freespace(e)")) ret = LCD_FREE_SPACE_E;
    else if (strTest.Equals("lcd.freespace(f)")) ret = LCD_FREE_SPACE_F;
    else if (strTest.Equals("lcd.freespace(g)")) ret = LCD_FREE_SPACE_G;
    else if (strTest.Equals("lcd.Time21")) ret = LCD_TIME_21; // Small LCD numbers
    else if (strTest.Equals("lcd.Time22")) ret = LCD_TIME_22;
    else if (strTest.Equals("lcd.TimeWide21")) ret = LCD_TIME_W21; // Medium LCD numbers
    else if (strTest.Equals("lcd.TimeWide22")) ret = LCD_TIME_W22;
    else if (strTest.Equals("lcd.Time41")) ret = LCD_TIME_41; // Big LCD numbers
    else if (strTest.Equals("lcd.Time42")) ret = LCD_TIME_42;
    else if (strTest.Equals("lcd.Time43")) ret = LCD_TIME_43;
    else if (strTest.Equals("lcd.Time44")) ret = LCD_TIME_44;
  }
  else if (strCategory.Equals("network"))
  {
    if (strTest.Equals("network.ipaddress")) ret = NETWORK_IP_ADDRESS;
    if (strTest.Equals("network.isdhcp")) ret = NETWORK_IS_DHCP;
    if (strTest.Equals("network.linkstate")) ret = NETWORK_LINK_STATE;
    if (strTest.Equals("network.macaddress")) ret = NETWORK_MAC_ADDRESS;
    if (strTest.Equals("network.subnetaddress")) ret = NETWORK_SUBNET_ADDRESS;
    if (strTest.Equals("network.gatewayaddress")) ret = NETWORK_GATEWAY_ADDRESS;
    if (strTest.Equals("network.dns1address")) ret = NETWORK_DNS1_ADDRESS;
    if (strTest.Equals("network.dns2address")) ret = NETWORK_DNS2_ADDRESS;
    if (strTest.Equals("network.dhcpaddress")) ret = NETWORK_DHCP_ADDRESS;
  }
  else if (strCategory.Equals("musicplayer"))
  {
    CStdString info = strTest.Mid(strCategory.GetLength() + 1);
    if (info.Left(9).Equals("position("))
    {
      int position = atoi(info.Mid(9));
      int value = TranslateMusicPlayerString(info.Mid(info.Find(".")+1));
      ret = AddMultiInfo(GUIInfo(value, 0, position));
    }
    else if (info.Left(7).Equals("offset("))
    {
      int position = atoi(info.Mid(7));
      int value = TranslateMusicPlayerString(info.Mid(info.Find(".")+1));
      ret = AddMultiInfo(GUIInfo(value, 1, position));
    }
    else if (info.Left(13).Equals("timeremaining")) return AddMultiInfo(GUIInfo(PLAYER_TIME_REMAINING, TranslateTimeFormat(info.Mid(13))));
    else if (info.Left(9).Equals("timespeed")) return AddMultiInfo(GUIInfo(PLAYER_TIME_SPEED, TranslateTimeFormat(info.Mid(9))));
    else if (info.Left(4).Equals("time")) return AddMultiInfo(GUIInfo(PLAYER_TIME, TranslateTimeFormat(info.Mid(4))));
    else if (info.Left(8).Equals("duration")) return AddMultiInfo(GUIInfo(PLAYER_DURATION, TranslateTimeFormat(info.Mid(8))));
    else if (info.Left(9).Equals("property("))
      return AddListItemProp(info.Mid(9, info.GetLength() - 10), MUSICPLAYER_PROPERTY_OFFSET);
    else
      ret = TranslateMusicPlayerString(strTest.Mid(12));
  }
  else if (strCategory.Equals("videoplayer"))
  {
    if (strTest.Equals("videoplayer.title")) ret = VIDEOPLAYER_TITLE;
    else if (strTest.Equals("videoplayer.genre")) ret = VIDEOPLAYER_GENRE;
    else if (strTest.Equals("videoplayer.country")) ret = VIDEOPLAYER_COUNTRY;
    else if (strTest.Equals("videoplayer.originaltitle")) ret = VIDEOPLAYER_ORIGINALTITLE;
    else if (strTest.Equals("videoplayer.director")) ret = VIDEOPLAYER_DIRECTOR;
    else if (strTest.Equals("videoplayer.year")) ret = VIDEOPLAYER_YEAR;
    else if (strTest.Left(25).Equals("videoplayer.timeremaining")) ret = AddMultiInfo(GUIInfo(PLAYER_TIME_REMAINING, TranslateTimeFormat(strTest.Mid(25))));
    else if (strTest.Left(21).Equals("videoplayer.timespeed")) ret = AddMultiInfo(GUIInfo(PLAYER_TIME_SPEED, TranslateTimeFormat(strTest.Mid(21))));
    else if (strTest.Left(16).Equals("videoplayer.time")) ret = AddMultiInfo(GUIInfo(PLAYER_TIME, TranslateTimeFormat(strTest.Mid(16))));
    else if (strTest.Left(20).Equals("videoplayer.duration")) ret = AddMultiInfo(GUIInfo(PLAYER_DURATION, TranslateTimeFormat(strTest.Mid(20))));
    else if (strTest.Equals("videoplayer.cover")) ret = VIDEOPLAYER_COVER;
    else if (strTest.Equals("videoplayer.usingoverlays")) ret = VIDEOPLAYER_USING_OVERLAYS;
    else if (strTest.Equals("videoplayer.isfullscreen")) ret = VIDEOPLAYER_ISFULLSCREEN;
    else if (strTest.Equals("videoplayer.hasmenu")) ret = VIDEOPLAYER_HASMENU;
    else if (strTest.Equals("videoplayer.playlistlength")) ret = VIDEOPLAYER_PLAYLISTLEN;
    else if (strTest.Equals("videoplayer.playlistposition")) ret = VIDEOPLAYER_PLAYLISTPOS;
    else if (strTest.Equals("videoplayer.plot")) ret = VIDEOPLAYER_PLOT;
    else if (strTest.Equals("videoplayer.plotoutline")) ret = VIDEOPLAYER_PLOT_OUTLINE;
    else if (strTest.Equals("videoplayer.episode")) ret = VIDEOPLAYER_EPISODE;
    else if (strTest.Equals("videoplayer.season")) ret = VIDEOPLAYER_SEASON;
    else if (strTest.Equals("videoplayer.rating")) ret = VIDEOPLAYER_RATING;
    else if (strTest.Equals("videoplayer.ratingandvotes")) ret = VIDEOPLAYER_RATING_AND_VOTES;
    else if (strTest.Equals("videoplayer.tvshowtitle")) ret = VIDEOPLAYER_TVSHOW;
    else if (strTest.Equals("videoplayer.premiered")) ret = VIDEOPLAYER_PREMIERED;
    else if (strTest.Left(19).Equals("videoplayer.content")) return AddMultiInfo(GUIInfo(bNegate ? -VIDEOPLAYER_CONTENT : VIDEOPLAYER_CONTENT, ConditionalStringParameter(strTest.Mid(20,strTest.size()-21)), 0));
    else if (strTest.Equals("videoplayer.studio")) ret = VIDEOPLAYER_STUDIO;
    else if (strTest.Equals("videoplayer.mpaa")) return VIDEOPLAYER_MPAA;
    else if (strTest.Equals("videoplayer.top250")) return VIDEOPLAYER_TOP250;
    else if (strTest.Equals("videoplayer.cast")) return VIDEOPLAYER_CAST;
    else if (strTest.Equals("videoplayer.castandrole")) return VIDEOPLAYER_CAST_AND_ROLE;
    else if (strTest.Equals("videoplayer.artist")) return VIDEOPLAYER_ARTIST;
    else if (strTest.Equals("videoplayer.album")) return VIDEOPLAYER_ALBUM;
    else if (strTest.Equals("videoplayer.writer")) return VIDEOPLAYER_WRITER;
    else if (strTest.Equals("videoplayer.tagline")) return VIDEOPLAYER_TAGLINE;
    else if (strTest.Equals("videoplayer.hasinfo")) return VIDEOPLAYER_HAS_INFO;
    else if (strTest.Equals("videoplayer.trailer")) return VIDEOPLAYER_TRAILER;
    else if (strTest.Equals("videoplayer.videocodec")) return VIDEOPLAYER_VIDEO_CODEC;
    else if (strTest.Equals("videoplayer.videoresolution")) return VIDEOPLAYER_VIDEO_RESOLUTION;
    else if (strTest.Equals("videoplayer.videoaspect")) return VIDEOPLAYER_VIDEO_ASPECT;
    else if (strTest.Equals("videoplayer.audiocodec")) return VIDEOPLAYER_AUDIO_CODEC;
    else if (strTest.Equals("videoplayer.audiochannels")) return VIDEOPLAYER_AUDIO_CHANNELS;
    else if (strTest.Equals("videoplayer.hasteletext")) return VIDEOPLAYER_HASTELETEXT;
    else if (strTest.Equals("videoplayer.lastplayed")) return VIDEOPLAYER_LASTPLAYED;
    else if (strTest.Equals("videoplayer.playcount")) return VIDEOPLAYER_PLAYCOUNT;
  }
  else if (strCategory.Equals("playlist"))
  {
    if (strTest.Equals("playlist.length")) ret = PLAYLIST_LENGTH;
    else if (strTest.Equals("playlist.position")) ret = PLAYLIST_POSITION;
    else if (strTest.Equals("playlist.random")) ret = PLAYLIST_RANDOM;
    else if (strTest.Equals("playlist.repeat")) ret = PLAYLIST_REPEAT;
    else if (strTest.Equals("playlist.israndom")) ret = PLAYLIST_ISRANDOM;
    else if (strTest.Equals("playlist.isrepeat")) ret = PLAYLIST_ISREPEAT;
    else if (strTest.Equals("playlist.isrepeatone")) ret = PLAYLIST_ISREPEATONE;
  }
  else if (strCategory.Equals("musicpartymode"))
  {
    if (strTest.Equals("musicpartymode.enabled")) ret = MUSICPM_ENABLED;
    else if (strTest.Equals("musicpartymode.songsplayed")) ret = MUSICPM_SONGSPLAYED;
    else if (strTest.Equals("musicpartymode.matchingsongs")) ret = MUSICPM_MATCHINGSONGS;
    else if (strTest.Equals("musicpartymode.matchingsongspicked")) ret = MUSICPM_MATCHINGSONGSPICKED;
    else if (strTest.Equals("musicpartymode.matchingsongsleft")) ret = MUSICPM_MATCHINGSONGSLEFT;
    else if (strTest.Equals("musicpartymode.relaxedsongspicked")) ret = MUSICPM_RELAXEDSONGSPICKED;
    else if (strTest.Equals("musicpartymode.randomsongspicked")) ret = MUSICPM_RANDOMSONGSPICKED;
  }
  else if (strCategory.Equals("audioscrobbler"))
  {
    if (strTest.Equals("audioscrobbler.enabled")) ret = AUDIOSCROBBLER_ENABLED;
    else if (strTest.Equals("audioscrobbler.connectstate")) ret = AUDIOSCROBBLER_CONN_STATE;
    else if (strTest.Equals("audioscrobbler.submitinterval")) ret = AUDIOSCROBBLER_SUBMIT_INT;
    else if (strTest.Equals("audioscrobbler.filescached")) ret = AUDIOSCROBBLER_FILES_CACHED;
    else if (strTest.Equals("audioscrobbler.submitstate")) ret = AUDIOSCROBBLER_SUBMIT_STATE;
  }
  else if (strCategory.Equals("lastfm"))
  {
    if (strTest.Equals("lastfm.radioplaying")) ret = LASTFM_RADIOPLAYING;
    else if (strTest.Equals("lastfm.canlove")) ret = LASTFM_CANLOVE;
    else if (strTest.Equals("lastfm.canban")) ret = LASTFM_CANBAN;
  }
  else if (strCategory.Equals("slideshow"))
    ret = CPictureInfoTag::TranslateString(strTest.Mid(strCategory.GetLength() + 1));
  else if (strCategory.Left(9).Equals("container"))
  {
    int id = atoi(strCategory.Mid(10, strCategory.GetLength() - 11));
    CStdString info = strTest.Mid(strCategory.GetLength() + 1);
    if (info.Left(14).Equals("listitemnowrap"))
    {
      int offset = atoi(info.Mid(15, info.GetLength() - 16));
      ret = TranslateListItem(info.Mid(info.Find(".")+1));
      if (offset || id)
        return AddMultiInfo(GUIInfo(bNegate ? -ret : ret, id, offset));
    }
    else if (info.Left(16).Equals("listitemposition"))
    {
      int offset = atoi(info.Mid(17, info.GetLength() - 18));
      ret = TranslateListItem(info.Mid(info.Find(".")+1));
      if (offset || id)
        return AddMultiInfo(GUIInfo(bNegate ? -ret : ret, id, offset, INFOFLAG_LISTITEM_POSITION));
    }
    else if (info.Left(8).Equals("listitem"))
    {
      int offset = atoi(info.Mid(9, info.GetLength() - 10));
      ret = TranslateListItem(info.Mid(info.Find(".")+1));
      if (offset || id)
        return AddMultiInfo(GUIInfo(bNegate ? -ret : ret, id, offset, INFOFLAG_LISTITEM_WRAP));
    }
    else if (info.Equals("hasfiles")) ret = CONTAINER_HASFILES;
    else if (info.Equals("hasfolders")) ret = CONTAINER_HASFOLDERS;
    else if (info.Equals("isstacked")) ret = CONTAINER_STACKED;
    else if (info.Equals("folderthumb")) ret = CONTAINER_FOLDERTHUMB;
    else if (info.Equals("tvshowthumb")) ret = CONTAINER_TVSHOWTHUMB;
    else if (info.Equals("seasonthumb")) ret = CONTAINER_SEASONTHUMB;
    else if (info.Equals("folderpath")) ret = CONTAINER_FOLDERPATH;
    else if (info.Equals("foldername")) ret = CONTAINER_FOLDERNAME;
    else if (info.Equals("pluginname")) ret = CONTAINER_PLUGINNAME;
    else if (info.Equals("viewmode")) ret = CONTAINER_VIEWMODE;
    else if (info.Equals("onnext")) ret = CONTAINER_MOVE_NEXT;
    else if (info.Equals("onprevious")) ret = CONTAINER_MOVE_PREVIOUS;
    else if (info.Equals("onscrollnext")) ret = CONTAINER_SCROLL_NEXT;
    else if (info.Equals("onscrollprevious")) ret = CONTAINER_SCROLL_PREVIOUS;
    else if (info.Equals("totaltime")) ret = CONTAINER_TOTALTIME;
    else if (info.Equals("scrolling"))
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_SCROLLING : CONTAINER_SCROLLING, id, 0));
    else if (info.Equals("hasnext"))
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_HAS_NEXT : CONTAINER_HAS_NEXT, id, 0));
    else if (info.Equals("hasprevious"))
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_HAS_PREVIOUS : CONTAINER_HAS_PREVIOUS, id, 0));
    else if (info.Left(8).Equals("content("))
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_CONTENT : CONTAINER_CONTENT, ConditionalStringParameter(info.Mid(8,info.GetLength()-9)), 0));
    else if (info.Left(4).Equals("row("))
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_ROW : CONTAINER_ROW, id, atoi(info.Mid(4, info.GetLength() - 5))));
    else if (info.Left(7).Equals("column("))
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_COLUMN : CONTAINER_COLUMN, id, atoi(info.Mid(7, info.GetLength() - 8))));
    else if (info.Left(8).Equals("position"))
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_POSITION : CONTAINER_POSITION, id, atoi(info.Mid(9, info.GetLength() - 10))));
    else if (info.Left(8).Equals("subitem("))
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_SUBITEM : CONTAINER_SUBITEM, id, atoi(info.Mid(8, info.GetLength() - 9))));
    else if (info.Equals("hasthumb")) ret = CONTAINER_HAS_THUMB;
    else if (info.Equals("numpages")) ret = CONTAINER_NUM_PAGES;
    else if (info.Equals("numitems")) ret = CONTAINER_NUM_ITEMS;
    else if (info.Equals("currentpage")) ret = CONTAINER_CURRENT_PAGE;
    else if (info.Equals("sortmethod")) ret = CONTAINER_SORT_METHOD;
    else if (info.Left(13).Equals("sortdirection"))
    {
      CStdString direction = info.Mid(14, info.GetLength() - 15);
      SORT_ORDER order = SORT_ORDER_NONE;
      if (direction == "ascending")
        order = SORT_ORDER_ASC;
      else if (direction == "descending")
        order = SORT_ORDER_DESC;
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_SORT_DIRECTION : CONTAINER_SORT_DIRECTION, order));
    }
    else if (info.Left(5).Equals("sort("))
    {
      SORT_METHOD sort = SORT_METHOD_NONE;
      CStdString method(info.Mid(5, info.GetLength() - 6));
      if (method.Equals("songrating")) sort = SORT_METHOD_SONG_RATING;
      if (sort != SORT_METHOD_NONE)
        return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_SORT_METHOD : CONTAINER_SORT_METHOD, sort));
    }
    else if (id && info.Left(9).Equals("hasfocus("))
    {
      int itemID = atoi(info.Mid(9, info.GetLength() - 10));
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_HAS_FOCUS : CONTAINER_HAS_FOCUS, id, itemID));
    }
    else if (info.Left(9).Equals("property("))
    {
      int compareString = ConditionalStringParameter(info.Mid(9, info.GetLength() - 10));
      return AddMultiInfo(GUIInfo(CONTAINER_PROPERTY, id, compareString));
    }
    else if (info.Equals("showplot")) ret = CONTAINER_SHOWPLOT;
    if (id && ((ret >= CONTAINER_SCROLL_PREVIOUS && ret <= CONTAINER_SCROLL_NEXT) || ret == CONTAINER_NUM_PAGES ||
               ret == CONTAINER_NUM_ITEMS || ret == CONTAINER_CURRENT_PAGE))
      return AddMultiInfo(GUIInfo(bNegate ? -ret : ret, id));
  }
  else if (strCategory.Left(8).Equals("listitem"))
  {
    int offset = atoi(strCategory.Mid(9, strCategory.GetLength() - 10));
    ret = TranslateListItem(strTest.Mid(strCategory.GetLength() + 1));
    if (offset || ret == LISTITEM_ISSELECTED || ret == LISTITEM_ISPLAYING || ret == LISTITEM_IS_FOLDER)
      return AddMultiInfo(GUIInfo(bNegate ? -ret : ret, 0, offset, INFOFLAG_LISTITEM_WRAP));
  }
  else if (strCategory.Left(16).Equals("listitemposition"))
  {
    int offset = atoi(strCategory.Mid(17, strCategory.GetLength() - 18));
    ret = TranslateListItem(strCategory.Mid(strCategory.GetLength()+1));
    if (offset || ret == LISTITEM_ISSELECTED || ret == LISTITEM_ISPLAYING || ret == LISTITEM_IS_FOLDER)
      return AddMultiInfo(GUIInfo(bNegate ? -ret : ret, 0, offset, INFOFLAG_LISTITEM_POSITION));
  }
  else if (strCategory.Left(14).Equals("listitemnowrap"))
  {
    int offset = atoi(strCategory.Mid(15, strCategory.GetLength() - 16));
    ret = TranslateListItem(strTest.Mid(strCategory.GetLength() + 1));
    if (offset || ret == LISTITEM_ISSELECTED || ret == LISTITEM_ISPLAYING || ret == LISTITEM_IS_FOLDER)
      return AddMultiInfo(GUIInfo(bNegate ? -ret : ret, 0, offset));
  }
  else if (strCategory.Equals("visualisation"))
  {
    if (strTest.Equals("visualisation.locked")) ret = VISUALISATION_LOCKED;
    else if (strTest.Equals("visualisation.preset")) ret = VISUALISATION_PRESET;
    else if (strTest.Equals("visualisation.name")) ret = VISUALISATION_NAME;
    else if (strTest.Equals("visualisation.enabled")) ret = VISUALISATION_ENABLED;
  }
  else if (strCategory.Equals("fanart"))
  {
    if (strTest.Equals("fanart.color1")) ret = FANART_COLOR1;
    else if (strTest.Equals("fanart.color2")) ret = FANART_COLOR2;
    else if (strTest.Equals("fanart.color3")) ret = FANART_COLOR3;
    else if (strTest.Equals("fanart.image")) ret = FANART_IMAGE;
  }
  else if (strCategory.Equals("skin"))
  {
    if (strTest.Equals("skin.currenttheme"))
      ret = SKIN_THEME;
    else if (strTest.Equals("skin.currentcolourtheme"))
      ret = SKIN_COLOUR_THEME;
    else if (strTest.Left(12).Equals("skin.string("))
    {
      int pos = strTest.Find(",");
      if (pos >= 0)
      {
        int skinOffset = g_settings.TranslateSkinString(strTest.Mid(12, pos - 12));
        int compareString = ConditionalStringParameter(strTest.Mid(pos + 1, strTest.GetLength() - (pos + 2)));
        return AddMultiInfo(GUIInfo(bNegate ? -SKIN_STRING : SKIN_STRING, skinOffset, compareString));
      }
      int skinOffset = g_settings.TranslateSkinString(strTest.Mid(12, strTest.GetLength() - 13));
      return AddMultiInfo(GUIInfo(bNegate ? -SKIN_STRING : SKIN_STRING, skinOffset));
    }
    else if (strTest.Left(16).Equals("skin.hassetting("))
    {
      int skinOffset = g_settings.TranslateSkinBool(strTest.Mid(16, strTest.GetLength() - 17));
      return AddMultiInfo(GUIInfo(bNegate ? -SKIN_BOOL : SKIN_BOOL, skinOffset));
    }
    else if (strTest.Left(14).Equals("skin.hastheme("))
      ret = SKIN_HAS_THEME_START + ConditionalStringParameter(strTest.Mid(14, strTest.GetLength() -  15));
  }
  else if (strCategory.Left(6).Equals("window"))
  {
    CStdString info = strTest.Mid(strCategory.GetLength() + 1);
    // special case for window.xml parameter, fails above
    if (info.Left(5).Equals("xml)."))
      info = info.Mid(5, info.GetLength() + 1);
    if (info.Left(9).Equals("property("))
    {
      int winID = 0;
      if (strTest.Left(7).Equals("window("))
      {
        CStdString window(strTest.Mid(7, strTest.Find(")", 7) - 7).ToLower());
        winID = CButtonTranslator::TranslateWindow(window);
      }
      if (winID != WINDOW_INVALID)
      {
        int compareString = ConditionalStringParameter(info.Mid(9, info.GetLength() - 10));
        return AddMultiInfo(GUIInfo(WINDOW_PROPERTY, winID, compareString));
      }
    }
    else if (info.Left(9).Equals("isactive("))
    {
      CStdString window(strTest.Mid(16, strTest.GetLength() - 17).ToLower());
      if (window.Find("xml") >= 0)
        return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_ACTIVE : WINDOW_IS_ACTIVE, 0, ConditionalStringParameter(window)));
      int winID = CButtonTranslator::TranslateWindow(window);
      if (winID != WINDOW_INVALID)
        return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_ACTIVE : WINDOW_IS_ACTIVE, winID, 0));
    }
    else if (info.Left(7).Equals("ismedia")) return WINDOW_IS_MEDIA;
    else if (info.Left(10).Equals("istopmost("))
    {
      CStdString window(strTest.Mid(17, strTest.GetLength() - 18).ToLower());
      if (window.Find("xml") >= 0)
        return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_TOPMOST : WINDOW_IS_TOPMOST, 0, ConditionalStringParameter(window)));
      int winID = CButtonTranslator::TranslateWindow(window);
      if (winID != WINDOW_INVALID)
        return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_TOPMOST : WINDOW_IS_TOPMOST, winID, 0));
    }
    else if (info.Left(10).Equals("isvisible("))
    {
      CStdString window(strTest.Mid(17, strTest.GetLength() - 18).ToLower());
      if (window.Find("xml") >= 0)
        return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_VISIBLE : WINDOW_IS_VISIBLE, 0, ConditionalStringParameter(window)));
      int winID = CButtonTranslator::TranslateWindow(window);
      if (winID != WINDOW_INVALID)
        return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_VISIBLE : WINDOW_IS_VISIBLE, winID, 0));
    }
    else if (info.Left(9).Equals("previous("))
    {
      CStdString window(strTest.Mid(16, strTest.GetLength() - 17).ToLower());
      if (window.Find("xml") >= 0)
        return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_PREVIOUS : WINDOW_PREVIOUS, 0, ConditionalStringParameter(window)));
      int winID = CButtonTranslator::TranslateWindow(window);
      if (winID != WINDOW_INVALID)
        return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_PREVIOUS : WINDOW_PREVIOUS, winID, 0));
    }
    else if (info.Left(5).Equals("next("))
    {
      CStdString window(strTest.Mid(12, strTest.GetLength() - 13).ToLower());
      if (window.Find("xml") >= 0)
        return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_NEXT : WINDOW_NEXT, 0, ConditionalStringParameter(window)));
      int winID = CButtonTranslator::TranslateWindow(window);
      if (winID != WINDOW_INVALID)
        return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_NEXT : WINDOW_NEXT, winID, 0));
    }
  }
  else if (strTest.Left(17).Equals("control.hasfocus("))
  {
    int controlID = atoi(strTest.Mid(17, strTest.GetLength() - 18).c_str());
    if (controlID)
      return AddMultiInfo(GUIInfo(bNegate ? -CONTROL_HAS_FOCUS : CONTROL_HAS_FOCUS, controlID, 0));
  }
  else if (strTest.Left(18).Equals("control.isvisible("))
  {
    int controlID = atoi(strTest.Mid(18, strTest.GetLength() - 19).c_str());
    if (controlID)
      return AddMultiInfo(GUIInfo(bNegate ? -CONTROL_IS_VISIBLE : CONTROL_IS_VISIBLE, controlID, 0));
  }
  else if (strTest.Left(18).Equals("control.isenabled("))
  {
    int controlID = atoi(strTest.Mid(18, strTest.GetLength() - 19).c_str());
    if (controlID)
      return AddMultiInfo(GUIInfo(bNegate ? -CONTROL_IS_ENABLED : CONTROL_IS_ENABLED, controlID, 0));
  }
  else if (strTest.Left(17).Equals("control.getlabel("))
  {
    int controlID = atoi(strTest.Mid(17, strTest.GetLength() - 18).c_str());
    if (controlID)
      return AddMultiInfo(GUIInfo(bNegate ? -CONTROL_GET_LABEL : CONTROL_GET_LABEL, controlID, 0));
  }
  else if (strTest.Left(13).Equals("controlgroup("))
  {
    int groupID = atoi(strTest.Mid(13).c_str());
    int controlID = 0;
    int controlPos = strTest.Find(".hasfocus(");
    if (controlPos > 0)
      controlID = atoi(strTest.Mid(controlPos + 10).c_str());
    if (groupID)
    {
      return AddMultiInfo(GUIInfo(bNegate ? -CONTROL_GROUP_HAS_FOCUS : CONTROL_GROUP_HAS_FOCUS, groupID, controlID));
    }
  }
  else if (strTest.Left(24).Equals("buttonscroller.hasfocus("))
  {
    int controlID = atoi(strTest.Mid(24, strTest.GetLength() - 24).c_str());
    if (controlID)
      return AddMultiInfo(GUIInfo(bNegate ? -BUTTON_SCROLLER_HAS_ICON : BUTTON_SCROLLER_HAS_ICON, controlID, 0));
  }

  return bNegate ? -ret : ret;
}

int CGUIInfoManager::TranslateListItem(const CStdString &info)
{
  if (info.Equals("thumb")) return LISTITEM_THUMB;
  else if (info.Equals("icon")) return LISTITEM_ICON;
  else if (info.Equals("actualicon")) return LISTITEM_ACTUAL_ICON;
  else if (info.Equals("overlay")) return LISTITEM_OVERLAY;
  else if (info.Equals("label")) return LISTITEM_LABEL;
  else if (info.Equals("label2")) return LISTITEM_LABEL2;
  else if (info.Equals("title")) return LISTITEM_TITLE;
  else if (info.Equals("tracknumber")) return LISTITEM_TRACKNUMBER;
  else if (info.Equals("artist")) return LISTITEM_ARTIST;
  else if (info.Equals("album")) return LISTITEM_ALBUM;
  else if (info.Equals("albumartist")) return LISTITEM_ALBUM_ARTIST;
  else if (info.Equals("year")) return LISTITEM_YEAR;
  else if (info.Equals("genre")) return LISTITEM_GENRE;
  else if (info.Equals("director")) return LISTITEM_DIRECTOR;
  else if (info.Equals("filename")) return LISTITEM_FILENAME;
  else if (info.Equals("filenameandpath")) return LISTITEM_FILENAME_AND_PATH;
  else if (info.Equals("date")) return LISTITEM_DATE;
  else if (info.Equals("size")) return LISTITEM_SIZE;
  else if (info.Equals("rating")) return LISTITEM_RATING;
  else if (info.Equals("ratingandvotes")) return LISTITEM_RATING_AND_VOTES;
  else if (info.Equals("programcount")) return LISTITEM_PROGRAM_COUNT;
  else if (info.Equals("duration")) return LISTITEM_DURATION;
  else if (info.Equals("isselected")) return LISTITEM_ISSELECTED;
  else if (info.Equals("isplaying")) return LISTITEM_ISPLAYING;
  else if (info.Equals("plot")) return LISTITEM_PLOT;
  else if (info.Equals("plotoutline")) return LISTITEM_PLOT_OUTLINE;
  else if (info.Equals("episode")) return LISTITEM_EPISODE;
  else if (info.Equals("season")) return LISTITEM_SEASON;
  else if (info.Equals("tvshowtitle")) return LISTITEM_TVSHOW;
  else if (info.Equals("premiered")) return LISTITEM_PREMIERED;
  else if (info.Equals("comment")) return LISTITEM_COMMENT;
  else if (info.Equals("path")) return LISTITEM_PATH;
  else if (info.Equals("foldername")) return LISTITEM_FOLDERNAME;
  else if (info.Equals("folderpath")) return LISTITEM_FOLDERPATH;
  else if (info.Equals("picturepath")) return LISTITEM_PICTURE_PATH;
  else if (info.Equals("pictureresolution")) return LISTITEM_PICTURE_RESOLUTION;
  else if (info.Equals("picturedatetime")) return LISTITEM_PICTURE_DATETIME;
  else if (info.Equals("studio")) return LISTITEM_STUDIO;
  else if (info.Equals("country")) return LISTITEM_COUNTRY;
  else if (info.Equals("mpaa")) return LISTITEM_MPAA;
  else if (info.Equals("cast")) return LISTITEM_CAST;
  else if (info.Equals("castandrole")) return LISTITEM_CAST_AND_ROLE;
  else if (info.Equals("writer")) return LISTITEM_WRITER;
  else if (info.Equals("tagline")) return LISTITEM_TAGLINE;
  else if (info.Equals("top250")) return LISTITEM_TOP250;
  else if (info.Equals("trailer")) return LISTITEM_TRAILER;
  else if (info.Equals("starrating")) return LISTITEM_STAR_RATING;
  else if (info.Equals("sortletter")) return LISTITEM_SORT_LETTER;
  else if (info.Equals("videocodec")) return LISTITEM_VIDEO_CODEC;
  else if (info.Equals("videoresolution")) return LISTITEM_VIDEO_RESOLUTION;
  else if (info.Equals("videoaspect")) return LISTITEM_VIDEO_ASPECT;
  else if (info.Equals("audiocodec")) return LISTITEM_AUDIO_CODEC;
  else if (info.Equals("audiochannels")) return LISTITEM_AUDIO_CHANNELS;
  else if (info.Equals("audiolanguage")) return LISTITEM_AUDIO_LANGUAGE;
  else if (info.Equals("subtitlelanguage")) return LISTITEM_SUBTITLE_LANGUAGE;
  else if (info.Equals("isfolder")) return LISTITEM_IS_FOLDER;
  else if (info.Equals("originaltitle")) return LISTITEM_ORIGINALTITLE;
  else if (info.Equals("lastplayed")) return LISTITEM_LASTPLAYED;
  else if (info.Equals("playcount")) return LISTITEM_PLAYCOUNT;
  else if (info.Equals("discnumber")) return LISTITEM_DISC_NUMBER;
  else if (info.Left(9).Equals("property(")) return AddListItemProp(info.Mid(9, info.GetLength() - 10));
  return 0;
}

int CGUIInfoManager::TranslateMusicPlayerString(const CStdString &info) const
{
  if (info.Equals("title")) return MUSICPLAYER_TITLE;
  else if (info.Equals("album")) return MUSICPLAYER_ALBUM;
  else if (info.Equals("artist")) return MUSICPLAYER_ARTIST;
  else if (info.Equals("albumartist")) return MUSICPLAYER_ALBUM_ARTIST;
  else if (info.Equals("year")) return MUSICPLAYER_YEAR;
  else if (info.Equals("genre")) return MUSICPLAYER_GENRE;
  else if (info.Equals("duration")) return MUSICPLAYER_DURATION;
  else if (info.Equals("tracknumber")) return MUSICPLAYER_TRACK_NUMBER;
  else if (info.Equals("cover")) return MUSICPLAYER_COVER;
  else if (info.Equals("bitrate")) return MUSICPLAYER_BITRATE;
  else if (info.Equals("playlistlength")) return MUSICPLAYER_PLAYLISTLEN;
  else if (info.Equals("playlistposition")) return MUSICPLAYER_PLAYLISTPOS;
  else if (info.Equals("channels")) return MUSICPLAYER_CHANNELS;
  else if (info.Equals("bitspersample")) return MUSICPLAYER_BITSPERSAMPLE;
  else if (info.Equals("samplerate")) return MUSICPLAYER_SAMPLERATE;
  else if (info.Equals("codec")) return MUSICPLAYER_CODEC;
  else if (info.Equals("discnumber")) return MUSICPLAYER_DISC_NUMBER;
  else if (info.Equals("rating")) return MUSICPLAYER_RATING;
  else if (info.Equals("comment")) return MUSICPLAYER_COMMENT;
  else if (info.Equals("lyrics")) return MUSICPLAYER_LYRICS;
  else if (info.Equals("playlistplaying")) return MUSICPLAYER_PLAYLISTPLAYING;
  else if (info.Equals("exists")) return MUSICPLAYER_EXISTS;
  else if (info.Equals("hasprevious")) return MUSICPLAYER_HASPREVIOUS;
  else if (info.Equals("hasnext")) return MUSICPLAYER_HASNEXT;
  else if (info.Equals("playcount")) return MUSICPLAYER_PLAYCOUNT;
  else if (info.Equals("lastplayed")) return MUSICPLAYER_LASTPLAYED;
  return 0;
}

TIME_FORMAT CGUIInfoManager::TranslateTimeFormat(const CStdString &format)
{
  if (format.IsEmpty()) return TIME_FORMAT_GUESS;
  else if (format.Equals("(hh)")) return TIME_FORMAT_HH;
  else if (format.Equals("(mm)")) return TIME_FORMAT_MM;
  else if (format.Equals("(ss)")) return TIME_FORMAT_SS;
  else if (format.Equals("(hh:mm)")) return TIME_FORMAT_HH_MM;
  else if (format.Equals("(mm:ss)")) return TIME_FORMAT_MM_SS;
  else if (format.Equals("(hh:mm:ss)")) return TIME_FORMAT_HH_MM_SS;
  else if (format.Equals("(h)")) return TIME_FORMAT_H;
  else if (format.Equals("(h:mm:ss)")) return TIME_FORMAT_H_MM_SS;
  return TIME_FORMAT_GUESS;
}

CStdString CGUIInfoManager::GetLabel(int info, int contextWindow)
{
  CStdString strLabel;
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow);

  if (info >= SLIDE_INFO_START && info <= SLIDE_INFO_END)
    return GetPictureLabel(info);

  if (info >= LISTITEM_PROPERTY_START+MUSICPLAYER_PROPERTY_OFFSET &&
      info - (LISTITEM_PROPERTY_START+MUSICPLAYER_PROPERTY_OFFSET) < (int)m_listitemProperties.size())
  { // grab the property
    if (!m_currentFile)
      return "";

    CStdString property = m_listitemProperties[info - LISTITEM_PROPERTY_START-MUSICPLAYER_PROPERTY_OFFSET];
    return m_currentFile->GetProperty(property);
  }

  if (info >= LISTITEM_START && info <= LISTITEM_END)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS); // true for has list items
    if (window)
    {
      CFileItemPtr item = window->GetCurrentListItem();
      strLabel = GetItemLabel(item.get(), info);
    }

    return strLabel;
  }

  switch (info)
  {
  case WEATHER_CONDITIONS:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_COND);
    strLabel = strLabel.Trim();
    break;
  case WEATHER_TEMPERATURE:
    strLabel.Format("%s%s", g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_TEMP), g_langInfo.GetTempUnitString().c_str());
    break;
  case WEATHER_LOCATION:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_LOCATION);
    break;
  case WEATHER_FANART_CODE:
    strLabel = URIUtils::GetFileName(g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON));
    URIUtils::RemoveExtension(strLabel);
    break;
  case WEATHER_PLUGIN:
    strLabel = g_guiSettings.GetString("weather.script");
    break;
  case SYSTEM_DATE:
    strLabel = GetDate();
    break;
  case LCD_DATE:
    strLabel = GetDate(true);
    break;
  case SYSTEM_FPS:
    strLabel.Format("%02.2f", m_fps);
    break;
  case PLAYER_VOLUME:
    strLabel.Format("%2.1f dB", (float)(g_settings.m_nVolumeLevel + g_settings.m_dynamicRangeCompressionLevel) * 0.01f);
    break;
  case PLAYER_SUBTITLE_DELAY:
    strLabel.Format("%2.3f s", g_settings.m_currentVideoSettings.m_SubtitleDelay);
    break;
  case PLAYER_AUDIO_DELAY:
    strLabel.Format("%2.3f s", g_settings.m_currentVideoSettings.m_AudioDelay);
    break;
  case PLAYER_CHAPTER:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel.Format("%02d", g_application.m_pPlayer->GetChapter());
    break;
  case PLAYER_CHAPTERCOUNT:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel.Format("%02d", g_application.m_pPlayer->GetChapterCount());
    break;
  case PLAYER_CHAPTERNAME:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      g_application.m_pPlayer->GetChapterName(strLabel);
    break;
  case PLAYER_CACHELEVEL:
    {
      int iLevel = 0;
      if(g_application.IsPlaying() && ((iLevel = GetInt(PLAYER_CACHELEVEL)) >= 0))
        strLabel.Format("%i", iLevel);
    }
    break;
  case PLAYER_TIME:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel = GetCurrentPlayTime(TIME_FORMAT_HH_MM);
    break;
  case PLAYER_DURATION:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel = GetDuration(TIME_FORMAT_HH_MM);
    break;
  case PLAYER_PATH:
  case PLAYER_FILEPATH:
    if (m_currentFile)
    {
      if (m_currentFile->HasMusicInfoTag())
        strLabel = m_currentFile->GetMusicInfoTag()->GetURL();
      else if (m_currentFile->HasVideoInfoTag())
        strLabel = m_currentFile->GetVideoInfoTag()->m_strFileNameAndPath;
      if (strLabel.IsEmpty())
        strLabel = m_currentFile->m_strPath;
    }
    if (info == PLAYER_PATH)
    {
      // do this twice since we want the path outside the archive if this
      // is to be of use.
      if (URIUtils::IsInArchive(strLabel))
        strLabel = URIUtils::GetParentPath(strLabel);
      strLabel = URIUtils::GetParentPath(strLabel);
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
  case VIDEOPLAYER_PLAYCOUNT:
  case VIDEOPLAYER_LASTPLAYED:
    strLabel = GetVideoLabel(info);
  break;
  case VIDEOPLAYER_VIDEO_CODEC:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel = g_application.m_pPlayer->GetVideoCodecName();
    break;
  case VIDEOPLAYER_VIDEO_RESOLUTION:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      return CStreamDetails::VideoDimsToResolutionDescription(g_application.m_pPlayer->GetPictureWidth(), g_application.m_pPlayer->GetPictureHeight());
    break;
  case VIDEOPLAYER_AUDIO_CODEC:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel = g_application.m_pPlayer->GetAudioCodecName();
    break;
  case VIDEOPLAYER_VIDEO_ASPECT:
    if (g_application.IsPlaying() && g_application.m_pPlayer)
    {
      float aspect;
      g_application.m_pPlayer->GetVideoAspectRatio(aspect);
      strLabel = CStreamDetails::VideoAspectToAspectDescription(aspect);
    }
    break;
  case VIDEOPLAYER_AUDIO_CHANNELS:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel.Format("%i", g_application.m_pPlayer->GetChannels());
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
  case SYSTEM_FREE_SPACE_C:
  case SYSTEM_FREE_SPACE_E:
  case SYSTEM_FREE_SPACE_F:
  case SYSTEM_FREE_SPACE_G:
  case SYSTEM_USED_SPACE:
  case SYSTEM_USED_SPACE_C:
  case SYSTEM_USED_SPACE_E:
  case SYSTEM_USED_SPACE_F:
  case SYSTEM_USED_SPACE_G:
  case SYSTEM_TOTAL_SPACE:
  case SYSTEM_TOTAL_SPACE_C:
  case SYSTEM_TOTAL_SPACE_E:
  case SYSTEM_TOTAL_SPACE_F:
  case SYSTEM_TOTAL_SPACE_G:
  case SYSTEM_FREE_SPACE_PERCENT:
  case SYSTEM_FREE_SPACE_PERCENT_C:
  case SYSTEM_FREE_SPACE_PERCENT_E:
  case SYSTEM_FREE_SPACE_PERCENT_F:
  case SYSTEM_FREE_SPACE_PERCENT_G:
  case SYSTEM_USED_SPACE_PERCENT:
  case SYSTEM_USED_SPACE_PERCENT_C:
  case SYSTEM_USED_SPACE_PERCENT_E:
  case SYSTEM_USED_SPACE_PERCENT_F:
  case SYSTEM_USED_SPACE_PERCENT_G:
  case SYSTEM_USED_SPACE_X:
  case SYSTEM_FREE_SPACE_X:
  case SYSTEM_TOTAL_SPACE_X:
  case SYSTEM_USED_SPACE_Y:
  case SYSTEM_FREE_SPACE_Y:
  case SYSTEM_TOTAL_SPACE_Y:
  case SYSTEM_USED_SPACE_Z:
  case SYSTEM_FREE_SPACE_Z:
  case SYSTEM_TOTAL_SPACE_Z:
    return g_sysinfo.GetHddSpaceInfo(info);
  break;

  case LCD_FREE_SPACE_C:
  case LCD_FREE_SPACE_E:
  case LCD_FREE_SPACE_F:
  case LCD_FREE_SPACE_G:
    return g_sysinfo.GetHddSpaceInfo(info, true);
    break;

  case SYSTEM_CPU_TEMPERATURE:
  case SYSTEM_GPU_TEMPERATURE:
  case SYSTEM_FAN_SPEED:
  case LCD_CPU_TEMPERATURE:
  case LCD_GPU_TEMPERATURE:
  case LCD_FAN_SPEED:
  case SYSTEM_CPU_USAGE:
    return GetSystemHeatInfo(info);
    break;

  case SYSTEM_VIDEO_ENCODER_INFO:
  case NETWORK_MAC_ADDRESS:
  case SYSTEM_KERNEL_VERSION:
  case SYSTEM_CPUFREQUENCY:
  case SYSTEM_INTERNET_STATE:
  case SYSTEM_UPTIME:
  case SYSTEM_TOTALUPTIME:
    return g_sysinfo.GetInfo(info);
    break;

  case SYSTEM_SCREEN_RESOLUTION:
    if (g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].bFullScreen)
      strLabel.Format("%ix%i@%.2fHz - %s (%02.2f fps)",
        g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth,
        g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight,
        g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].fRefreshRate,
        g_localizeStrings.Get(244), GetFPS());
    else
      strLabel.Format("%ix%i - %s (%02.2f fps)",
        g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth,
        g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight,
        g_localizeStrings.Get(242), GetFPS());
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
          strLabel = CURL(((CGUIMediaWindow*)window)->CurrentDirectory().m_strPath).GetWithoutUserDetails();
      }
      break;
    }
  case CONTAINER_PLUGINNAME:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
      {
        CURL url(((CGUIMediaWindow*)window)->CurrentDirectory().m_strPath);
        if (url.GetProtocol().Equals("plugin"))
        {
          strLabel = url.GetFileName();
          URIUtils::RemoveSlashAtEnd(strLabel);
        }
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
          strLabel = ((CGUIBaseContainer *)control)->GetLabel();
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
  case CONTAINER_CURRENT_PAGE:
    return GetMultiInfoLabel(GUIInfo(info), contextWindow);
    break;
  case CONTAINER_SHOWPLOT:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("showplot");
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
      MEMORYSTATUS stat;
      GlobalMemoryStatus(&stat);
      int iMemPercentFree = 100 - ((int)( 100.0f* (stat.dwTotalPhys - stat.dwAvailPhys)/stat.dwTotalPhys + 0.5f ));
      int iMemPercentUsed = 100 - iMemPercentFree;

      if (info == SYSTEM_FREE_MEMORY)
        strLabel.Format("%luMB", (ULONG)(stat.dwAvailPhys/MB));
      else if (info == SYSTEM_FREE_MEMORY_PERCENT)
        strLabel.Format("%i%%", iMemPercentFree);
      else if (info == SYSTEM_USED_MEMORY)
        strLabel.Format("%luMB", (ULONG)((stat.dwTotalPhys - stat.dwAvailPhys)/MB));
      else if (info == SYSTEM_USED_MEMORY_PERCENT)
        strLabel.Format("%i%%", iMemPercentUsed);
      else if (info == SYSTEM_TOTAL_MEMORY)
        strLabel.Format("%luMB", (ULONG)(stat.dwTotalPhys/MB));
    }
    break;
  case SYSTEM_SCREEN_MODE:
    strLabel = g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].strMode;
    break;
  case SYSTEM_SCREEN_WIDTH:
    strLabel.Format("%i", g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].iWidth);
    break;
  case SYSTEM_SCREEN_HEIGHT:
    strLabel.Format("%i", g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].iHeight);
    break;
  case SYSTEM_CURRENT_WINDOW:
    return g_localizeStrings.Get(g_windowManager.GetFocusedWindow());
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
        strLabel.Format(g_localizeStrings.Get(13213).c_str(),g_alarmClock.GetRemaining("shutdowntimer")/60.f);
      else
        strLabel.Format(g_localizeStrings.Get(13214).c_str(),g_alarmClock.GetRemaining("shutdowntimer"));
    }
    break;
  case SYSTEM_PROFILENAME:
    strLabel = g_settings.GetCurrentProfile().getName();
    break;
  case SYSTEM_PROFILECOUNT:
    strLabel.Format("%i", g_settings.GetNumProfiles());
    break;
  case SYSTEM_LANGUAGE:
    strLabel = g_guiSettings.GetString("locale.language");
    break;
  case SYSTEM_TEMPERATURE_UNITS:
    strLabel = g_langInfo.GetTempUnitString();
    break;
  case SYSTEM_PROGRESS_BAR:
    {
      int percent = GetInt(SYSTEM_PROGRESS_BAR);
      if (percent)
        strLabel.Format("%i", percent);
    }
    break;
  case LCD_PLAY_ICON:
    {
      int iPlaySpeed = g_application.GetPlaySpeed();
      if (g_application.IsPaused())
        strLabel.Format("\7");
      else if (iPlaySpeed < 1)
        strLabel.Format("\3:%ix", iPlaySpeed);
      else if (iPlaySpeed > 1)
        strLabel.Format("\4:%ix", iPlaySpeed);
      else
        strLabel.Format("\5");
    }
    break;

  case LCD_TIME_21:
  case LCD_TIME_22:
  case LCD_TIME_W21:
  case LCD_TIME_W22:
  case LCD_TIME_41:
  case LCD_TIME_42:
  case LCD_TIME_43:
  case LCD_TIME_44:
    //alternatively, set strLabel
    return GetLcdTime( info );
    break;

  case SKIN_THEME:
    if (g_guiSettings.GetString("lookandfeel.skintheme").Equals("skindefault"))
      strLabel = g_localizeStrings.Get(15109);
    else
      strLabel = g_guiSettings.GetString("lookandfeel.skintheme");
    break;
  case SKIN_COLOUR_THEME:
    if (g_guiSettings.GetString("lookandfeel.skincolors").Equals("skindefault"))
      strLabel = g_localizeStrings.Get(15109);
    else
      strLabel = g_guiSettings.GetString("lookandfeel.skincolors");
    break;
#ifdef HAS_LCD
  case LCD_PROGRESS_BAR:
    if (g_lcd) strLabel = g_lcd->GetProgressBar(g_application.GetTime(), g_application.GetTotalTime());
    break;
#endif
  case NETWORK_IP_ADDRESS:
    {
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface)
        return iface->GetCurrentIPAddress();
    }
    break;
  case NETWORK_SUBNET_ADDRESS:
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
      vector<CStdString> nss = g_application.getNetwork().GetNameServers();
      if (nss.size() >= 1)
        return nss[0];
    }
    break;
  case NETWORK_DNS2_ADDRESS:
    {
      vector<CStdString> nss = g_application.getNetwork().GetNameServers();
      if (nss.size() >= 2)
        return nss[1];
    }
    break;
  case NETWORK_DHCP_ADDRESS:
    {
      CStdString dhcpserver;
      return dhcpserver;
    }
    break;
  case NETWORK_LINK_STATE:
    {
      CStdString linkStatus = g_localizeStrings.Get(151);
      linkStatus += " ";
      CNetworkInterface* iface = g_application.getNetwork().GetFirstConnectedInterface();
      if (iface && iface->IsConnected())
        linkStatus += g_localizeStrings.Get(15207);
      else
        linkStatus += g_localizeStrings.Get(15208);
      return linkStatus;
    }
    break;

  case AUDIOSCROBBLER_CONN_STATE:
  case AUDIOSCROBBLER_SUBMIT_INT:
  case AUDIOSCROBBLER_FILES_CACHED:
  case AUDIOSCROBBLER_SUBMIT_STATE:
    strLabel=GetAudioScrobblerLabel(info);
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
      strLabel = g_guiSettings.GetString("musicplayer.visualisation");
      if (CAddonMgr::Get().GetAddon(strLabel,addon) && addon)
        strLabel = addon->Name();
    }
    break;
  case FANART_COLOR1:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color1");
    }
    break;
  case FANART_COLOR2:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color2");
    }
    break;
  case FANART_COLOR3:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_color3");
    }
    break;
  case FANART_IMAGE:
    {
      CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
      if (window)
        return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("fanart_image");
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
int CGUIInfoManager::GetInt(int info, int contextWindow) const
{
  switch( info )
  {
    case PLAYER_VOLUME:
      return g_application.GetVolume();
    case PLAYER_SUBTITLE_DELAY:
      return g_application.GetSubtitleDelay();
    case PLAYER_AUDIO_DELAY:
      return g_application.GetAudioDelay();
    case PLAYER_PROGRESS:
    case PLAYER_PROGRESS_CACHE:
    case PLAYER_SEEKBAR:
    case PLAYER_CACHELEVEL:
    case PLAYER_CHAPTER:
    case PLAYER_CHAPTERCOUNT:
      {
        if( g_application.IsPlaying() && g_application.m_pPlayer)
        {
          switch( info )
          {
          case PLAYER_PROGRESS:
            return (int)(g_application.GetPercentage());
          case PLAYER_PROGRESS_CACHE:
            return (int)(g_application.GetCachePercentage());
          case PLAYER_SEEKBAR:
            {
              CGUIDialogSeekBar *seekBar = (CGUIDialogSeekBar*)g_windowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR);
              return seekBar ? (int)seekBar->GetPercentage() : 0;
            }
          case PLAYER_CACHELEVEL:
            return (int)(g_application.m_pPlayer->GetCacheLevel());
          case PLAYER_CHAPTER:
            return g_application.m_pPlayer->GetChapter();
          case PLAYER_CHAPTERCOUNT:
            return g_application.m_pPlayer->GetChapterCount();
          }
        }
      }
      break;
    case SYSTEM_FREE_MEMORY:
    case SYSTEM_USED_MEMORY:
      {
        MEMORYSTATUS stat;
        GlobalMemoryStatus(&stat);
        int memPercentUsed = (int)( 100.0f* (stat.dwTotalPhys - stat.dwAvailPhys)/stat.dwTotalPhys + 0.5f );
        if (info == SYSTEM_FREE_MEMORY)
          return 100 - memPercentUsed;
        return memPercentUsed;
      }
    case SYSTEM_PROGRESS_BAR:
      {
        CGUIDialogProgress *bar = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
        if (bar && bar->IsDialogRunning())
          return bar->GetPercentage();
      }
    case SYSTEM_FREE_SPACE:
    case SYSTEM_FREE_SPACE_C:
    case SYSTEM_FREE_SPACE_E:
    case SYSTEM_FREE_SPACE_F:
    case SYSTEM_FREE_SPACE_G:
    case SYSTEM_USED_SPACE:
    case SYSTEM_USED_SPACE_C:
    case SYSTEM_USED_SPACE_E:
    case SYSTEM_USED_SPACE_F:
    case SYSTEM_USED_SPACE_G:
    case SYSTEM_FREE_SPACE_X:
    case SYSTEM_USED_SPACE_X:
    case SYSTEM_FREE_SPACE_Y:
    case SYSTEM_USED_SPACE_Y:
    case SYSTEM_FREE_SPACE_Z:
    case SYSTEM_USED_SPACE_Z:
      {
        int ret = 0;
        g_sysinfo.GetHddSpaceInfo(ret, info, true);
        return ret;
      }
    case SYSTEM_CPU_USAGE:
      return g_cpuInfo.getUsedPercentage();
  }
  return 0;
}
// checks the condition and returns it as necessary.  Currently used
// for toggle button controls and visibility of images.
bool CGUIInfoManager::GetBool(int condition1, int contextWindow, const CGUIListItem *item)
{
  // check our cache
  bool bReturn = false;
  if (!item && IsCached(condition1, contextWindow, bReturn)) // never use cache for list items
    return bReturn;

  int condition = abs(condition1);

  if(condition >= COMBINED_VALUES_START && (condition - COMBINED_VALUES_START) < (int)(m_CombinedValues.size()) )
  {
    const CCombinedValue &comb = m_CombinedValues[condition - COMBINED_VALUES_START];
    if (!EvaluateBooleanExpression(comb, bReturn, contextWindow, item))
      bReturn = false;
  }
  else if (item && condition >= LISTITEM_START && condition < LISTITEM_END)
    bReturn = GetItemBool(item, condition);
  // Ethernet Link state checking
  // Will check if the Xbox has a Ethernet Link connection! [Cable in!]
  // This can used for the skinner to switch off Network or Inter required functions
  else if ( condition == SYSTEM_ALWAYS_TRUE)
    bReturn = true;
  else if (condition == SYSTEM_ALWAYS_FALSE)
    bReturn = false;
  else if (condition == SYSTEM_ETHERNET_LINK_ACTIVE)
    bReturn = true;
  else if (condition > SYSTEM_IDLE_TIME_START && condition <= SYSTEM_IDLE_TIME_FINISH)
    bReturn = (g_application.GlobalIdleTime() >= condition - SYSTEM_IDLE_TIME_START);
  else if (condition == WINDOW_IS_MEDIA)
  { // note: This doesn't return true for dialogs (content, favourites, login, videoinfo)
    CGUIWindow *pWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
    bReturn = (pWindow && pWindow->IsMediaWindow());
  }
  else if (condition == PLAYER_MUTED)
    bReturn = g_settings.m_bMute;
  else if (condition >= LIBRARY_HAS_MUSIC && condition <= LIBRARY_HAS_MUSICVIDEOS)
    bReturn = GetLibraryBool(condition);
  else if (condition == LIBRARY_IS_SCANNING)
  {
    CGUIDialogMusicScan *musicScanner = (CGUIDialogMusicScan *)g_windowManager.GetWindow(WINDOW_DIALOG_MUSIC_SCAN);
    CGUIDialogVideoScan *videoScanner = (CGUIDialogVideoScan *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_SCAN);
    if (musicScanner->IsScanning() || videoScanner->IsScanning())
      bReturn = true;
    else
      bReturn = false;
  }
  else if (condition == SYSTEM_PLATFORM_LINUX)
#if defined(_LINUX) && !defined(__APPLE__)
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_WINDOWS)
#ifdef WIN32
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_PLATFORM_OSX)
#ifdef __APPLE__
    bReturn = true;
#else
    bReturn = false;
#endif
  else if (condition == SYSTEM_MEDIA_DVD)
    bReturn = g_mediaManager.IsDiscInDrive();
#ifdef HAS_DVD_DRIVE
  else if (condition == SYSTEM_HAS_DRIVE_F)
    bReturn = CIoSupport::DriveExists('F');
  else if (condition == SYSTEM_HAS_DRIVE_G)
    bReturn = CIoSupport::DriveExists('G');
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

  else if (condition == PLAYER_SHOWINFO)
    bReturn = m_playerShowInfo;
  else if (condition == PLAYER_SHOWCODEC)
    bReturn = m_playerShowCodec;
  else if (condition >= SKIN_HAS_THEME_START && condition <= SKIN_HAS_THEME_END)
  { // Note that the code used here could probably be extended to general
    // settings conditions (parameter would need to store both the setting name an
    // the and the comparison string)
    CStdString theme = g_guiSettings.GetString("lookandfeel.skintheme");
    theme.ToLower();
    URIUtils::RemoveExtension(theme);
    bReturn = theme.Equals(m_stringParameters[condition - SKIN_HAS_THEME_START]);
  }
  else if (condition >= MULTI_INFO_START && condition <= MULTI_INFO_END)
  {
    // cache return value
    bool result = GetMultiInfoBool(m_multiInfo[condition - MULTI_INFO_START], contextWindow, item);
    if (!item)
      CacheBool(condition1, contextWindow, result);
    return result;
  }
  else if (condition == SYSTEM_HASLOCKS)
    bReturn = g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE;
  else if (condition == SYSTEM_ISMASTER)
    bReturn = g_settings.GetMasterProfile().getLockMode() != LOCK_MODE_EVERYONE && g_passwordManager.bMasterUser;
  else if (condition == SYSTEM_LOGGEDON)
    bReturn = !(g_windowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN);
  else if (condition == SYSTEM_SHOW_EXIT_BUTTON)
    bReturn = g_advancedSettings.m_showExitButton;
  else if (condition == SYSTEM_HAS_LOGINSCREEN)
    bReturn = g_settings.UsingLoginScreen();
  else if (condition == WEATHER_IS_FETCHED)
    bReturn = g_weatherManager.IsFetched();
  else if (condition == SYSTEM_INTERNET_STATE)
  {
    g_sysinfo.GetInfo(condition);
    bReturn = g_sysinfo.HasInternet();
  }
  else if (condition == SKIN_HAS_VIDEO_OVERLAY)
  {
    bReturn = g_windowManager.IsOverlayAllowed() && g_application.IsPlayingVideo();
  }
  else if (condition == SKIN_HAS_MUSIC_OVERLAY)
  {
    bReturn = g_windowManager.IsOverlayAllowed() && g_application.IsPlayingAudio();
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
      bReturn = ((CGUIMediaWindow*)pWindow)->CurrentDirectory().GetProperty("isstacked")=="1";
  }
  else if (condition == CONTAINER_HAS_THUMB)
  {
    CGUIWindow *pWindow = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (pWindow)
      bReturn = ((CGUIMediaWindow*)pWindow)->CurrentDirectory().HasThumbnail();
  }
  else if (condition == VIDEOPLAYER_HAS_INFO)
    bReturn = (m_currentFile->HasVideoInfoTag() && !m_currentFile->GetVideoInfoTag()->IsEmpty());
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
  else if (g_application.IsPlaying())
  {
    switch (condition)
    {
    case PLAYER_HAS_MEDIA:
      bReturn = true;
      break;
    case PLAYER_HAS_AUDIO:
      bReturn = g_application.IsPlayingAudio();
      break;
    case PLAYER_HAS_VIDEO:
      bReturn = g_application.IsPlayingVideo();
      break;
    case PLAYER_PLAYING:
      bReturn = !g_application.IsPaused() && (g_application.GetPlaySpeed() == 1);
      break;
    case PLAYER_PAUSED:
      bReturn = g_application.IsPaused();
      break;
    case PLAYER_REWINDING:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() < 1;
      break;
    case PLAYER_FORWARDING:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() > 1;
      break;
    case PLAYER_REWINDING_2x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == -2;
      break;
    case PLAYER_REWINDING_4x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == -4;
      break;
    case PLAYER_REWINDING_8x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == -8;
      break;
    case PLAYER_REWINDING_16x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == -16;
      break;
    case PLAYER_REWINDING_32x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == -32;
      break;
    case PLAYER_FORWARDING_2x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == 2;
      break;
    case PLAYER_FORWARDING_4x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == 4;
      break;
    case PLAYER_FORWARDING_8x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == 8;
      break;
    case PLAYER_FORWARDING_16x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == 16;
      break;
    case PLAYER_FORWARDING_32x:
      bReturn = !g_application.IsPaused() && g_application.GetPlaySpeed() == 32;
      break;
    case PLAYER_CAN_RECORD:
      bReturn = g_application.m_pPlayer->CanRecord();
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
        CGUIDialogSeekBar *seekBar = (CGUIDialogSeekBar*)g_windowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR);
        bReturn = seekBar ? seekBar->IsDialogRunning() : false;
      }
    break;
    case PLAYER_SEEKING:
      bReturn = m_playerSeeking;
    break;
    case PLAYER_SHOWTIME:
      bReturn = m_playerShowTime;
    break;
    case PLAYER_PASSTHROUGH:
      bReturn = g_application.m_pPlayer && g_application.m_pPlayer->IsPassthrough();
      break;
    case MUSICPM_ENABLED:
      bReturn = g_partyModeManager.IsEnabled();
    break;
    case AUDIOSCROBBLER_ENABLED:
      bReturn = CLastFmManager::GetInstance()->IsLastFmEnabled();
    break;
    case LASTFM_RADIOPLAYING:
      bReturn = CLastFmManager::GetInstance()->IsRadioEnabled();
      break;
    case LASTFM_CANLOVE:
      bReturn = CLastFmManager::GetInstance()->CanLove();
      break;
    case LASTFM_CANBAN:
      bReturn = CLastFmManager::GetInstance()->CanBan();
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
        if (g_application.IsPlayingAudio() && g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_MUSIC)
          bReturn = true;
      }
      break;
    case VIDEOPLAYER_USING_OVERLAYS:
      bReturn = (g_guiSettings.GetInt("videoplayer.rendermethod") == RENDER_OVERLAYS);
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
      bReturn = !g_guiSettings.GetString("musicplayer.visualisation").IsEmpty();
    break;
    default: // default, use integer value different from 0 as true
      bReturn = GetInt(condition) != 0;
    }
  }
  // cache return value
  if (condition1 < 0) bReturn = !bReturn;

  if (!item) // don't cache item properties
    CacheBool(condition1, contextWindow, bReturn);

  return bReturn;
}

/// \brief Examines the multi information sent and returns true or false accordingly.
bool CGUIInfoManager::GetMultiInfoBool(const GUIInfo &info, int contextWindow, const CGUIListItem *item)
{
  bool bReturn = false;
  int condition = abs(info.m_info);

  if (condition >= LISTITEM_START && condition <= LISTITEM_END)
  {
    // TODO: We currently don't use the item that is passed in to here, as these
    //       conditions only come from Container(id).ListItem(offset).* at this point.
    CGUIListItemPtr item;
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
        item = ((CGUIBaseContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag());
    }

    if (item) // If we got a valid item, do the lookup
      bReturn = GetItemBool(item.get(), condition); // Image prioritizes images over labels (in the case of music item ratings for instance)
  }
  else
  {
    switch (condition)
    {
      case SKIN_BOOL:
        {
          bReturn = g_settings.GetSkinBool(info.GetData1());
        }
        break;
      case SKIN_STRING:
        {
          if (info.GetData2())
            bReturn = g_settings.GetSkinString(info.GetData1()).Equals(m_stringParameters[info.GetData2()]);
          else
            bReturn = !g_settings.GetSkinString(info.GetData1()).IsEmpty();
        }
        break;
      case STRING_IS_EMPTY:
        // note: Get*Image() falls back to Get*Label(), so this should cover all of them
        if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
          bReturn = GetItemImage((const CFileItem *)item, info.GetData1()).IsEmpty();
        else
          bReturn = GetImage(info.GetData1(), contextWindow).IsEmpty();
        break;
      case STRING_COMPARE:
        {
          CStdString compare;
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
            bReturn = GetItemImage((const CFileItem *)item, info.GetData1()).Equals(compare);
          else
            bReturn = GetImage(info.GetData1(), contextWindow).Equals(compare);
        }
        break;
      case INTEGER_GREATER_THAN:
        {
          CStdString value;

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
        break;
      case STRING_STR:
          {
            CStdString compare = m_stringParameters[info.GetData2()];
            // our compare string is already in lowercase, so lower case our label as well
            // as CStdString::Find() is case sensitive
            CStdString label;
            if (item && item->IsFileItem() && info.GetData1() >= LISTITEM_START && info.GetData1() < LISTITEM_END)
              label = GetItemImage((const CFileItem *)item, info.GetData1()).ToLower();
            else
              label = GetImage(info.GetData1(), contextWindow).ToLower();
            if (compare.Right(5).Equals(",left"))
              bReturn = label.Find(compare.Mid(0,compare.size()-5)) == 0;
            else if (compare.Right(6).Equals(",right"))
            {
              compare = compare.Mid(0,compare.size()-6);
              bReturn = label.Find(compare) == (int)(label.size()-compare.size());
            }
            else
              bReturn = label.Find(compare) > -1;
          }
        break;
    case SYSTEM_ALARM_LESS_OR_EQUAL:
    {
      int time = lrint(g_alarmClock.GetRemaining(m_stringParameters[info.GetData1()]));
      int timeCompare = atoi(m_stringParameters[info.GetData2()]);
      if (time > 0)
        bReturn = timeCompare >= time;
      else
        bReturn = false;
    }
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
      case BUTTON_SCROLLER_HAS_ICON:
        {
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            CGUIControl *pControl = window->GetFocusedControl();
            if (pControl && pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTONBAR)
              bReturn = ((CGUIButtonScroller *)pControl)->GetActiveButtonID() == (int)info.GetData1();
          }
        }
        break;
      case WINDOW_NEXT:
        if (info.GetData1())
          bReturn = ((int)info.GetData1() == m_nextWindowID);
        else
        {
          CGUIWindow *window = g_windowManager.GetWindow(m_nextWindowID);
          if (window && URIUtils::GetFileName(window->GetProperty("xmlfile")).Equals(m_stringParameters[info.GetData2()]))
            bReturn = true;
        }
        break;
      case WINDOW_PREVIOUS:
        if (info.GetData1())
          bReturn = ((int)info.GetData1() == m_prevWindowID);
        else
        {
          CGUIWindow *window = g_windowManager.GetWindow(m_prevWindowID);
          if (window && URIUtils::GetFileName(window->GetProperty("xmlfile")).Equals(m_stringParameters[info.GetData2()]))
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
        bReturn = g_guiSettings.GetBool(m_stringParameters[info.GetData1()]);
        break;
      case SYSTEM_HAS_CORE_ID:
        bReturn = g_cpuInfo.HasCoreId(info.GetData1());
        break;
      case SYSTEM_SETTING:
        {
          if ( m_stringParameters[info.GetData1()].Equals("hidewatched") )
          {
            CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              bReturn = g_settings.GetWatchMode(((CGUIMediaWindow *)window)->CurrentDirectory().GetContent()) == VIDEO_SHOW_UNWATCHED;
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
          CStdString content;
          CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
          if (window)
          {
            if (window->GetID() == WINDOW_DIALOG_MUSIC_INFO)
              content = ((CGUIDialogMusicInfo *)window)->CurrentDirectory().GetContent();
            else if (window->GetID() == WINDOW_DIALOG_VIDEO_INFO)
              content = ((CGUIDialogVideoInfo *)window)->CurrentDirectory().GetContent();
          }
          if (content.IsEmpty())
          {
            window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
            if (window)
              content = ((CGUIMediaWindow *)window)->CurrentDirectory().GetContent();
          }
          bReturn = m_stringParameters[info.GetData1()].Equals(content);
        }
        break;
      case CONTAINER_ROW:
      case CONTAINER_COLUMN:
      case CONTAINER_POSITION:
      case CONTAINER_HAS_NEXT:
      case CONTAINER_HAS_PREVIOUS:
      case CONTAINER_SCROLLING:
      case CONTAINER_SUBITEM:
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
              CFileItemPtr item = boost::static_pointer_cast<CFileItem>(((CGUIBaseContainer *)control)->GetListItem(0));
              if (item && item->m_iprogramCount == info.GetData2())  // programcount used to store item id
                bReturn = true;
            }
          }
          break;
        }
      case VIDEOPLAYER_CONTENT:
        {
          CStdString strContent="movies";
          if (!m_currentFile->HasVideoInfoTag() || m_currentFile->GetVideoInfoTag()->IsEmpty())
            strContent = "files";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_iSeason > -1) // episode
            strContent = "episodes";
          if (m_currentFile->HasVideoInfoTag() && !m_currentFile->GetVideoInfoTag()->m_strArtist.IsEmpty())
            strContent = "musicvideos";
          if (m_currentFile->HasVideoInfoTag() && m_currentFile->GetVideoInfoTag()->m_strStatus == "livetv")
            strContent = "livetv";
          bReturn = m_stringParameters[info.GetData1()].Equals(strContent);
        }
        break;
      case CONTAINER_SORT_METHOD:
      {
        CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
        if (window)
        {
          const CGUIViewState *viewState = ((CGUIMediaWindow*)window)->GetViewState();
          if (viewState)
            bReturn = ((unsigned int)viewState->GetSortMethod() == info.GetData1());
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
            bReturn = ((unsigned int)viewState->GetDisplaySortOrder() == info.GetData1());
        }
        break;
      }
      case SYSTEM_DATE:
        {
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
              return false;
            index += g_playlistPlayer.GetCurrentSong();
          }
          if (index >= 0 && index < g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size())
            return true;
          return false;
        }
        break;
    }
  }
  return (info.m_info < 0) ? !bReturn : bReturn;
}

/// \brief Examines the multi information sent and returns the string as appropriate
CStdString CGUIInfoManager::GetMultiInfoLabel(const GUIInfo &info, int contextWindow) const
{
  if (info.m_info == SKIN_STRING)
  {
    return g_settings.GetSkinString(info.GetData1());
  }
  else if (info.m_info == SKIN_BOOL)
  {
    bool bInfo = g_settings.GetSkinBool(info.GetData1());
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
        item = boost::static_pointer_cast<CFileItem>(((CGUIBaseContainer *)control)->GetListItem(info.GetData2(), info.GetInfoFlag()));
    }

    if (item) // If we got a valid item, do the lookup
      return GetItemImage(item.get(), info.m_info); // Image prioritizes images over labels (in the case of music item ratings for instance)
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
    CDateTime time = CDateTime::GetCurrentDateTime();
    time += CDateTimeSpan(0, 0, 0, GetPlayTimeRemaining());
    return LocalizeTime(time, (TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == PLAYER_TIME_SPEED)
  {
    CStdString strTime;
    if (g_application.GetPlaySpeed() != 1)
      strTime.Format("%s (%ix)", GetCurrentPlayTime((TIME_FORMAT)info.GetData1()).c_str(), g_application.GetPlaySpeed());
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
    TIME_FORMAT format = (TIME_FORMAT)info.GetData1();
    if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
      format = TIME_FORMAT_HH_MM_SS;
    CGUIDialogSeekBar *seekBar = (CGUIDialogSeekBar*)g_windowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR);
    if (seekBar)
      return seekBar->GetSeekTimeLabel(format);
  }
  else if (info.m_info == PLAYER_SEEKOFFSET)
  {
    CStdString seekOffset = StringUtils::SecondsToTimeString(abs(m_seekOffset), (TIME_FORMAT)info.GetData1());
    if (m_seekOffset < 0)
      return "-" + seekOffset;
    if (m_seekOffset > 0)
      return "+" + seekOffset;
  }
  else if (info.m_info == SYSTEM_TIME)
  {
    return GetTime((TIME_FORMAT)info.GetData1());
  }
  else if (info.m_info == CONTAINER_NUM_PAGES || info.m_info == CONTAINER_CURRENT_PAGE ||
           info.m_info == CONTAINER_NUM_ITEMS || info.m_info == CONTAINER_POSITION)
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
        return ((CGUIBaseContainer *)control)->GetLabel(info.m_info);
      else if (control->GetControlType() == CGUIControl::GUICONTROL_TEXTBOX)
        return ((CGUITextBox *)control)->GetLabel(info.m_info);
    }
  }
  else if (info.m_info == SYSTEM_GET_CORE_USAGE)
  {
    CStdString strCpu;
    strCpu.Format("%4.2f", g_cpuInfo.GetCoreInfo(info.GetData1()).m_fPct);
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
      return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty(m_stringParameters[info.GetData2()]);
  }
  else if (info.m_info == CONTROL_GET_LABEL)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, 0);
    if (window)
    {
      const CGUIControl *control = window->GetControl(info.GetData1());
      if (control)
        return control->GetDescription();
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
      return window->GetProperty(m_stringParameters[info.GetData2()]);
  }
  else if (info.m_info == SYSTEM_ADDON_TITLE ||
           info.m_info == SYSTEM_ADDON_ICON)
  {
    AddonPtr addon;
    if (info.GetData2() == 0)
      CAddonMgr::Get().GetAddon(const_cast<CGUIInfoManager*>(this)->GetLabel(info.GetData1(), contextWindow),addon);
    else
      CAddonMgr::Get().GetAddon(m_stringParameters[info.GetData1()],addon);
    if (addon && info.m_info == SYSTEM_ADDON_TITLE)
      return addon->Name();
    if (addon && info.m_info == SYSTEM_ADDON_ICON)
      return addon->Icon();
  }

  return StringUtils::EmptyString;
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
CStdString CGUIInfoManager::GetImage(int info, int contextWindow)
{
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START], contextWindow);
  }
  else if (info == WEATHER_CONDITIONS)
    return g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON);
  else if (info == SYSTEM_PROFILETHUMB)
  {
    CStdString thumb = g_settings.GetCurrentProfile().getThumb();
    if (thumb.IsEmpty())
      thumb = "unknown-user.png";
    return thumb;
  }
  else if (info == MUSICPLAYER_COVER)
  {
    if (!g_application.IsPlayingAudio()) return "";
    return m_currentFile->HasThumbnail() ? m_currentFile->GetThumbnailImage() : "DefaultAlbumCover.png";
  }
  else if (info == MUSICPLAYER_RATING)
  {
    if (!g_application.IsPlayingAudio()) return "";
    return GetItemImage(m_currentFile, LISTITEM_RATING);
  }
  else if (info == PLAYER_STAR_RATING)
  {
    if (!g_application.IsPlaying()) return "";
    return GetItemImage(m_currentFile, LISTITEM_STAR_RATING);
  }
  else if (info == VIDEOPLAYER_COVER)
  {
    if (!g_application.IsPlayingVideo()) return "";
    if(m_currentMovieThumb.IsEmpty())
      return m_currentFile->HasThumbnail() ? m_currentFile->GetThumbnailImage() : "DefaultVideoCover.png";
    else return m_currentMovieThumb;
  }
  else if (info == CONTAINER_FOLDERTHUMB)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      return GetItemImage(&const_cast<CFileItemList&>(((CGUIMediaWindow*)window)->CurrentDirectory()), LISTITEM_THUMB);
  }
  else if (info == CONTAINER_TVSHOWTHUMB)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("tvshowthumb");
  }
  else if (info == CONTAINER_SEASONTHUMB)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_IS_MEDIA_WINDOW);
    if (window)
      return ((CGUIMediaWindow *)window)->CurrentDirectory().GetProperty("seasonthumb");
  }
  else if (info == LISTITEM_THUMB || info == LISTITEM_ICON || info == LISTITEM_ACTUAL_ICON ||
          info == LISTITEM_OVERLAY || info == LISTITEM_RATING || info == LISTITEM_STAR_RATING)
  {
    CGUIWindow *window = GetWindowWithCondition(contextWindow, WINDOW_CONDITION_HAS_LIST_ITEMS);
    if (window)
    {
      CFileItemPtr item = window->GetCurrentListItem();
      if (item)
        return GetItemImage(item.get(), info);
    }
  }
  return GetLabel(info, contextWindow);
}

CStdString CGUIInfoManager::GetDate(bool bNumbersOnly)
{
  CDateTime time=CDateTime::GetCurrentDateTime();
  return time.GetAsLocalizedDate(!bNumbersOnly);
}

CStdString CGUIInfoManager::GetTime(TIME_FORMAT format) const
{
  CDateTime time=CDateTime::GetCurrentDateTime();
  return LocalizeTime(time, format);
}

CStdString CGUIInfoManager::GetLcdTime( int _eInfo ) const
{
  CDateTime time=CDateTime::GetCurrentDateTime();
  CStdString strLcdTime;

#ifdef HAS_LCD

  UINT       nCharset;
  UINT       nLine;
  CStdString strTimeMarker;

  nCharset = 0;
  nLine = 0;

  switch ( _eInfo )
  {
    case LCD_TIME_21:
      nCharset = 1; // CUSTOM_CHARSET_SMALLCHAR;
      nLine = 0;
      strTimeMarker = ".";
    break;
    case LCD_TIME_22:
      nCharset = 1; // CUSTOM_CHARSET_SMALLCHAR;
      nLine = 1;
      strTimeMarker = ".";
    break;

    case LCD_TIME_W21:
      nCharset = 2; // CUSTOM_CHARSET_MEDIUMCHAR;
      nLine = 0;
      strTimeMarker = ".";
    break;
    case LCD_TIME_W22:
      nCharset = 2; // CUSTOM_CHARSET_MEDIUMCHAR;
      nLine = 1;
      strTimeMarker = ".";
    break;

    case LCD_TIME_41:
      nCharset = 3; // CUSTOM_CHARSET_BIGCHAR;
      nLine = 0;
      strTimeMarker = " ";
    break;
    case LCD_TIME_42:
      nCharset = 3; // CUSTOM_CHARSET_BIGCHAR;
      nLine = 1;
      strTimeMarker = "o";
    break;
    case LCD_TIME_43:
      nCharset = 3; // CUSTOM_CHARSET_BIGCHAR;
      nLine = 2;
      strTimeMarker = "o";
    break;
    case LCD_TIME_44:
      nCharset = 3; // CUSTOM_CHARSET_BIGCHAR;
      nLine = 3;
      strTimeMarker = " ";
    break;
  }

  strLcdTime += g_lcd->GetBigDigit( nCharset, time.GetHour()  , nLine, 2, 2, true );
  strLcdTime += strTimeMarker;
  strLcdTime += g_lcd->GetBigDigit( nCharset, time.GetMinute(), nLine, 2, 2, false );
  strLcdTime += strTimeMarker;
  strLcdTime += g_lcd->GetBigDigit( nCharset, time.GetSecond(), nLine, 2, 2, false );

#endif

  return strLcdTime;
}

CStdString CGUIInfoManager::LocalizeTime(const CDateTime &time, TIME_FORMAT format) const
{
  const CStdString timeFormat = g_langInfo.GetTimeFormat();
  bool use12hourclock = timeFormat.Find('h') != -1;
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
    return time.GetAsLocalizedTime("", true);
  case TIME_FORMAT_H:
    return time.GetAsLocalizedTime("h", false);
  case TIME_FORMAT_H_MM_SS:
    return time.GetAsLocalizedTime("h:mm:ss", true);
  default:
    break;
  }
  return time.GetAsLocalizedTime("", false);
}

CStdString CGUIInfoManager::GetDuration(TIME_FORMAT format) const
{
  if (g_application.IsPlayingAudio() && m_currentFile->HasMusicInfoTag())
  {
    const CMusicInfoTag& tag = *m_currentFile->GetMusicInfoTag();
    if (tag.GetDuration() > 0)
      return StringUtils::SecondsToTimeString(tag.GetDuration(), format);
  }
  if (g_application.IsPlayingVideo() && !m_currentMovieDuration.IsEmpty())
    return m_currentMovieDuration;  // for tuxbox
  unsigned int iTotal = (unsigned int)g_application.GetTotalTime();
  if (iTotal > 0)
    return StringUtils::SecondsToTimeString(iTotal, format);
  return "";
}

CStdString CGUIInfoManager::GetMusicPartyModeLabel(int item)
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
    CStdString strLabel;
    strLabel.Format("%i", iSongs);
    return strLabel;
  }
  return "";
}

const CStdString CGUIInfoManager::GetMusicPlaylistInfo(const GUIInfo& info) const
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
  if (!playlistItem->HasThumbnail())
  {
    playlistItem->SetMusicThumb();
    // still no thumb? then just the set the default cover
    if (!playlistItem->HasThumbnail())
      playlistItem->SetThumbnailImage("DefaultAlbumCover.png");
  }
  if (info.m_info == MUSICPLAYER_PLAYLISTPOS)
  {
    CStdString strPosition = "";
    strPosition.Format("%i", index + 1);
    return strPosition;
  }
  else if (info.m_info == MUSICPLAYER_COVER)
    return playlistItem->GetThumbnailImage();
  return GetMusicTagLabel(info.m_info, playlistItem.get());
}

CStdString CGUIInfoManager::GetPlaylistLabel(int item) const
{
  if (!g_application.IsPlaying()) return "";
  int iPlaylist = g_playlistPlayer.GetCurrentPlaylist();
  switch (item)
  {
  case PLAYLIST_LENGTH:
    {
      CStdString strLength = "";
      strLength.Format("%i", g_playlistPlayer.GetPlaylist(iPlaylist).size());
      return strLength;
    }
  case PLAYLIST_POSITION:
    {
      CStdString strPosition = "";
      strPosition.Format("%i", g_playlistPlayer.GetCurrentSong() + 1);
      return strPosition;
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

CStdString CGUIInfoManager::GetMusicLabel(int item)
{
  if (!g_application.IsPlayingAudio() || !m_currentFile->HasMusicInfoTag()) return "";
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
      float fTimeSpan = (float)(CTimeUtils::GetFrameTime() - m_lastMusicBitrateTime);
      if (fTimeSpan >= 500.0f)
      {
        m_MusicBitrate = g_application.m_pPlayer->GetAudioBitrate();
        m_lastMusicBitrateTime = CTimeUtils::GetFrameTime();
      }
      CStdString strBitrate = "";
      if (m_MusicBitrate > 0)
        strBitrate.Format("%i", MathUtils::round_int((double)m_MusicBitrate / 1000.0));
      return strBitrate;
    }
    break;
  case MUSICPLAYER_CHANNELS:
    {
      CStdString strChannels = "";
      if (g_application.m_pPlayer->GetChannels() > 0)
      {
        strChannels.Format("%i", g_application.m_pPlayer->GetChannels());
      }
      return strChannels;
    }
    break;
  case MUSICPLAYER_BITSPERSAMPLE:
    {
      CStdString strBitsPerSample = "";
      if (g_application.m_pPlayer->GetBitsPerSample() > 0)
      {
        strBitsPerSample.Format("%i", g_application.m_pPlayer->GetBitsPerSample());
      }
      return strBitsPerSample;
    }
    break;
  case MUSICPLAYER_SAMPLERATE:
    {
      CStdString strSampleRate = "";
      if (g_application.m_pPlayer->GetSampleRate() > 0)
      {
        strSampleRate.Format("%i",g_application.m_pPlayer->GetSampleRate());
      }
      return strSampleRate;
    }
    break;
  case MUSICPLAYER_CODEC:
    {
      CStdString strCodec;
      strCodec.Format("%s", g_application.m_pPlayer->GetAudioCodecName().c_str());
      return strCodec;
    }
    break;
  case MUSICPLAYER_LYRICS:
    return GetItemLabel(m_currentFile, AddListItemProp("lyrics"));
  }
  return GetMusicTagLabel(item, m_currentFile);
}

CStdString CGUIInfoManager::GetMusicTagLabel(int info, const CFileItem *item) const
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
    if (tag.GetArtist().size()) { return tag.GetArtist(); }
    break;
  case MUSICPLAYER_ALBUM_ARTIST:
    if (tag.GetAlbumArtist().size()) { return tag.GetAlbumArtist(); }
    break;
  case MUSICPLAYER_YEAR:
    if (tag.GetYear()) { return tag.GetYearString(); }
    break;
  case MUSICPLAYER_GENRE:
    if (tag.GetGenre().size()) { return tag.GetGenre(); }
    break;
  case MUSICPLAYER_LYRICS:
    if (tag.GetLyrics().size()) { return tag.GetLyrics(); }
  break;
  case MUSICPLAYER_TRACK_NUMBER:
    {
      CStdString strTrack;
      if (tag.Loaded() && tag.GetTrackNumber() > 0)
      {
        strTrack.Format("%02i", tag.GetTrackNumber());
        return strTrack;
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
  case MUSICPLAYER_PLAYCOUNT:
    return GetItemLabel(item, LISTITEM_PLAYCOUNT);
  case MUSICPLAYER_LASTPLAYED:
    return GetItemLabel(item, LISTITEM_LASTPLAYED);
  }
  return "";
}

CStdString CGUIInfoManager::GetVideoLabel(int item)
{
  if (!g_application.IsPlayingVideo())
    return "";

  if (item == VIDEOPLAYER_TITLE)
  {
    if (m_currentFile->HasVideoInfoTag() && !m_currentFile->GetVideoInfoTag()->m_strTitle.IsEmpty())
      return m_currentFile->GetVideoInfoTag()->m_strTitle;
    // don't have the title, so use dvdplayer, label, or drop down to title from path
    if (!g_application.m_pPlayer->GetPlayingTitle().IsEmpty())
      return g_application.m_pPlayer->GetPlayingTitle();
    if (!m_currentFile->GetLabel().IsEmpty())
      return m_currentFile->GetLabel();
    return CUtil::GetTitleFromPath(m_currentFile->m_strPath);
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
  else if (m_currentFile->HasVideoInfoTag())
  {
    switch (item)
    {
    case VIDEOPLAYER_ORIGINALTITLE:
      return m_currentFile->GetVideoInfoTag()->m_strOriginalTitle;
      break;
    case VIDEOPLAYER_GENRE:
      return m_currentFile->GetVideoInfoTag()->m_strGenre;
      break;
    case VIDEOPLAYER_DIRECTOR:
      return m_currentFile->GetVideoInfoTag()->m_strDirector;
      break;
    case VIDEOPLAYER_RATING:
      {
        CStdString strRating;
        if (m_currentFile->GetVideoInfoTag()->m_fRating > 0.f)
          strRating.Format("%.1f", m_currentFile->GetVideoInfoTag()->m_fRating);
        return strRating;
      }
      break;
    case VIDEOPLAYER_RATING_AND_VOTES:
      {
        CStdString strRatingAndVotes;
        if (m_currentFile->GetVideoInfoTag()->m_fRating > 0.f)
        {
          if (m_currentFile->GetVideoInfoTag()->m_strVotes.IsEmpty())
            strRatingAndVotes.Format("%.1f", m_currentFile->GetVideoInfoTag()->m_fRating);
          else
            strRatingAndVotes.Format("%.1f (%s %s)", m_currentFile->GetVideoInfoTag()->m_fRating, m_currentFile->GetVideoInfoTag()->m_strVotes, g_localizeStrings.Get(20350));
        }
        return strRatingAndVotes;
      }
      break;
    case VIDEOPLAYER_YEAR:
      {
        CStdString strYear;
        if (m_currentFile->GetVideoInfoTag()->m_iYear > 0)
          strYear.Format("%i", m_currentFile->GetVideoInfoTag()->m_iYear);
        return strYear;
      }
      break;
    case VIDEOPLAYER_PREMIERED:
      {
        if (!m_currentFile->GetVideoInfoTag()->m_strFirstAired.IsEmpty())
          return m_currentFile->GetVideoInfoTag()->m_strFirstAired;
        if (!m_currentFile->GetVideoInfoTag()->m_strPremiered.IsEmpty())
          return m_currentFile->GetVideoInfoTag()->m_strPremiered;
      }
      break;
    case VIDEOPLAYER_PLOT:
      return m_currentFile->GetVideoInfoTag()->m_strPlot;
    case VIDEOPLAYER_TRAILER:
      return m_currentFile->GetVideoInfoTag()->m_strTrailer;
    case VIDEOPLAYER_PLOT_OUTLINE:
      return m_currentFile->GetVideoInfoTag()->m_strPlotOutline;
    case VIDEOPLAYER_EPISODE:
      {
        CStdString strYear;
        if (m_currentFile->GetVideoInfoTag()->m_iSpecialSortEpisode > 0)
          strYear.Format("S%i", m_currentFile->GetVideoInfoTag()->m_iSpecialSortEpisode);
        else if(m_currentFile->GetVideoInfoTag()->m_iEpisode > 0)
          strYear.Format("%i", m_currentFile->GetVideoInfoTag()->m_iEpisode);
        return strYear;
      }
      break;
    case VIDEOPLAYER_SEASON:
      {
        CStdString strYear;
        if (m_currentFile->GetVideoInfoTag()->m_iSpecialSortSeason > 0)
          strYear.Format("%i", m_currentFile->GetVideoInfoTag()->m_iSpecialSortSeason);
        else if(m_currentFile->GetVideoInfoTag()->m_iSeason > 0)
          strYear.Format("%i", m_currentFile->GetVideoInfoTag()->m_iSeason);
        return strYear;
      }
      break;
    case VIDEOPLAYER_TVSHOW:
      return m_currentFile->GetVideoInfoTag()->m_strShowTitle;

    case VIDEOPLAYER_STUDIO:
      return m_currentFile->GetVideoInfoTag()->m_strStudio;
    case VIDEOPLAYER_COUNTRY:
      return m_currentFile->GetVideoInfoTag()->m_strCountry;
    case VIDEOPLAYER_MPAA:
      return m_currentFile->GetVideoInfoTag()->m_strMPAARating;
    case VIDEOPLAYER_TOP250:
      {
        CStdString strTop250;
        if (m_currentFile->GetVideoInfoTag()->m_iTop250 > 0)
          strTop250.Format("%i", m_currentFile->GetVideoInfoTag()->m_iTop250);
        return strTop250;
      }
      break;
    case VIDEOPLAYER_CAST:
      return m_currentFile->GetVideoInfoTag()->GetCast();
    case VIDEOPLAYER_CAST_AND_ROLE:
      return m_currentFile->GetVideoInfoTag()->GetCast(true);
    case VIDEOPLAYER_ARTIST:
      return m_currentFile->GetVideoInfoTag()->m_strArtist;
    case VIDEOPLAYER_ALBUM:
      return m_currentFile->GetVideoInfoTag()->m_strAlbum;
    case VIDEOPLAYER_WRITER:
      return m_currentFile->GetVideoInfoTag()->m_strWritingCredits;
    case VIDEOPLAYER_TAGLINE:
      return m_currentFile->GetVideoInfoTag()->m_strTagLine;
    case VIDEOPLAYER_LASTPLAYED:
      return m_currentFile->GetVideoInfoTag()->m_lastPlayed;
    case VIDEOPLAYER_PLAYCOUNT:
      {
        CStdString strPlayCount;
        if (m_currentFile->GetVideoInfoTag()->m_playCount > 0)
          strPlayCount.Format("%i", m_currentFile->GetVideoInfoTag()->m_playCount);
        return strPlayCount;
      }
    }
  }
  return "";
}

__int64 CGUIInfoManager::GetPlayTime() const
{
  if (g_application.IsPlaying())
  {
    __int64 lPTS = (__int64)(g_application.GetTime() * 1000);
    if (lPTS < 0) lPTS = 0;
    return lPTS;
  }
  return 0;
}

CStdString CGUIInfoManager::GetCurrentPlayTime(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  if (g_application.IsPlayingAudio() || g_application.IsPlayingVideo())
    return StringUtils::SecondsToTimeString((int)(GetPlayTime()/1000), format);
  return "";
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

CStdString CGUIInfoManager::GetCurrentPlayTimeRemaining(TIME_FORMAT format) const
{
  if (format == TIME_FORMAT_GUESS && GetTotalPlayTime() >= 3600)
    format = TIME_FORMAT_HH_MM_SS;
  int timeRemaining = GetPlayTimeRemaining();
  if (timeRemaining && (g_application.IsPlayingAudio() || g_application.IsPlayingVideo()))
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
}

void CGUIInfoManager::SetCurrentAlbumThumb(const CStdString thumbFileName)
{
  if (CFile::Exists(thumbFileName))
    m_currentFile->SetThumbnailImage(thumbFileName);
  else
  {
    m_currentFile->SetThumbnailImage("");
    m_currentFile->FillInDefaultIcon();
  }
}

void CGUIInfoManager::SetCurrentSong(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentSong(%s)",item.m_strPath.c_str());
  *m_currentFile = item;

  m_currentFile->LoadMusicTag();
  if (m_currentFile->GetMusicInfoTag()->GetTitle().IsEmpty())
  {
    // No title in tag, show filename only
    m_currentFile->GetMusicInfoTag()->SetTitle(CUtil::GetTitleFromPath(m_currentFile->m_strPath));
  }
  m_currentFile->GetMusicInfoTag()->SetLoaded(true);

  // find a thumb for this file.
  if (m_currentFile->IsInternetStream())
  {
    if (!g_application.m_strPlayListFile.IsEmpty())
    {
      CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
      CFileItem streamingItem(g_application.m_strPlayListFile,false);
      streamingItem.SetMusicThumb();
      CStdString strThumb = streamingItem.GetThumbnailImage();
      if (CFile::Exists(strThumb))
        m_currentFile->SetThumbnailImage(strThumb);
    }
  }
  else
    m_currentFile->SetMusicThumb();
    if (!m_currentFile->HasProperty("fanart_image"))
    {
      if (m_currentFile->CacheLocalFanart())
        m_currentFile->SetProperty("fanart_image", m_currentFile->GetCachedFanart());
    }
  m_currentFile->FillInDefaultIcon();

  CMusicInfoLoader::LoadAdditionalTagInfo(m_currentFile);
}

void CGUIInfoManager::SetCurrentMovie(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentMovie(%s)",item.m_strPath.c_str());
  *m_currentFile = item;

  CVideoDatabase dbs;
  if (dbs.Open())
  {
    dbs.LoadVideoInfo(item.m_strPath, *m_currentFile->GetVideoInfoTag());
    dbs.Close();
  }

  // Find a thumb for this file.
  item.SetVideoThumb();
  if (!item.HasThumbnail())
  {
    CStdString strPath, strFileName;
    URIUtils::Split(item.GetCachedVideoThumb(), strPath, strFileName);

    // create unique thumb for auto generated thumbs
    CStdString cachedThumb = strPath + "auto-" + strFileName;
    if (CFile::Exists(cachedThumb))
      item.SetThumbnailImage(cachedThumb);
  }

  // find a thumb for this stream
  if (item.IsInternetStream())
  {
    // case where .strm is used to start an audio stream
    if (g_application.IsPlayingAudio())
    {
      SetCurrentSong(item);
      return;
    }

    // else its a video
    if (!g_application.m_strPlayListFile.IsEmpty())
    {
      CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
      CFileItem thumbItem(g_application.m_strPlayListFile,false);
      thumbItem.SetVideoThumb();
      if (thumbItem.HasThumbnail())
        item.SetThumbnailImage(thumbItem.GetThumbnailImage());
    }
  }

  item.FillInDefaultIcon();
  m_currentMovieThumb = item.GetThumbnailImage();
}

string CGUIInfoManager::GetSystemHeatInfo(int info)
{
  if (CTimeUtils::GetFrameTime() - m_lastSysHeatInfoTime >= SYSHEATUPDATEINTERVAL)
  { // update our variables
    m_lastSysHeatInfoTime = CTimeUtils::GetFrameTime();
#if defined(_LINUX)
    m_cpuTemp = g_cpuInfo.getTemperature();
    m_gpuTemp = GetGPUTemperature();
#endif
  }

  CStdString text;
  switch(info)
  {
    case LCD_CPU_TEMPERATURE:
    case SYSTEM_CPU_TEMPERATURE:
      return m_cpuTemp.IsValid() ? m_cpuTemp.ToString() : "?";
      break;
    case LCD_GPU_TEMPERATURE:
    case SYSTEM_GPU_TEMPERATURE:
      return m_gpuTemp.IsValid() ? m_gpuTemp.ToString() : "?";
      break;
    case LCD_FAN_SPEED:
    case SYSTEM_FAN_SPEED:
      text.Format("%i%%", m_fanSpeed * 2);
      break;
    case SYSTEM_CPU_USAGE:
#if defined(__APPLE__) || defined(_WIN32)
      text.Format("%d%%", g_cpuInfo.getUsedPercentage());
#else
      text.Format("%s", g_cpuInfo.GetCoresUsageString());
#endif
      break;
  }
  return text;
}

CTemperature CGUIInfoManager::GetGPUTemperature()
{
  CStdString  cmd   = g_advancedSettings.m_gpuTempCmd;
  int         value = 0,
              ret   = 0;
  char        scale = 0;
  FILE        *p    = NULL;

  if (cmd.IsEmpty() || !(p = popen(cmd.c_str(), "r")))
    return CTemperature();

  ret = fscanf(p, "%d %c", &value, &scale);
  pclose(p);

  if (ret != 2)
    return CTemperature();

  if (scale == 'C' || scale == 'c')
    return CTemperature::CreateFromCelsius(value);
  if (scale == 'F' || scale == 'f')
    return CTemperature::CreateFromFahrenheit(value);
  return CTemperature();
}

CStdString CGUIInfoManager::GetVersion()
{
  CStdString tmp;
#ifdef GIT_REV
  tmp.Format("%s Git:%s", VERSION_STRING, GIT_REV);
#else
  tmp.Format("%s", VERSION_STRING);
#endif
  return tmp;
}

CStdString CGUIInfoManager::GetBuild()
{
  CStdString tmp;
  tmp.Format("%s", __DATE__);
  return tmp;
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

CStdString CGUIInfoManager::GetAudioScrobblerLabel(int item)
{
  switch (item)
  {
  case AUDIOSCROBBLER_CONN_STATE:
    return CLastfmScrobbler::GetInstance()->GetConnectionState();
    break;
  case AUDIOSCROBBLER_SUBMIT_INT:
    return CLastfmScrobbler::GetInstance()->GetSubmitInterval();
    break;
  case AUDIOSCROBBLER_FILES_CACHED:
    return CLastfmScrobbler::GetInstance()->GetFilesCached();
    break;
  case AUDIOSCROBBLER_SUBMIT_STATE:
    return CLastfmScrobbler::GetInstance()->GetSubmitState();
    break;
  }

  return "";
}

int CGUIInfoManager::GetOperator(const char ch)
{
  if (ch == '[')
    return 5;
  else if (ch == ']')
    return 4;
  else if (ch == '!')
    return OPERATOR_NOT;
  else if (ch == '+')
    return OPERATOR_AND;
  else if (ch == '|')
    return OPERATOR_OR;
  else
    return 0;
}

bool CGUIInfoManager::EvaluateBooleanExpression(const CCombinedValue &expression, bool &result, int contextWindow, const CGUIListItem *item)
{
  // stack to save our bool state as we go
  stack<bool> save;

  for (list<int>::const_iterator it = expression.m_postfix.begin(); it != expression.m_postfix.end(); ++it)
  {
    int expr = *it;
    if (expr == -OPERATOR_NOT)
    { // NOT the top item on the stack
      if (save.size() < 1) return false;
      bool expr = save.top();
      save.pop();
      save.push(!expr);
    }
    else if (expr == -OPERATOR_AND)
    { // AND the top two items on the stack
      if (save.size() < 2) return false;
      bool right = save.top(); save.pop();
      bool left = save.top(); save.pop();
      save.push(left && right);
    }
    else if (expr == -OPERATOR_OR)
    { // OR the top two items on the stack
      if (save.size() < 2) return false;
      bool right = save.top(); save.pop();
      bool left = save.top(); save.pop();
      save.push(left || right);
    }
    else  // operator
      save.push(GetBool(expr, contextWindow, item));
  }
  if (save.size() != 1) return false;
  result = save.top();
  return true;
}

int CGUIInfoManager::TranslateBooleanExpression(const CStdString &expression)
{
  CCombinedValue comb;
  comb.m_info = expression;
  comb.m_id = COMBINED_VALUES_START + m_CombinedValues.size();

  // operator stack
  stack<char> save;

  CStdString operand;

  for (unsigned int i = 0; i < expression.size(); i++)
  {
    if (GetOperator(expression[i]))
    {
      // cleanup any operand, translate and put into our expression list
      if (!operand.IsEmpty())
      {
        int iOp = TranslateSingleString(operand);
        if (iOp)
          comb.m_postfix.push_back(iOp);
        operand.clear();
      }
      // handle closing parenthesis
      if (expression[i] == ']')
      {
        while (save.size())
        {
          char oper = save.top();
          save.pop();

          if (oper == '[')
            break;

          comb.m_postfix.push_back(-GetOperator(oper));
        }
      }
      else
      {
        // all other operators we pop off the stack any operator
        // that has a higher priority than the one we have.
        while (!save.empty() && GetOperator(save.top()) > GetOperator(expression[i]))
        {
          // only handle parenthesis once they're closed.
          if (save.top() == '[' && expression[i] != ']')
            break;

          comb.m_postfix.push_back(-GetOperator(save.top()));  // negative denotes operator
          save.pop();
        }
        save.push(expression[i]);
      }
    }
    else
    {
      operand += expression[i];
    }
  }

  if (!operand.empty())
  {
    int op = TranslateSingleString(operand);
    if (op)
      comb.m_postfix.push_back(op);
  }

  // finish up by adding any operators
  while (!save.empty())
  {
    comb.m_postfix.push_back(-GetOperator(save.top()));
    save.pop();
  }

  // test evaluate
  bool test;
  if (!EvaluateBooleanExpression(comb, test, WINDOW_INVALID))
    CLog::Log(LOGERROR, "Error evaluating boolean expression %s", expression.c_str());
  // success - add to our combined values
  m_CombinedValues.push_back(comb);
  return comb.m_id;
}

void CGUIInfoManager::Clear()
{
  m_CombinedValues.clear();
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

int CGUIInfoManager::AddListItemProp(const CStdString &str, int offset)
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

int CGUIInfoManager::ConditionalStringParameter(const CStdString &parameter)
{
  // check to see if we have this parameter already
  for (unsigned int i = 0; i < m_stringParameters.size(); i++)
    if (parameter.Equals(m_stringParameters[i]))
      return (int)i;
  // return the new offset
  m_stringParameters.push_back(parameter);
  return (int)m_stringParameters.size() - 1;
}

CStdString CGUIInfoManager::GetItemLabel(const CFileItem *item, int info) const
{
  if (!item) return "";

  if (info >= LISTITEM_PROPERTY_START && info - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { // grab the property
    CStdString property = m_listitemProperties[info - LISTITEM_PROPERTY_START];
    return item->GetProperty(property);
  }

  switch (info)
  {
  case LISTITEM_LABEL:
    return item->GetLabel();
  case LISTITEM_LABEL2:
    return item->GetLabel2();
  case LISTITEM_TITLE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strTitle;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetTitle();
    break;
  case LISTITEM_ORIGINALTITLE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strOriginalTitle;
    break;
  case LISTITEM_PLAYCOUNT:
    {
      CStdString strPlayCount;
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_playCount > 0)
        strPlayCount.Format("%i", item->GetVideoInfoTag()->m_playCount);
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetPlayCount() > 0)
        strPlayCount.Format("%i", item->GetMusicInfoTag()->GetPlayCount());
      return strPlayCount;
    }
  case LISTITEM_LASTPLAYED:
    {
      CStdString strLastPlayed;
      if (item->HasVideoInfoTag())
        return item->GetVideoInfoTag()->m_lastPlayed;
      if (item->HasMusicInfoTag())
        return item->GetMusicInfoTag()->GetLastPlayed();
      break;
    }
  case LISTITEM_TRACKNUMBER:
    {
      CStdString track;
      if (item->HasMusicInfoTag())
        track.Format("%i", item->GetMusicInfoTag()->GetTrackNumber());

      return track;
    }
  case LISTITEM_DISC_NUMBER:
    {
      CStdString disc;
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetDiscNumber() > 0)
        disc.Format("%i", item->GetMusicInfoTag()->GetDiscNumber());
      return disc;
    }
  case LISTITEM_ARTIST:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strArtist;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetArtist();
    break;
  case LISTITEM_ALBUM_ARTIST:
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetAlbumArtist();
    break;
  case LISTITEM_DIRECTOR:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strDirector;
  case LISTITEM_ALBUM:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strAlbum;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetAlbum();
    break;
  case LISTITEM_YEAR:
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      if (item->GetVideoInfoTag()->m_iYear > 0)
        strResult.Format("%i",item->GetVideoInfoTag()->m_iYear);
      return strResult;
    }
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetYearString();
    break;
  case LISTITEM_PREMIERED:
    if (item->HasVideoInfoTag())
    {
      if (!item->GetVideoInfoTag()->m_strFirstAired.IsEmpty())
        return item->GetVideoInfoTag()->m_strFirstAired;
      if (!item->GetVideoInfoTag()->m_strPremiered.IsEmpty())
        return item->GetVideoInfoTag()->m_strPremiered;
    }
    break;
  case LISTITEM_GENRE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strGenre;
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetGenre();
    break;
  case LISTITEM_FILENAME:
    if (item->IsMusicDb() && item->HasMusicInfoTag())
      return URIUtils::GetFileName(item->GetMusicInfoTag()->GetURL());
    if (item->IsVideoDb() && item->HasVideoInfoTag())
      return URIUtils::GetFileName(item->GetVideoInfoTag()->m_strFileNameAndPath);
    return URIUtils::GetFileName(item->m_strPath);
  case LISTITEM_DATE:
    if (item->m_dateTime.IsValid())
      return item->m_dateTime.GetAsLocalizedDate();
    break;
  case LISTITEM_SIZE:
    if (!item->m_bIsFolder || item->m_dwSize)
      return StringUtils::SizeToString(item->m_dwSize);
    break;
  case LISTITEM_RATING:
    {
      CStdString rating;
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_fRating > 0.f) // movie rating
        rating.Format("%.1f", item->GetVideoInfoTag()->m_fRating);
      else if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetRating() > '0')
      { // song rating.  Images will probably be better than numbers for this in the long run
        rating = item->GetMusicInfoTag()->GetRating();
      }
      return rating;
    }
  case LISTITEM_RATING_AND_VOTES:
    {
      if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_fRating > 0.f) // movie rating
      {
        CStdString strRatingAndVotes;
        if (item->GetVideoInfoTag()->m_strVotes.IsEmpty())
          strRatingAndVotes.Format("%.1f", item->GetVideoInfoTag()->m_fRating);
        else
          strRatingAndVotes.Format("%.1f (%s %s)", item->GetVideoInfoTag()->m_fRating, item->GetVideoInfoTag()->m_strVotes, g_localizeStrings.Get(20350));
        return strRatingAndVotes;
      }
    }
    break;
  case LISTITEM_PROGRAM_COUNT:
    {
      CStdString count;
      count.Format("%i", item->m_iprogramCount);
      return count;
    }
  case LISTITEM_DURATION:
    {
      CStdString duration;
      if (item->HasVideoInfoTag())
      {
        if (item->GetVideoInfoTag()->m_streamDetails.GetVideoDuration() > 0)
          duration.Format("%i", item->GetVideoInfoTag()->m_streamDetails.GetVideoDuration() / 60);
        else if (!item->GetVideoInfoTag()->m_strRuntime.IsEmpty())
          duration = item->GetVideoInfoTag()->m_strRuntime;
      }
      if (item->HasMusicInfoTag())
      {
        if (item->GetMusicInfoTag()->GetDuration() > 0)
          duration = StringUtils::SecondsToTimeString(item->GetMusicInfoTag()->GetDuration());
      }
      return duration;
    }
  case LISTITEM_PLOT:
    if (item->HasVideoInfoTag())
    {
      if (!(!item->GetVideoInfoTag()->m_strShowTitle.IsEmpty() && item->GetVideoInfoTag()->m_iSeason == -1)) // dont apply to tvshows
        if (item->GetVideoInfoTag()->m_playCount == 0 && !g_guiSettings.GetBool("videolibrary.showunwatchedplots"))
          return g_localizeStrings.Get(20370);

      return item->GetVideoInfoTag()->m_strPlot;
    }
    break;
  case LISTITEM_PLOT_OUTLINE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strPlotOutline;
    break;
  case LISTITEM_EPISODE:
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      if (item->GetVideoInfoTag()->m_iSpecialSortEpisode > 0)
        strResult.Format("S%d",item->GetVideoInfoTag()->m_iEpisode);
      else if (item->GetVideoInfoTag()->m_iEpisode > 0) // if m_iEpisode = -1 there's no episode detail
        strResult.Format("%d",item->GetVideoInfoTag()->m_iEpisode);
      return strResult;
    }
    break;
  case LISTITEM_SEASON:
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      if (item->GetVideoInfoTag()->m_iSpecialSortSeason > 0)
        strResult.Format("%d",item->GetVideoInfoTag()->m_iSpecialSortSeason);
      else if (item->GetVideoInfoTag()->m_iSeason > 0) // if m_iSeason = -1 there's no season detail
        strResult.Format("%d",item->GetVideoInfoTag()->m_iSeason);
      return strResult;
    }
    break;
  case LISTITEM_TVSHOW:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strShowTitle;
    break;
  case LISTITEM_COMMENT:
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetComment();
    break;
  case LISTITEM_ACTUAL_ICON:
    return item->GetIconImage();
  case LISTITEM_ICON:
    {
      CStdString strThumb = item->GetThumbnailImage();
      if(!strThumb.IsEmpty() && !g_TextureManager.CanLoad(strThumb))
        strThumb = "";

      if(strThumb.IsEmpty() && !item->GetIconImage().IsEmpty())
        strThumb = item->GetIconImage();
      return strThumb;
    }
  case LISTITEM_OVERLAY:
    return item->GetOverlayImage();
  case LISTITEM_THUMB:
    return item->GetThumbnailImage();
  case LISTITEM_FOLDERPATH:
    return CURL(item->m_strPath).GetWithoutUserDetails();
  case LISTITEM_FOLDERNAME:
  case LISTITEM_PATH:
    {
      CStdString path;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        URIUtils::GetDirectory(item->GetMusicInfoTag()->GetURL(), path);
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
      {
        if( item->m_bIsFolder )
          path = item->GetVideoInfoTag()->m_strPath;
        else
          URIUtils::GetParentPath(item->GetVideoInfoTag()->m_strFileNameAndPath, path);
      }
      else
        URIUtils::GetParentPath(item->m_strPath, path);
      path = CURL(path).GetWithoutUserDetails();
      if (info==LISTITEM_FOLDERNAME)
      {
        URIUtils::RemoveSlashAtEnd(path);
        path=URIUtils::GetFileName(path);
      }
      CURL::Decode(path);
      return path;
    }
  case LISTITEM_FILENAME_AND_PATH:
    {
      CStdString path;
      if (item->IsMusicDb() && item->HasMusicInfoTag())
        path = item->GetMusicInfoTag()->GetURL();
      else if (item->IsVideoDb() && item->HasVideoInfoTag())
        path = item->GetVideoInfoTag()->m_strFileNameAndPath;
      else
        path = item->m_strPath;
      path = CURL(path).GetWithoutUserDetails();
      CURL::Decode(path);
      return path;
    }
  case LISTITEM_PICTURE_PATH:
    if (item->IsPicture() && (!item->IsZIP() || item->IsRAR() || item->IsCBZ() || item->IsCBR()))
      return item->m_strPath;
    break;
  case LISTITEM_PICTURE_DATETIME:
    if (item->HasPictureInfoTag())
      return item->GetPictureInfoTag()->GetInfo(SLIDE_EXIF_DATE_TIME);
    break;
  case LISTITEM_PICTURE_RESOLUTION:
    if (item->HasPictureInfoTag())
      return item->GetPictureInfoTag()->GetInfo(SLIDE_RESOLUTION);
    break;
  case LISTITEM_STUDIO:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strStudio;
    break;
  case LISTITEM_COUNTRY:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strCountry;
    break;
  case LISTITEM_MPAA:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strMPAARating;
    break;
  case LISTITEM_CAST:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->GetCast();
    break;
  case LISTITEM_CAST_AND_ROLE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->GetCast(true);
    break;
  case LISTITEM_WRITER:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strWritingCredits;
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
      CStdString strResult;
      if (item->GetVideoInfoTag()->m_iTop250 > 0)
        strResult.Format("%i",item->GetVideoInfoTag()->m_iTop250);
      return strResult;
    }
    break;
  case LISTITEM_SORT_LETTER:
    {
      CStdString letter;
      g_charsetConverter.wToUTF8(item->GetSortLabel().Left(1).ToUpper(), letter);
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
      CStdString strResult;
      int iChannels = item->GetVideoInfoTag()->m_streamDetails.GetAudioChannels();
      if (iChannels > -1)
        strResult.Format("%i", iChannels);
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
  }
  return "";
}

CStdString CGUIInfoManager::GetItemImage(const CFileItem *item, int info) const
{
  switch (info)
  {
  case LISTITEM_RATING:  // old song rating format
    {
      CStdString rating;
      if (item->HasMusicInfoTag())
      {
        rating.Format("songrating%c.png", item->GetMusicInfoTag()->GetRating());
        return rating;
      }
    }
    break;
  case LISTITEM_STAR_RATING:
    {
      CStdString rating;
      if (item->HasVideoInfoTag())
      { // rating for videos is assumed 0..10, so convert to 0..5
        rating.Format("rating%d.png", (long)((item->GetVideoInfoTag()->m_fRating * 0.5f) + 0.5f));
      }
      else if (item->HasMusicInfoTag())
      { // song rating.
        rating.Format("rating%c.png", item->GetMusicInfoTag()->GetRating());
      }
      return rating;
    }
    break;
  }  /* switch (info) */

  return GetItemLabel(item, info);
}

bool CGUIInfoManager::GetItemBool(const CGUIListItem *item, int condition) const
{
  if (!item) return false;
  if (condition >= LISTITEM_PROPERTY_START && condition - LISTITEM_PROPERTY_START < (int)m_listitemProperties.size())
  { // grab the property
    CStdString property = m_listitemProperties[condition - LISTITEM_PROPERTY_START];
    CStdString val = item->GetProperty(property);
    return (val == "1" || val.CompareNoCase("true") == 0);
  }
  else if (condition == LISTITEM_ISPLAYING)
  {
    if (item->IsFileItem() && !m_currentFile->m_strPath.IsEmpty())
    {
      if (!g_application.m_strPlayListFile.IsEmpty())
      {
        //playlist file that is currently playing or the playlistitem that is currently playing.
        return g_application.m_strPlayListFile.Equals(((const CFileItem *)item)->m_strPath) || m_currentFile->IsSamePath((const CFileItem *)item);
      }
      return m_currentFile->IsSamePath((const CFileItem *)item);
    }
  }
  else if (condition == LISTITEM_ISSELECTED)
    return item->IsSelected();
  else if (condition == LISTITEM_IS_FOLDER)
    return item->m_bIsFolder;
  return false;
}

void CGUIInfoManager::ResetCache()
{
  CSingleLock lock(m_critInfo);
  m_boolCache.clear();
  // reset any animation triggers as well
  m_containerMoves.clear();
}

void CGUIInfoManager::ResetPersistentCache()
{
  CSingleLock lock(m_critInfo);
  m_persistentBoolCache.clear();
}

inline void CGUIInfoManager::CacheBool(int condition, int contextWindow, bool result, bool persistent)
{
  // windows have id's up to 13100 or thereabouts (ie 2^14 needed)
  // conditionals have id's up to 100000 or thereabouts (ie 2^18 needed)
  CSingleLock lock(m_critInfo);
  int hash = ((contextWindow & 0x3fff) << 18) | (condition & 0x3ffff);
  if (persistent)
    m_persistentBoolCache.insert(pair<int, bool>(hash, result));
  else
    m_boolCache.insert(pair<int, bool>(hash, result));
}

bool CGUIInfoManager::IsCached(int condition, int contextWindow, bool &result) const
{
  // windows have id's up to 13100 or thereabouts (ie 2^14 needed)
  // conditionals have id's up to 100000 or thereabouts (ie 2^18 needed)

  CSingleLock lock(m_critInfo);
  int hash = ((contextWindow & 0x3fff) << 18) | (condition & 0x3ffff);
  map<int, bool>::const_iterator it = m_boolCache.find(hash);
  if (it != m_boolCache.end())
  {
    result = (*it).second;
    return true;
  }
  it = m_persistentBoolCache.find(hash);
  if (it != m_persistentBoolCache.end())
  {
    result = (*it).second;
    return true;
  }

  return false;
}

// Called from tuxbox service thread to update current status
void CGUIInfoManager::UpdateFromTuxBox()
{
  if(g_tuxbox.vVideoSubChannel.mode)
    m_currentFile->GetVideoInfoTag()->m_strTitle = g_tuxbox.vVideoSubChannel.current_name;

  // Set m_currentMovieDuration
  if(!g_tuxbox.sCurSrvData.current_event_duration.IsEmpty() &&
    !g_tuxbox.sCurSrvData.next_event_description.IsEmpty() &&
    !g_tuxbox.sCurSrvData.current_event_duration.Equals("-") &&
    !g_tuxbox.sCurSrvData.next_event_description.Equals("-"))
  {
    g_tuxbox.sCurSrvData.current_event_duration.Replace("(","");
    g_tuxbox.sCurSrvData.current_event_duration.Replace(")","");

    m_currentMovieDuration.Format("%s: %s %s (%s - %s)",
      g_localizeStrings.Get(180),
      g_tuxbox.sCurSrvData.current_event_duration,
      g_localizeStrings.Get(12391),
      g_tuxbox.sCurSrvData.current_event_time,
      g_tuxbox.sCurSrvData.next_event_time);
  }

  //Set strVideoGenre
  if (!g_tuxbox.sCurSrvData.current_event_description.IsEmpty() &&
    !g_tuxbox.sCurSrvData.next_event_description.IsEmpty() &&
    !g_tuxbox.sCurSrvData.current_event_description.Equals("-") &&
    !g_tuxbox.sCurSrvData.next_event_description.Equals("-"))
  {
    m_currentFile->GetVideoInfoTag()->m_strGenre.Format("%s %s  -  (%s: %s)",
      g_localizeStrings.Get(143),
      g_tuxbox.sCurSrvData.current_event_description,
      g_localizeStrings.Get(209),
      g_tuxbox.sCurSrvData.next_event_description);
  }

  //Set m_currentMovie.m_strDirector
  if (!g_tuxbox.sCurSrvData.current_event_details.Equals("-") &&
    !g_tuxbox.sCurSrvData.current_event_details.IsEmpty())
  {
    m_currentFile->GetVideoInfoTag()->m_strDirector = g_tuxbox.sCurSrvData.current_event_details;
  }
}

CStdString CGUIInfoManager::GetPictureLabel(int info) const
{
  if (info == SLIDE_FILE_NAME)
    return GetItemLabel(m_currentSlide, LISTITEM_FILENAME);
  else if (info == SLIDE_FILE_PATH)
  {
    CStdString path;
    URIUtils::GetDirectory(m_currentSlide->m_strPath, path);
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
      CStdString index;
      index.Format("%d/%d", slideshow->CurrentSlide(), slideshow->NumSlides());
      return index;
    }
  }
  if (m_currentSlide->HasPictureInfoTag())
    return m_currentSlide->GetPictureInfoTag()->GetInfo(info);
  return "";
}

void CGUIInfoManager::SetCurrentSlide(CFileItem &item)
{
  if (m_currentSlide->m_strPath != item.m_strPath)
  {
    if (!item.HasPictureInfoTag() && !item.GetPictureInfoTag()->Loaded())
      item.GetPictureInfoTag()->Load(item.m_strPath);
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
    case LIBRARY_HAS_TVSHOWS:
      m_libraryHasTVShows = value ? 1 : 0;
      break;
    case LIBRARY_HAS_MUSICVIDEOS:
      m_libraryHasMusicVideos = value ? 1 : 0;
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
  else if (condition == LIBRARY_HAS_VIDEO)
  {
    return (GetLibraryBool(LIBRARY_HAS_MOVIES) ||
            GetLibraryBool(LIBRARY_HAS_TVSHOWS) ||
            GetLibraryBool(LIBRARY_HAS_MUSICVIDEOS));
  }
  return false;
}
