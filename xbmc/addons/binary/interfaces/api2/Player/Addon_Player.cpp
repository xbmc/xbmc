/*
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Addon_Player.h"

#include "Application.h"
#include "FileItem.h"
#include "addons/Addon.h"
#include "addons/binary/AddonInterfaceManager.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/AddonInterfaces.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "messaging/ApplicationMessenger.h"
#include "playlists/PlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"

using namespace ADDON;
using namespace KODI::MESSAGING;

namespace V2
{
namespace KodiAPI
{

namespace Player
{
extern "C"
{

void CAddOnPlayer::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->AddonPlayer.GetSupportedMedia            = V2::KodiAPI::Player::CAddOnPlayer::GetSupportedMedia;
  interfaces->AddonPlayer.New                          = V2::KodiAPI::Player::CAddOnPlayer::New;
  interfaces->AddonPlayer.Delete                       = V2::KodiAPI::Player::CAddOnPlayer::Delete;
  interfaces->AddonPlayer.SetCallbacks                 = V2::KodiAPI::Player::CAddOnPlayer::SetCallbacks;
  interfaces->AddonPlayer.PlayFile                     = V2::KodiAPI::Player::CAddOnPlayer::PlayFile;
  interfaces->AddonPlayer.PlayFileItem                 = V2::KodiAPI::Player::CAddOnPlayer::PlayFileItem;
  interfaces->AddonPlayer.Stop                         = V2::KodiAPI::Player::CAddOnPlayer::Stop;
  interfaces->AddonPlayer.Pause                        = V2::KodiAPI::Player::CAddOnPlayer::Pause;
  interfaces->AddonPlayer.PlayNext                     = V2::KodiAPI::Player::CAddOnPlayer::PlayNext;
  interfaces->AddonPlayer.PlayPrevious                 = V2::KodiAPI::Player::CAddOnPlayer::PlayPrevious;
  interfaces->AddonPlayer.PlaySelected                 = V2::KodiAPI::Player::CAddOnPlayer::PlaySelected;
  interfaces->AddonPlayer.IsPlaying                    = V2::KodiAPI::Player::CAddOnPlayer::IsPlaying;
  interfaces->AddonPlayer.IsPlayingAudio               = V2::KodiAPI::Player::CAddOnPlayer::IsPlayingAudio;
  interfaces->AddonPlayer.IsPlayingVideo               = V2::KodiAPI::Player::CAddOnPlayer::IsPlayingVideo;
  interfaces->AddonPlayer.IsPlayingRDS                 = V2::KodiAPI::Player::CAddOnPlayer::IsPlayingRDS;
  interfaces->AddonPlayer.GetPlayingFile               = V2::KodiAPI::Player::CAddOnPlayer::GetPlayingFile;
  interfaces->AddonPlayer.GetTotalTime                 = V2::KodiAPI::Player::CAddOnPlayer::GetTotalTime;
  interfaces->AddonPlayer.GetTime                      = V2::KodiAPI::Player::CAddOnPlayer::GetTime;
  interfaces->AddonPlayer.SeekTime                     = V2::KodiAPI::Player::CAddOnPlayer::SeekTime;
  interfaces->AddonPlayer.GetAvailableVideoStreams     = V2::KodiAPI::Player::CAddOnPlayer::GetAvailableVideoStreams;
  interfaces->AddonPlayer.SetVideoStream               = V2::KodiAPI::Player::CAddOnPlayer::SetVideoStream;
  interfaces->AddonPlayer.GetAvailableAudioStreams     = V2::KodiAPI::Player::CAddOnPlayer::GetAvailableAudioStreams;
  interfaces->AddonPlayer.SetAudioStream               = V2::KodiAPI::Player::CAddOnPlayer::SetAudioStream;
  interfaces->AddonPlayer.GetAvailableSubtitleStreams  = V2::KodiAPI::Player::CAddOnPlayer::GetAvailableSubtitleStreams;
  interfaces->AddonPlayer.SetSubtitleStream            = V2::KodiAPI::Player::CAddOnPlayer::SetSubtitleStream;
  interfaces->AddonPlayer.ShowSubtitles                = V2::KodiAPI::Player::CAddOnPlayer::ShowSubtitles;
  interfaces->AddonPlayer.GetCurrentSubtitleName       = V2::KodiAPI::Player::CAddOnPlayer::GetCurrentSubtitleName;
  interfaces->AddonPlayer.AddSubtitle                  = V2::KodiAPI::Player::CAddOnPlayer::AddSubtitle;
  interfaces->AddonPlayer.ClearList                    = V2::KodiAPI::Player::CAddOnPlayer::ClearList;
}

char* CAddOnPlayer::GetSupportedMedia(void* hdl, int mediaType)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p')",
                                        __FUNCTION__, helper);
    std::string result;
    if (mediaType == PlayList_Music)
      result = g_advancedSettings.m_videoExtensions;
    else if (mediaType == PlayList_Video)
      result = g_advancedSettings.GetMusicExtensions();
    else if (mediaType == PlayList_Picture)
      result = g_advancedSettings.m_pictureExtensions;
    else
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (mediaType='%i')",
                                        __FUNCTION__, mediaType);
    char* ret = strdup(result.c_str());
    return ret;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void* CAddOnPlayer::New(void *hdl)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p')",
                                        __FUNCTION__, helper);

    CAddOnPlayerControl* player = new CAddOnPlayerControl(helper->GetAddon());
    return player;
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnPlayer::Delete(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    delete static_cast<CAddOnPlayerControl*>(handle);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayer::SetCallbacks(
        void*             hdl,
        void*      handle,
        void*      cbhdl,
        void      (*CBOnPlayBackStarted)     (void* cbhdl),
        void      (*CBOnPlayBackEnded)       (void* cbhdl),
        void      (*CBOnPlayBackStopped)     (void* cbhdl),
        void      (*CBOnPlayBackPaused)      (void* cbhdl),
        void      (*CBOnPlayBackResumed)     (void* cbhdl),
        void      (*CBOnQueueNextItem)       (void* cbhdl),
        void      (*CBOnPlayBackSpeedChanged)(void* cbhdl, int iSpeed),
        void      (*CBOnPlayBackSeek)        (void* cbhdl, int iTime, int seekOffset),
        void      (*CBOnPlayBackSeekChapter) (void* cbhdl, int iChapter))
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle || !cbhdl)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p', cbhdl='%p')",
                                        __FUNCTION__, helper, handle, cbhdl);

    CAddOnPlayerControl* pAddonControl = static_cast<CAddOnPlayerControl*>(handle);

    CSingleLock lock(pAddonControl->m_playStateMutex);
    pAddonControl->m_cbhdl                    = cbhdl;
    pAddonControl->CBOnPlayBackStarted        = CBOnPlayBackStarted;
    pAddonControl->CBOnPlayBackEnded          = CBOnPlayBackEnded;
    pAddonControl->CBOnPlayBackStopped        = CBOnPlayBackStopped;
    pAddonControl->CBOnPlayBackPaused         = CBOnPlayBackPaused;
    pAddonControl->CBOnPlayBackResumed        = CBOnPlayBackResumed;
    pAddonControl->CBOnQueueNextItem          = CBOnQueueNextItem;
    pAddonControl->CBOnPlayBackSpeedChanged   = CBOnPlayBackSpeedChanged;
    pAddonControl->CBOnPlayBackSeek           = CBOnPlayBackSeek;
    pAddonControl->CBOnPlayBackSeekChapter    = CBOnPlayBackSeekChapter;
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnPlayer::PlayFile(void *hdl, void* handle, const char* item, bool windowed)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle || !item)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p', item='%p')",
                                        __FUNCTION__, helper, handle, item);

    return static_cast<CAddOnPlayerControl*>(handle)->PlayFile(item, windowed);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnPlayer::PlayFileItem(void *hdl, void* handle, const void* listitem, bool windowed)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle || !listitem)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p', listitem='%p')",
                                        __FUNCTION__, helper, handle, listitem);

    return static_cast<CAddOnPlayerControl*>(handle)->PlayFileItem(static_cast<const CFileItem*>(listitem), windowed);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnPlayer::PlayList(void *hdl, void* handle, const void* list, int playListId, bool windowed, int startpos)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle || !list)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p', list='%p')",
                                        __FUNCTION__, helper, handle, list);

    return static_cast<CAddOnPlayerControl*>(handle)->PlayList(static_cast<const PLAYLIST::CPlayList*>(list), playListId, windowed, startpos);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnPlayer::Stop(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->Stop();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayer::Pause(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->Pause();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayer::PlayNext(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->PlayNext();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayer::PlayPrevious(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->PlayPrevious();
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayer::PlaySelected(void *hdl, void* handle, int selected)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->PlaySelected(selected);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnPlayer::IsPlaying(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    return static_cast<CAddOnPlayerControl*>(handle)->IsPlaying();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnPlayer::IsPlayingAudio(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    return static_cast<CAddOnPlayerControl*>(handle)->IsPlayingAudio();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnPlayer::IsPlayingVideo(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    return static_cast<CAddOnPlayerControl*>(handle)->IsPlayingVideo();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnPlayer::IsPlayingRDS(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    return static_cast<CAddOnPlayerControl*>(handle)->IsPlayingRDS();
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

bool CAddOnPlayer::GetPlayingFile(void *hdl, void* handle, char& file, unsigned int& iMaxStringSize)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    std::string fileName = static_cast<CAddOnPlayerControl*>(handle)->GetPlayingFile();
    strncpy(&file, fileName.c_str(), iMaxStringSize);
    iMaxStringSize = fileName.length();
    return (iMaxStringSize != 0);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

double CAddOnPlayer::GetTotalTime(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    return static_cast<CAddOnPlayerControl*>(handle)->GetTotalTime();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0;
}

double CAddOnPlayer::GetTime(void *hdl, void* handle)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    return static_cast<CAddOnPlayerControl*>(handle)->GetTime();
  }
  HANDLE_ADDON_EXCEPTION

  return 0.0;
}

void CAddOnPlayer::SeekTime(void *hdl, void* handle, double seekTime)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->SeekTime(seekTime);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnPlayer::GetAvailableVideoStreams(void *hdl, void* handle, char**& streams, unsigned int& entries)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    std::vector<std::string> streamsInt = static_cast<CAddOnPlayerControl*>(handle)->GetAvailableVideoStreams();
    if (!streamsInt.empty())
    {
      entries = streamsInt.size();
      streams = (char**)malloc(entries*sizeof(char*));
      for (unsigned int i = 0; i < entries; ++i)
        streams[i] = strdup(streamsInt[i].c_str());

      return true;
    }
    else
      entries = 0;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnPlayer::SetVideoStream(void *hdl, void* handle, int iStream)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->SetVideoStream(iStream);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnPlayer::GetAvailableAudioStreams(void *hdl, void* handle, char**& streams, unsigned int& entries)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    std::vector<std::string> streamsInt = static_cast<CAddOnPlayerControl*>(handle)->GetAvailableAudioStreams();
    if (!streamsInt.empty())
    {
      entries = streamsInt.size();
      streams = (char**)malloc(entries*sizeof(char*));
      for (unsigned int i = 0; i < entries; ++i)
        streams[i] = strdup(streamsInt[i].c_str());

      return true;
    }
    else
      entries = 0;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnPlayer::SetAudioStream(void *hdl, void* handle, int iStream)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->SetAudioStream(iStream);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnPlayer::GetAvailableSubtitleStreams(void *hdl, void* handle, char**& streams, unsigned int& entries)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    std::vector<std::string> streamsInt = static_cast<CAddOnPlayerControl*>(handle)->GetAvailableSubtitleStreams();
    if (!streamsInt.empty())
    {
      entries = streamsInt.size();
      streams = (char**)malloc(entries*sizeof(char*));
      for (unsigned int i = 0; i < entries; ++i)
        streams[i] = strdup(streamsInt[i].c_str());

      return true;
    }
    else
      entries = 0;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnPlayer::SetSubtitleStream(void *hdl, void* handle, int iStream)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->SetSubtitleStream(iStream);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayer::ShowSubtitles(void *hdl, void* handle, bool bVisible)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->ShowSubtitles(bVisible);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnPlayer::GetCurrentSubtitleName(void *hdl, void* handle, char& name, unsigned int& iMaxStringSize)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    std::string strName = static_cast<CAddOnPlayerControl*>(handle)->GetCurrentSubtitleName();
    strncpy(&name, strName.c_str(), iMaxStringSize);
    iMaxStringSize = strName.length();

    return (iMaxStringSize != 0);
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

void CAddOnPlayer::AddSubtitle(void *hdl, void* handle, const char* strSubPath)
{
  try
  {
    CAddonInterfaces* helper = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);;
    if (!helper || !handle)
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid data (addonData='%p', handle='%p')",
                                        __FUNCTION__, helper, handle);

    static_cast<CAddOnPlayerControl*>(handle)->AddSubtitle(strSubPath);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayer::ClearList(char**& path, unsigned int entries)
{
  try
  {
    if (path)
    {
      for (unsigned int i = 0; i < entries; ++i)
      {
        if (path[i])
          free(path[i]);
      }
      free(path);
    }
    else
      throw ADDON::WrongValueException("CAddOnPlayer - %s - invalid add-on data for path", __FUNCTION__);
  }
  HANDLE_ADDON_EXCEPTION
}

/*\_____________________________________________________________________________
\*/

