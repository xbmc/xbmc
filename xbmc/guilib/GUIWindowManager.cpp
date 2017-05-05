/*
 *      Copyright (C) 2005-2015 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIWindowManager.h"
#include "GUIAudioManager.h"
#include "GUIDialog.h"
#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "messaging/helpers/DialogHelper.h"
#include "GUIPassword.h"
#include "GUIInfoManager.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "settings/AdvancedSettings.h"
#include "addons/Skin.h"
#include "GUITexture.h"
#include "utils/Variant.h"
#include "input/Key.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "utils/SeekHandler.h"

#include "windows/GUIWindowHome.h"
#include "events/windows/GUIWindowEventLog.h"
#include "favourites/GUIDialogFavourites.h"
#include "settings/windows/GUIWindowSettings.h"
#include "windows/GUIWindowFileManager.h"
#include "settings/windows/GUIWindowSettingsCategory.h"
#include "music/windows/GUIWindowMusicPlaylist.h"
#include "music/windows/GUIWindowMusicNav.h"
#include "music/windows/GUIWindowMusicPlaylistEditor.h"
#include "video/windows/GUIWindowVideoPlaylist.h"
#include "music/dialogs/GUIDialogMusicInfo.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoNav.h"
#include "profiles/windows/GUIWindowSettingsProfile.h"
#ifdef HAS_GL
#include "rendering/gl/GUIWindowTestPatternGL.h"
#endif
#ifdef HAS_DX
#include "rendering/dx/GUIWindowTestPatternDX.h"
#endif
#include "settings/windows/GUIWindowSettingsScreenCalibration.h"
#include "programs/GUIWindowPrograms.h"
#include "pictures/GUIWindowPictures.h"
#include "windows/GUIWindowWeather.h"
#include "windows/GUIWindowLoginScreen.h"
#include "addons/GUIWindowAddonBrowser.h"
#include "music/windows/GUIWindowVisualisation.h"
#include "windows/GUIWindowDebugInfo.h"
#include "windows/GUIWindowPointer.h"
#include "windows/GUIWindowSystemInfo.h"
#include "windows/GUIWindowScreensaver.h"
#include "windows/GUIWindowScreensaverDim.h"
#include "pictures/GUIWindowSlideShow.h"
#include "windows/GUIWindowSplash.h"
#include "windows/GUIWindowStartup.h"
#include "video/windows/GUIWindowFullScreen.h"
#include "video/dialogs/GUIDialogVideoOSD.h"

// Dialog includes
#include "music/dialogs/GUIDialogMusicOSD.h"
#include "music/dialogs/GUIDialogVisualisationPresetList.h"
#include "dialogs/GUIDialogTextViewer.h"
#include "network/GUIDialogNetworkSetup.h"
#include "dialogs/GUIDialogMediaSource.h"
#ifdef HAS_GL
#include "video/dialogs/GUIDialogCMSSettings.h"
#endif
#include "video/dialogs/GUIDialogVideoSettings.h"
#include "video/dialogs/GUIDialogAudioSubtitleSettings.h"
#include "video/dialogs/GUIDialogVideoBookmarks.h"
#include "profiles/dialogs/GUIDialogProfileSettings.h"
#include "profiles/dialogs/GUIDialogLockSettings.h"
#include "settings/dialogs/GUIDialogContentSettings.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogKeyboardGeneric.h"
#include "dialogs/GUIDialogKeyboardTouch.h"
#include "dialogs/GUIDialogYesNo.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "dialogs/GUIDialogSelect.h"
#include "dialogs/GUIDialogSeekBar.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogVolumeBar.h"
#include "dialogs/GUIDialogNumeric.h"
#include "dialogs/GUIDialogGamepad.h"
#include "dialogs/GUIDialogSubMenu.h"
#include "dialogs/GUIDialogButtonMenu.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogPlayerControls.h"
#include "dialogs/GUIDialogPlayerProcessInfo.h"
#include "music/dialogs/GUIDialogSongInfo.h"
#include "dialogs/GUIDialogSmartPlaylistEditor.h"
#include "dialogs/GUIDialogSmartPlaylistRule.h"
#include "pictures/GUIDialogPictureInfo.h"
#include "addons/settings/GUIDialogAddonSettings.h"
#include "addons/GUIDialogAddonInfo.h"
#ifdef HAS_LINUX_NETWORK
#include "network/GUIDialogAccessPoints.h"
#endif

/* PVR related include Files */
#include "pvr/PVRManager.h"
#include "pvr/windows/GUIWindowPVRChannels.h"
#include "pvr/windows/GUIWindowPVRRecordings.h"
#include "pvr/windows/GUIWindowPVRGuide.h"
#include "pvr/windows/GUIWindowPVRTimers.h"
#include "pvr/windows/GUIWindowPVRTimerRules.h"
#include "pvr/windows/GUIWindowPVRSearch.h"
#include "pvr/dialogs/GUIDialogPVRChannelManager.h"
#include "pvr/dialogs/GUIDialogPVRChannelsOSD.h"
#include "pvr/dialogs/GUIDialogPVRGroupManager.h"
#include "pvr/dialogs/GUIDialogPVRGuideInfo.h"
#include "pvr/dialogs/GUIDialogPVRChannelGuide.h"
#include "pvr/dialogs/GUIDialogPVRGuideSearch.h"
#include "pvr/dialogs/GUIDialogPVRRadioRDSInfo.h"
#include "pvr/dialogs/GUIDialogPVRRecordingInfo.h"
#include "pvr/dialogs/GUIDialogPVRTimerSettings.h"

#include "video/dialogs/GUIDialogTeletext.h"
#include "dialogs/GUIDialogSlider.h"
#include "dialogs/GUIDialogPlayEject.h"
#include "dialogs/GUIDialogMediaFilter.h"
#include "video/dialogs/GUIDialogSubtitles.h"

#include "peripherals/dialogs/GUIDialogPeripheralSettings.h"
#include "addons/interfaces/AddonInterfaces.h"

/* Game related include files */
#include "games/controllers/windows/GUIControllerWindow.h"
#include "games/windows/GUIWindowGames.h"

using namespace PVR;
using namespace PERIPHERALS;
using namespace KODI::MESSAGING;

CGUIWindowManager::CGUIWindowManager()
{
  m_pCallback = nullptr;
  m_iNested = 0;
  m_initialized = false;
}

CGUIWindowManager::~CGUIWindowManager()
{
}

void CGUIWindowManager::Initialize()
{
  m_tracker.SelectAlgorithm();

  m_initialized = true;

  LoadNotOnDemandWindows();

  CApplicationMessenger::GetInstance().RegisterReceiver(this);
}

