#include "stdafx.h"
#include "../GUIDialogSeekBar.h"
#include "../GUIMediaWindow.h"
#include "../GUIDialogFileBrowser.h"
#include "../GUIDialogContentSettings.h"
#include "../Application.h"
#include "../Util.h"
#include "../lib/libscrobbler/scrobbler.h"
#include "../utils/TuxBoxUtil.h"
#include "KaiClient.h"
#include "Weather.h"
#include "../playlistplayer.h"
#include "../PartyModeManager.h"
#include "../Visualizations/Visualisation.h"
#include "../ButtonTranslator.h"
#include "../MusicDatabase.h"
#include "../utils/Alarmclock.h"
#ifdef HAS_LCD
#include "../utils/lcd.h"
#endif
#include "../GUIPassword.h"
#ifdef HAS_XBOX_HARDWARE
#include "FanController.h"
#include "../xbox/xkhdd.h"
#endif
#include "SystemInfo.h"
#include "GUIButtonScroller.h"
#include "GUIInfoManager.h"
#include <stack>
#include "../xbox/network.h"
#include "GUIWindowSlideShow.h"

// stuff for current song
#ifdef HAS_FILESYSTEM
#include "../filesystem/SndtrkDirectory.h"
#endif
#include "../musicInfoTagLoaderFactory.h"

#include "GUILabelControl.h"  // for CInfoPortion

using namespace XFILE;
using namespace DIRECTORY;

CGUIInfoManager g_infoManager;

void CGUIInfoManager::CCombinedValue::operator =(const CGUIInfoManager::CCombinedValue& mSrc)
{
  this->m_info = mSrc.m_info;
  this->m_id = mSrc.m_id;
  this->m_postfix = mSrc.m_postfix;
}

CGUIInfoManager::CGUIInfoManager(void)
{
  m_lastSysHeatInfoTime = 0;
  m_lastMusicBitrateTime = 0;
  m_fanSpeed = 0;
  m_AfterSeekTimeout = 0;
  m_playerSeeking = false;
  m_performingSeek = false;
  m_nextWindowID = WINDOW_INVALID;
  m_prevWindowID = WINDOW_INVALID;
  m_stringParameters.push_back("__ZZZZ__");   // to offset the string parameters by 1 to assure that all entries are non-zero
}

CGUIInfoManager::~CGUIInfoManager(void)
{
}

