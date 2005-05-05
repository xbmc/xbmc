#include "../stdafx.h"
#include "GUIInfoManager.h"
#include "Weather.h"
#include "../Application.h"
#include "../Util.h"
#include "../lib/libscrobbler/scrobbler.h"

#define VERSION_STRING "1.1.0"

// stuff for current song
#include "../filesystem/CDDADirectory.h"
#include "../musicInfoTagLoaderFactory.h"
#include "../filesystem/SndtrkDirectory.h"

#include "FanController.h"

extern char g_szTitleIP[32];

#define PLAYER_HAS_MEDIA              1
#define PLAYER_HAS_AUDIO              2
#define PLAYER_HAS_VIDEO              3
#define PLAYER_PLAYING                4
#define PLAYER_PAUSED                 5 
#define PLAYER_REWINDING              6
#define PLAYER_REWINDING_2x           7
#define PLAYER_REWINDING_4x           8
#define PLAYER_REWINDING_8x           9
#define PLAYER_REWINDING_16x         10
#define PLAYER_REWINDING_32x         11
#define PLAYER_FORWARDING            12
#define PLAYER_FORWARDING_2x         13
#define PLAYER_FORWARDING_4x         14
#define PLAYER_FORWARDING_8x         15
#define PLAYER_FORWARDING_16x        16
#define PLAYER_FORWARDING_32x        17
#define PLAYER_CAN_RECORD            18
#define PLAYER_RECORDING             19
#define PLAYER_CACHING               20
#define PLAYER_DISPLAY_AFTER_SEEK    21

#define WEATHER_CONDITIONS          100
#define WEATHER_TEMPERATURE         101
#define WEATHER_LOCATION            102

#define SYSTEM_TIME                 110
#define SYSTEM_DATE                 111
#define SYSTEM_CPU_TEMPERATURE      112
#define SYSTEM_GPU_TEMPERATURE      113
#define SYSTEM_FAN_SPEED            114
#define SYSTEM_FREE_SPACE_C         115
// 116 is reserved for space on D
#define SYSTEM_FREE_SPACE_E         117
#define SYSTEM_FREE_SPACE_F         118
#define SYSTEM_FREE_SPACE_G         119
#define SYSTEM_BUILD_VERSION        120
#define SYSTEM_BUILD_DATE           121

#define NETWORK_IP_ADDRESS          190

#define MUSICPLAYER_TITLE           200
#define MUSICPLAYER_ALBUM           201
#define MUSICPLAYER_ARTIST          202
#define MUSICPLAYER_GENRE           203
#define MUSICPLAYER_YEAR            204
#define MUSICPLAYER_TIME            205
#define MUSICPLAYER_TIME_REMAINING  206
#define MUSICPLAYER_TIME_SPEED      207
#define MUSICPLAYER_TRACK_NUMBER    208
#define MUSICPLAYER_DURATION        209
#define MUSICPLAYER_COVER           210

#define VIDEOPLAYER_TITLE           250
#define VIDEOPLAYER_GENRE           251
#define VIDEOPLAYER_DIRECTOR        252
#define VIDEOPLAYER_YEAR            253
#define VIDEOPLAYER_TIME            254
#define VIDEOPLAYER_TIME_REMAINING  255
#define VIDEOPLAYER_TIME_SPEED      256
#define VIDEOPLAYER_DURATION        257
#define VIDEOPLAYER_COVER           258

#define AUDIOSCROBBLER_ENABLED      300
#define AUDIOSCROBBLER_CONN_STATE   301
#define AUDIOSCROBBLER_SUBMIT_INT   302
#define AUDIOSCROBBLER_FILES_CACHED 303
#define AUDIOSCROBBLER_SUBMIT_STATE 304

CGUIInfoManager g_infoManager;

CGUIInfoManager::CGUIInfoManager(void)
{
  m_lastSysHeatInfoTime = 0;
  m_fanSpeed = 0;
  m_gpuTemp = 0;
  m_cpuTemp = 0;
  m_AfterSeekTimeout = 0;
  m_bPerformingSeek = false;
}