CAddOnPlayerControl::CAddOnPlayerControl(ADDON::CAddon* addon)
  : CBOnPlayBackStarted(nullptr),
    CBOnPlayBackEnded(nullptr),
    CBOnPlayBackStopped(nullptr),
    CBOnPlayBackPaused(nullptr),
    CBOnPlayBackResumed(nullptr),
    CBOnQueueNextItem(nullptr),
    CBOnPlayBackSpeedChanged(nullptr),
    CBOnPlayBackSeek(nullptr),
    CBOnPlayBackSeekChapter(nullptr),
    m_cbhdl(nullptr),
    m_addon(addon),
    m_playList(PLAYLIST_MUSIC)
{
  CServiceBroker::GetAddonInterfaceManager().RegisterPlayerCallBack(this);
}

CAddOnPlayerControl::~CAddOnPlayerControl()
{
  CServiceBroker::GetAddonInterfaceManager().UnregisterPlayerCallBack(this);
}

inline bool CAddOnPlayerControl::PlayFile(std::string item, bool windowed)
{
  // set fullscreen or windowed
  CMediaSettings::GetInstance().SetVideoStartWindowed(windowed);
  CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(item, false)));

  return true;
}

inline bool CAddOnPlayerControl::PlayFileItem(const CFileItem* item, bool windowed)
{
  // set fullscreen or windowed
  CMediaSettings::GetInstance().SetVideoStartWindowed(windowed);
  CApplicationMessenger::GetInstance().PostMsg(TMSG_MEDIA_PLAY, 0, 0, static_cast<void*>(new CFileItem(*item)));

  return true;
}