bool CGUIInfoManager::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_NOTIFY_ALL)
  {
    if (message.GetParam1() == GUI_MSG_UPDATE_ITEM && message.GetLPVOID())
    {
      CFileItem *item = (CFileItem *)message.GetLPVOID();
      if (m_currentFile.m_strPath.Equals(item->m_strPath))
        m_currentFile = *item;
      return true;
    }
  }
  return false;
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data. Can handle combined strings on the form
/// Player.Caching + VideoPlayer.IsFullscreen (Logical and)
/// Player.HasVideo | Player.HasAudio (Logical or)
int CGUIInfoManager::TranslateString(const CStdString &strCondition)
{
  if (strCondition.find_first_of("|") != strCondition.npos ||
      strCondition.find_first_of("+") != strCondition.npos ||
      strCondition.find_first_of("[") != strCondition.npos ||
      strCondition.find_first_of("]") != strCondition.npos)
  {
    // Have a boolean expression
    // Check if this was added before
    std::vector<CCombinedValue>::iterator it;
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
  if (strCondition.IsEmpty()) return 0;
  CStdString strTest = strCondition;
  strTest.ToLower();
  strTest.TrimLeft(" ");
  strTest.TrimRight(" ");
  bool bNegate = strTest[0] == '!';
  int ret = 0;

  if(bNegate)
    strTest.Delete(0, 1);

  CStdString strCategory = strTest.Left(strTest.Find("."));

  // translate conditions...
  if (strTest.Equals("false") || strTest.Equals("no") || strTest.Equals("off") || strTest.Equals("disabled")) ret = SYSTEM_ALWAYS_FALSE;
  else if (strTest.Equals("true") || strTest.Equals("yes") || strTest.Equals("on") || strTest.Equals("enabled")) ret = SYSTEM_ALWAYS_TRUE;
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
    else if (strTest.Equals("player.seekbar")) ret = PLAYER_SEEKBAR;
    else if (strTest.Equals("player.progress")) ret = PLAYER_PROGRESS;
    else if (strTest.Equals("player.seeking")) ret = PLAYER_SEEKING;
    else if (strTest.Equals("player.showtime")) ret = PLAYER_SHOWTIME;
    else if (strTest.Equals("player.showcodec")) ret = PLAYER_SHOWCODEC;
    else if (strTest.Equals("player.showinfo")) ret = PLAYER_SHOWINFO;
    else if (strTest.Left(15).Equals("player.seektime")) return AddMultiInfo(GUIInfo(PLAYER_SEEKTIME, TranslateTimeFormat(strTest.Mid(15))));
    else if (strTest.Left(20).Equals("player.timeremaining")) return AddMultiInfo(GUIInfo(PLAYER_TIME_REMAINING, TranslateTimeFormat(strTest.Mid(20))));
    else if (strTest.Left(16).Equals("player.timespeed")) return AddMultiInfo(GUIInfo(PLAYER_TIME_SPEED, TranslateTimeFormat(strTest.Mid(16))));
    else if (strTest.Left(11).Equals("player.time")) return AddMultiInfo(GUIInfo(PLAYER_TIME, TranslateTimeFormat(strTest.Mid(11))));
    else if (strTest.Left(15).Equals("player.duration")) return AddMultiInfo(GUIInfo(PLAYER_DURATION, TranslateTimeFormat(strTest.Mid(15))));
    else if (strTest.Left(17).Equals("player.finishtime")) return AddMultiInfo(GUIInfo(PLAYER_FINISH_TIME, TranslateTimeFormat(strTest.Mid(17))));
    else if (strTest.Equals("player.volume")) ret = PLAYER_VOLUME;
    else if (strTest.Equals("player.muted")) ret = PLAYER_MUTED;
    else if (strTest.Equals("player.hasduration")) ret = PLAYER_HASDURATION;
    else if (strTest.Equals("player.chapter")) ret = PLAYER_CHAPTER;
    else if (strTest.Equals("player.chaptercount")) ret = PLAYER_CHAPTERCOUNT;
  }
  else if (strCategory.Equals("weather"))
  {
    if (strTest.Equals("weather.conditions")) ret = WEATHER_CONDITIONS;
    else if (strTest.Equals("weather.temperature")) ret = WEATHER_TEMPERATURE;
    else if (strTest.Equals("weather.location")) ret = WEATHER_LOCATION;
    else if (strTest.Equals("weather.isfetched")) ret = WEATHER_IS_FETCHED;
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
    else if (strTest.Left(11).Equals("system.time")) return AddMultiInfo(GUIInfo(SYSTEM_TIME, TranslateTimeFormat(strTest.Mid(11))));
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
    else if (strTest.Equals("system.kaiconnected")) ret = SYSTEM_KAI_CONNECTED;
    else if (strTest.Equals("system.kaienabled")) ret = SYSTEM_KAI_ENABLED;
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
    else if (strTest.Equals("system.screenmode")) ret = SYSTEM_SCREEN_MODE;
    else if (strTest.Equals("system.screenwidth")) ret = SYSTEM_SCREEN_WIDTH;
    else if (strTest.Equals("system.screenheight")) ret = SYSTEM_SCREEN_HEIGHT;
    else if (strTest.Equals("system.currentwindow")) ret = SYSTEM_CURRENT_WINDOW;
    else if (strTest.Equals("system.currentcontrol")) ret = SYSTEM_CURRENT_CONTROL;
    else if (strTest.Equals("system.xboxnickname")) ret = SYSTEM_XBOX_NICKNAME;
    else if (strTest.Equals("system.dvdlabel")) ret = SYSTEM_DVD_LABEL;
    else if (strTest.Equals("system.haslocks")) ret = SYSTEM_HASLOCKS;
    else if (strTest.Equals("system.hasloginscreen")) ret = SYSTEM_HAS_LOGINSCREEN;
    else if (strTest.Equals("system.ismaster")) ret = SYSTEM_ISMASTER;
    else if (strTest.Equals("system.internetstate")) ret = SYSTEM_INTERNET_STATE;
    else if (strTest.Equals("system.loggedon")) ret = SYSTEM_LOGGEDON;
    else if (strTest.Equals("system.hasdrivef")) ret = SYSTEM_HAS_DRIVE_F;
    else if (strTest.Equals("system.hasdriveg")) ret = SYSTEM_HAS_DRIVE_G;
    else if (strTest.Equals("system.hddtemperature")) ret = SYSTEM_HDD_TEMPERATURE;
    else if (strTest.Equals("system.hddinfomodel")) ret = SYSTEM_HDD_MODEL;
    else if (strTest.Equals("system.hddinfofirmware")) ret = SYSTEM_HDD_FIRMWARE;
    else if (strTest.Equals("system.hddinfoserial")) ret = SYSTEM_HDD_SERIAL;
    else if (strTest.Equals("system.hddinfopw")) ret = SYSTEM_HDD_PASSWORD;
    else if (strTest.Equals("system.hddinfolockstate")) ret = SYSTEM_HDD_LOCKSTATE;
    else if (strTest.Equals("system.hddlockkey")) ret = SYSTEM_HDD_LOCKKEY;
    else if (strTest.Equals("system.hddbootdate")) ret = SYSTEM_HDD_BOOTDATE;
    else if (strTest.Equals("system.hddcyclecount")) ret = SYSTEM_HDD_CYCLECOUNT;
    else if (strTest.Equals("system.dvdinfomodel")) ret = SYSTEM_DVD_MODEL;
    else if (strTest.Equals("system.dvdinfofirmware")) ret = SYSTEM_DVD_FIRMWARE;
    else if (strTest.Equals("system.mplayerversion")) ret = SYSTEM_MPLAYER_VERSION;
    else if (strTest.Equals("system.kernelversion")) ret = SYSTEM_KERNEL_VERSION;
    else if (strTest.Equals("system.uptime")) ret = SYSTEM_UPTIME;
    else if (strTest.Equals("system.totaluptime")) ret = SYSTEM_TOTALUPTIME;
    else if (strTest.Equals("system.cpufrequency")) ret = SYSTEM_CPUFREQUENCY;
    else if (strTest.Equals("system.xboxversion")) ret = SYSTEM_XBOX_VERSION;
    else if (strTest.Equals("system.avpackinfo")) ret = SYSTEM_AV_PACK_INFO;
    else if (strTest.Equals("system.screenresolution")) ret = SYSTEM_SCREEN_RESOLUTION;
    else if (strTest.Equals("system.videoencoderinfo")) ret = SYSTEM_VIDEO_ENCODER_INFO;
    else if (strTest.Equals("system.xboxproduceinfo")) ret = SYSTEM_XBOX_PRODUCE_INFO;
    else if (strTest.Equals("system.xboxserial")) ret = SYSTEM_XBOX_SERIAL;
    else if (strTest.Equals("system.xberegion")) ret = SYSTEM_XBE_REGION;
    else if (strTest.Equals("system.dvdzone")) ret = SYSTEM_DVD_ZONE;
    else if (strTest.Equals("system.bios")) ret = SYSTEM_XBOX_BIOS;
    else if (strTest.Equals("system.modchip")) ret = SYSTEM_XBOX_MODCHIP;
    else if (strTest.Left(22).Equals("system.controllerport("))
    {
      int i_ControllerPort = atoi((strTest.Mid(22, strTest.GetLength() - 23).c_str()));
      if (i_ControllerPort == 1) ret = SYSTEM_CONTROLLER_PORT_1;
      else if (i_ControllerPort == 2) ret = SYSTEM_CONTROLLER_PORT_2;
      else if (i_ControllerPort == 3) ret = SYSTEM_CONTROLLER_PORT_3;
      else if (i_ControllerPort == 4)ret = SYSTEM_CONTROLLER_PORT_4;
      else ret = SYSTEM_CONTROLLER_PORT_1;
    }
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
    else if (strTest.Equals("system.profilename")) ret = SYSTEM_PROFILENAME;
    else if (strTest.Equals("system.profilethumb")) ret = SYSTEM_PROFILETHUMB;
    else if (strTest.Equals("system.launchxbe")) ret = SYSTEM_LAUNCHING_XBE;
    else if (strTest.Equals("system.progressbar")) ret = SYSTEM_PROGRESS_BAR;
  }
  else if (strCategory.Equals("xlinkkai"))
  {
    if (strTest.Equals("xlinkkai.username")) ret = XLINK_KAI_USERNAME;
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
  }
  else if (strCategory.Equals("network"))
  {
    if (strTest.Equals("network.ipaddress")) ret = NETWORK_IP_ADDRESS;
    if (strTest.Equals("network.isdhcp")) ret = NETWORK_IS_DHCP;
    if (strTest.Equals("network.linkstate")) ret = NETWORK_LINK_STATE;
#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
    if (strTest.Equals("network.macadress") || strTest.Equals("network.macaddress")) ret = NETWORK_MAC_ADDRESS;
    if (strTest.Equals("network.subetadress") || strTest.Equals("network.subnetaddress")) ret = NETWORK_SUBNET_ADDRESS;
    if (strTest.Equals("network.gatewayadress") || strTest.Equals("network.gatewayaddress")) ret = NETWORK_GATEWAY_ADDRESS;
    if (strTest.Equals("network.dns1adress") || strTest.Equals("network.dns1address")) ret = NETWORK_DNS1_ADDRESS;
    if (strTest.Equals("network.dns2adress") || strTest.Equals("network.dns2address")) ret = NETWORK_DNS2_ADDRESS;
    if (strTest.Equals("network.dhcpadress") || strTest.Equals("network.dhcpaddress")) ret = NETWORK_DHCP_ADDRESS;
#else
    if (strTest.Equals("network.macaddress")) ret = NETWORK_MAC_ADDRESS;
    if (strTest.Equals("network.subnetaddress")) ret = NETWORK_SUBNET_ADDRESS;
    if (strTest.Equals("network.gatewayaddress")) ret = NETWORK_GATEWAY_ADDRESS;
    if (strTest.Equals("network.dns1address")) ret = NETWORK_DNS1_ADDRESS;
    if (strTest.Equals("network.dns2address")) ret = NETWORK_DNS2_ADDRESS;
    if (strTest.Equals("network.dhcpaddress")) ret = NETWORK_DHCP_ADDRESS;
#endif
  }
  else if (strCategory.Equals("musicplayer"))
  {
    if (strTest.Equals("musicplayer.title")) ret = MUSICPLAYER_TITLE;
    else if (strTest.Equals("musicplayer.album")) ret = MUSICPLAYER_ALBUM;
    else if (strTest.Equals("musicplayer.artist")) ret = MUSICPLAYER_ARTIST;
    else if (strTest.Equals("musicplayer.year")) ret = MUSICPLAYER_YEAR;
    else if (strTest.Equals("musicplayer.genre")) ret = MUSICPLAYER_GENRE;
    else if (strTest.Left(25).Equals("musicplayer.timeremaining")) ret = AddMultiInfo(GUIInfo(PLAYER_TIME_REMAINING, TranslateTimeFormat(strTest.Mid(25))));
    else if (strTest.Left(21).Equals("musicplayer.timespeed")) ret = AddMultiInfo(GUIInfo(PLAYER_TIME_SPEED, TranslateTimeFormat(strTest.Mid(21))));
    else if (strTest.Left(16).Equals("musicplayer.time")) ret = AddMultiInfo(GUIInfo(PLAYER_TIME, TranslateTimeFormat(strTest.Mid(16))));
    else if (strTest.Left(20).Equals("musicplayer.duration")) ret = AddMultiInfo(GUIInfo(PLAYER_DURATION, TranslateTimeFormat(strTest.Mid(20))));
    else if (strTest.Equals("musicplayer.tracknumber")) ret = MUSICPLAYER_TRACK_NUMBER;
    else if (strTest.Equals("musicplayer.cover")) ret = MUSICPLAYER_COVER;
    else if (strTest.Equals("musicplayer.bitrate")) ret = MUSICPLAYER_BITRATE;
    else if (strTest.Equals("musicplayer.playlistlength")) ret = MUSICPLAYER_PLAYLISTLEN;
    else if (strTest.Equals("musicplayer.playlistposition")) ret = MUSICPLAYER_PLAYLISTPOS;
    else if (strTest.Equals("musicplayer.channels")) ret = MUSICPLAYER_CHANNELS;
    else if (strTest.Equals("musicplayer.bitspersample")) ret = MUSICPLAYER_BITSPERSAMPLE;
    else if (strTest.Equals("musicplayer.samplerate")) ret = MUSICPLAYER_SAMPLERATE;
    else if (strTest.Equals("musicplayer.codec")) ret = MUSICPLAYER_CODEC;
    else if (strTest.Equals("musicplayer.discnumber")) ret = MUSICPLAYER_DISC_NUMBER;
    else if (strTest.Equals("musicplayer.rating")) ret = MUSICPLAYER_RATING;
    else if (strTest.Equals("musicplayer.comment")) ret = MUSICPLAYER_COMMENT;
  }
  else if (strCategory.Equals("videoplayer"))
  {
    if (strTest.Equals("videoplayer.title")) ret = VIDEOPLAYER_TITLE;
    else if (strTest.Equals("videoplayer.genre")) ret = VIDEOPLAYER_GENRE;
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
    else if (strTest.Equals("videoplayer.tvshowtitle")) ret = VIDEOPLAYER_TVSHOW;
    else if (strTest.Equals("videoplayer.premiered")) ret = VIDEOPLAYER_PREMIERED;
    else if (strTest.Left(19).Equals("videoplayer.content")) return AddMultiInfo(GUIInfo(bNegate ? -VIDEOPLAYER_CONTENT : VIDEOPLAYER_CONTENT, ConditionalStringParameter(strTest.Mid(20,strTest.size()-21)), 0));
    else if (strTest.Equals("videoplayer.studio")) ret = VIDEOPLAYER_STUDIO;
    else if (strTest.Equals("videoplayer.mpaa")) return VIDEOPLAYER_MPAA;
    else if (strTest.Equals("videoplayer.cast")) return VIDEOPLAYER_CAST;
    else if (strTest.Equals("videoplayer.castandrole")) return VIDEOPLAYER_CAST_AND_ROLE;
    else if (strTest.Equals("videoplayer.artist")) return VIDEOPLAYER_ARTIST;
    else if (strTest.Equals("videoplayer.album")) return VIDEOPLAYER_ALBUM;
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
  else if (strCategory.Equals("slideshow"))
    ret = CPictureInfoTag::TranslateString(strTest.Mid(strCategory.GetLength() + 1));
  else if (strCategory.Left(9).Equals("container"))
  {
    int id = atoi(strCategory.Mid(10, strCategory.GetLength() - 11));
    CStdString info = strTest.Mid(strCategory.GetLength() + 1);
    if (info.Left(8).Equals("listitem"))
    {
      int offset = atoi(info.Mid(9, info.GetLength() - 10));
      ret = TranslateListItem(info.Mid(info.Find(".")+1));
      if (offset || id)
        return AddMultiInfo(GUIInfo(bNegate ? -ret : ret, id, offset));
    }
    else if (info.Equals("folderthumb")) ret = CONTAINER_FOLDERTHUMB;
    else if (info.Equals("folderpath")) ret = CONTAINER_FOLDERPATH;
    else if (info.Equals("onnext")) ret = CONTAINER_ON_NEXT;
    else if (info.Equals("onprevious")) ret = CONTAINER_ON_PREVIOUS;
    else if (info.Left(8).Equals("content("))
      return AddMultiInfo(GUIInfo(bNegate ? -CONTAINER_CONTENT : CONTAINER_CONTENT, ConditionalStringParameter(info.Mid(8,info.GetLength()-9)), 0));
    else if (info.Equals("hasthumb")) ret = CONTAINER_HAS_THUMB;
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
    if (id && (ret == CONTAINER_ON_NEXT || ret == CONTAINER_ON_PREVIOUS))
      return AddMultiInfo(GUIInfo(bNegate ? -ret : ret, id));
  }
  else if (strCategory.Left(8).Equals("listitem"))
  {
    int offset = atoi(strCategory.Mid(9, strCategory.GetLength() - 10));
    ret = TranslateListItem(strTest.Mid(strCategory.GetLength() + 1));
    if (offset || ret == LISTITEM_ISSELECTED || ret == LISTITEM_ISPLAYING)
      return AddMultiInfo(GUIInfo(bNegate ? -ret : ret, 0, offset));
  }
  else if (strCategory.Equals("visualisation"))
  {
    if (strTest.Equals("visualisation.locked")) ret = VISUALISATION_LOCKED;
    else if (strTest.Equals("visualisation.preset")) ret = VISUALISATION_PRESET;
    else if (strTest.Equals("visualisation.name")) ret = VISUALISATION_NAME;
    else if (strTest.Equals("visualisation.enabled")) ret = VISUALISATION_ENABLED;
  }
  else if (strCategory.Equals("skin"))
  {
    if (strTest.Equals("skin.currenttheme"))
      ret = SKIN_THEME;
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
  else if (strTest.Left(16).Equals("window.isactive("))
  {
    CStdString window(strTest.Mid(16, strTest.GetLength() - 17).ToLower());
    if (window.Find("xml") >= 0)
      return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_ACTIVE : WINDOW_IS_ACTIVE, 0, ConditionalStringParameter(window)));
    int winID = g_buttonTranslator.TranslateWindowString(window.c_str());
    if (winID != WINDOW_INVALID)
      return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_ACTIVE : WINDOW_IS_ACTIVE, winID, 0));
  }
  else if (strTest.Equals("window.ismedia")) return WINDOW_IS_MEDIA;
  else if (strTest.Left(17).Equals("window.istopmost("))
  {
    CStdString window(strTest.Mid(17, strTest.GetLength() - 18).ToLower());
    if (window.Find("xml") >= 0)
      return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_TOPMOST : WINDOW_IS_TOPMOST, 0, ConditionalStringParameter(window)));
    int winID = g_buttonTranslator.TranslateWindowString(window.c_str());
    if (winID != WINDOW_INVALID)
      return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_TOPMOST : WINDOW_IS_TOPMOST, winID, 0));
  }
  else if (strTest.Left(17).Equals("window.isvisible("))
  {
    CStdString window(strTest.Mid(17, strTest.GetLength() - 18).ToLower());
    if (window.Find("xml") >= 0)
      return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_VISIBLE : WINDOW_IS_VISIBLE, 0, ConditionalStringParameter(window)));
    int winID = g_buttonTranslator.TranslateWindowString(window.c_str());
    if (winID != WINDOW_INVALID)
      return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_IS_VISIBLE : WINDOW_IS_VISIBLE, winID, 0));
  }
  else if (strTest.Left(16).Equals("window.previous("))
  {
    CStdString window(strTest.Mid(16, strTest.GetLength() - 17).ToLower());
    if (window.Find("xml") >= 0)
      return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_PREVIOUS : WINDOW_PREVIOUS, 0, ConditionalStringParameter(window)));
    int winID = g_buttonTranslator.TranslateWindowString(window.c_str());
    if (winID != WINDOW_INVALID)
      return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_PREVIOUS : WINDOW_PREVIOUS, winID, 0));
  }
  else if (strTest.Left(12).Equals("window.next("))
  {
    CStdString window(strTest.Mid(12, strTest.GetLength() - 13).ToLower());
    if (window.Find("xml") >= 0)
      return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_NEXT : WINDOW_NEXT, 0, ConditionalStringParameter(window)));
    int winID = g_buttonTranslator.TranslateWindowString(window.c_str());
    if (winID != WINDOW_INVALID)
      return AddMultiInfo(GUIInfo(bNegate ? -WINDOW_NEXT : WINDOW_NEXT, winID, 0));
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
  else if (info.Equals("year")) return LISTITEM_YEAR;
  else if (info.Equals("genre")) return LISTITEM_GENRE;
  else if (info.Equals("director")) return LISTITEM_DIRECTOR;
  else if (info.Equals("filename")) return LISTITEM_FILENAME;
  else if (info.Equals("date")) return LISTITEM_DATE;
  else if (info.Equals("size")) return LISTITEM_SIZE;
  else if (info.Equals("rating")) return LISTITEM_RATING;
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
  else if (info.Equals("picturepath")) return LISTITEM_PICTURE_PATH;
  else if (info.Equals("pictureresolution")) return LISTITEM_PICTURE_RESOLUTION;
  else if (info.Equals("picturedatetime")) return LISTITEM_PICTURE_DATETIME;
  else if (info.Equals("studio")) return LISTITEM_STUDIO;
  else if (info.Equals("mpaa")) return LISTITEM_MPAA;
  else if (info.Equals("cast")) return LISTITEM_CAST;
  else if (info.Equals("castandrole")) return LISTITEM_CAST_AND_ROLE;
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
  return TIME_FORMAT_GUESS;
}