CGUIInfoManager::~CGUIInfoManager(void)
{
}

/// \brief Translates a string as given by the skin into an int that we use for more
/// efficient retrieval of data.
int CGUIInfoManager::TranslateString(const CStdString &strCondition)
{
  if (strCondition.IsEmpty()) return 0;
  CStdString strTest = strCondition;
  strTest = strTest.ToLower();
  bool bNegate = strTest[0] == '!';
  int ret = 0;

  if(bNegate)
    strTest.Delete(0, 1);

  // check playing conditions...
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
  else if (strTest.Equals("weather.conditions")) ret = WEATHER_CONDITIONS;
  else if (strTest.Equals("weather.temperature")) ret = WEATHER_TEMPERATURE;
  else if (strTest.Equals("weather.location")) ret = WEATHER_LOCATION;
  else if (strTest.Equals("system.date")) ret = SYSTEM_DATE;
  else if (strTest.Equals("system.time")) ret = SYSTEM_TIME;
  else if (strTest.Equals("system.cputemperature")) ret = SYSTEM_CPU_TEMPERATURE;
  else if (strTest.Equals("system.gputemperature")) ret = SYSTEM_GPU_TEMPERATURE;
  else if (strTest.Equals("system.fanspeed")) ret = SYSTEM_FAN_SPEED;
  else if (strTest.Equals("system.freespace(c)")) ret = SYSTEM_FREE_SPACE_C;
  else if (strTest.Equals("system.freespace(e)")) ret = SYSTEM_FREE_SPACE_E;
  else if (strTest.Equals("system.freespace(f)")) ret = SYSTEM_FREE_SPACE_F;
  else if (strTest.Equals("system.freespace(g)")) ret = SYSTEM_FREE_SPACE_G;
  else if (strTest.Equals("system.buildversion")) ret = SYSTEM_BUILD_VERSION;
  else if (strTest.Equals("system.builddate")) ret = SYSTEM_BUILD_DATE;
  else if (strTest.Equals("network.ipaddress")) ret = NETWORK_IP_ADDRESS;
  else if (strTest.Equals("musicplayer.title")) ret = MUSICPLAYER_TITLE;
  else if (strTest.Equals("musicplayer.album")) ret = MUSICPLAYER_ALBUM;
  else if (strTest.Equals("musicplayer.artist")) ret = MUSICPLAYER_ARTIST;
  else if (strTest.Equals("musicplayer.year")) ret = MUSICPLAYER_YEAR;
  else if (strTest.Equals("musicplayer.genre")) ret = MUSICPLAYER_GENRE;
  else if (strTest.Equals("musicplayer.time")) ret = MUSICPLAYER_TIME;
  else if (strTest.Equals("musicplayer.timeremaining")) ret = MUSICPLAYER_TIME_REMAINING;
  else if (strTest.Equals("musicplayer.timespeed")) ret = MUSICPLAYER_TIME_SPEED;
  else if (strTest.Equals("musicplayer.tracknumber")) ret = MUSICPLAYER_TRACK_NUMBER;
  else if (strTest.Equals("musicplayer.duration")) ret = MUSICPLAYER_DURATION;
  else if (strTest.Equals("musicplayer.cover")) ret = MUSICPLAYER_COVER;
  else if (strTest.Equals("videoplayer.title")) ret = VIDEOPLAYER_TITLE;
  else if (strTest.Equals("videoplayer.genre")) ret = VIDEOPLAYER_GENRE;
  else if (strTest.Equals("videoplayer.director")) ret = VIDEOPLAYER_DIRECTOR;
  else if (strTest.Equals("videoplayer.year")) ret = VIDEOPLAYER_YEAR;
  else if (strTest.Equals("videoplayer.time")) ret = VIDEOPLAYER_TIME;
  else if (strTest.Equals("videoplayer.timeremaining")) ret = VIDEOPLAYER_TIME_REMAINING;
  else if (strTest.Equals("videoplayer.timespeed")) ret = VIDEOPLAYER_TIME_SPEED;
  else if (strTest.Equals("videoplayer.duration")) ret = VIDEOPLAYER_DURATION;
  else if (strTest.Equals("videoplayer.cover")) ret = VIDEOPLAYER_COVER;
  else if (strTest.Equals("audioscrobbler.enabled")) ret = AUDIOSCROBBLER_ENABLED;
  else if (strTest.Equals("audioscrobbler.connectstate")) ret = AUDIOSCROBBLER_CONN_STATE;
  else if (strTest.Equals("audioscrobbler.submitinterval")) ret = AUDIOSCROBBLER_SUBMIT_INT;
  else if (strTest.Equals("audioscrobbler.filescached")) ret = AUDIOSCROBBLER_FILES_CACHED;
  else if (strTest.Equals("audioscrobbler.submitstate")) ret = AUDIOSCROBBLER_SUBMIT_STATE;
  return bNegate ? -ret : ret;
}

