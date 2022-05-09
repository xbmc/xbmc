/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ApplicationPlayer.h"
#include "ApplicationStackHelper.h"
#include "ServiceManager.h"
#include "application/ApplicationPlayerCallback.h"
#include "application/ApplicationPowerHandling.h"
#include "application/ApplicationSkinHandling.h"
#include "application/ApplicationVolumeHandling.h"
#include "cores/IPlayerCallback.h"
#include "guilib/IMsgTargetCallback.h"
#include "guilib/IWindowManagerCallback.h"
#include "messaging/IMessageTarget.h"
#include "settings/ISubSettings.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISettingsHandler.h"
#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
#include "storage/DetectDVDType.h"
#endif
#include "threads/SystemClock.h"
#include "threads/Thread.h"
#include "utils/GlobalsHandling.h"
#include "utils/Stopwatch.h"
#include "windowing/Resolution.h"
#include "windowing/XBMC_events.h"

#include <atomic>
#include <chrono>
#include <deque>
#include <memory>
#include <string>
#include <vector>

class CAction;
class CFileItem;
class CFileItemList;
class CKey;
class CSeekHandler;
class CInertialScrollingHandler;
class CSplash;
class CBookmark;
class IActionListener;
class CGUIComponent;
class CAppInboundProtocol;
class CSettingsComponent;

namespace ADDON
{
  class CSkinInfo;
  class IAddon;
  typedef std::shared_ptr<IAddon> AddonPtr;
}

namespace ANNOUNCEMENT
{
  class CAnnouncementManager;
}

namespace MEDIA_DETECT
{
  class CAutorun;
}

namespace PLAYLIST
{
  class CPlayList;
}

namespace ActiveAE
{
  class CActiveAE;
}

namespace VIDEO
{
  class CVideoInfoScanner;
}

namespace MUSIC_INFO
{
  class CMusicInfoScanner;
}

// replay gain settings struct for quick access by the player multiple
// times per second (saves doing settings lookup)
struct ReplayGainSettings
{
  int iPreAmp;
  int iNoGainPreAmp;
  int iType;
  bool bAvoidClipping;
};

enum StartupAction
{
  STARTUP_ACTION_NONE = 0,
  STARTUP_ACTION_PLAY_TV,
  STARTUP_ACTION_PLAY_RADIO
};

// Do not change the numbers; external scripts depend on them
enum
{
  EXITCODE_QUIT = 0,
  EXITCODE_POWERDOWN = 64,
  EXITCODE_RESTARTAPP = 65,
  EXITCODE_REBOOT = 66,
};