CStdString CGUIInfoManager::GetLabel(int info)
{
  CStdString strLabel;
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START]);

  if (info >= SLIDE_INFO_START && info <= SLIDE_INFO_END)
    return GetPictureLabel(info);

  switch (info)
  {
  case WEATHER_CONDITIONS:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_COND);
    break;
  case WEATHER_TEMPERATURE:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_CURRENT_TEMP);
    break;
  case WEATHER_LOCATION:
    strLabel = g_weatherManager.GetInfo(WEATHER_LABEL_LOCATION);
    break;
  case SYSTEM_DATE:
    strLabel = GetDate();
    break;
  case SYSTEM_LAUNCHING_XBE:
    strLabel = m_launchingXBE;
    break;
  case LCD_DATE:
    strLabel = GetDate(true);
    break;
  case SYSTEM_FPS:
    strLabel.Format("%02.2f", m_fps);
    break;
  case PLAYER_VOLUME:
    strLabel.Format("%2.1f dB", (float)(g_stSettings.m_nVolumeLevel + g_stSettings.m_dynamicRangeCompressionLevel) * 0.01f);
    break;
  case PLAYER_CHAPTER:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel.Format("%02d", g_application.m_pPlayer->GetChapter());
    break;
  case PLAYER_CHAPTERCOUNT:
    if(g_application.IsPlaying() && g_application.m_pPlayer)
      strLabel.Format("%02d", g_application.m_pPlayer->GetChapterCount());
    break;
  case MUSICPLAYER_TITLE:
  case MUSICPLAYER_ALBUM:
  case MUSICPLAYER_ARTIST:
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
  case VIDEOPLAYER_TVSHOW:
  case VIDEOPLAYER_PREMIERED:
  case VIDEOPLAYER_STUDIO:
  case VIDEOPLAYER_MPAA:
  case VIDEOPLAYER_CAST:
  case VIDEOPLAYER_CAST_AND_ROLE:
  case VIDEOPLAYER_ARTIST:
  case VIDEOPLAYER_ALBUM:
    strLabel = GetVideoLabel(info);
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

#ifdef HAS_XBOX_HARDWARE
  case SYSTEM_DVD_TRAY_STATE:
    return g_sysinfo.GetTrayState();
    break;
#endif

  case SYSTEM_CPU_TEMPERATURE:
  case SYSTEM_GPU_TEMPERATURE:
  case SYSTEM_FAN_SPEED:
  case LCD_CPU_TEMPERATURE:
  case LCD_GPU_TEMPERATURE:
  case LCD_FAN_SPEED:
  case SYSTEM_CPU_USAGE:
    return GetSystemHeatInfo(info);
    break;

#ifdef HAS_XBOX_HARDWARE
  case LCD_HDD_TEMPERATURE:
  case SYSTEM_HDD_MODEL:
  case SYSTEM_HDD_SERIAL:
  case SYSTEM_HDD_FIRMWARE:
  case SYSTEM_HDD_PASSWORD:
  case SYSTEM_HDD_LOCKSTATE:
  case SYSTEM_DVD_MODEL:
  case SYSTEM_DVD_FIRMWARE:
  case SYSTEM_HDD_TEMPERATURE:
  case SYSTEM_XBOX_MODCHIP:
  case SYSTEM_CPUFREQUENCY:
  case SYSTEM_XBOX_VERSION:
  case SYSTEM_AV_PACK_INFO:
  case SYSTEM_VIDEO_ENCODER_INFO:
  case NETWORK_MAC_ADDRESS:
  case SYSTEM_XBOX_SERIAL:
  case SYSTEM_XBE_REGION:
  case SYSTEM_DVD_ZONE:
  case SYSTEM_XBOX_PRODUCE_INFO:
  case SYSTEM_XBOX_BIOS:
  case SYSTEM_HDD_LOCKKEY:
  case SYSTEM_HDD_CYCLECOUNT:
  case SYSTEM_HDD_BOOTDATE:  
  case SYSTEM_MPLAYER_VERSION:
  case SYSTEM_KERNEL_VERSION:
#endif
  case SYSTEM_INTERNET_STATE:
  case SYSTEM_UPTIME:
  case SYSTEM_TOTALUPTIME:
    return g_sysinfo.GetInfo(info);
    break;

  case SYSTEM_SCREEN_RESOLUTION:
    strLabel.Format("%s %ix%i %s %02.2f Hz.",g_localizeStrings.Get(13287),
    g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iWidth,
    g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].iHeight,
    g_settings.m_ResInfo[g_guiSettings.m_LookAndFeelResolution].strMode,GetFPS());
    return strLabel;
    break;
#ifdef HAS_XBOX_HARDWARE
  case SYSTEM_CONTROLLER_PORT_1:
    return g_sysinfo.GetUnits(1);
    break;
  case SYSTEM_CONTROLLER_PORT_2:
    return g_sysinfo.GetUnits(2);
    break;
  case SYSTEM_CONTROLLER_PORT_3:
    return g_sysinfo.GetUnits(3);
    break;
  case SYSTEM_CONTROLLER_PORT_4:
    return g_sysinfo.GetUnits(4);
    break;
#endif  
  case CONTAINER_FOLDERPATH:
    {
      CGUIWindow *window = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
      if (window && window->IsMediaWindow())
      {
        CURL url(((CGUIMediaWindow*)window)->CurrentDirectory().m_strPath);
        url.GetURLWithoutUserDetails(strLabel);
      }
      break;
    }
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
        strLabel.Format("%iMB", stat.dwAvailPhys /MB);      
      else if (info == SYSTEM_FREE_MEMORY_PERCENT)
        strLabel.Format("%i%%", iMemPercentFree);
      else if (info == SYSTEM_USED_MEMORY)
        strLabel.Format("%iMB", (stat.dwTotalPhys - stat.dwAvailPhys)/MB);
      else if (info == SYSTEM_USED_MEMORY_PERCENT)
        strLabel.Format("%i%%", iMemPercentUsed);
      else if (info == SYSTEM_TOTAL_MEMORY)
        strLabel.Format("%iMB", stat.dwTotalPhys/MB);
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
    return g_localizeStrings.Get(m_gWindowManager.GetActiveWindow());
    break;
  case SYSTEM_CURRENT_CONTROL:
    {
      CGUIWindow *window = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
      if (window)
      {
        CGUIControl *control = window->GetFocusedControl();
        if (control)
          strLabel = control->GetDescription();
      }
    }
    break;
  case SYSTEM_XBOX_NICKNAME:
    {
      if (!CUtil::GetXBOXNickName(strLabel))
        strLabel=g_localizeStrings.Get(416); // 
      break;
    }
  case SYSTEM_DVD_LABEL:
    strLabel = CDetectDVDMedia::GetDVDLabel();
    break;
  case SYSTEM_ALARM_POS:
    if (g_alarmClock.GetRemaining("shutdowntimer") == 0.f)
      strLabel = "";
    else
    {
      double fTime = g_alarmClock.GetRemaining("shutdowntimer");
      if (fTime > 60.f)
        strLabel.Format("%2.0fm",g_alarmClock.GetRemaining("shutdowntimer")/60.f);
      else
        strLabel.Format("%2.0fs",g_alarmClock.GetRemaining("shutdowntimer")/60.f);
    }
    break;
  case SYSTEM_PROFILENAME:
    strLabel = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getName();
    break;
  case SYSTEM_LANGUAGE:
    strLabel = g_guiSettings.GetString("locale.language");
    break;
  case SYSTEM_PROGRESS_BAR:
    {
      int percent = GetInt(SYSTEM_PROGRESS_BAR);
      if (percent)
        strLabel.Format("%i", percent);
    }
    break;
  case XLINK_KAI_USERNAME:
    strLabel = g_guiSettings.GetString("xlinkkai.username");
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
  case SKIN_THEME:
    if (g_guiSettings.GetString("lookandfeel.skintheme").Equals("skindefault"))
      strLabel = g_localizeStrings.Get(15109);
    else
      strLabel = g_guiSettings.GetString("lookandfeel.skintheme");
    break;
#ifdef HAS_LCD
  case LCD_PROGRESS_BAR:
    if (g_lcd) strLabel = g_lcd->GetProgressBar(g_application.GetTime(), g_application.GetTotalTime());
    break;
#endif
  case NETWORK_IP_ADDRESS:
    {
      CStdString ip;
      ip.Format("%s: %s", g_localizeStrings.Get(150).c_str(), g_network.m_networkinfo.ip);
      return ip;
    }
    break;
  case NETWORK_SUBNET_ADDRESS:
    {
      CStdString subnet;
      subnet.Format("%s: %s", g_localizeStrings.Get(13159), g_network.m_networkinfo.subnet);
      return subnet;
    }
    break;
  case NETWORK_GATEWAY_ADDRESS:
    {
      CStdString gateway;
      gateway.Format("%s: %s", g_localizeStrings.Get(13160), g_network.m_networkinfo.gateway);
      return gateway;
    }
    break;
  case NETWORK_DNS1_ADDRESS:
    {
      CStdString dns;
      dns.Format("%s: %s", g_localizeStrings.Get(13161), g_network.m_networkinfo.DNS1);
      return dns;
    }
    break;
  case NETWORK_DNS2_ADDRESS:
    {
      CStdString dns;
      dns.Format("%s: %s", g_localizeStrings.Get(20307), g_network.m_networkinfo.DNS2);
      return dns;
    }
    break;
  case NETWORK_DHCP_ADDRESS:
    {
      CStdString dhcpserver;
      dhcpserver.Format("%s: %s", g_localizeStrings.Get(20308), g_network.m_networkinfo.dhcpserver);
      return dhcpserver;
    }
    break;
  case NETWORK_IS_DHCP:
    {
      CStdString dhcp;
      if(g_network.m_networkinfo.DHCP)
        dhcp.Format("%s %s", g_localizeStrings.Get(146), g_localizeStrings.Get(148)); // is dhcp ip
      else
        dhcp.Format("%s %s", g_localizeStrings.Get(146), g_localizeStrings.Get(147)); // is fixed ip
     return dhcp;
    }
    break;