wstring CGUIInfoManager::GetLabel(int info)
{
  CStdString strLabel;
  switch (info)
  {
  case WEATHER_CONDITIONS:
    strLabel = g_weatherManager.GetLabel(WEATHER_LABEL_CURRENT_COND);
    break;
  case WEATHER_TEMPERATURE:
    strLabel = g_weatherManager.GetLabel(WEATHER_LABEL_CURRENT_TEMP);
    break;
  case WEATHER_LOCATION:
    strLabel = g_weatherManager.GetLabel(WEATHER_LABEL_LOCATION);
    break;
  case SYSTEM_TIME:
    return GetTime();
    break;
  case SYSTEM_DATE:
    return GetDate();
    break;
  case MUSICPLAYER_TITLE:
  case MUSICPLAYER_ALBUM:
  case MUSICPLAYER_ARTIST:
  case MUSICPLAYER_GENRE:
  case MUSICPLAYER_YEAR:
  case MUSICPLAYER_TIME:
  case MUSICPLAYER_TIME_REMAINING:
  case MUSICPLAYER_TIME_SPEED:
  case MUSICPLAYER_TRACK_NUMBER:
  case MUSICPLAYER_DURATION:
    strLabel = GetMusicLabel(info);
  break;
  case VIDEOPLAYER_TITLE:
  case VIDEOPLAYER_GENRE:
  case VIDEOPLAYER_DIRECTOR:
  case VIDEOPLAYER_YEAR:
  case VIDEOPLAYER_TIME:
  case VIDEOPLAYER_TIME_REMAINING:
  case VIDEOPLAYER_TIME_SPEED:
  case VIDEOPLAYER_DURATION:
    strLabel = GetVideoLabel(info);
  break;
  case SYSTEM_FREE_SPACE_C:
  case SYSTEM_FREE_SPACE_E:
  case SYSTEM_FREE_SPACE_F:
  case SYSTEM_FREE_SPACE_G:
    return GetFreeSpace(info);
  break;
  case SYSTEM_CPU_TEMPERATURE:
    return GetSystemHeatInfo("cpu");
    break;
  case SYSTEM_GPU_TEMPERATURE:
    return GetSystemHeatInfo("gpu");
    break;
  case SYSTEM_FAN_SPEED:
    return GetSystemHeatInfo("fan");
    break;
  case SYSTEM_BUILD_VERSION:
    strLabel = GetVersion();
    break;
  case SYSTEM_BUILD_DATE:
    strLabel = GetBuild();
    break;
  case NETWORK_IP_ADDRESS:
    {
      const WCHAR* pszIP = g_localizeStrings.Get(150).c_str();
      WCHAR wzIP[32];
      swprintf(wzIP, L"%s: %S", pszIP, g_szTitleIP);
      wstring strReturn = wzIP;
      return strReturn;
    }
    break;
  case AUDIOSCROBBLER_CONN_STATE:
  case AUDIOSCROBBLER_SUBMIT_INT:
  case AUDIOSCROBBLER_FILES_CACHED:
  case AUDIOSCROBBLER_SUBMIT_STATE:
    strLabel=GetAudioScrobblerLabel(info);
    break;
  }
  // convert our CStdString to a wstring (which the label expects!)
  WCHAR szLabel[256];
  swprintf(szLabel, L"%S", strLabel.c_str() );
  wstring strReturn = szLabel;
  return strReturn;
}