inline bool CAddOnPlayerControl::PlayList(const PLAYLIST::CPlayList* playlist, int playListId, bool windowed, int startpos)
{
  if (playlist != nullptr)
  {
    // set fullscreen or windowed
    CMediaSettings::GetInstance().SetVideoStartWindowed(windowed);

    // play playlist (a playlist from playlistplayer.cpp)
    m_playList = playListId;
    g_playlistPlayer.SetCurrentPlaylist(m_playList);
    if (startpos > -1)
      g_playlistPlayer.SetCurrentSong(startpos);
    CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_PLAY, startpos);
    return true;
  }
  return false;
}

inline void CAddOnPlayerControl::Stop()
{
  CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_STOP);
}

inline void CAddOnPlayerControl::Pause()
{
  CApplicationMessenger::GetInstance().SendMsg(TMSG_MEDIA_PAUSE);
}

inline void CAddOnPlayerControl::PlayNext()
{
  CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_NEXT);
}

inline void CAddOnPlayerControl::PlayPrevious()
{
  CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_PREV);
}

inline void CAddOnPlayerControl::PlaySelected(int selected)
{
  if (g_playlistPlayer.GetCurrentPlaylist() != m_playList)
  {
    g_playlistPlayer.SetCurrentPlaylist(m_playList);
  }
  g_playlistPlayer.SetCurrentSong(selected);

  CApplicationMessenger::GetInstance().SendMsg(TMSG_PLAYLISTPLAYER_PLAY, selected);
}