void CGUIWindowManager::CreateWindows()
{
  Add(new CGUIWindowHome);
  Add(new CGUIWindowPrograms);
  Add(new CGUIWindowPictures);
  Add(new CGUIWindowFileManager);
  Add(new CGUIWindowSettings);
  Add(new CGUIWindowSystemInfo);
#ifdef HAS_GL
  Add(new CGUIWindowTestPatternGL);
#endif
#ifdef HAS_DX
  Add(new CGUIWindowTestPatternDX);
#endif
  Add(new CGUIWindowSettingsScreenCalibration);
  Add(new CGUIWindowSettingsCategory);
  Add(new CGUIWindowVideoNav);
  Add(new CGUIWindowVideoPlaylist);
  Add(new CGUIWindowLoginScreen);
  Add(new CGUIWindowSettingsProfile);
  Add(new CGUIWindow(WINDOW_SKIN_SETTINGS, "SkinSettings.xml"));
  Add(new CGUIWindowAddonBrowser);
  Add(new CGUIWindowScreensaverDim);
  Add(new CGUIWindowDebugInfo);
  Add(new CGUIWindowPointer);
  Add(new CGUIDialogYesNo);
  Add(new CGUIDialogProgress);
  Add(new CGUIDialogExtendedProgressBar);
  Add(new CGUIDialogKeyboardGeneric);
  Add(new CGUIDialogKeyboardTouch);
  Add(new CGUIDialogVolumeBar);
  Add(new CGUIDialogSeekBar);
  Add(new CGUIDialogSubMenu);
  Add(new CGUIDialogContextMenu);
  Add(new CGUIDialogKaiToast);
  Add(new CGUIDialogNumeric);
  Add(new CGUIDialogGamepad);
  Add(new CGUIDialogButtonMenu);
  Add(new CGUIDialogPlayerControls);
  Add(new CGUIDialogPlayerProcessInfo);
  Add(new CGUIDialogSlider);
  Add(new CGUIDialogMusicOSD);
  Add(new CGUIDialogVisualisationPresetList);
#ifdef HAS_GL
  Add(new CGUIDialogCMSSettings);
#endif
  Add(new CGUIDialogVideoSettings);
  Add(new CGUIDialogAudioSubtitleSettings);
  Add(new CGUIDialogVideoBookmarks);
  // Don't add the filebrowser dialog - it's created and added when it's needed
  Add(new CGUIDialogNetworkSetup);
  Add(new CGUIDialogMediaSource);
  Add(new CGUIDialogProfileSettings);
  Add(new CGUIDialogFavourites);
  Add(new CGUIDialogSongInfo);
  Add(new CGUIDialogSmartPlaylistEditor);
  Add(new CGUIDialogSmartPlaylistRule);
  Add(new CGUIDialogBusy);
  Add(new CGUIDialogPictureInfo);
  Add(new CGUIDialogAddonInfo);
  Add(new CGUIDialogAddonSettings);
#ifdef HAS_LINUX_NETWORK
  Add(new CGUIDialogAccessPoints);
#endif

  Add(new CGUIDialogLockSettings);

  Add(new CGUIDialogContentSettings);

  Add(new CGUIDialogPlayEject);

  Add(new CGUIDialogPeripheralSettings);

  Add(new CGUIDialogMediaFilter);
  Add(new CGUIDialogSubtitles);

  Add(new CGUIWindowMusicPlayList);
  Add(new CGUIWindowMusicNav);
  Add(new CGUIWindowMusicPlaylistEditor);

  /* Load PVR related Windows and Dialogs */
  Add(new CGUIDialogTeletext);
  Add(new CGUIWindowPVRTVChannels);
  Add(new CGUIWindowPVRTVRecordings);
  Add(new CGUIWindowPVRTVGuide);
  Add(new CGUIWindowPVRTVTimers);
  Add(new CGUIWindowPVRTVTimerRules);
  Add(new CGUIWindowPVRTVSearch);
  Add(new CGUIWindowPVRRadioChannels);
  Add(new CGUIWindowPVRRadioRecordings);
  Add(new CGUIWindowPVRRadioGuide);
  Add(new CGUIWindowPVRRadioTimers);
  Add(new CGUIWindowPVRRadioTimerRules);
  Add(new CGUIWindowPVRRadioSearch);
  Add(new CGUIDialogPVRRadioRDSInfo);
  Add(new CGUIDialogPVRGuideInfo);
  Add(new CGUIDialogPVRRecordingInfo);
  Add(new CGUIDialogPVRTimerSettings);
  Add(new CGUIDialogPVRGroupManager);
  Add(new CGUIDialogPVRChannelManager);
  Add(new CGUIDialogPVRGuideSearch);
  Add(new CGUIDialogPVRChannelsOSD);
  Add(new CGUIDialogPVRChannelGuide);

  Add(new CGUIDialogSelect);
  Add(new CGUIDialogMusicInfo);
  Add(new CGUIDialogOK);
  Add(new CGUIDialogVideoInfo);
  Add(new CGUIDialogTextViewer);
  Add(new CGUIWindowFullScreen);
  Add(new CGUIWindowVisualisation);
  Add(new CGUIWindowSlideShow);

  Add(new CGUIDialogVideoOSD);
  Add(new CGUIWindowScreensaver);
  Add(new CGUIWindowWeather);
  Add(new CGUIWindowStartup);
  Add(new CGUIWindowSplash);

  Add(new CGUIWindowEventLog);

  Add(new GAME::CGUIControllerWindow);
  Add(new GAME::CGUIWindowGames);
}