// checks the condition and returns it as necessary.  Currently used
// for toggle button controls and visibility of images.
bool CGUIInfoManager::GetBool(int condition1) const
{
  int condition = abs(condition1);
  bool bReturn = false;
  if (g_application.IsPlaying())
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
      bReturn = !g_application.m_pPlayer->IsPaused() && (g_application.GetPlaySpeed() == 1);
      break;
    case PLAYER_PAUSED:
      bReturn = g_application.m_pPlayer->IsPaused();
      break;
    case PLAYER_REWINDING:
      bReturn = g_application.GetPlaySpeed() < 1;
      break;
    case PLAYER_FORWARDING:
      bReturn = g_application.GetPlaySpeed() > 1;
      break;
    case PLAYER_REWINDING_2x:
      bReturn = g_application.GetPlaySpeed() == -2;
      break;
    case PLAYER_REWINDING_4x:
      bReturn = g_application.GetPlaySpeed() == -4;
      break;
    case PLAYER_REWINDING_8x:
      bReturn = g_application.GetPlaySpeed() == -8;
      break;
    case PLAYER_REWINDING_16x:
      bReturn = g_application.GetPlaySpeed() == -16;
      break;
    case PLAYER_REWINDING_32x:
      bReturn = g_application.GetPlaySpeed() == -32;
      break;
    case PLAYER_FORWARDING_2x:
      bReturn = g_application.GetPlaySpeed() == 2;
      break;
    case PLAYER_FORWARDING_4x:
      bReturn = g_application.GetPlaySpeed() == 4;
      break;
    case PLAYER_FORWARDING_8x:
      bReturn = g_application.GetPlaySpeed() == 8;
      break;
    case PLAYER_FORWARDING_16x:
      bReturn = g_application.GetPlaySpeed() == 16;
      break;
    case PLAYER_FORWARDING_32x:
      bReturn = g_application.GetPlaySpeed() == 32;
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
    case AUDIOSCROBBLER_ENABLED:
      bReturn = g_guiSettings.GetBool("MusicLibrary.UseAudioScrobbler");
    break;
    }
    
  }
  return (condition1 < 0) ? !bReturn : bReturn;
}

/// \brief Obtains the filename of the image to show from whichever subsystem is needed
CStdString CGUIInfoManager::GetImage(int info)
{
  if (info == WEATHER_CONDITIONS)
    return g_weatherManager.GetCurrentIcon();
  else if (info == MUSICPLAYER_COVER)
  {
    if (!g_application.IsPlayingAudio()) return "";
    return m_currentSong.HasThumbnail() ? m_currentSong.GetThumbnailImage() : "music.jpg";
  }
  else if (info == VIDEOPLAYER_COVER)
  {
    if (!g_application.IsPlayingVideo()) return "";
    return m_currentMovieThumb;
  }
  return "";
}