class CApplication : public IWindowManagerCallback,
                     public IMsgTargetCallback,
                     public ISettingCallback,
                     public ISettingsHandler,
                     public ISubSettings,
                     public KODI::MESSAGING::IMessageTarget,
                     public CApplicationPlayerCallback,
                     public CApplicationPowerHandling,
                     public CApplicationSkinHandling,
                     public CApplicationVolumeHandling
{
friend class CAppInboundProtocol;

public:

  // If playback time of current item is greater than this value, ACTION_PREV_ITEM will seek to start
  // of currently playing item, otherwise it will seek to start of the previous item in playlist
  static const unsigned int ACTION_PREV_ITEM_THRESHOLD = 3; // seconds;

  CApplication(void);
  ~CApplication(void) override;

  bool Create();
  bool Initialize();
  int Run();
  bool Cleanup();

  void FrameMove(bool processEvents, bool processGUI = true) override;
  void Render() override;

  bool IsInitialized() const { return !m_bInitializing; }
  bool IsStopping() const { return m_bStop; }

  bool CreateGUI();
  bool InitWindow(RESOLUTION res = RES_INVALID);

  bool Stop(int exitCode);
  void ReloadSkin(bool confirm = false);
  const std::string& CurrentFile();
  CFileItem& CurrentFileItem();
  std::shared_ptr<CFileItem> CurrentFileItemPtr();
  CFileItem& CurrentUnstackedItem();
  bool OnMessage(CGUIMessage& message) override;
  CApplicationPlayer& GetAppPlayer();
  std::string GetCurrentPlayer();
  CApplicationStackHelper& GetAppStackHelper();

  int  GetMessageMask() override;
  void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;

  bool PlayMedia(CFileItem& item, const std::string &player, int iPlaylist);
  bool ProcessAndStartPlaylist(const std::string& strPlayList, PLAYLIST::CPlayList& playlist, int iPlaylist, int track=0);
  bool PlayFile(CFileItem item, const std::string& player, bool bRestart = false);
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  bool IsPlayingFullScreenVideo() const;
  bool IsFullScreen();
  bool OnAction(const CAction &action);
  void CloseNetworkShares();

  void ConfigureAndEnableAddons();
  void ShowAppMigrationMessage();
  void Process() override;
  void ProcessSlow();
  int GetSubtitleDelay();
  int GetAudioDelay();
  /*!
   \brief Returns the total time in fractional seconds of the currently playing media

   Beware that this method returns fractional seconds whereas IPlayer::GetTotalTime() returns milliseconds.
   */
  double GetTotalTime() const;
  /*!
   \brief Returns the current time in fractional seconds of the currently playing media

   Beware that this method returns fractional seconds whereas IPlayer::GetTime() returns milliseconds.
   */
  double GetTime() const;
  float GetPercentage() const;

  // Get the percentage of data currently cached/buffered (aq/vq + FileCache) from the input stream if applicable.
  float GetCachePercentage() const;

  void SeekPercentage(float percent);
  void SeekTime( double dTime = 0.0 );

  void UpdateLibraries();

  void UpdateCurrentPlayArt();

  bool ExecuteXBMCAction(std::string action, const CGUIListItemPtr &item = NULL);

#ifdef HAS_DVD_DRIVE
  std::unique_ptr<MEDIA_DETECT::CAutorun> m_Autorun;
#endif

  std::string m_strPlayListFile;

  bool IsAppFocused() const { return m_AppFocused; }

  bool GetRenderGUI() const override { return m_renderGUI; }

  bool SetLanguage(const std::string &strLanguage);
  bool LoadLanguage(bool reload);

  ReplayGainSettings& GetReplayGainSettings() { return m_replayGainSettings; }

  void SetLoggingIn(bool switchingProfiles);

  /*!
   \brief Register an action listener.
   \param listener The listener to register
   */
  void RegisterActionListener(IActionListener *listener);
  /*!
   \brief Unregister an action listener.
   \param listener The listener to unregister
   */
  void UnregisterActionListener(IActionListener *listener);

  std::unique_ptr<CServiceManager> m_ServiceManager;

  /*!
  \brief Locks calls from outside kodi (e.g. python) until framemove is processed.
  */
  void LockFrameMoveGuard();

  /*!
  \brief Unlocks calls from outside kodi (e.g. python).
  */
  void UnlockFrameMoveGuard();

protected:
  bool OnSettingsSaving() const override;
  bool Load(const TiXmlNode *settings) override;
  bool Save(TiXmlNode *settings) const override;
  void OnSettingChanged(const std::shared_ptr<const CSetting>& setting) override;
  void OnSettingAction(const std::shared_ptr<const CSetting>& setting) override;
  bool OnSettingUpdate(const std::shared_ptr<CSetting>& setting,
                       const char* oldSettingId,
                       const TiXmlNode* oldSettingNode) override;

  void PlaybackCleanup();

  // inbound protocol
  bool OnEvent(XBMC_Event& newEvent);

  /*!
   \brief Delegates the action to all registered action handlers.
   \param action The action
   \return true, if the action was taken by one of the action listener.
   */
  bool NotifyActionListeners(const CAction &action) const;

  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_pAnnouncementManager;
  std::unique_ptr<CGUIComponent> m_pGUI;
  std::unique_ptr<CWinSystemBase> m_pWinSystem;
  std::unique_ptr<ActiveAE::CActiveAE> m_pActiveAE;
  std::shared_ptr<CAppInboundProtocol> m_pAppPort;
  std::deque<XBMC_Event> m_portEvents;
  CCriticalSection m_portSection;

  bool m_ignoreSkinSettingChanges = false;

#if defined(TARGET_DARWIN_IOS)
  friend class CWinEventsIOS;
#endif
#if defined(TARGET_DARWIN_TVOS)
  friend class CWinEventsTVOS;
#endif
#if defined(TARGET_ANDROID)
  friend class CWinEventsAndroid;
#endif
  // timer information
  CStopWatch m_restartPlayerTimer;
  CStopWatch m_frameTime;
  CStopWatch m_slowTimer;
  XbmcThreads::EndTime<> m_guiRefreshTimer;

  std::string m_prevMedia;
  bool m_bInitializing = true;

  int m_nextPlaylistItem = -1;

  std::chrono::time_point<std::chrono::steady_clock> m_lastRenderTime;
  bool m_skipGuiRender = false;

  std::unique_ptr<MUSIC_INFO::CMusicInfoScanner> m_musicInfoScanner;

  bool PlayStack(CFileItem& item, bool bRestart);

  void HandlePortEvents();

  CInertialScrollingHandler *m_pInertialScrollingHandler;

  ReplayGainSettings m_replayGainSettings;
  std::vector<IActionListener *> m_actionListeners;
  std::vector<ADDON::AddonInfoPtr>
      m_incompatibleAddons; /*!< Result of addon migration (incompatible addon infos) */

public:
  bool m_bStop{false};
  bool m_AppFocused{true};

private:
  void PrintStartupLog();
  void ResetCurrentItem();

  void RegisterSettings();
  void UnregisterSettings();

  mutable CCriticalSection m_critSection; /*!< critical section for all changes to this class, except for changes to triggers */

  CCriticalSection m_frameMoveGuard;              /*!< critical section for synchronizing GUI actions from inside and outside (python) */
  std::atomic_uint m_WaitingExternalCalls;        /*!< counts threads which are waiting to be processed in FrameMove */
  unsigned int m_ProcessedExternalCalls = 0;      /*!< counts calls which are processed during one "door open" cycle in FrameMove */
  unsigned int m_ProcessedExternalDecay = 0;      /*!< counts to close door after a few frames of no python activity */
  CApplicationPlayer m_appPlayer;
  CApplicationStackHelper m_stackHelper;
  int m_ExitCode{EXITCODE_QUIT};
};

XBMC_GLOBAL_REF(CApplication,g_application);
#define g_application XBMC_GLOBAL_USE(CApplication)