bool CGUIWindowManager::DestroyWindows()
{
  try
  {
    DestroyWindow(WINDOW_SPLASH);
    DestroyWindow(WINDOW_MUSIC_PLAYLIST);
    DestroyWindow(WINDOW_MUSIC_PLAYLIST_EDITOR);
    DestroyWindow(WINDOW_MUSIC_NAV);
    DestroyWindow(WINDOW_DIALOG_MUSIC_INFO);
    DestroyWindow(WINDOW_DIALOG_VIDEO_INFO);
    DestroyWindow(WINDOW_VIDEO_PLAYLIST);
    DestroyWindow(WINDOW_VIDEO_NAV);
    DestroyWindow(WINDOW_FILES);
    DestroyWindow(WINDOW_DIALOG_YES_NO);
    DestroyWindow(WINDOW_DIALOG_PROGRESS);
    DestroyWindow(WINDOW_DIALOG_NUMERIC);
    DestroyWindow(WINDOW_DIALOG_GAMEPAD);
    DestroyWindow(WINDOW_DIALOG_SUB_MENU);
    DestroyWindow(WINDOW_DIALOG_BUTTON_MENU);
    DestroyWindow(WINDOW_DIALOG_CONTEXT_MENU);
    DestroyWindow(WINDOW_DIALOG_PLAYER_CONTROLS);
    DestroyWindow(WINDOW_DIALOG_PLAYER_PROCESS_INFO);
    DestroyWindow(WINDOW_DIALOG_MUSIC_OSD);
    DestroyWindow(WINDOW_DIALOG_VIS_PRESET_LIST);
    DestroyWindow(WINDOW_DIALOG_SELECT);
    DestroyWindow(WINDOW_DIALOG_OK);
    DestroyWindow(WINDOW_DIALOG_KEYBOARD);
    DestroyWindow(WINDOW_DIALOG_KEYBOARD_TOUCH);
    DestroyWindow(WINDOW_FULLSCREEN_VIDEO);
    DestroyWindow(WINDOW_DIALOG_PROFILE_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_LOCK_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_NETWORK_SETUP);
    DestroyWindow(WINDOW_DIALOG_MEDIA_SOURCE);
    DestroyWindow(WINDOW_DIALOG_CMS_OSD_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_VIDEO_BOOKMARKS);
    DestroyWindow(WINDOW_DIALOG_CONTENT_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_FAVOURITES);
    DestroyWindow(WINDOW_DIALOG_SONG_INFO);
    DestroyWindow(WINDOW_DIALOG_SMART_PLAYLIST_EDITOR);
    DestroyWindow(WINDOW_DIALOG_SMART_PLAYLIST_RULE);
    DestroyWindow(WINDOW_DIALOG_BUSY);
    DestroyWindow(WINDOW_DIALOG_PICTURE_INFO);
    DestroyWindow(WINDOW_DIALOG_ADDON_INFO);
    DestroyWindow(WINDOW_DIALOG_ADDON_SETTINGS);
    DestroyWindow(WINDOW_DIALOG_ACCESS_POINTS);
    DestroyWindow(WINDOW_DIALOG_SLIDER);
    DestroyWindow(WINDOW_DIALOG_MEDIA_FILTER);
    DestroyWindow(WINDOW_DIALOG_SUBTITLES);

    /* Delete PVR related windows and dialogs */
    DestroyWindow(WINDOW_TV_CHANNELS);
    DestroyWindow(WINDOW_TV_RECORDINGS);
    DestroyWindow(WINDOW_TV_GUIDE);
    DestroyWindow(WINDOW_TV_TIMERS);
    DestroyWindow(WINDOW_TV_TIMER_RULES);
    DestroyWindow(WINDOW_TV_SEARCH);
    DestroyWindow(WINDOW_RADIO_CHANNELS);
    DestroyWindow(WINDOW_RADIO_RECORDINGS);
    DestroyWindow(WINDOW_RADIO_GUIDE);
    DestroyWindow(WINDOW_RADIO_TIMERS);
    DestroyWindow(WINDOW_RADIO_TIMER_RULES);
    DestroyWindow(WINDOW_RADIO_SEARCH);
    DestroyWindow(WINDOW_DIALOG_PVR_GUIDE_INFO);
    DestroyWindow(WINDOW_DIALOG_PVR_RECORDING_INFO);
    DestroyWindow(WINDOW_DIALOG_PVR_TIMER_SETTING);
    DestroyWindow(WINDOW_DIALOG_PVR_GROUP_MANAGER);
    DestroyWindow(WINDOW_DIALOG_PVR_CHANNEL_MANAGER);
    DestroyWindow(WINDOW_DIALOG_PVR_GUIDE_SEARCH);
    DestroyWindow(WINDOW_DIALOG_PVR_CHANNEL_SCAN);
    DestroyWindow(WINDOW_DIALOG_PVR_RADIO_RDS_INFO);
    DestroyWindow(WINDOW_DIALOG_PVR_UPDATE_PROGRESS);
    DestroyWindow(WINDOW_DIALOG_PVR_OSD_CHANNELS);
    DestroyWindow(WINDOW_DIALOG_PVR_CHANNEL_GUIDE);
    DestroyWindow(WINDOW_DIALOG_OSD_TELETEXT);

    DestroyWindow(WINDOW_DIALOG_TEXT_VIEWER);
    DestroyWindow(WINDOW_DIALOG_PLAY_EJECT);
    DestroyWindow(WINDOW_STARTUP_ANIM);
    DestroyWindow(WINDOW_LOGIN_SCREEN);
    DestroyWindow(WINDOW_VISUALISATION);
    DestroyWindow(WINDOW_SETTINGS_MENU);
    DestroyWindow(WINDOW_SETTINGS_PROFILES);
    DestroyWindow(WINDOW_TEST_PATTERN);
    DestroyWindow(WINDOW_SCREEN_CALIBRATION);
    DestroyWindow(WINDOW_SYSTEM_INFORMATION);
    DestroyWindow(WINDOW_SCREENSAVER);
    DestroyWindow(WINDOW_DIALOG_VIDEO_OSD);
    DestroyWindow(WINDOW_SLIDESHOW);
    DestroyWindow(WINDOW_ADDON_BROWSER);
    DestroyWindow(WINDOW_SKIN_SETTINGS);

    DestroyWindow(WINDOW_HOME);
    DestroyWindow(WINDOW_PROGRAMS);
    DestroyWindow(WINDOW_PICTURES);
    DestroyWindow(WINDOW_WEATHER);
    DestroyWindow(WINDOW_DIALOG_GAME_CONTROLLERS);
    DestroyWindow(WINDOW_GAMES);

    Remove(WINDOW_SETTINGS_SERVICE);
    Remove(WINDOW_SETTINGS_MYPVR);
    Remove(WINDOW_SETTINGS_PLAYER);
    Remove(WINDOW_SETTINGS_MEDIA);
    Remove(WINDOW_SETTINGS_INTERFACE);
    Remove(WINDOW_SETTINGS_MYGAMES);
    DestroyWindow(WINDOW_SETTINGS_SYSTEM);  // all the settings categories

    Remove(WINDOW_DIALOG_KAI_TOAST);
    Remove(WINDOW_DIALOG_SEEK_BAR);
    Remove(WINDOW_DIALOG_VOLUME_BAR);

    DestroyWindow(WINDOW_EVENT_LOG);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Exception in CGUIWindowManager::DestroyWindows()");
    return false;
  }

  return true;
}

void CGUIWindowManager::DestroyWindow(int id)
{
  CSingleLock lock(g_graphicsContext);
  CGUIWindow *pWindow = GetWindow(id);
  if (pWindow)
  {
    Remove(id);
    pWindow->FreeResources(true);
    delete pWindow;
  }
}

bool CGUIWindowManager::SendMessage(int message, int senderID, int destID, int param1, int param2)
{
  CGUIMessage msg(message, senderID, destID, param1, param2);
  return SendMessage(msg);
}