wstring CGUIInfoManager::GetDate(bool bNumbersOnly)
{
  WCHAR szText[128];
  SYSTEMTIME time;
  GetLocalTime(&time);

  if (bNumbersOnly)
  {
    CStdString strDate;
    if (g_guiSettings.GetBool("LookAndFeel.SwapMonthAndDay"))
      swprintf(szText, L"%02.2i-%02.2i-%02.2i", time.wDay, time.wMonth, time.wYear);
    else
      swprintf(szText, L"%02.2i-%02.2i-%02.2i", time.wMonth, time.wDay, time.wYear);
  }
  else
  {
    const WCHAR* day;
    switch (time.wDayOfWeek)
    {
    case 1 : day = g_localizeStrings.Get(11).c_str(); break;
    case 2 : day = g_localizeStrings.Get(12).c_str(); break;
    case 3 : day = g_localizeStrings.Get(13).c_str(); break;
    case 4 : day = g_localizeStrings.Get(14).c_str(); break;
    case 5 : day = g_localizeStrings.Get(15).c_str(); break;
    case 6 : day = g_localizeStrings.Get(16).c_str(); break;
    default: day = g_localizeStrings.Get(17).c_str(); break;
    }

    const WCHAR* month;
    switch (time.wMonth)
    {
    case 1 : month = g_localizeStrings.Get(21).c_str(); break;
    case 2 : month = g_localizeStrings.Get(22).c_str(); break;
    case 3 : month = g_localizeStrings.Get(23).c_str(); break;
    case 4 : month = g_localizeStrings.Get(24).c_str(); break;
    case 5 : month = g_localizeStrings.Get(25).c_str(); break;
    case 6 : month = g_localizeStrings.Get(26).c_str(); break;
    case 7 : month = g_localizeStrings.Get(27).c_str(); break;
    case 8 : month = g_localizeStrings.Get(28).c_str(); break;
    case 9 : month = g_localizeStrings.Get(29).c_str(); break;
    case 10: month = g_localizeStrings.Get(30).c_str(); break;
    case 11: month = g_localizeStrings.Get(31).c_str(); break;
    default: month = g_localizeStrings.Get(32).c_str(); break;
    }

    if (day && month)
    {
      if (g_guiSettings.GetBool("LookAndFeel.SwapMonthAndDay"))
        swprintf(szText, L"%s, %d %s", day, time.wDay, month);
      else
        swprintf(szText, L"%s, %s %d", day, month, time.wDay);
    }
    else
      swprintf(szText, L"no date");
  }
  wstring strText = szText;
  return strText;
}

wstring CGUIInfoManager::GetTime(bool bSeconds)
{
  WCHAR szText[128];
  SYSTEMTIME time;
  GetLocalTime(&time);

  INT iHour = time.wHour;

  if (g_guiSettings.GetBool("LookAndFeel.Clock12Hour"))
  {
    if (iHour > 11)
    {
      iHour -= (12 * (iHour > 12));
      if (bSeconds)
        swprintf(szText, L"%2d:%02d:%02d PM", iHour, time.wMinute, time.wSecond);
      else
        swprintf(szText, L"%2d:%02d PM", iHour, time.wMinute);
    }
    else
    {
      iHour += (12 * (iHour < 1));
      if (bSeconds)
        swprintf(szText, L"%2d:%02d:%02d AM", iHour, time.wMinute, time.wSecond);
      else
        swprintf(szText, L"%2d:%02d AM", iHour, time.wMinute);
    }
  }
  else
  {
    if (bSeconds)
      swprintf(szText, L"%2d:%02d:%02d", iHour, time.wMinute, time.wSecond);
    else
      swprintf(szText, L"%02d:%02d", iHour, time.wMinute);
  }
  wstring strText = szText;
  return strText;
}

CStdString CGUIInfoManager::GetMusicLabel(int item)
{
  if (!g_application.IsPlayingAudio()) return "";
  CMusicInfoTag& tag = m_currentSong.m_musicInfoTag;
  if (!tag.Loaded()) return "";
  switch (item)
  {
  case MUSICPLAYER_TITLE: 
    return tag.GetTitle();
    break;
  case MUSICPLAYER_ALBUM: 
    return tag.GetAlbum();
    break;
  case MUSICPLAYER_ARTIST: 
    return tag.GetArtist();
    break;
  case MUSICPLAYER_YEAR: 
    return tag.GetYear();
    break;
  case MUSICPLAYER_GENRE: 
    return tag.GetGenre();
    break;
  case MUSICPLAYER_TIME: 
    return GetCurrentPlayTime();
    break;
  case MUSICPLAYER_TIME_REMAINING: 
    return GetCurrentPlayTimeRemaining();
    break;
  case MUSICPLAYER_TIME_SPEED:
    {
      CStdString strTime;
      if (g_application.GetPlaySpeed() != 1)
        strTime.Format("%s (%ix)", GetCurrentPlayTime().c_str(), g_application.GetPlaySpeed());
      else
        strTime = GetCurrentPlayTime();
      return strTime;
    }
    break;
  case MUSICPLAYER_TRACK_NUMBER:
    {
      CStdString strTrack;
      if (tag.GetTrackNumber() > 0)
        strTrack.Format("%02i", tag.GetTrackNumber());
      return strTrack;
    }
    break;
  case MUSICPLAYER_DURATION:
    {
      CStdString strDuration = "00:00";
      if (tag.GetDuration() > 0)
        CUtil::SecondsToHMSString(tag.GetDuration(), strDuration);
      else
      {
        unsigned int iTotal = g_application.m_pPlayer->GetTotalTime();
        if (iTotal > 0)
          CUtil::SecondsToHMSString(iTotal, strDuration, true);
      }
      return strDuration;
    }
    break;
  }
  return "";
}