void CAddOnPlayerControl::OnPlayBackEnded()
{
  try
  {
    CSingleLock lock(m_playStateMutex);
    if (CBOnPlayBackEnded)
      CBOnPlayBackEnded(m_cbhdl);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayerControl::OnPlayBackStarted()
{
  try
  {
    CSingleLock lock(m_playStateMutex);
    if (CBOnPlayBackStarted)
      CBOnPlayBackStarted(m_cbhdl);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayerControl::OnPlayBackPaused()
{
  try
  {
    CSingleLock lock(m_playStateMutex);
    if (CBOnPlayBackPaused)
      CBOnPlayBackPaused(m_cbhdl);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayerControl::OnPlayBackResumed()
{
  try
  {
    CSingleLock lock(m_playStateMutex);
    if (CBOnPlayBackResumed)
      CBOnPlayBackResumed(m_cbhdl);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayerControl::OnPlayBackStopped()
{
  try
  {
    CSingleLock lock(m_playStateMutex);
    if (CBOnPlayBackStopped)
      CBOnPlayBackStopped(m_cbhdl);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayerControl::OnQueueNextItem()
{
  try
  {
    CSingleLock lock(m_playStateMutex);
    if (CBOnQueueNextItem)
      CBOnQueueNextItem(m_cbhdl);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayerControl::OnPlayBackSeek(int iTime, int seekOffset)
{
  try
  {
    CSingleLock lock(m_playStateMutex);
    if (CBOnPlayBackSeek)
      CBOnPlayBackSeek(m_cbhdl, iTime, seekOffset);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayerControl::OnPlayBackSeekChapter(int iChapter)
{
  try
  {
    CSingleLock lock(m_playStateMutex);
    if (CBOnPlayBackSeekChapter)
      CBOnPlayBackSeekChapter(m_cbhdl, iChapter);
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnPlayerControl::OnPlayBackSpeedChanged(int iSpeed)
{
  try
  {
    CSingleLock lock(m_playStateMutex);
    if (CBOnPlayBackSpeedChanged)
      CBOnPlayBackSpeedChanged(m_cbhdl, iSpeed);
  }
  HANDLE_ADDON_EXCEPTION
}

inline bool CAddOnPlayerControl::IsPlaying()
{
  return g_application.m_pPlayer->IsPlaying();
}

inline bool CAddOnPlayerControl::IsPlayingAudio()
{
  return g_application.m_pPlayer->IsPlayingAudio();
}

inline bool CAddOnPlayerControl::IsPlayingVideo()
{
  return g_application.m_pPlayer->IsPlayingVideo();
}

inline bool CAddOnPlayerControl::IsPlayingRDS()
{
  return g_application.m_pPlayer->IsPlayingRDS();
}

inline std::string CAddOnPlayerControl::GetPlayingFile()
{
  if (!g_application.m_pPlayer->IsPlaying())
    throw ADDON::UnimplementedException("CAddOnPlayerControl::GetPlayingFile", "Kodi is not playing any media file");

  return g_application.CurrentFile();
}

inline double CAddOnPlayerControl::GetTotalTime()
{
  if (!g_application.m_pPlayer->IsPlaying())
    throw ADDON::UnimplementedException("CAddOnPlayerControl::GetTotalTime", "Kodi is not playing any media file");

  return g_application.GetTotalTime();
}

inline double CAddOnPlayerControl::GetTime()
{
  if (!g_application.m_pPlayer->IsPlaying())
    throw ADDON::UnimplementedException("CAddOnPlayerControl::GetTime", "Kodi is not playing any media file");

  return g_application.GetTime();
}

inline void CAddOnPlayerControl::SeekTime(double seekTime)
{
  if (!g_application.m_pPlayer->IsPlaying())
    throw ADDON::UnimplementedException("CAddOnPlayerControl::SeekTime", "Kodi is not playing any media file");

  g_application.SeekTime(seekTime);
}

inline std::vector<std::string> CAddOnPlayerControl::GetAvailableVideoStreams()
{
  if (g_application.m_pPlayer->HasPlayer())
  {
    int streamCount = g_application.m_pPlayer->GetVideoStreamCount();
    std::vector<std::string> ret(streamCount);
    for (int iStream = 0; iStream < streamCount; iStream++)
    {
      SPlayerVideoStreamInfo info;
      g_application.m_pPlayer->GetVideoStreamInfo(iStream, info);

      if (info.language.length() > 0)
        ret[iStream] = info.language;
      else
        ret[iStream] = info.name;
    }
    return ret;
  }

  return std::vector<std::string>();
}

inline void CAddOnPlayerControl::SetVideoStream(int iStream)
{
  if (g_application.m_pPlayer->HasPlayer())
  {
    int streamCount = g_application.m_pPlayer->GetVideoStreamCount();
    if (iStream < streamCount)
      g_application.m_pPlayer->SetVideoStream(iStream);
  }
}

inline std::vector<std::string> CAddOnPlayerControl::GetAvailableAudioStreams()
{
  if (g_application.m_pPlayer->HasPlayer())
  {
    int streamCount = g_application.m_pPlayer->GetAudioStreamCount();
    std::vector<std::string> ret(streamCount);
    for (int iStream = 0; iStream < streamCount; iStream++)
    {
      SPlayerAudioStreamInfo info;
      g_application.m_pPlayer->GetAudioStreamInfo(iStream, info);

      if (info.language.length() > 0)
        ret[iStream] = info.language;
      else
        ret[iStream] = info.name;
    }
    return ret;
  }

  return std::vector<std::string>();
}

inline void CAddOnPlayerControl::SetAudioStream(int iStream)
{
  if (g_application.m_pPlayer->HasPlayer())
  {
    int streamCount = g_application.m_pPlayer->GetAudioStreamCount();
    if (iStream < streamCount)
      g_application.m_pPlayer->SetAudioStream(iStream);
  }
}

inline std::vector<std::string> CAddOnPlayerControl::GetAvailableSubtitleStreams()
{
  if (g_application.m_pPlayer->HasPlayer())
  {
    int subtitleCount = g_application.m_pPlayer->GetSubtitleCount();
    std::vector<std::string> ret(subtitleCount);
    for (int iStream = 0; iStream < subtitleCount; iStream++)
    {
      SPlayerSubtitleStreamInfo info;
      g_application.m_pPlayer->GetSubtitleStreamInfo(iStream, info);

      if (!info.language.empty())
        ret[iStream] = info.language;
      else
        ret[iStream] = info.name;
    }
    return ret;
  }

  return std::vector<std::string>();
}

inline void CAddOnPlayerControl::SetSubtitleStream(int iStream)
{
  if (g_application.m_pPlayer->HasPlayer())
  {
    int streamCount = g_application.m_pPlayer->GetSubtitleCount();
    if(iStream < streamCount)
    {
      g_application.m_pPlayer->SetSubtitle(iStream);
      g_application.m_pPlayer->SetSubtitleVisible(true);
    }
  }
}

inline void CAddOnPlayerControl::ShowSubtitles(bool bVisible)
{
  if (g_application.m_pPlayer->HasPlayer())
  {
    g_application.m_pPlayer->SetSubtitleVisible(bVisible);
  }
}

inline std::string CAddOnPlayerControl::GetCurrentSubtitleName()
{
  if (g_application.m_pPlayer->HasPlayer())
  {
    SPlayerSubtitleStreamInfo info;
    g_application.m_pPlayer->GetSubtitleStreamInfo(g_application.m_pPlayer->GetSubtitle(), info);

    if (!info.language.empty())
      return info.language;
    else
      return info.name;
  }

  return "";
}

inline void CAddOnPlayerControl::AddSubtitle(const std::string& strSubPath)
{
  if (g_application.m_pPlayer->HasPlayer())
  {
    g_application.m_pPlayer->AddSubtitle(strSubPath);
  }
}

} /* extern "C" */
} /* namespace Player */

} /* namespace KodiAPI */
} /* namespace V2 */