bool CGUIWindowManager::SendMessage(CGUIMessage& message)
{
  bool handled = false;
//  CLog::Log(LOGDEBUG,"SendMessage: mess=%d send=%d control=%d param1=%d", message.GetMessage(), message.GetSenderId(), message.GetControlId(), message.GetParam1());
  // Send the message to all none window targets
  for (int i = 0; i < int(m_vecMsgTargets.size()); i++)
  {
    IMsgTargetCallback* pMsgTarget = m_vecMsgTargets[i];

    if (pMsgTarget)
    {
      if (pMsgTarget->OnMessage( message )) handled = true;
    }
  }

  //  A GUI_MSG_NOTIFY_ALL is send to any active modal dialog
  //  and all windows whether they are active or not
  if (message.GetMessage()==GUI_MSG_NOTIFY_ALL)
  {
    CSingleLock lock(g_graphicsContext);

    for (auto it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
    {
      (*it)->OnMessage(message);
    }

    for (const auto& entry : m_mapWindows)
    {
      entry.second->OnMessage(message);
    }

    return true;
  }

  // Normal messages are sent to:
  // 1. All active modeless dialogs
  // 2. The topmost dialog that accepts the message
  // 3. The underlying window (only if it is the sender or receiver if a modal dialog is active)

  bool hasModalDialog(false);
  bool modalAcceptedMessage(false);
  // don't use an iterator for this loop, as some messages mean that m_activeDialogs is altered,
  // which will invalidate any iterator
  CSingleLock lock(g_graphicsContext);
  unsigned int topWindow = m_activeDialogs.size();
  while (topWindow)
  {
    CGUIWindow* dialog = m_activeDialogs[--topWindow];
    lock.Leave();
    if (!modalAcceptedMessage && dialog->IsModalDialog())
    { // modal window
      hasModalDialog = true;
      if (!modalAcceptedMessage && dialog->OnMessage( message ))
      {
        modalAcceptedMessage = handled = true;
      }
    }
    else if (!dialog->IsModalDialog())
    { // modeless
      if (dialog->OnMessage( message ))
        handled = true;
    }
    lock.Enter();
    if (topWindow > m_activeDialogs.size())
      topWindow = m_activeDialogs.size();
  }
  lock.Leave();

  // now send to the underlying window
  CGUIWindow* window = GetWindow(GetActiveWindow());
  if (window)
  {
    if (hasModalDialog)
    {
      // only send the message to the underlying window if it's the recipient
      // or sender (or we have no sender)
      if (message.GetSenderId() == window->GetID() ||
          message.GetControlId() == window->GetID() ||
          message.GetSenderId() == 0 )
      {
        if (window->OnMessage(message)) handled = true;
      }
    }
    else
    {
      if (window->OnMessage(message)) handled = true;
    }
  }
  return handled;
}

bool CGUIWindowManager::SendMessage(CGUIMessage& message, int window)
{
  if (window == 0)
    // send to no specified windows.
    return SendMessage(message);
  CGUIWindow* pWindow = GetWindow(window);
  if(pWindow)
    return pWindow->OnMessage(message);
  else
    return false;
}

void CGUIWindowManager::AddUniqueInstance(CGUIWindow *window)
{
  CSingleLock lock(g_graphicsContext);
  // increment our instance (upper word of windowID)
  // until we get a window we don't have
  int instance = 0;
  while (GetWindow(window->GetID()))
    window->SetID(window->GetID() + (++instance << 16));
  Add(window);
}

void CGUIWindowManager::Add(CGUIWindow* pWindow)
{
  if (!pWindow)
  {
    CLog::Log(LOGERROR, "Attempted to add a NULL window pointer to the window manager.");
    return;
  }
  // push back all the windows if there are more than one covered by this class
  CSingleLock lock(g_graphicsContext);

  for (int id : pWindow->GetIDRange())
  {
    auto it = m_mapWindows.find(id);
    if (it != m_mapWindows.end())
    {
      CLog::Log(LOGERROR, "Error, trying to add a second window with id %u "
                          "to the window manager", id);
      return;
    }

    m_mapWindows.insert(std::make_pair(id, pWindow));
  }
}

void CGUIWindowManager::AddCustomWindow(CGUIWindow* pWindow)
{
  CSingleLock lock(g_graphicsContext);
  Add(pWindow);
  m_vecCustomWindows.push_back(pWindow);
}

void CGUIWindowManager::RegisterDialog(CGUIWindow* dialog)
{
  CSingleLock lock(g_graphicsContext);
  // only add the window if it does not exists
  for (const auto& window : m_activeDialogs)
  {
    if (window->GetID() == dialog->GetID())
      return;
  }
  m_activeDialogs.push_back(dialog);
}

void CGUIWindowManager::Remove(int id)
{
  CSingleLock lock(g_graphicsContext);

  auto it = m_mapWindows.find(id);
  if (it != m_mapWindows.end())
  {
    CGUIWindow *window = it->second;
    m_activeDialogs.erase(std::remove_if(m_activeDialogs.begin(),
                                         m_activeDialogs.end(),
                                         [window](CGUIWindow* w){ return w == window; }),
                          m_activeDialogs.end());
    m_mapWindows.erase(it);
  }
  else
  {
    CLog::Log(LOGWARNING, "Attempted to remove window %u "
                          "from the window manager when it didn't exist",
              id);
  }
}

// removes and deletes the window.  Should only be called
// from the class that created the window using new.
void CGUIWindowManager::Delete(int id)
{
  CSingleLock lock(g_graphicsContext);
  CGUIWindow *pWindow = GetWindow(id);
  if (pWindow)
  {
    Remove(id);
    m_deleteWindows.push_back(pWindow);
  }
}

void CGUIWindowManager::PreviousWindow()
{
  // deactivate any window
  CSingleLock lock(g_graphicsContext);
  CLog::Log(LOGDEBUG,"CGUIWindowManager::PreviousWindow: Deactivate");
  int currentWindow = GetActiveWindow();
  CGUIWindow *pCurrentWindow = GetWindow(currentWindow);
  if (!pCurrentWindow)
    return;     // no windows or window history yet

  // check to see whether our current window has a <previouswindow> tag
  if (pCurrentWindow->GetPreviousWindow() != WINDOW_INVALID)
  {
    //! @todo we may need to test here for the
    //!       whether our history should be changed

    // don't reactivate the previouswindow if it is ourselves.
    if (currentWindow != pCurrentWindow->GetPreviousWindow())
      ActivateWindow(pCurrentWindow->GetPreviousWindow());
    return;
  }
  // get the previous window in our stack
  if (m_windowHistory.size() < 2)
  {
    // no previous window history yet - check if we should just activate home
    if (GetActiveWindow() != WINDOW_INVALID && GetActiveWindow() != WINDOW_HOME)
    {
      CloseWindowSync(pCurrentWindow);
      ClearWindowHistory();
      ActivateWindow(WINDOW_HOME);
    }
    return;
  }
  m_windowHistory.pop();
  int previousWindow = GetActiveWindow();
  m_windowHistory.push(currentWindow);

  CGUIWindow *pNewWindow = GetWindow(previousWindow);
  if (!pNewWindow)
  {
    CLog::Log(LOGERROR, "Unable to activate the previous window");
    CloseWindowSync(pCurrentWindow);
    ClearWindowHistory();
    ActivateWindow(WINDOW_HOME);
    return;
  }

  // ok to go to the previous window now

  // pause game when leaving fullscreen or resume game when entering fullscreen
  if (g_application.m_pPlayer->IsPlayingGame())
  {
    if (previousWindow == WINDOW_FULLSCREEN_VIDEO && g_application.m_pPlayer->IsPaused())
      g_application.OnAction(ACTION_PAUSE);
    else if (currentWindow == WINDOW_FULLSCREEN_VIDEO && !g_application.m_pPlayer->IsPaused())
      g_application.OnAction(ACTION_PAUSE);
  }

  // tell our info manager which window we are going to
  g_infoManager.SetNextWindow(previousWindow);

  // deinitialize our window
  CloseWindowSync(pCurrentWindow);

  g_infoManager.SetNextWindow(WINDOW_INVALID);
  g_infoManager.SetPreviousWindow(currentWindow);

  // remove the current window off our window stack
  m_windowHistory.pop();

  // ok, initialize the new window
  CLog::Log(LOGDEBUG,"CGUIWindowManager::PreviousWindow: Activate new");
  CGUIMessage msg2(GUI_MSG_WINDOW_INIT, 0, 0, WINDOW_INVALID, GetActiveWindow());
  pNewWindow->OnMessage(msg2);

  g_infoManager.SetPreviousWindow(WINDOW_INVALID);
  return;
}

void CGUIWindowManager::ChangeActiveWindow(int newWindow, const std::string& strPath)
{
  std::vector<std::string> params;
  if (!strPath.empty())
    params.push_back(strPath);
  ActivateWindow(newWindow, params, true);
}

void CGUIWindowManager::ActivateWindow(int iWindowID, const std::string& strPath)
{
  std::vector<std::string> params;
  if (!strPath.empty())
    params.push_back(strPath);
  ActivateWindow(iWindowID, params, false);
}

void CGUIWindowManager::ForceActivateWindow(int iWindowID, const std::string& strPath)
{
  std::vector<std::string> params;
  if (!strPath.empty())
    params.push_back(strPath);
  ActivateWindow(iWindowID, params, false, true);
}

void CGUIWindowManager::ActivateWindow(int iWindowID, const std::vector<std::string>& params, bool swappingWindows /* = false */, bool force /* = false */)
{
  if (!g_application.IsCurrentThread())
  {
    // make sure graphics lock is not held
    CSingleExit leaveIt(g_graphicsContext);
    CApplicationMessenger::GetInstance().SendMsg(TMSG_GUI_ACTIVATE_WINDOW, iWindowID, swappingWindows ? 1 : 0, nullptr, "", params);
  }
  else
  {
    CSingleLock lock(g_graphicsContext);
    ActivateWindow_Internal(iWindowID, params, swappingWindows, force);
  }
}

void CGUIWindowManager::ActivateWindow_Internal(int iWindowID, const std::vector<std::string>& params, bool swappingWindows, bool force /* = false */)
{
  // translate virtual windows
  if (iWindowID == WINDOW_START)
  { // virtual start window
    iWindowID = g_SkinInfo->GetStartWindow();
  }

  // debug
  CLog::Log(LOGDEBUG, "Activating window ID: %i", iWindowID);

  if (!g_passwordManager.CheckMenuLock(iWindowID))
  {
    CLog::Log(LOGERROR, "MasterCode is Wrong: Window with id %d will not be loaded! Enter a correct MasterCode!", iWindowID);
    if (GetActiveWindow() == WINDOW_INVALID && iWindowID != WINDOW_HOME)
      ActivateWindow(WINDOW_HOME);
    return;
  }

  // first check existence of the window we wish to activate.
  CGUIWindow *pNewWindow = GetWindow(iWindowID);
  if (!pNewWindow)
  { // nothing to see here - move along
    CLog::Log(LOGERROR, "Unable to locate window with id %d.  Check skin files", iWindowID - WINDOW_HOME);
    if (GetActiveWindowID() == WINDOW_STARTUP_ANIM)
      ActivateWindow(WINDOW_HOME);
    return ;
  }
  else if (!pNewWindow->CanBeActivated())
  {
    if (GetActiveWindowID() == WINDOW_STARTUP_ANIM)
      ActivateWindow(WINDOW_HOME);
    return;
  }
  else if (pNewWindow->IsDialog())
  { // if we have a dialog, we do a DoModal() rather than activate the window
    if (!pNewWindow->IsDialogRunning())
    {
      CSingleExit exitit(g_graphicsContext);
      static_cast<CGUIDialog *>(pNewWindow)->Open(params.size() > 0 ? params[0] : "");
    }
    return;
  }

  // don't activate a window if there are active modal dialogs of type MODAL
  if (!force && HasModalDialog({ DialogModalityType::MODAL }))
  {
    CLog::Log(LOGINFO, "Activate of window '%i' refused because there are active modal dialogs", iWindowID);
    g_audioManager.PlayActionSound(CAction(ACTION_ERROR));
    return;
  }

  // pause game when leaving fullscreen or resume game when entering fullscreen
  if (g_application.m_pPlayer->IsPlayingGame())
  {
    if (GetActiveWindow() == WINDOW_FULLSCREEN_VIDEO && !g_application.m_pPlayer->IsPaused())
      g_application.OnAction(ACTION_PAUSE);
    else if (iWindowID == WINDOW_FULLSCREEN_VIDEO && g_application.m_pPlayer->IsPaused())
      g_application.OnAction(ACTION_PAUSE);
  }

  g_infoManager.SetNextWindow(iWindowID);

  // deactivate any window
  int currentWindow = GetActiveWindow();
  CGUIWindow *pWindow = GetWindow(currentWindow);
  if (pWindow)
    CloseWindowSync(pWindow, iWindowID);
  g_infoManager.SetNextWindow(WINDOW_INVALID);

  // Add window to the history list (we must do this before we activate it,
  // as all messages done in WINDOW_INIT will want to be sent to the new
  // topmost window).  If we are swapping windows, we pop the old window
  // off the history stack
  if (swappingWindows && !m_windowHistory.empty())
    m_windowHistory.pop();
  AddToWindowHistory(iWindowID);

  g_infoManager.SetPreviousWindow(currentWindow);
  // Send the init message
  CGUIMessage msg(GUI_MSG_WINDOW_INIT, 0, 0, currentWindow, iWindowID);
  msg.SetStringParams(params);
  pNewWindow->OnMessage(msg);
//  g_infoManager.SetPreviousWindow(WINDOW_INVALID);
}

void CGUIWindowManager::CloseDialogs(bool forceClose) const
{
  CSingleLock lock(g_graphicsContext);

  //This is to avoid an assert about out of bounds iterator
  //when m_activeDialogs happens to be empty
  if (m_activeDialogs.empty())
    return;

  auto activeDialogs = m_activeDialogs;
  for (const auto& window : activeDialogs)
  {
    window->Close(forceClose);
  }
}

void CGUIWindowManager::CloseInternalModalDialogs(bool forceClose) const
{
  CSingleLock lock(g_graphicsContext);
  if (m_activeDialogs.empty())
    return;

  auto activeDialogs = m_activeDialogs;
  for (const auto& window : activeDialogs)
  {
    if (window->IsModalDialog() && !IsAddonWindow(window->GetID()) && !IsPythonWindow(window->GetID()))
      window->Close(forceClose);
  }
}

void CGUIWindowManager::OnApplicationMessage(ThreadMessage* pMsg)
{
  switch (pMsg->dwMessage)
  {
  case TMSG_GUI_DIALOG_OPEN:
  {
    if (pMsg->lpVoid)
      static_cast<CGUIDialog*>(pMsg->lpVoid)->Open(pMsg->strParam);
    else
    {
      CGUIDialog* pDialog = static_cast<CGUIDialog*>(GetWindow(pMsg->param1));
      if (pDialog)
        pDialog->Open(pMsg->strParam);
    }
  }
  break;

  case TMSG_GUI_WINDOW_CLOSE:
  {
    CGUIWindow *window = static_cast<CGUIWindow *>(pMsg->lpVoid);
    if (window)
      window->Close((pMsg->param1 & 0x1) ? true : false, pMsg->param1, (pMsg->param1 & 0x2) ? true : false);
  }
  break;

  case TMSG_GUI_ACTIVATE_WINDOW:
  {
    ActivateWindow(pMsg->param1, pMsg->params, pMsg->param2 > 0);
  }
  break;

  case TMSG_GUI_ADDON_DIALOG:
  {
    if (pMsg->lpVoid)
    {
      ADDON::CAddonInterfaces::OnApplicationMessage(pMsg);
    }
  }
  break;

#ifdef HAS_PYTHON
  case TMSG_GUI_PYTHON_DIALOG:
  {
    // This hack is not much better but at least I don't need to make ApplicationMessenger
    //  know about Addon (Python) specific classes.
    CAction caction(pMsg->param1);
    static_cast<CGUIWindow*>(pMsg->lpVoid)->OnAction(caction);
  }
  break;
#endif

  case TMSG_GUI_ACTION:
  {
    if (pMsg->lpVoid)
    {
      CAction *action = static_cast<CAction *>(pMsg->lpVoid);
      if (pMsg->param1 == WINDOW_INVALID)
        g_application.OnAction(*action);
      else
      {
        CGUIWindow *pWindow = GetWindow(pMsg->param1);
        if (pWindow)
          pWindow->OnAction(*action);
        else
          CLog::Log(LOGWARNING, "Failed to get window with ID %i to send an action to", pMsg->param1);
      }
      delete action;
    }
  }
  break;

  case TMSG_GUI_MESSAGE:
    if (pMsg->lpVoid)
    {
      CGUIMessage *message = static_cast<CGUIMessage *>(pMsg->lpVoid);
      SendMessage(*message, pMsg->param1);
      delete message;
    }
    break;

  case TMSG_GUI_DIALOG_YESNO:
    if (!pMsg->lpVoid && pMsg->param1 < 0 && pMsg->param2 < 0)
      return;

    auto dialog = static_cast<CGUIDialogYesNo*>(GetWindow(WINDOW_DIALOG_YES_NO));
    if (!dialog)
      return;

    if (pMsg->lpVoid)
      pMsg->SetResult(dialog->ShowAndGetInput(*static_cast<HELPERS::DialogYesNoMessage*>(pMsg->lpVoid)));
    else
    {
      HELPERS::DialogYesNoMessage options;
      options.heading = pMsg->param1;
      options.text = pMsg->param2;
      pMsg->SetResult(dialog->ShowAndGetInput(options));
    }

    break;
  }
}

int CGUIWindowManager::GetMessageMask()
{
  return TMSG_MASK_WINDOWMANAGER;
}

bool CGUIWindowManager::OnAction(const CAction &action) const
{
  CSingleLock lock(g_graphicsContext);
  unsigned int topMost = m_activeDialogs.size();
  while (topMost)
  {
    CGUIWindow *dialog = m_activeDialogs[--topMost];
    lock.Leave();
    if (dialog->IsModalDialog())
    { // we have the topmost modal dialog
      if (!dialog->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
      {
        bool fallThrough = (dialog->GetID() == WINDOW_DIALOG_FULLSCREEN_INFO);
        if (dialog->OnAction(action))
          return true;
        // dialog didn't want the action - we'd normally return false
        // but for some dialogs we want to drop the actions through
        if (fallThrough)
          break;
        return false;
      }
      CLog::Log(LOGWARNING, "CGUIWindowManager - %s - ignoring action %i, because topmost modal dialog closing animation is running",
                __FUNCTION__, action.GetID());
      return true; // do nothing with the action until the anim is finished
    }
    lock.Enter();
    if (topMost > m_activeDialogs.size())
      topMost = m_activeDialogs.size();
  }
  lock.Leave();
  CGUIWindow* window = GetWindow(GetActiveWindow());
  if (window)
    return window->OnAction(action);
  return false;
}

bool RenderOrderSortFunction(CGUIWindow *first, CGUIWindow *second)
{
  return first->GetRenderOrder() < second->GetRenderOrder();
}

void CGUIWindowManager::Process(unsigned int currentTime)
{
  assert(g_application.IsCurrentThread());
  CSingleLock lock(g_graphicsContext);

  CDirtyRegionList dirtyregions;

  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->DoProcess(currentTime, dirtyregions);

  // process all dialogs - visibility may change etc.
  for (const auto& entry : m_mapWindows)
  {
    CGUIWindow *pWindow = entry.second;
    if (pWindow && pWindow->IsDialog())
      pWindow->DoProcess(currentTime, dirtyregions);
  }

  for (CDirtyRegionList::iterator itr = dirtyregions.begin(); itr != dirtyregions.end(); ++itr)
    m_tracker.MarkDirtyRegion(*itr);
}

void CGUIWindowManager::MarkDirty()
{
  m_tracker.MarkDirtyRegion(CRect(0, 0, float(g_graphicsContext.GetWidth()), float(g_graphicsContext.GetHeight())));
}

void CGUIWindowManager::MarkDirty(const CRect& rect)
{
  m_tracker.MarkDirtyRegion(rect);
}

void CGUIWindowManager::RenderPass() const
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
  {
    pWindow->ClearBackground();
    pWindow->DoRender();
  }

  // we render the dialogs based on their render order.
  auto renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  for (const auto& window : renderList)
  {
    if (window->IsDialogRunning())
      window->DoRender();
  }
}

void CGUIWindowManager::RenderEx() const
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->RenderEx();

  // We don't call RenderEx for now on dialogs since it is used
  // to trigger non gui video rendering. We can activate it later at any time.
  /*
  vector<CGUIWindow *> &activeDialogs = m_activeDialogs;
  for (iDialog it = activeDialogs.begin(); it != activeDialogs.end(); ++it)
  {
    if ((*it)->IsDialogRunning())
      (*it)->RenderEx();
  }
  */
}