CStdString CGUIInfoManager::GetVideoLabel(int item)
{
  if (!g_application.IsPlayingVideo()) return "";
  switch (item)
  {
  case VIDEOPLAYER_TITLE:
    return m_currentMovie.m_strTitle;
    break;
  case VIDEOPLAYER_GENRE:
    return m_currentMovie.m_strGenre;
    break;
  case VIDEOPLAYER_DIRECTOR:
    return m_currentMovie.m_strDirector;
    break;
  case VIDEOPLAYER_YEAR:
    if (m_currentMovie.m_iYear > 0)
    {
      CStdString strYear;
      strYear.Format("%i", m_currentMovie.m_iYear);
      return strYear;
    }
    break;
  case VIDEOPLAYER_TIME:
    return GetCurrentPlayTime();
    break;
  case VIDEOPLAYER_TIME_REMAINING:
    return GetCurrentPlayTimeRemaining();
    break;
  case VIDEOPLAYER_TIME_SPEED:
    {
      CStdString strTime;
      if (g_application.GetPlaySpeed() != 1)
        strTime.Format("%s (%ix)", GetCurrentPlayTime().c_str(), g_application.GetPlaySpeed());
      else
        strTime = GetCurrentPlayTime();
      return strTime;
    }
    break;
  case VIDEOPLAYER_DURATION:
    {
      CStdString strDuration = "00:00:00";
      unsigned int iTotal = g_application.m_pPlayer->GetTotalTime();
      if (iTotal > 0)
        CUtil::SecondsToHMSString(iTotal, strDuration, true);
      return strDuration;
    }
    break;
  }
  return "";
}

__int64 CGUIInfoManager::GetPlayTime()
{
  if (g_application.IsPlayingAudio())
  {
    __int64 lPTS = g_application.m_pPlayer->GetTime() - (g_infoManager.GetCurrentSongStart() * (__int64)1000) / 75;
    if (lPTS < 0) lPTS = 0;
    return lPTS;
  }
  else if (g_application.IsPlayingVideo())
  {
    __int64 lPTS = g_application.m_pPlayer->GetTime();
    if (lPTS < 0) lPTS = 0;
    return lPTS;
  }
  return 0;
}

CStdString CGUIInfoManager::GetCurrentPlayTime()
{
  CStdString strTime;
  if (g_application.IsPlayingAudio())
    CUtil::SecondsToHMSString((int)(GetPlayTime()/1000), strTime);
  else if (g_application.IsPlayingVideo())
    CUtil::SecondsToHMSString((int)(GetPlayTime()/1000), strTime, true);
  return strTime;
}

int CGUIInfoManager::GetTotalPlayTime()
{
  int iTotalTime = 0;
  if (g_application.IsPlayingAudio())
  {
    iTotalTime = g_application.m_pPlayer->GetTotalTime();
    if (m_currentSong.m_musicInfoTag.GetDuration() > 0)
      iTotalTime = m_currentSong.m_musicInfoTag.GetDuration();
    else if (iTotalTime < 0)
      iTotalTime = 0;
  }
  else if (g_application.IsPlayingVideo())
  {
    iTotalTime = g_application.m_pPlayer->GetTotalTime();
    if (iTotalTime < 0)
      iTotalTime = 0;
  }
  return iTotalTime;
}