#ifdef HAS_XBOX_HARDWARE
  case NETWORK_LINK_STATE:
    {
      DWORD dwnetstatus = XNetGetEthernetLinkStatus();
      CStdString linkStatus = g_localizeStrings.Get(151);
      linkStatus += " ";
      if (dwnetstatus & XNET_ETHERNET_LINK_ACTIVE)
      {
        if (dwnetstatus & XNET_ETHERNET_LINK_100MBPS)
          linkStatus += "100mbps ";
        if (dwnetstatus & XNET_ETHERNET_LINK_10MBPS)
          linkStatus += "10mbps ";
        if (dwnetstatus & XNET_ETHERNET_LINK_FULL_DUPLEX)
          linkStatus += g_localizeStrings.Get(153);
        if (dwnetstatus & XNET_ETHERNET_LINK_HALF_DUPLEX)
          linkStatus += g_localizeStrings.Get(152);
      }
      else
        linkStatus += g_localizeStrings.Get(159);
      return linkStatus;
    }
    break;
#endif
  case AUDIOSCROBBLER_CONN_STATE:
  case AUDIOSCROBBLER_SUBMIT_INT:
  case AUDIOSCROBBLER_FILES_CACHED:
  case AUDIOSCROBBLER_SUBMIT_STATE:
    strLabel=GetAudioScrobblerLabel(info);
    break;
  case VISUALISATION_PRESET:
    {
      CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
      g_graphicsContext.SendMessage(msg);
      if (msg.GetLPVOID())
      {
        CVisualisation *pVis = (CVisualisation *)msg.GetLPVOID();
        char *preset = pVis->GetPreset();
        if (preset)
        {
          strLabel = preset;
          CUtil::RemoveExtension(strLabel);
        }
      }
    }
    break;
  case VISUALISATION_NAME:
    {
      strLabel = g_guiSettings.GetString("mymusic.visualisation");
      if (strLabel != "None" && strLabel.size() > 4)
      { // make it look pretty
        strLabel = strLabel.Left(strLabel.size() - 4);
        strLabel[0] = toupper(strLabel[0]);
      }
    }
    break;
  case LISTITEM_LABEL:
  case LISTITEM_LABEL2:
  case LISTITEM_TITLE:
  case LISTITEM_TRACKNUMBER:
  case LISTITEM_ARTIST:
  case LISTITEM_ALBUM:
  case LISTITEM_YEAR:
  case LISTITEM_PREMIERED:
  case LISTITEM_GENRE:
  case LISTITEM_DIRECTOR:
  case LISTITEM_FILENAME:
  case LISTITEM_DATE:
  case LISTITEM_SIZE:
  case LISTITEM_RATING:
  case LISTITEM_PROGRAM_COUNT:
  case LISTITEM_DURATION:
  case LISTITEM_PLOT:
  case LISTITEM_PLOT_OUTLINE:
  case LISTITEM_EPISODE:
  case LISTITEM_SEASON:
  case LISTITEM_TVSHOW:
  case LISTITEM_COMMENT:
  case LISTITEM_PATH:
  case LISTITEM_PICTURE_PATH:
  case LISTITEM_PICTURE_DATETIME:
  case LISTITEM_PICTURE_RESOLUTION:
  case LISTITEM_STUDIO:
  case LISTITEM_MPAA:
  case LISTITEM_CAST:
  case LISTITEM_CAST_AND_ROLE:
    {
      CGUIWindow *pWindow;
      int iDialog = m_gWindowManager.GetTopMostModalDialogID();
      if (iDialog == WINDOW_VIDEO_INFO)
        pWindow = m_gWindowManager.GetWindow(iDialog);
      else
        pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
      if (pWindow && (iDialog == WINDOW_VIDEO_INFO || pWindow->IsMediaWindow()))
      {
        strLabel = GetItemLabel(pWindow->GetCurrentListItem(), info);
      }
    }
    break;
  }

  return strLabel;
}