bool CGUIWindowManager::Render()
{
  assert(g_application.IsCurrentThread());
  CSingleExit lock(g_graphicsContext);

  CDirtyRegionList dirtyRegions = m_tracker.GetDirtyRegions();

  bool hasRendered = false;
  // If we visualize the regions we will always render the entire viewport
  if (g_advancedSettings.m_guiVisualizeDirtyRegions || g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_FILL_VIEWPORT_ALWAYS)
  {
    RenderPass();
    hasRendered = true;
  }
  else if (g_advancedSettings.m_guiAlgorithmDirtyRegions == DIRTYREGION_SOLVER_FILL_VIEWPORT_ON_CHANGE)
  {
    if (!dirtyRegions.empty())
    {
      RenderPass();
      hasRendered = true;
    }
  }
  else
  {
    for (CDirtyRegionList::const_iterator i = dirtyRegions.begin(); i != dirtyRegions.end(); ++i)
    {
      if (i->IsEmpty())
        continue;

      g_graphicsContext.SetScissors(*i);
      RenderPass();
      hasRendered = true;
    }
    g_graphicsContext.ResetScissors();
  }

  if (g_advancedSettings.m_guiVisualizeDirtyRegions)
  {
    g_graphicsContext.SetRenderingResolution(g_graphicsContext.GetResInfo(), false);
    const CDirtyRegionList &markedRegions  = m_tracker.GetMarkedRegions();
    for (CDirtyRegionList::const_iterator i = markedRegions.begin(); i != markedRegions.end(); ++i)
      CGUITexture::DrawQuad(*i, 0x0fff0000);
    for (CDirtyRegionList::const_iterator i = dirtyRegions.begin(); i != dirtyRegions.end(); ++i)
      CGUITexture::DrawQuad(*i, 0x4c00ff00);
  }

  return hasRendered;
}