int CGUIInfoManager::GetPlayTimeRemaining()
{
  int iReverse = 0;
  if (g_application.IsPlayingAudio())
  {
    int iTotalTime = g_application.m_pPlayer->GetTotalTime();
    if (m_currentSong.m_musicInfoTag.GetDuration() > 0)
      iTotalTime = m_currentSong.m_musicInfoTag.GetDuration();
    else if (iTotalTime < 0)
      iTotalTime = 0;

    __int64 lPTS = g_application.m_pPlayer->GetTime() - (g_infoManager.GetCurrentSongStart() * (__int64)1000) / 75;
    if (lPTS < 0) lPTS = 0;
    iReverse = iTotalTime - (int)(lPTS / 1000);
  }
  else if (g_application.IsPlayingVideo())
  {
    int iTotalTime = g_application.m_pPlayer->GetTotalTime();
    if (iTotalTime < 0)
      iTotalTime = 0;

    iReverse = iTotalTime - (int)(g_application.m_pPlayer->GetTime() / 1000);
  }
  return iReverse > 0 ? iReverse : 0;
}

CStdString CGUIInfoManager::GetCurrentPlayTimeRemaining()
{
  CStdString strTime;
  if (g_application.IsPlayingAudio())
    CUtil::SecondsToHMSString(GetPlayTimeRemaining(), strTime);
  else if (g_application.IsPlayingVideo())
    CUtil::SecondsToHMSString(GetPlayTimeRemaining(), strTime, true);
  return strTime;
}

void CGUIInfoManager::ResetCurrentItem()
{ 
  m_currentSong.Clear();
  m_currentMovie.Reset();
  m_currentMovieThumb = "";
}

void CGUIInfoManager::SetCurrentItem(CFileItem &item)
{
  ResetCurrentItem();
  if (item.IsAudio())
    SetCurrentSong(item);
  else
    SetCurrentMovie(item);
}

void CGUIInfoManager::SetCurrentSong(CFileItem &item)
{
  m_currentSong = item;

  // Get a reference to the item's tag
  CMusicInfoTag& tag = m_currentSong.m_musicInfoTag;
  // check if we don't have the tag already loaded
  if (!tag.Loaded())
  {
    // we have a audio file.
    // Look if we have this file in database...
    bool bFound = false;
    if (g_musicDatabase.Open())
    {
      CSong song;
      bFound = g_musicDatabase.GetSongByFileName(m_currentSong.m_strPath, song);
      m_currentSong.m_musicInfoTag.SetSong(song);
      g_musicDatabase.Close();
    }

    if (!bFound)
    {
      // always get id3 info for the overlay
      CMusicInfoTagLoaderFactory factory;
      auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(m_currentSong.m_strPath));
      // Do we have a tag loader for this file type?
      if (NULL != pLoader.get())
        pLoader->Load(m_currentSong.m_strPath, tag);
    }
  }

  // If we have tag information, ...
  if (tag.Loaded())
  {
    if (!tag.GetTitle().size())
    {
      // No title in tag, show filename only
      CSndtrkDirectory dir;
      char NameOfSong[64];
      if (dir.FindTrackName(m_currentSong.m_strPath, NameOfSong))
        tag.SetTitle(NameOfSong);
      else
        tag.SetTitle( CUtil::GetTitleFromPath(m_currentSong.m_strPath) );
    }
  } // if (tag.Loaded())
  else
  {
    // If we have a cdda track without cddb information,...
    if (m_currentSong.IsCDDA())
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
      tag.SetTitle( CUtil::GetTitleFromPath(m_currentSong.m_strPath) );
    } // we now have at least the title
    tag.SetLoaded(true);
  }

  // find a thumb for this file.
  if (m_currentSong.IsInternetStream())
  {
    CFileItem* pItemTemp = new CFileItem(g_application.m_strPlayListFile,false);
    pItemTemp->SetMusicThumb();
    CStdString strThumb = pItemTemp->GetThumbnailImage();
    if (CUtil::FileExists(strThumb))
      m_currentSong.SetThumbnailImage(strThumb);
  }
  else
    m_currentSong.SetMusicThumb();
  m_currentSong.FillInDefaultIcon();
}

