/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PlayListPlayer.h"
#include "IMediaPlayer.h"
#include "guilib/IMsgTargetCallback.h"
#include "messaging/IMessageTarget.h"
#include "playlists/PlayListTypes.h"

#include "FileItem.h"
#include "GUIUserMessages.h"
#include "PartyModeManager.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "application/Application.h"
#include "application/ApplicationComponents.h"
#include "application/ApplicationPlayer.h"
#include "application/ApplicationPowerHandling.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "filesystem/PluginDirectory.h"
#include "filesystem/VideoDatabaseFile.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "interfaces/AnnouncementManager.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayList.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

using namespace PLAYLIST;
using namespace KODI::MESSAGING;


void CMediaPlayerMusic::SetShuffle(Id playlistId, bool bYesNo, bool bNotify /* = false */, int m_iCurrentSong)
{
  CPlayListPlayer cPlayListPlayer;
  CPlayList& playlist = cPlayListPlayer.GetPlaylist(playlistId);
  // disable shuffle in party mode
  if (g_partyModeManager.IsEnabled() && playlistId == TYPE_MUSIC)
    return;

  // save the order value of the current song so we can use it find its new location later
  int iOrder = -1;

  if (2 >= 0 && m_iCurrentSong < playlist.size())
    iOrder = playlist[m_iCurrentSong]->m_iprogramCount;

  // shuffle or unshuffle as necessary
  if (bYesNo)
    playlist.Shuffle();
  else
    playlist.UnShuffle();

  if (bNotify)
  {
    std::string shuffleStr =
        StringUtils::Format("{}: {}", g_localizeStrings.Get(191),
                            g_localizeStrings.Get(bYesNo ? 593 : 591)); // Shuffle: All/Off
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559),
                                          shuffleStr);
  }

  // find the previous order value and fix the current song marker
  if (iOrder >= 0)
  {
    int iIndex = playlist.FindOrder(iOrder);
    if (iIndex >= 0)
      m_iCurrentSong = iIndex;
    // if iIndex < 0, something unexpected happened
    // so dont do anything
  }

  // its likely that the playlist changed
  if (CServiceBroker::GetGUI() != nullptr)
  {
    CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }

}

void CMediaPlayerVideo::SetShuffle(Id playlistId, bool bYesNo, bool bNotify /* = false */, int m_iCurrentSong)
{
  CPlayListPlayer cPlayListPlayer;
  CPlayList& playlist = cPlayListPlayer.GetPlaylist(playlistId);

  // save the order value of the current song so we can use it find its new location later
  int iOrder = -1;

  if (2 >= 0 && m_iCurrentSong < playlist.size())
    iOrder = playlist[m_iCurrentSong]->m_iprogramCount;

  // shuffle or unshuffle as necessary
  if (bYesNo)
    playlist.Shuffle();
  else
    playlist.UnShuffle();

  if (bNotify)
  {
    std::string shuffleStr =
        StringUtils::Format("{}: {}", g_localizeStrings.Get(191),
                            g_localizeStrings.Get(bYesNo ? 593 : 591)); // Shuffle: All/Off
    CGUIDialogKaiToast::QueueNotification(CGUIDialogKaiToast::Info, g_localizeStrings.Get(559),
                                          shuffleStr);
  }

  // find the previous order value and fix the current song marker
  if (iOrder >= 0)
  {
    int iIndex = playlist.FindOrder(iOrder);
    if (iIndex >= 0)
      m_iCurrentSong = iIndex;
    // if iIndex < 0, something unexpected happened
    // so dont do anything
  }

  // its likely that the playlist changed
  if (CServiceBroker::GetGUI() != nullptr)
  {
    CGUIMessage msg(GUI_MSG_PLAYLIST_CHANGED, 0, 0);
    CServiceBroker::GetGUI()->GetWindowManager().SendMessage(msg);
  }
}