// tries to get a integer value for use in progressbars/sliders and such
int CGUIInfoManager::GetInt(int info) const
{
  switch( info )
  {
    case PLAYER_VOLUME:
      return g_application.GetVolume();
    case PLAYER_PROGRESS:
    case PLAYER_SEEKBAR:
    case PLAYER_CACHING:
    case PLAYER_CHAPTER:
    case PLAYER_CHAPTERCOUNT:
      {
        if( g_application.IsPlaying() && g_application.m_pPlayer)
        {
          switch( info )
          {
          case PLAYER_PROGRESS:
            return (int)(g_application.GetPercentage());
          case PLAYER_SEEKBAR:
            {
              CGUIDialogSeekBar *seekBar = (CGUIDialogSeekBar*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR);
              return seekBar ? (int)seekBar->GetPercentage() : 0;
            }
          case PLAYER_CACHING:
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
        CGUIDialogProgress *bar = (CGUIDialogProgress *)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
        if (bar && bar->IsDialogRunning())
          return bar->GetPercentage();
      }
#ifdef HAS_XBOX_HARDWARE
    case SYSTEM_HDD_TEMPERATURE:
      return atoi(g_sysinfo.GetInfo(LCD_HDD_TEMPERATURE));
    case SYSTEM_CPU_TEMPERATURE:
      return atoi(CFanController::Instance()->GetCPUTemp().ToString());
    case SYSTEM_GPU_TEMPERATURE:
      return atoi(CFanController::Instance()->GetGPUTemp().ToString());
    case SYSTEM_FAN_SPEED:
      return CFanController::Instance()->GetFanSpeed() * 2;
#endif
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
      return 100 - ((int)(100.0f *g_application.m_idleThread.GetRelativeUsage()));
  }
  return 0;
}
// checks the condition and returns it as necessary.  Currently used
// for toggle button controls and visibility of images.
bool CGUIInfoManager::GetBool(int condition1, DWORD dwContextWindow)
{
  // check our cache
  bool result;
  if (IsCached(condition1, dwContextWindow, result))
    return result;

  if(condition1 >= COMBINED_VALUES_START && (condition1 - COMBINED_VALUES_START) < (int)(m_CombinedValues.size()) )
  {
    const CCombinedValue &comb = m_CombinedValues[condition1 - COMBINED_VALUES_START];
    bool result;
    if (!EvaluateBooleanExpression(comb, result, dwContextWindow))
      result = false;
    CacheBool(condition1, dwContextWindow, result);
    return result;
  }

  int condition = abs(condition1);
  bool bReturn = false;

  // Ethernet Link state checking
  // Will check if the Xbox has a Ethernet Link connection! [Cable in!]
  // This can used for the skinner to switch off Network or Inter required functions
  if ( condition == SYSTEM_ALWAYS_TRUE)
    bReturn = true;
  else if (condition == SYSTEM_ALWAYS_FALSE)
    bReturn = false;
  else if (condition == SYSTEM_ETHERNET_LINK_ACTIVE)
#ifdef HAS_XBOX_NETWORK
    bReturn = (XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE);
#else
    bReturn = true;
#endif
  else if (condition > SYSTEM_IDLE_TIME_START && condition <= SYSTEM_IDLE_TIME_FINISH)
    bReturn = (g_application.GlobalIdleTime() >= condition - SYSTEM_IDLE_TIME_START);
  else if (condition == WINDOW_IS_MEDIA)
  {
    CGUIWindow *pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
    bReturn = (pWindow && pWindow->IsMediaWindow());
  }
  else if (condition == PLAYER_MUTED)
    bReturn = g_stSettings.m_bMute;
  else if (condition == SYSTEM_KAI_CONNECTED)
    bReturn = g_network.IsAvailable(false) && g_guiSettings.GetBool("xlinkkai.enabled") && CKaiClient::GetInstance()->IsEngineConnected();
  else if (condition == SYSTEM_KAI_ENABLED)
    bReturn = g_network.IsAvailable(false) && g_guiSettings.GetBool("xlinkkai.enabled");
  else if (condition == SYSTEM_MEDIA_DVD)
  {
    // we must: 1.  Check tray state.
    //          2.  Check that we actually have a disc in the drive (detection
    //              of disk type takes a while from a separate thread).

    int iTrayState = CIoSupport::GetTrayState();
    if ( iTrayState == DRIVE_CLOSED_MEDIA_PRESENT || iTrayState == TRAY_CLOSED_MEDIA_PRESENT )
      bReturn = CDetectDVDMedia::IsDiscInDrive();
    else 
      bReturn = false;
  }
  else if (condition == SYSTEM_HAS_DRIVE_F)
    bReturn = CIoSupport::DriveExists('F');
  else if (condition == SYSTEM_HAS_DRIVE_G)
    bReturn = CIoSupport::DriveExists('G');
  else if (condition == SYSTEM_DVDREADY)
    bReturn = CDetectDVDMedia::DriveReady() != DRIVE_NOT_READY;
  else if (condition == SYSTEM_TRAYOPEN)
    bReturn = CDetectDVDMedia::DriveReady() == DRIVE_OPEN;
  else if (condition == PLAYER_SHOWINFO)
    bReturn = m_playerShowInfo;
  else if (condition == PLAYER_SHOWCODEC)
    bReturn = m_playerShowCodec;
  else if (condition >= SKIN_HAS_THEME_START && condition <= SKIN_HAS_THEME_END)
  { // Note that the code used here could probably be extended to general
    // settings conditions (parameter would need to store both the setting name and
    // the and the comparison string)
    CStdString theme = g_guiSettings.GetString("lookandfeel.skintheme");
    theme.ToLower();
    CUtil::RemoveExtension(theme);
    bReturn = theme.Equals(m_stringParameters[condition - SKIN_HAS_THEME_START]);
  }
  else if (condition >= MULTI_INFO_START && condition <= MULTI_INFO_END)
  {
    // cache return value
    bool result = GetMultiInfoBool(m_multiInfo[condition - MULTI_INFO_START], dwContextWindow);
    CacheBool(condition1, dwContextWindow, result);
    return result;
  }
  else if (condition == SYSTEM_HASLOCKS)  
    bReturn = g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE;
  else if (condition == SYSTEM_ISMASTER)
    bReturn = g_settings.m_vecProfiles[0].getLockMode() != LOCK_MODE_EVERYONE && g_passwordManager.bMasterUser;
  else if (condition == SYSTEM_LOGGEDON)
    bReturn = !(m_gWindowManager.GetActiveWindow() == WINDOW_LOGIN_SCREEN);
  else if (condition == SYSTEM_HAS_LOGINSCREEN)
    bReturn = g_settings.bUseLoginScreen;
  else if (condition == WEATHER_IS_FETCHED)
    bReturn = g_weatherManager.IsFetched();
  else if (condition == SYSTEM_INTERNET_STATE)
  {
    g_sysinfo.GetInfo(condition);
    bReturn = g_sysinfo.m_bInternetState;
  }
  else if (condition == SKIN_HAS_VIDEO_OVERLAY)
  {
    bReturn = !g_application.IsInScreenSaver() && m_gWindowManager.IsOverlayAllowed() &&
              g_application.IsPlayingVideo() && m_gWindowManager.GetActiveWindow() != WINDOW_FULLSCREEN_VIDEO;
  }
  else if (condition == SKIN_HAS_MUSIC_OVERLAY)
  {
    bReturn = !g_application.IsInScreenSaver() && m_gWindowManager.IsOverlayAllowed() &&
              g_application.IsPlayingAudio();
  }
  else if (condition == CONTAINER_HAS_THUMB)
  {
    CGUIWindow *pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
    if (pWindow && pWindow->IsMediaWindow())
      bReturn = ((CGUIMediaWindow*)pWindow)->CurrentDirectory().HasThumbnail();
  }
  else if (condition == CONTAINER_ON_NEXT || condition == CONTAINER_ON_PREVIOUS)
  {
    // no parameters, so we assume it's just requested for a media window.  It therefore
    // can only happen if the list has focus.
    CGUIWindow *pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
    if (pWindow && pWindow->IsMediaWindow())
    {
      map<int,int>::const_iterator it = m_containerMoves.find(pWindow->GetViewContainerID());
      if (it != m_containerMoves.end())
        bReturn = condition == CONTAINER_ON_NEXT ? it->second > 0 : it->second < 0;
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
        CGUIDialogSeekBar *seekBar = (CGUIDialogSeekBar*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR);
        bReturn = seekBar ? seekBar->IsDialogRunning() : false;
      }
    break;
    case PLAYER_SEEKING:
      bReturn = m_playerSeeking;
    break;
    case PLAYER_SHOWTIME:
      bReturn = m_playerShowTime;
    break;
    case MUSICPM_ENABLED:
      bReturn = g_partyModeManager.IsEnabled();
    break;
    case AUDIOSCROBBLER_ENABLED:
      bReturn = g_guiSettings.GetBool("lastfm.enable");
    break;
    case VIDEOPLAYER_USING_OVERLAYS:
      bReturn = (g_guiSettings.GetInt("videoplayer.rendermethod") == RENDER_OVERLAYS);
    break;
    case VIDEOPLAYER_ISFULLSCREEN:
      bReturn = m_gWindowManager.GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO;
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
    case VISUALISATION_LOCKED:
      {
        CGUIMessage msg(GUI_MSG_GET_VISUALISATION, 0, 0);
        g_graphicsContext.SendMessage(msg);
        if (msg.GetLPVOID())
        {
          CVisualisation *pVis = (CVisualisation *)msg.GetLPVOID();
          bReturn = pVis->IsLocked();
        }
      }
    break;
    case VISUALISATION_ENABLED:
      bReturn = g_guiSettings.GetString("mymusic.visualisation") != "None";
    break;
    default: // default, use integer value different from 0 as true
      bReturn = GetInt(condition) != 0;
    }
  }
  // cache return value
  if (condition1 < 0) bReturn = !bReturn;
  CacheBool(condition1, dwContextWindow, bReturn);
  return bReturn;
}

/// \brief Examines the multi information sent and returns true or false accordingly.
bool CGUIInfoManager::GetMultiInfoBool(const GUIInfo &info, DWORD dwContextWindow)
{
  bool bReturn = false;
  int condition = abs(info.m_info);
  switch (condition)
  {
    case SKIN_BOOL:
      {
        bReturn = g_settings.GetSkinBool(info.m_data1);
      }
      break;
    case SKIN_STRING:
      {
        if (info.m_data2)
          bReturn = g_settings.GetSkinString(info.m_data1).Equals(m_stringParameters[info.m_data2]);
        else
          bReturn = !g_settings.GetSkinString(info.m_data1).IsEmpty();
      }
      break;
    case CONTROL_GROUP_HAS_FOCUS:
      {
        CGUIWindow *pWindow = m_gWindowManager.GetWindow(dwContextWindow);
        if (!pWindow) pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
        if (pWindow) 
          bReturn = pWindow->ControlGroupHasFocus(info.m_data1, info.m_data2);
      }
      break;
    case CONTROL_IS_VISIBLE:
      {
        CGUIWindow *pWindow = m_gWindowManager.GetWindow(dwContextWindow);
        if (!pWindow) pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
        if (pWindow)
        {
          // Note: This'll only work for unique id's
          const CGUIControl *control = pWindow->GetControl(info.m_data1);
          if (control)
            bReturn = control->IsVisible();
        }
      }
      break;
    case CONTROL_IS_ENABLED:
      {
        CGUIWindow *pWindow = m_gWindowManager.GetWindow(dwContextWindow);
        if (!pWindow) pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
        if (pWindow)
        {
          // Note: This'll only work for unique id's
          const CGUIControl *control = pWindow->GetControl(info.m_data1);
          if (control)
            bReturn = !control->IsDisabled();
        }
      }
      break;
    case CONTROL_HAS_FOCUS:
      {
        CGUIWindow *pWindow = m_gWindowManager.GetWindow(dwContextWindow);
        if (!pWindow) pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
        if (pWindow)
          bReturn = (pWindow->GetFocusedControlID() == info.m_data1);
      }
      break;
    case BUTTON_SCROLLER_HAS_ICON:
      {
        CGUIWindow *pWindow = m_gWindowManager.GetWindow(dwContextWindow);
        if( !pWindow ) pWindow = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
        if (pWindow)
        {
          CGUIControl *pControl = pWindow->GetFocusedControl();
          if (pControl && pControl->GetControlType() == CGUIControl::GUICONTROL_BUTTONBAR)
            bReturn = ((CGUIButtonScroller *)pControl)->GetActiveButtonID() == info.m_data1;
        }
      }
      break;
    case WINDOW_NEXT:
      if (info.m_data1)
        bReturn = (info.m_data1 == m_nextWindowID);
      else
      {
        CGUIWindow *window = m_gWindowManager.GetWindow(m_nextWindowID);
        if (window && CUtil::GetFileName(window->GetXMLFile()).Equals(m_stringParameters[info.m_data2]))
          bReturn = true;
      }
      break;
    case WINDOW_PREVIOUS:
      if (info.m_data1)
        bReturn = (info.m_data1 == m_prevWindowID);
      else
      {
        CGUIWindow *window = m_gWindowManager.GetWindow(m_prevWindowID);
        if (window && CUtil::GetFileName(window->GetXMLFile()).Equals(m_stringParameters[info.m_data2]))
          bReturn = true;
      }
      break;
    case WINDOW_IS_VISIBLE:
      if (info.m_data1)
        bReturn = m_gWindowManager.IsWindowVisible(info.m_data1);
      else
        bReturn = m_gWindowManager.IsWindowVisible(m_stringParameters[info.m_data2]);
      break;
    case WINDOW_IS_TOPMOST:
      if (info.m_data1)
        bReturn = m_gWindowManager.IsWindowTopMost(info.m_data1);
      else
        bReturn = m_gWindowManager.IsWindowTopMost(m_stringParameters[info.m_data2]);
      break;
    case WINDOW_IS_ACTIVE:
      if (info.m_data1)
        bReturn = m_gWindowManager.IsWindowActive(info.m_data1);
      else
        bReturn = m_gWindowManager.IsWindowActive(m_stringParameters[info.m_data2]);
      break;
    case SYSTEM_HAS_ALARM:
      bReturn = g_alarmClock.hasAlarm(m_stringParameters[info.m_data1]);
      break;
    case CONTAINER_CONTENT:
      bReturn = m_stringParameters[info.m_data1].Equals(m_content);
      break;
    case CONTAINER_ON_NEXT:
    case CONTAINER_ON_PREVIOUS:
      {
        map<int,int>::const_iterator it = m_containerMoves.find(info.m_data1);
        if (it != m_containerMoves.end())
          bReturn = condition == CONTAINER_ON_NEXT ? it->second > 0 : it->second < 0;
      }
      break;
    case CONTAINER_HAS_FOCUS:
      { // grab our container
        CGUIWindow *window = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
        if (window)
        {
          const CGUIControl *control = window->GetControl(info.m_data1);
          if (control && control->IsContainer())
          {
            CFileItem *item = (CFileItem *)((CGUIBaseContainer *)control)->GetListItem(0);
            if (item && item->m_iprogramCount == info.m_data2)  // programcount used to store item id
              return true;
          }
        }
        break;
      }
    case LISTITEM_ISSELECTED:
    case LISTITEM_ISPLAYING:
      {
        CGUIWindow *window = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
        if (window)
        {
          CFileItem *item=NULL;
          if (!info.m_data1)
          { // assumes a media window
            if (window->IsMediaWindow())
              item = window->GetCurrentListItem(info.m_data2);
          }
          else
          {
            const CGUIControl *control = window->GetControl(info.m_data1);
            if (control && control->IsContainer())
              item = (CFileItem *)((CGUIBaseContainer *)control)->GetListItem(info.m_data2);
          }
          if (item)
            bReturn = GetItemBool(item, info.m_info, dwContextWindow);
        }
      }
      break;
    case VIDEOPLAYER_CONTENT:
      {
        CStdString strContent="movies";
        if (!m_currentFile.HasVideoInfoTag())
          strContent = "files";
        if (m_currentFile.HasVideoInfoTag() && m_currentFile.GetVideoInfoTag()->m_iSeason > -1) // episode
          strContent = "episodes";
        if (m_currentFile.HasVideoInfoTag() && m_currentFile.GetVideoInfoTag()->m_artist.size() > 0)
          strContent = "musicvideos";
        bReturn = m_stringParameters[info.m_data1].Equals(strContent);
      }
      break;
    case CONTAINER_SORT_METHOD:
    {
      CGUIWindow *window = m_gWindowManager.GetWindow(dwContextWindow);
      if( !window ) window = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
      if (window && window->IsMediaWindow())
      {
        const CFileItemList &item = ((CGUIMediaWindow*)window)->CurrentDirectory();
        SORT_METHOD method = item.GetSortMethod();
        bReturn = (method == info.m_data1);
      }
      break;
    }
  }
  return (info.m_info < 0) ? !bReturn : bReturn;
}

/// \brief Examines the multi information sent and returns the string as appropriate
CStdString CGUIInfoManager::GetMultiInfoLabel(const GUIInfo &info, DWORD contextWindow) const
{
  if (info.m_info == SKIN_STRING)
  {
    return g_settings.GetSkinString(info.m_data1);
  }
  else if (info.m_info == SKIN_BOOL)
  {
    bool bInfo = g_settings.GetSkinBool(info.m_data1);
    if (bInfo)
      return g_localizeStrings.Get(20122);
  }
  if (info.m_info >= LISTITEM_START && info.m_info <= LISTITEM_END)
  {
    CGUIWindow *window = m_gWindowManager.GetWindow(contextWindow);
    if (!window)
      window = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
    CFileItem *item = NULL;
    if (window)
    {
      if (!info.m_data1)
      { // no id given, must be a media window
        if (window->IsMediaWindow())
          item = window->GetCurrentListItem(info.m_data2);
      }
      else
      {
        const CGUIControl *control = window->GetControl(info.m_data1);
        if (control && control->IsContainer())
          item = (CFileItem *)((CGUIBaseContainer *)control)->GetListItem(info.m_data2);
      }
    }
    if (item)
      return GetItemImage(item, info.m_info); // Image prioritizes images over labels (in the case of music item ratings for instance)
  }
  else if (info.m_info == PLAYER_TIME)
  {
    return GetCurrentPlayTime((TIME_FORMAT)info.m_data1);
  }
  else if (info.m_info == PLAYER_TIME_REMAINING)
  {
    return GetCurrentPlayTimeRemaining((TIME_FORMAT)info.m_data1);
  }
  else if (info.m_info == PLAYER_FINISH_TIME)
  {
    CDateTime time = CDateTime::GetCurrentDateTime();
    time += CDateTimeSpan(0, 0, 0, GetPlayTimeRemaining());
    return LocalizeTime(time, (TIME_FORMAT)info.m_data1);
  }
  else if (info.m_info == PLAYER_TIME_SPEED)
  {
    CStdString strTime;
    if (g_application.GetPlaySpeed() != 1)
      strTime.Format("%s (%ix)", GetCurrentPlayTime((TIME_FORMAT)info.m_data1).c_str(), g_application.GetPlaySpeed());
    else
      strTime = GetCurrentPlayTime();
    return strTime;
  }
  else if (info.m_info == PLAYER_DURATION)
  {
    return GetDuration((TIME_FORMAT)info.m_data1);
  }
  else if (info.m_info == PLAYER_SEEKTIME)
  {
    CGUIDialogSeekBar *seekBar = (CGUIDialogSeekBar*)m_gWindowManager.GetWindow(WINDOW_DIALOG_SEEK_BAR);
    if (seekBar)
      return seekBar->GetSeekTimeLabel((TIME_FORMAT)info.m_data1);
  }
  else if (info.m_info == SYSTEM_TIME)
  {
    return GetTime((TIME_FORMAT)info.m_data1);
  }
  return StringUtils::EmptyString;
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
CStdString CGUIInfoManager::GetImage(int info, DWORD contextWindow)
{
  if (info >= MULTI_INFO_START && info <= MULTI_INFO_END)
  {
    return GetMultiInfoLabel(m_multiInfo[info - MULTI_INFO_START]);
  }
  else if (info == WEATHER_CONDITIONS)
    return g_weatherManager.GetInfo(WEATHER_IMAGE_CURRENT_ICON);
  else if (info == SYSTEM_PROFILETHUMB)
  {
    CStdString thumb = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getThumb();
    if (thumb.IsEmpty())
      thumb = "unknown-user.png";
    return thumb;
  }
  else if (info == MUSICPLAYER_COVER)
  {
    if (!g_application.IsPlayingAudio()) return "";
    return m_currentFile.HasThumbnail() ? m_currentFile.GetThumbnailImage() : "defaultAlbumCover.png";
  }
  else if (info == MUSICPLAYER_RATING)
  {
    if (!g_application.IsPlayingAudio()) return "";
    return GetItemImage(&m_currentFile, LISTITEM_RATING);
  }
  else if (info == VIDEOPLAYER_COVER)
  {
    if (!g_application.IsPlayingVideo()) return "";
    if(m_currentMovieThumb.IsEmpty())
      return m_currentFile.HasThumbnail() ? m_currentFile.GetThumbnailImage() : "defaultVideoCover.png";
    else return m_currentMovieThumb;
  }
  else if (info == LISTITEM_THUMB || info == LISTITEM_ICON || info == LISTITEM_ACTUAL_ICON ||
          info == LISTITEM_OVERLAY || info == CONTAINER_FOLDERTHUMB || info == LISTITEM_RATING)
  {
    CGUIWindow *window = m_gWindowManager.GetWindow(contextWindow);
    if (!window || !window->IsMediaWindow())
      window = m_gWindowManager.GetWindow(m_gWindowManager.GetActiveWindow());
    if (window && window->IsMediaWindow())
    {
      CFileItem* item;

      if (info == CONTAINER_FOLDERTHUMB)
      {
        item = &const_cast<CFileItemList&>(((CGUIMediaWindow*)window)->CurrentDirectory());
        info = LISTITEM_THUMB;
      }
      else
        item = window->GetCurrentListItem();
      if (item)
        return GetItemImage(item, info);
    }
  }
  return GetLabel(info);
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
    return time.GetAsLocalizedTime(use12hourclock ? "h" : "H", false);
  case TIME_FORMAT_HH_MM:
    return time.GetAsLocalizedTime(use12hourclock ? "h:mm" : "H:mm", false);
  case TIME_FORMAT_HH_MM_SS:
    return time.GetAsLocalizedTime("", true);
  }
  return time.GetAsLocalizedTime("", false);
}

CStdString CGUIInfoManager::GetDuration(TIME_FORMAT format) const
{
  CStdString strDuration;
  if (g_application.IsPlayingAudio() && m_currentFile.HasMusicInfoTag())
  {
    const CMusicInfoTag& tag = *m_currentFile.GetMusicInfoTag();
    if (tag.GetDuration() > 0)
      StringUtils::SecondsToTimeString(tag.GetDuration(), strDuration, format);
  }
  if (g_application.IsPlayingVideo() && !m_currentMovieDuration.IsEmpty())
    return m_currentMovieDuration;  // for tuxbox
  if (strDuration.IsEmpty())
  {
    unsigned int iTotal = (unsigned int)g_application.GetTotalTime();
    if (iTotal > 0)
      StringUtils::SecondsToTimeString(iTotal, strDuration, format);
  }
  return strDuration;
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

CStdString CGUIInfoManager::GetPlaylistLabel(int item)
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
  if (!g_application.IsPlayingAudio() || !m_currentFile.HasMusicInfoTag()) return "";
  CMusicInfoTag& tag = *m_currentFile.GetMusicInfoTag();
  switch (item)
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
  case MUSICPLAYER_YEAR: 
    if (tag.GetYear().size()) { return tag.GetYear(); }
    break;
  case MUSICPLAYER_GENRE: 
    if (tag.GetGenre().size()) { return tag.GetGenre(); }
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
      float fTimeSpan = (float)(timeGetTime() - m_lastMusicBitrateTime);
      if (fTimeSpan >= 500.0f)
      {
        m_MusicBitrate = g_application.m_pPlayer->GetBitrate();
        m_lastMusicBitrateTime = timeGetTime();
      }
      CStdString strBitrate = "";
      if (m_MusicBitrate > 0)
        strBitrate.Format("%i", m_MusicBitrate);
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
      strCodec.Format("%s", g_application.m_pPlayer->GetCodecName().c_str());
      return strCodec;
    }
    break;
  case MUSICPLAYER_DISC_NUMBER:
    {
      CStdString strDisc;
      if (tag.Loaded() && tag.GetDiscNumber() > 0)
      {
        strDisc.Format("%02i", tag.GetDiscNumber());
        return strDisc;
      } 
    }
    break;
  case MUSICPLAYER_RATING:
    return GetItemLabel(&m_currentFile, LISTITEM_RATING);
  case MUSICPLAYER_COMMENT:
    return GetItemLabel(&m_currentFile, LISTITEM_COMMENT);
  }
  return "";
}