void CGUIWindowManager::AfterRender()
{
  m_tracker.CleanMarkedRegions();

  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->AfterRender();

  // make copy of vector as we may remove items from it as we go
  auto activeDialogs = m_activeDialogs;
  for (const auto& window : activeDialogs)
  {
    if (window->IsDialogRunning())
      window->AfterRender();
  }
}

void CGUIWindowManager::FrameMove()
{
  assert(g_application.IsCurrentThread());
  CSingleLock lock(g_graphicsContext);

  if(m_iNested == 0)
  {
    // delete any windows queued for deletion
    for (const auto& window : m_deleteWindows)
    {
      // Free any window resources
      window->FreeResources(true);
      delete window;
    }
    m_deleteWindows.clear();
  }

  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->FrameMove();
  // update any dialogs - we take a copy of the vector as some dialogs may close themselves
  // during this call
  auto dialogs = m_activeDialogs;
  for (const auto& window : dialogs)
  {
    window->FrameMove();
  }

  g_infoManager.UpdateAVInfo();
}

CGUIDialog* CGUIWindowManager::GetDialog(int id) const
{
  CGUIWindow *window = GetWindow(id);
  if (window && window->IsDialog())
    return dynamic_cast<CGUIDialog*>(window);
  return nullptr;
}

CGUIWindow* CGUIWindowManager::GetWindow(int id) const
{
  if (id == 0 || id == WINDOW_INVALID)
    return nullptr;

  CSingleLock lock(g_graphicsContext);

  auto it = m_mapWindows.find(id);
  if (it != m_mapWindows.end())
    return it->second;
  return nullptr;
}