void CGUIInfoManager::SetCurrentMovie(CFileItem &item)
{
  CVideoDatabase dbs;
  dbs.Open();
  if (dbs.HasMovieInfo(item.m_strPath))
  {
    dbs.GetMovieInfo(item.m_strPath, m_currentMovie);
  }
  dbs.Close();

  if (m_currentMovie.m_strTitle.IsEmpty())
  { // at least fill in the filename
    m_currentMovie.m_strTitle = CUtil::GetTitleFromPath(item.m_strPath);
  }
  if (m_currentMovie.m_strPath.IsEmpty())
  {
    m_currentMovie.m_strPath = item.m_strPath;
  }
  // Find a thumb for this file.
  item.SetThumb();
  if (!item.HasThumbnail())
  { // get IMDb thumb if we have one
    CStdString strThumb;
    CUtil::GetVideoThumbnail(m_currentMovie.m_strIMDBNumber, strThumb);
    if (CUtil::FileExists(strThumb))
      item.SetThumbnailImage(strThumb);
  }
  item.FillInDefaultIcon();
  m_currentMovieThumb = item.GetThumbnailImage();
}

wstring CGUIInfoManager::GetSystemHeatInfo(const CStdString &strInfo)
{
  if (timeGetTime() - m_lastSysHeatInfoTime >= 1000)
  { // update our variables
    m_lastSysHeatInfoTime = timeGetTime();
    m_fanSpeed = CFanController::Instance()->GetFanSpeed();
    m_gpuTemp = CFanController::Instance()->GetGPUTemp();
    m_cpuTemp = CFanController::Instance()->GetCPUTemp();
  }
  if (strInfo == "cpu")
  {
    WCHAR CPUText[32];
    if (g_guiSettings.GetInt("Weather.TemperatureUnits") == 1 /*DEGREES_F*/)
      swprintf(CPUText, L"%s %2.2f%cF", g_localizeStrings.Get(140).c_str(), ((9.0 / 5.0) * m_cpuTemp) + 32.0, 176);
    else
      swprintf(CPUText, L"%s %2.2f%cC", g_localizeStrings.Get(140).c_str(), m_cpuTemp, 176);
    return CPUText;
  }
  else if (strInfo == "gpu")
  {
    WCHAR GPUText[32];
    if (g_guiSettings.GetInt("Weather.TemperatureUnits") == 1 /*DEGREES_F*/)
      swprintf(GPUText, L"%s %2.2f%cF", g_localizeStrings.Get(141).c_str(), ((9.0 / 5.0) * m_gpuTemp) + 32.0, 176);
    else
      swprintf(GPUText, L"%s %2.2f%cC", g_localizeStrings.Get(141).c_str(), m_gpuTemp, 176);
    return GPUText;
  }
  else if (strInfo == "fan")
  {
    WCHAR FanText[32];
    swprintf(FanText, L"%s: %i%%", g_localizeStrings.Get(13300).c_str(), m_fanSpeed * 2);
    return FanText;
  }
  return L"";
}

wstring CGUIInfoManager::GetFreeSpace(int drive)
{
  ULARGE_INTEGER lTotalFreeBytes;
  WCHAR wszHD[64];
  wstring strReturn;

  char cDrive = drive - SYSTEM_FREE_SPACE_C + 'C';
  CStdString strDriveFind;
  strDriveFind.Format("%c:\\", cDrive);
  const WCHAR *pszDrive = g_localizeStrings.Get(155).c_str();
  const WCHAR *pszFree = g_localizeStrings.Get(160).c_str();
  const WCHAR *pszUnavailable = g_localizeStrings.Get(161).c_str();
  if (GetDiskFreeSpaceEx( strDriveFind.c_str(), NULL, NULL, &lTotalFreeBytes))
  {
    swprintf(wszHD, L"%s %c: %u Mb ", pszDrive, cDrive, lTotalFreeBytes.QuadPart / 1048576); //To make it MB
    wcscat(wszHD, pszFree);
  }
  else
  {
    swprintf(wszHD, L"%s %c: ", pszDrive, cDrive);
    wcscat(wszHD, pszUnavailable);
  }
  strReturn = wszHD;
  return strReturn;
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