CStdString CGUIInfoManager::GetVideoLabel(int item)
{
  if (!g_application.IsPlayingVideo()) 
    return "";
  
  switch (item)
  {
  case VIDEOPLAYER_TITLE:
    return m_currentFile.GetVideoInfoTag()->m_strTitle;
    break;
  case VIDEOPLAYER_ORIGINALTITLE:
    return m_currentFile.GetVideoInfoTag()->m_strOriginalTitle;
    break;
  case VIDEOPLAYER_GENRE:
    return m_currentFile.GetVideoInfoTag()->m_strGenre;
    break;
  case VIDEOPLAYER_DIRECTOR:
    return m_currentFile.GetVideoInfoTag()->m_strDirector;
    break;
  case VIDEOPLAYER_RATING:
    {
      CStdString strYear;
      strYear.Format("%2.2f", m_currentFile.GetVideoInfoTag()->m_fRating);
      return strYear;
    }
  case VIDEOPLAYER_YEAR:
    {
      CStdString strYear;
      if (m_currentFile.GetVideoInfoTag()->m_iYear > 0)
        strYear.Format("%i", m_currentFile.GetVideoInfoTag()->m_iYear);
      return strYear;
    }
    break;
  case VIDEOPLAYER_PREMIERED:
    {
      CStdString strYear;
      if (!m_currentFile.GetVideoInfoTag()->m_strPremiered.IsEmpty())
        strYear = m_currentFile.GetVideoInfoTag()->m_strPremiered;
      else if (!m_currentFile.GetVideoInfoTag()->m_strFirstAired.IsEmpty())
        strYear = m_currentFile.GetVideoInfoTag()->m_strFirstAired;
      return strYear;
    }
    break;
  case VIDEOPLAYER_PLOT:
    return m_currentFile.GetVideoInfoTag()->m_strPlot;
  case VIDEOPLAYER_PLOT_OUTLINE:
    return m_currentFile.GetVideoInfoTag()->m_strPlotOutline;
  case VIDEOPLAYER_EPISODE:
    if (m_currentFile.GetVideoInfoTag()->m_iEpisode > 0)
    {
      CStdString strYear;
      if (m_currentFile.GetVideoInfoTag()->m_iSpecialSortEpisode > 0)
        strYear.Format("S%i", m_currentFile.GetVideoInfoTag()->m_iEpisode);
      else
        strYear.Format("%i", m_currentFile.GetVideoInfoTag()->m_iEpisode);
      return strYear;
    }
    break;
  case VIDEOPLAYER_SEASON:
    if (m_currentFile.GetVideoInfoTag()->m_iSeason > -1)
    {
      CStdString strYear;
      if (m_currentFile.GetVideoInfoTag()->m_iSpecialSortSeason > 0)
        strYear.Format("%i", m_currentFile.GetVideoInfoTag()->m_iSpecialSortSeason);
      else
        strYear.Format("%i", m_currentFile.GetVideoInfoTag()->m_iSeason);
      return strYear;
    }
    break;
  case VIDEOPLAYER_TVSHOW:
    return m_currentFile.GetVideoInfoTag()->m_strShowTitle;

  case VIDEOPLAYER_PLAYLISTLEN:
    {
      if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
        return GetPlaylistLabel(PLAYLIST_LENGTH);
    }
    break;
  case VIDEOPLAYER_PLAYLISTPOS:
    {
      if (g_playlistPlayer.GetCurrentPlaylist() == PLAYLIST_VIDEO)
        return GetPlaylistLabel(PLAYLIST_POSITION);
    }
    break;
  case VIDEOPLAYER_STUDIO:
    return m_currentFile.GetVideoInfoTag()->m_strPlot;
  case VIDEOPLAYER_MPAA:
    return m_currentFile.GetVideoInfoTag()->m_strMPAARating;
  case VIDEOPLAYER_CAST:
    return m_currentFile.GetVideoInfoTag()->GetCast();
  case VIDEOPLAYER_CAST_AND_ROLE:
    return m_currentFile.GetVideoInfoTag()->GetCast(true);
  case VIDEOPLAYER_ARTIST:
    if (m_currentFile.GetVideoInfoTag()->m_artist.size() > 0)
      return m_currentFile.GetVideoInfoTag()->GetArtist();
    break;
  case VIDEOPLAYER_ALBUM:
    return m_currentFile.GetVideoInfoTag()->m_strAlbum;
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
  CStdString strTime;
  if (g_application.IsPlayingAudio())
    StringUtils::SecondsToTimeString((int)(GetPlayTime()/1000), strTime, format);
  else if (g_application.IsPlayingVideo())
    StringUtils::SecondsToTimeString((int)(GetPlayTime()/1000), strTime, format);
  return strTime;
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
  CStdString strTime;
  int timeRemaining = GetPlayTimeRemaining();
  if (timeRemaining)
  {
    if (g_application.IsPlayingAudio())
      StringUtils::SecondsToTimeString(timeRemaining, strTime, format);
    else if (g_application.IsPlayingVideo())
      StringUtils::SecondsToTimeString(timeRemaining, strTime, format);
  }
  return strTime;
}

void CGUIInfoManager::ResetCurrentItem()
{ 
  m_currentFile.Reset();
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
    m_currentFile.SetThumbnailImage(thumbFileName);
  else
  {
    m_currentFile.SetThumbnailImage("");
    m_currentFile.FillInDefaultIcon();
  }
}

void CGUIInfoManager::SetCurrentSong(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentSong(%s)",item.m_strPath.c_str());
  m_currentFile = item;

  // Get a reference to the item's tag
  CMusicInfoTag& tag = *m_currentFile.GetMusicInfoTag();
  // check if we don't have the tag already loaded
  if (!tag.Loaded())
  {
    // we have a audio file.
    // Look if we have this file in database...
    bool bFound = false;
    CMusicDatabase musicdatabase;
    if (musicdatabase.Open())
    {
      CSong song;
      bFound = musicdatabase.GetSongByFileName(m_currentFile.m_strPath, song);
      m_currentFile.GetMusicInfoTag()->SetSong(song);
      musicdatabase.Close();
    }

    if (!bFound)
    {
      // always get id3 info for the overlay
      CMusicInfoTagLoaderFactory factory;
      auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(m_currentFile.m_strPath));
      // Do we have a tag loader for this file type?
      if (NULL != pLoader.get())
        pLoader->Load(m_currentFile.m_strPath, tag);
    }
  }

  // If we have tag information, ...
  if (tag.Loaded())
  {
    if (!tag.GetTitle().size())
    {
      // No title in tag, show filename only
#ifdef HAS_FILESYSTEM      
      CSndtrkDirectory dir;
      char NameOfSong[64];
      if (dir.FindTrackName(m_currentFile.m_strPath, NameOfSong))
        tag.SetTitle(NameOfSong);
      else
#endif
        tag.SetTitle( CUtil::GetTitleFromPath(m_currentFile.m_strPath) );
    }
  } // if (tag.Loaded())
  else
  {
    // If we have a cdda track without cddb information,...
    if (m_currentFile.IsCDDA())
    {
      // we have the tracknumber...
      int iTrack = tag.GetTrackNumber();
      if (iTrack >= 1)
      {
        CStdString strText = g_localizeStrings.Get(435); // "Track"
        if (strText.GetAt(strText.size() - 1) != ' ')
          strText += " ";
        CStdString strTrack;
        strTrack.Format(strText + "%i", iTrack);
        tag.SetTitle(strTrack);
        tag.SetLoaded(true);
      }
    } // if (!tag.Loaded() && url.GetProtocol()=="cdda" )
    else
    { // at worse, set our title as the filename
      tag.SetTitle( CUtil::GetTitleFromPath(m_currentFile.m_strPath) );
    } // we now have at least the title
    tag.SetLoaded(true);
  }

  // find a thumb for this file.
  if (m_currentFile.IsInternetStream())
  {
    if (!g_application.m_strPlayListFile.IsEmpty())
    {
      CLog::Log(LOGDEBUG,"Streaming media detected... using %s to find a thumb", g_application.m_strPlayListFile.c_str());
      CFileItem streamingItem(g_application.m_strPlayListFile,false);
      streamingItem.SetMusicThumb();
      CStdString strThumb = streamingItem.GetThumbnailImage();
      if (CFile::Exists(strThumb))
        m_currentFile.SetThumbnailImage(strThumb);
    }
  }
  else
    m_currentFile.SetMusicThumb();
  m_currentFile.FillInDefaultIcon();
}