void CGUIWindowManager::ProcessRenderLoop(bool renderOnly /*= false*/)
{
  if (g_application.IsCurrentThread() && m_pCallback)
  {
    m_iNested++;
    if (!renderOnly)
      m_pCallback->Process();
    m_pCallback->FrameMove(!renderOnly);
    m_pCallback->Render();
    m_iNested--;
  }
}

void CGUIWindowManager::SetCallback(IWindowManagerCallback& callback)
{
  m_pCallback = &callback;
}

void CGUIWindowManager::DeInitialize()
{
  CSingleLock lock(g_graphicsContext);
  for (const auto& entry : m_mapWindows)
  {
    CGUIWindow* pWindow = entry.second;
    if (IsWindowActive(entry.first, false))
    {
      pWindow->DisableAnimations();
      pWindow->Close(true);
    }
    pWindow->ResetControlStates();
    pWindow->FreeResources(true);
  }
  UnloadNotOnDemandWindows();

  m_vecMsgTargets.erase( m_vecMsgTargets.begin(), m_vecMsgTargets.end() );

  // destroy our custom windows...
  for (int i = 0; i < int(m_vecCustomWindows.size()); i++)
  {
    CGUIWindow *pWindow = m_vecCustomWindows[i];
    RemoveFromWindowHistory(pWindow->GetID());
    Remove(pWindow->GetID());
    delete pWindow;
  }

  // clear our vectors of windows
  m_vecCustomWindows.clear();
  m_activeDialogs.clear();

  m_initialized = false;
}

/// \brief Unroute window
/// \param id ID of the window routed
void CGUIWindowManager::RemoveDialog(int id)
{
  CSingleLock lock(g_graphicsContext);
  m_activeDialogs.erase(std::remove_if(m_activeDialogs.begin(),
                                       m_activeDialogs.end(),
                                       [id](CGUIWindow* dialog) { return dialog->GetID() == id; }),
                         m_activeDialogs.end());
}