void CGUIInfoManager::SetCurrentMovie(CFileItem &item)
{
  CLog::Log(LOGDEBUG,"CGUIInfoManager::SetCurrentMovie(%s)",item.m_strPath.c_str());
  m_currentFile = item;
  
  if (!m_currentFile.HasVideoInfoTag())
  { // attempt to get some information
    CVideoDatabase dbs;
    dbs.Open();
    if (dbs.HasMovieInfo(item.m_strPath))
    {
      dbs.GetMovieInfo(item.m_strPath, *m_currentFile.GetVideoInfoTag());
      CLog::Log(LOGDEBUG,__FUNCTION__", got movie info!");
      CLog::Log(LOGDEBUG,"  Title = %s", m_currentFile.GetVideoInfoTag()->m_strTitle.c_str());
    }
    else if (dbs.HasEpisodeInfo(item.m_strPath))
    {
      dbs.GetEpisodeInfo(item.m_strPath, *m_currentFile.GetVideoInfoTag());
      CLog::Log(LOGDEBUG,__FUNCTION__", got episode info!");
      CLog::Log(LOGDEBUG,"  Title = %s", m_currentFile.GetVideoInfoTag()->m_strTitle.c_str());
    }
    dbs.Close();

    if (m_currentFile.GetVideoInfoTag()->m_strTitle.IsEmpty())
    { // at least fill in the filename
      if (!item.GetLabel().IsEmpty())
        m_currentFile.GetVideoInfoTag()->m_strTitle = item.GetLabel();
      else
        m_currentFile.GetVideoInfoTag()->m_strTitle = CUtil::GetTitleFromPath(item.m_strPath);
    }
  }
  // Find a thumb for this file.
  item.SetVideoThumb();

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
#ifdef HAS_XBOX_HARDWARE
  if (timeGetTime() - m_lastSysHeatInfoTime >= 1000)
  { // update our variables
    m_lastSysHeatInfoTime = timeGetTime();
    m_fanSpeed = CFanController::Instance()->GetFanSpeed();
    m_gpuTemp = CFanController::Instance()->GetGPUTemp();
    m_cpuTemp = CFanController::Instance()->GetCPUTemp();
  }

#endif

  CStdString text;
  switch(info)
  {
    case SYSTEM_CPU_TEMPERATURE:
      text.Format("%s %s", g_localizeStrings.Get(140).c_str(), m_cpuTemp.ToString());
      break;
    case SYSTEM_GPU_TEMPERATURE:
      text.Format("%s %s", g_localizeStrings.Get(141).c_str(), m_gpuTemp.ToString());
      break;
    case SYSTEM_FAN_SPEED:
      text.Format("%s: %i%%", g_localizeStrings.Get(13300).c_str(), m_fanSpeed * 2);
      break;
    case LCD_CPU_TEMPERATURE:
      return m_cpuTemp.ToString();
      break;
    case LCD_GPU_TEMPERATURE:
      return m_gpuTemp.ToString();
      break;
    case LCD_FAN_SPEED:
      text.Format("%i%%", m_fanSpeed * 2);
      break;
    case SYSTEM_CPU_USAGE:
      text.Format("%s %2.0f%%", g_localizeStrings.Get(13271).c_str(), (1.0f - g_application.m_idleThread.GetRelativeUsage())*100);
      break;
  }
  return text;
}

CStdString CGUIInfoManager::GetVersion()
{
  CStdString tmp;
  tmp.Format("%s", VERSION_STRING);
  return tmp;
}

CStdString CGUIInfoManager::GetBuild()
{
  CStdString tmp;
  tmp.Format("%s", __DATE__);
  return tmp;
}

void CGUIInfoManager::SetDisplayAfterSeek(DWORD dwTimeOut)
{
  if(dwTimeOut>0)    
    m_AfterSeekTimeout = timeGetTime() +  dwTimeOut;
  else
    m_AfterSeekTimeout = 0;
}

bool CGUIInfoManager::GetDisplayAfterSeek() const
{
  return (timeGetTime() < m_AfterSeekTimeout);
}

CStdString CGUIInfoManager::GetAudioScrobblerLabel(int item)
{
  switch (item)
  {
  case AUDIOSCROBBLER_CONN_STATE:
    return CScrobbler::GetInstance()->GetConnectionState();
    break;
  case AUDIOSCROBBLER_SUBMIT_INT:
    return CScrobbler::GetInstance()->GetSubmitInterval();
    break;
  case AUDIOSCROBBLER_FILES_CACHED:
    return CScrobbler::GetInstance()->GetFilesCached();
    break;
  case AUDIOSCROBBLER_SUBMIT_STATE:
    return CScrobbler::GetInstance()->GetSubmitState();
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

bool CGUIInfoManager::EvaluateBooleanExpression(const CCombinedValue &expression, bool &result, DWORD dwContextWindow)
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
      save.push(GetBool(expr, dwContextWindow));
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
    comb.m_postfix.push_back(TranslateSingleString(operand));

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
  m_currentFile.Reset();
  m_CombinedValues.clear();
}

void CGUIInfoManager::UpdateFPS()
{
  m_frameCounter++;
  float fTimeSpan = (float)(timeGetTime() - m_lastFPSTime);
  if (fTimeSpan >= 1000.0f)
  {
    fTimeSpan /= 1000.0f;
    m_fps = m_frameCounter / fTimeSpan;
    m_lastFPSTime = timeGetTime();
    m_frameCounter = 0;
  }
}

int CGUIInfoManager::AddMultiInfo(const GUIInfo &info)
{
  // check to see if we have this info already
  for (unsigned int i = 0; i < m_multiInfo.size(); i++)
    if (m_multiInfo[i] == info)
      return (int)i + MULTI_INFO_START;
  // return the new offset
  m_multiInfo.push_back(info);
  return (int)m_multiInfo.size() + MULTI_INFO_START - 1;
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

CStdString CGUIInfoManager::GetItemMultiLabel(const CFileItem *item, const vector<CInfoPortion> &multiInfo)
{
  CStdString label;
  for (unsigned int i = 0; i < multiInfo.size(); i++)
  {
    const CInfoPortion &portion = multiInfo[i];
    if (portion.m_info)
    {
      CStdString infoLabel = g_infoManager.GetItemLabel(item, portion.m_info);
      if (!infoLabel.IsEmpty())
      {
        label += portion.m_prefix;
        label += infoLabel;
        label += portion.m_postfix;
      }
    }
    else
    { // no info, so just append the prefix
      label += portion.m_prefix;
    }
  }
  return label;
}

// This is required as in order for the "* All Albums" etc. items to sort
// correctly, they must have fake artist/album etc. information generated.
// This looks nasty if we attempt to render it to the GUI, thus this (further)
// workaround
const CStdString &CorrectAllItemsSortHack(const CStdString &item)
{
  if (item.size() == 1 && item[0] == 0x01 || item[1] == 0xff)
    return StringUtils::EmptyString;
  return item;
}

CStdString CGUIInfoManager::GetItemLabel(const CFileItem *item, int info) const
{
  if (!item) return "";
  switch (info)
  {
  case LISTITEM_LABEL:
    return item->GetLabel();
  case LISTITEM_LABEL2:
    return item->GetLabel2();
  case LISTITEM_TITLE:
    if (item->HasMusicInfoTag())
      return CorrectAllItemsSortHack(item->GetMusicInfoTag()->GetTitle());
    if (item->HasVideoInfoTag())
      return CorrectAllItemsSortHack(item->GetVideoInfoTag()->m_strTitle);
    break;
  case LISTITEM_TRACKNUMBER:
    {
      CStdString track;
      if (item->HasMusicInfoTag())
        track.Format("%i", item->GetMusicInfoTag()->GetTrackNumber());

      return track;
    }
  case LISTITEM_ARTIST:
    if (item->HasMusicInfoTag())
      return CorrectAllItemsSortHack(item->GetMusicInfoTag()->GetArtist());
    if (item->HasVideoInfoTag())
      if (item->GetVideoInfoTag()->m_artist.size() > 0)
        return item->GetVideoInfoTag()->GetArtist();
    break;
  case LISTITEM_DIRECTOR:
    if (item->HasVideoInfoTag())
      return CorrectAllItemsSortHack(item->GetVideoInfoTag()->m_strDirector);
  case LISTITEM_ALBUM:
    if (item->HasMusicInfoTag())
      return CorrectAllItemsSortHack(item->GetMusicInfoTag()->GetAlbum());
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strAlbum;
    break;
  case LISTITEM_YEAR:
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetYear();
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      if (item->GetVideoInfoTag()->m_iYear > 0)
        strResult.Format("%i",item->GetVideoInfoTag()->m_iYear);
      return strResult;
    }
    break;
  case LISTITEM_PREMIERED:
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      if (!item->GetVideoInfoTag()->m_strPremiered.IsEmpty())
        strResult = item->GetVideoInfoTag()->m_strPremiered;
      else if (!item->GetVideoInfoTag()->m_strFirstAired.IsEmpty())
        strResult = item->GetVideoInfoTag()->m_strFirstAired;
      return strResult;
    }
    break;
  case LISTITEM_GENRE:
    if (item->HasMusicInfoTag())
      return CorrectAllItemsSortHack(item->GetMusicInfoTag()->GetGenre());
    if (item->HasVideoInfoTag())
      return CorrectAllItemsSortHack(item->GetVideoInfoTag()->m_strGenre);
    break;
  case LISTITEM_FILENAME:
    return CUtil::GetFileName(item->m_strPath);
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
      if (item->HasMusicInfoTag() && item->GetMusicInfoTag()->GetRating() > '0')
      { // song rating.  Images will probably be better than numbers for this in the long run
        rating = item->GetMusicInfoTag()->GetRating();
      }
      else if (item->m_fRating > 0.f) // movie rating
        rating.Format("%2.2f", item->m_fRating);
      return rating;
    }
  case LISTITEM_PROGRAM_COUNT:
    {
      CStdString count;
      count.Format("%i", item->m_iprogramCount);
      return count;
    }
  case LISTITEM_DURATION:
    {
      CStdString duration;
      if (item->HasMusicInfoTag())
      {
        if (item->GetMusicInfoTag()->GetDuration() > 0)
          StringUtils::SecondsToTimeString(item->GetMusicInfoTag()->GetDuration(), duration);
      }
      if (item->HasVideoInfoTag())
      {
        duration = item->GetVideoInfoTag()->m_strRuntime;
      }

      return duration;
    }
  case LISTITEM_PLOT:
    if (item->HasVideoInfoTag())
    {
      if (!(!item->GetVideoInfoTag()->m_strShowTitle.IsEmpty() && item->GetVideoInfoTag()->m_iSeason == -1)) // dont apply to tvshows
        if (!item->GetVideoInfoTag()->m_bWatched && g_guiSettings.GetBool("myvideos.hideplots"))
          return g_localizeStrings.Get(20370);

      return item->GetVideoInfoTag()->m_strPlot;
    }
  case LISTITEM_PLOT_OUTLINE:
    if (item->HasVideoInfoTag())
      return item->GetVideoInfoTag()->m_strPlotOutline;
  case LISTITEM_EPISODE:
    if (item->HasVideoInfoTag())
    {
      CStdString strResult;
      if (item->GetVideoInfoTag()->m_iSpecialSortEpisode > 0)
        strResult.Format("S%d",item->GetVideoInfoTag()->m_iEpisode);
      else
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
      else
        strResult.Format("%d",item->GetVideoInfoTag()->m_iSeason);
      return strResult;
    }
    break;
  case LISTITEM_TVSHOW:
    if (item->HasVideoInfoTag())
    {
      return item->GetVideoInfoTag()->m_strShowTitle;
    }
    break;
  case LISTITEM_COMMENT:
    if (item->HasMusicInfoTag())
      return item->GetMusicInfoTag()->GetComment();
    break;
  case LISTITEM_ACTUAL_ICON:
    return item->GetIconImage();
    break;
  case LISTITEM_ICON:
    if (!item->HasThumbnail() && item->HasIcon())
    {
      CStdString strThumb = item->GetIconImage();
      if (info == LISTITEM_ICON)
        strThumb.Insert(strThumb.Find("."), "Big");
      return strThumb;
    }
    return item->GetThumbnailImage();
    break;
  case LISTITEM_OVERLAY:
    return item->GetOverlayImage();
    break;
  case LISTITEM_THUMB:
    return item->GetThumbnailImage();
    break;
  case LISTITEM_PATH:
    return item->m_strPath;
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
  }
  return "";
}

bool CGUIInfoManager::GetItemBool(const CFileItem *item, int info, DWORD contextWindow)
{
  bool ret = false;
  int absInfo = abs(info);
  switch (absInfo)
  {
  case LISTITEM_ISPLAYING:
    if (item && !m_currentFile.m_strPath.IsEmpty())
      ret = m_currentFile.IsSamePath(item);
    break;
  case LISTITEM_ISSELECTED:
    if (item)
      ret = item->IsSelected();
    break;
  default:
    return GetBool(info, contextWindow);
  }
  return (info < 0) ? !ret : ret;
}

CStdString CGUIInfoManager::GetMultiInfo(const vector<CInfoPortion> &multiInfo, DWORD contextWindow, bool preferImage)
{
  CStdString label;
  for (unsigned int i = 0; i < multiInfo.size(); i++)
  {
    const CInfoPortion &portion = multiInfo[i];
    if (portion.m_info)
    {
      CStdString infoLabel;
      if (preferImage)
        infoLabel = g_infoManager.GetImage(portion.m_info, contextWindow);
      if (infoLabel.IsEmpty())
        infoLabel = g_infoManager.GetLabel(portion.m_info);
      if (!infoLabel.IsEmpty())
      {
        label += portion.m_prefix;
        label += infoLabel;
        label += portion.m_postfix;
      }
    }
    else
    { // no info, so just append the prefix
      label += portion.m_prefix;
    }
  }
  return label;
}

void CGUIInfoManager::ParseLabel(const CStdString &strLabel, vector<CInfoPortion> &multiInfo)
{
  multiInfo.clear();
  CStdString work(strLabel);
  // Step 1: Replace all $LOCALIZE[number] with the real string
  int pos1 = work.Find("$LOCALIZE[");
  while (pos1 >= 0)
  {
    int pos2 = work.Find(']', pos1);
    if (pos2 > pos1)
    {
      CStdString left = work.Left(pos1);
      CStdString right = work.Mid(pos2 + 1);
      CStdString replace = g_localizeStrings.Get(atoi(work.Mid(pos1 + 10).c_str()));
      work = left + replace + right;
    }
    else
    {
      CLog::Log(LOGERROR, "Error parsing label - missing ']'");
      return;
    }
    pos1 = work.Find("$LOCALIZE[", pos1);
  }
  // Step 2: Find all $INFO[info,prefix,postfix] blocks
  pos1 = work.Find("$INFO[");
  while (pos1 >= 0)
  {
    // output the first block (contents before first $INFO)
    if (pos1 > 0)
      multiInfo.push_back(CInfoPortion(0, work.Left(pos1), ""));

    // ok, now decipher the $INFO block
    int pos2 = work.Find(']', pos1);
    if (pos2 > pos1)
    {
      // decipher the block
      CStdString block = work.Mid(pos1 + 6, pos2 - pos1 - 6);
      CStdStringArray params;
      StringUtils::SplitString(block, ",", params);
      int info = TranslateString(params[0]);
      CStdString prefix, postfix;
      if (params.size() > 1)
        prefix = params[1];
      if (params.size() > 2)
        postfix = params[2];
      multiInfo.push_back(CInfoPortion(info, prefix, postfix));
      // and delete it from our work string
      work = work.Mid(pos2 + 1);
    }
    else
    {
      CLog::Log(LOGERROR, "Error parsing label - missing ']'");
      return;
    }
    pos1 = work.Find("$INFO[");
  }
  // add any last block
  if (!work.IsEmpty())
    multiInfo.push_back(CInfoPortion(0, work, ""));
}

void CGUIInfoManager::ResetCache()
{
  CSingleLock lock(m_critInfo);
  m_boolCache.clear();
  // reset any animation triggers as well
  m_containerMoves.clear();
}

inline void CGUIInfoManager::CacheBool(int condition, DWORD contextWindow, bool result)
{
  // windows have id's up to 13100 or thereabouts (ie 2^14 needed)
  // conditionals have id's up to 100000 or thereabouts (ie 2^18 needed)
  CSingleLock lock(m_critInfo);
  int hash = ((contextWindow & 0x3fff) << 18) | (condition & 0x3ffff);
  m_boolCache.insert(pair<int, bool>(hash, result));
}

bool CGUIInfoManager::IsCached(int condition, DWORD contextWindow, bool &result) const
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

  return false;
}

CStdString CGUIInfoManager::GetItemImage(const CFileItem *item, int info) const
{
  if (info == LISTITEM_RATING)
  {
    if (item->HasMusicInfoTag())
    { // song rating.
      CStdString rating;
      rating.Format("songrating%c.png", item->GetMusicInfoTag()->GetRating());
      return rating;
    }
    return "";
  }
  else
    return GetItemLabel(item, info);
}

// Called from tuxbox service thread to update current status
void CGUIInfoManager::UpdateFromTuxBox()
{ 
  if(g_tuxbox.vVideoSubChannel.mode)
    m_currentFile.GetVideoInfoTag()->m_strTitle = g_tuxbox.vVideoSubChannel.current_name;

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
    m_currentFile.GetVideoInfoTag()->m_strGenre.Format("%s %s  -  (%s: %s)",
      g_localizeStrings.Get(143),
      g_tuxbox.sCurSrvData.current_event_description,
      g_localizeStrings.Get(209),
      g_tuxbox.sCurSrvData.next_event_description);
  }

  //Set m_currentMovie.m_strDirector
  if (!g_tuxbox.sCurSrvData.current_event_details.Equals("-") &&
    !g_tuxbox.sCurSrvData.current_event_details.IsEmpty())
  {
    m_currentFile.GetVideoInfoTag()->m_strDirector = g_tuxbox.sCurSrvData.current_event_details;
  }
}

CStdString CGUIInfoManager::GetPictureLabel(int info) const
{
  if (info == SLIDE_FILE_NAME)
    return GetItemLabel(&m_currentSlide, LISTITEM_FILENAME);
  else if (info == SLIDE_FILE_PATH)
  {
    CStdString path, displayPath;
    CUtil::GetDirectory(m_currentSlide.m_strPath, path);
    CURL(path).GetURLWithoutUserDetails(displayPath);
    return displayPath;
  }
  else if (info == SLIDE_FILE_SIZE)
    return GetItemLabel(&m_currentSlide, LISTITEM_SIZE);
  else if (info == SLIDE_FILE_DATE)
    return GetItemLabel(&m_currentSlide, LISTITEM_DATE);
  else if (info == SLIDE_INDEX)
  {
    CGUIWindowSlideShow *slideshow = (CGUIWindowSlideShow *)m_gWindowManager.GetWindow(WINDOW_SLIDESHOW);
    if (slideshow && slideshow->NumSlides())
    {
      CStdString index;
      index.Format("%d/%d", slideshow->CurrentSlide(), slideshow->NumSlides());
      return index;
    }
  }
  if (m_currentSlide.HasPictureInfoTag())
    return m_currentSlide.GetPictureInfoTag()->GetInfo(info);
  return "";
}

void CGUIInfoManager::SetCurrentSlide(CFileItem &item)
{
  if (m_currentSlide.m_strPath != item.m_strPath)
  {
    if (!item.HasPictureInfoTag() && !item.GetPictureInfoTag()->Loaded())
      item.GetPictureInfoTag()->Load(item.m_strPath);
    m_currentSlide = item;
  }
}

void CGUIInfoManager::ResetCurrentSlide()
{
  m_currentSlide.Reset();
}