bool CGUIWindowManager::HasModalDialog(const std::vector<DialogModalityType>& types, bool ignoreClosing /* = true */) const
{
  CSingleLock lock(g_graphicsContext);
  for (const auto& window : m_activeDialogs)
  {
    if (window->IsDialog() &&
        window->IsModalDialog() &&
        (!ignoreClosing || !window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
    {
      if (!types.empty())
      {
        CGUIDialog *dialog = static_cast<CGUIDialog*>(window);
        for (const auto &type : types)
        {
          if (dialog->GetModalityType() == type)
            return true;
        }
      }
      else
        return true;
    }
  }
  return false;
}

bool CGUIWindowManager::HasVisibleModalDialog(const std::vector<DialogModalityType>& types) const
{
  return HasModalDialog(types, false);
}

bool CGUIWindowManager::HasDialogOnScreen() const
{
  return (m_activeDialogs.size() > 0);
}

/// \brief Get the ID of the top most routed window
/// \return id ID of the window or WINDOW_INVALID if no routed window available
int CGUIWindowManager::GetTopMostModalDialogID(bool ignoreClosing /*= false*/) const
{
  CSingleLock lock(g_graphicsContext);
  for (auto it = m_activeDialogs.rbegin(); it != m_activeDialogs.rend(); ++it)
  {
    CGUIWindow *dialog = *it;
    if (dialog->IsModalDialog() && (!ignoreClosing || !dialog->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
    { // have a modal window
      return dialog->GetID();
    }
  }
  return WINDOW_INVALID;
}

void CGUIWindowManager::SendThreadMessage(CGUIMessage& message, int window /*= 0*/)
{
  CSingleLock lock(m_critSection);

  CGUIMessage* msg = new CGUIMessage(message);
  m_vecThreadMessages.push_back( std::pair<CGUIMessage*,int>(msg,window) );
}

void CGUIWindowManager::DispatchThreadMessages()
{
  // This method only be called in the xbmc main thread.

  // XXX: for more info of this method
  //      check the pr here: https://github.com/xbmc/xbmc/pull/2253

  // As a thread message queue service, it should follow these rules:
  // 1. [Must] Thread safe, message can be pushed into queue in arbitrary thread context.
  // 2. Messages [must] be processed in dispatch message thread context with the same
  //    order as they be pushed into the queue.
  // 3. Dispatch function [must] support call itself during message process procedure,
  //    and do not break other rules listed here. to make it clear: in the
  //    SendMessage(), it could start another xbmc main thread loop, calling
  //    DispatchThreadMessages() in it's internal loop, this must be supported.
  // 4. During DispatchThreadMessages() processing, any new pushed message [should] not
  //    be processed by the current loop in DispatchThreadMessages(), prevent dead loop.
  // 5. If possible, queued messages can be removed by certain filter condition
  //    and not break above.

  CSingleLock lock(m_critSection);

  for(int msgCount = m_vecThreadMessages.size(); !m_vecThreadMessages.empty() && msgCount > 0; --msgCount)
  {
    // pop up one message per time to make messages be processed by order.
    // this will ensure rule No.2 & No.3
    CGUIMessage *pMsg = m_vecThreadMessages.front().first;
    int window = m_vecThreadMessages.front().second;
    m_vecThreadMessages.pop_front();

    lock.Leave();

    // XXX: during SendMessage(), there could be a deeper 'xbmc main loop' inited by e.g. doModal
    //      which may loop there and callback to DispatchThreadMessages() multiple times.
    if (window)
      SendMessage( *pMsg, window );
    else
      SendMessage( *pMsg );
    delete pMsg;

    lock.Enter();
  }
}

int CGUIWindowManager::RemoveThreadMessageByMessageIds(int *pMessageIDList)
{
  CSingleLock lock(m_critSection);
  int removedMsgCount = 0;
  for (std::list < std::pair<CGUIMessage*,int> >::iterator it = m_vecThreadMessages.begin();
       it != m_vecThreadMessages.end();)
  {
    CGUIMessage *pMsg = it->first;
    int *pMsgID;
    for(pMsgID = pMessageIDList; *pMsgID != 0; ++pMsgID)
      if (pMsg->GetMessage() == *pMsgID)
        break;
    if (*pMsgID)
    {
      it = m_vecThreadMessages.erase(it);
      delete pMsg;
      ++removedMsgCount;
    }
    else
    {
      ++it;
    }
  }
  return removedMsgCount;
}

void CGUIWindowManager::AddMsgTarget( IMsgTargetCallback* pMsgTarget )
{
  m_vecMsgTargets.push_back( pMsgTarget );
}

int CGUIWindowManager::GetActiveWindow() const
{
  if (!m_windowHistory.empty())
    return m_windowHistory.top();
  return WINDOW_INVALID;
}

int CGUIWindowManager::GetActiveWindowID() const
{
  // Get the currently active window
  int iWin = GetActiveWindow() & WINDOW_ID_MASK;

  // If there is a dialog active get the dialog id instead
  if (HasModalDialog())
    iWin = GetTopMostModalDialogID() & WINDOW_ID_MASK;

  // If the window is FullScreenVideo check for special cases
  if (iWin == WINDOW_FULLSCREEN_VIDEO)
  {
    // check if we're in a DVD menu
    if (g_application.m_pPlayer->IsInMenu())
      iWin = WINDOW_VIDEO_MENU;
    // check for LiveTV and switch to it's virtual window
    else if (CServiceBroker::GetPVRManager().IsStarted() && g_application.CurrentFileItem().HasPVRChannelInfoTag())
      iWin = WINDOW_FULLSCREEN_LIVETV;
    // special casing for numeric seek
    else if (CSeekHandler::GetInstance().HasTimeCode())
      iWin = WINDOW_VIDEO_TIME_SEEK;
    // check if a game is playing
    else if (g_application.m_pPlayer->IsPlayingGame())
      iWin = WINDOW_FULLSCREEN_GAME;
  }
  if (iWin == WINDOW_VISUALISATION)
  {
    // special casing for PVR radio
    if (CServiceBroker::GetPVRManager().IsStarted() && g_application.CurrentFileItem().HasPVRChannelInfoTag())
      iWin = WINDOW_FULLSCREEN_RADIO;
    // special casing for numeric seek
    else if (CSeekHandler::GetInstance().HasTimeCode())
      iWin = WINDOW_VIDEO_TIME_SEEK;
  }
  // Return the window id
  return iWin;
}

// same as GetActiveWindow() except it first grabs dialogs
int CGUIWindowManager::GetFocusedWindow() const
{
  int dialog = GetTopMostModalDialogID(true);
  if (dialog != WINDOW_INVALID)
    return dialog;

  return GetActiveWindow();
}

bool CGUIWindowManager::IsWindowActive(int id, bool ignoreClosing /* = true */) const
{
  // mask out multiple instances of the same window
  id &= WINDOW_ID_MASK;
  if ((GetActiveWindow() & WINDOW_ID_MASK) == id) return true;
  // run through the dialogs
  CSingleLock lock(g_graphicsContext);
  for (const auto& window : m_activeDialogs)
  {
    if ((window->GetID() & WINDOW_ID_MASK) == id && (!ignoreClosing || !window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return true;
  }
  return false; // window isn't active
}

bool CGUIWindowManager::IsWindowActive(const std::string &xmlFile, bool ignoreClosing /* = true */) const
{
  CSingleLock lock(g_graphicsContext);
  CGUIWindow *window = GetWindow(GetActiveWindow());
  if (window && StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), xmlFile))
    return true;
  // run through the dialogs
  for (const auto& window : m_activeDialogs)
  {
    if (StringUtils::EqualsNoCase(URIUtils::GetFileName(window->GetProperty("xmlfile").asString()), xmlFile) &&
        (!ignoreClosing || !window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE)))
      return true;
  }
  return false; // window isn't active
}

bool CGUIWindowManager::IsWindowVisible(int id) const
{
  return IsWindowActive(id, false);
}

bool CGUIWindowManager::IsWindowVisible(const std::string &xmlFile) const
{
  return IsWindowActive(xmlFile, false);
}

void CGUIWindowManager::LoadNotOnDemandWindows()
{
  CSingleLock lock(g_graphicsContext);
  for (const auto& entry : m_mapWindows)
  {
    CGUIWindow *pWindow = entry.second;
    if (pWindow->GetLoadType() == CGUIWindow::LOAD_ON_GUI_INIT)
    {
      pWindow->FreeResources(true);
      pWindow->Initialize();
    }
  }
}

void CGUIWindowManager::UnloadNotOnDemandWindows()
{
  CSingleLock lock(g_graphicsContext);
  for (const auto& entry : m_mapWindows)
  {
    CGUIWindow *pWindow = entry.second;
    if (pWindow->GetLoadType() == CGUIWindow::LOAD_ON_GUI_INIT ||
        pWindow->GetLoadType() == CGUIWindow::KEEP_IN_MEMORY)
    {
      pWindow->FreeResources(true);
    }
  }
}

void CGUIWindowManager::AddToWindowHistory(int newWindowID)
{
  // Check the window stack to see if this window is in our history,
  // and if so, pop all the other windows off the stack so that we
  // always have a predictable "Back" behaviour for each window
  std::stack<int> historySave = m_windowHistory;
  while (!historySave.empty())
  {
    if (historySave.top() == newWindowID)
      break;
    historySave.pop();
  }
  if (!historySave.empty())
  { // found window in history
    m_windowHistory = historySave;
  }
  else if (newWindowID != WINDOW_SPLASH)
  {
    // didn't find window in history - add it to the stack
    // but do not add the splash window to history, as we never want to travel back to it
    m_windowHistory.push(newWindowID);
  }
}

void CGUIWindowManager::RemoveFromWindowHistory(int windowID)
{
  std::stack<int> stack = m_windowHistory;

  // pop windows from stack until we found the window
  while (!stack.empty())
  {
    if (stack.top() == windowID)
      break;
    stack.pop();
  }

  // found window in history
  if (!stack.empty())
  {
    stack.pop(); // remove window from stack
    m_windowHistory = stack;
  }
}

CGUIWindow *CGUIWindowManager::GetTopMostDialog() const
{
  CSingleLock lock(g_graphicsContext);
  // find the window with the lowest render order
  auto renderList = m_activeDialogs;
  stable_sort(renderList.begin(), renderList.end(), RenderOrderSortFunction);

  if (!renderList.size())
    return nullptr;

  // return the last window in the list
  return *renderList.rbegin();
}

bool CGUIWindowManager::IsWindowTopMost(int id) const
{
  CGUIWindow *topMost = GetTopMostDialog();
  if (topMost && (topMost->GetID() & WINDOW_ID_MASK) == id)
    return true;
  return false;
}

bool CGUIWindowManager::IsWindowTopMost(const std::string &xmlFile) const
{
  CGUIWindow *topMost = GetTopMostDialog();
  if (topMost && StringUtils::EqualsNoCase(URIUtils::GetFileName(topMost->GetProperty("xmlfile").asString()), xmlFile))
    return true;
  return false;
}

void CGUIWindowManager::ClearWindowHistory()
{
  while (!m_windowHistory.empty())
    m_windowHistory.pop();
}

void CGUIWindowManager::CloseWindowSync(CGUIWindow *window, int nextWindowID /*= 0*/)
{
  window->Close(false, nextWindowID);
  while (window->IsAnimating(ANIM_TYPE_WINDOW_CLOSE))
    ProcessRenderLoop(true);
}

#ifdef _DEBUG
void CGUIWindowManager::DumpTextureUse()
{
  CGUIWindow* pWindow = GetWindow(GetActiveWindow());
  if (pWindow)
    pWindow->DumpTextureUse();

  CSingleLock lock(g_graphicsContext);
  for (const auto& window : m_activeDialogs)
  {
    if (window->IsDialogRunning())
      window->DumpTextureUse();
  }
}
#endif
