/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/ApplicationComponents.h"
#include "application/ApplicationEnums.h"
#include "application/ApplicationPlayerCallback.h"
#include "application/ApplicationSettingsHandling.h"
#include "guilib/IMsgTargetCallback.h"
#include "guilib/IWindowManagerCallback.h"
#include "messaging/IMessageTarget.h"
#include "playlists/PlayListTypes.h"
#include "threads/SystemClock.h"
#include "utils/GlobalsHandling.h"
#include "utils/Stopwatch.h"
#include "windowing/Resolution.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class CAction;
class CAppInboundProtocol;
class CBookmark;
class CFileItem;
class CFileItemList;
class CGUIComponent;
class CInertialScrollingHandler;
class CKey;
class CSeekHandler;
class CServiceManager;
class CSettingsComponent;
class CSplash;
class CWinSystemBase;

namespace ADDON
{
  class CSkinInfo;
  class IAddon;
  typedef std::shared_ptr<IAddon> AddonPtr;
  class CAddonInfo;
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

namespace KODI::VIDEO
{
  class CVideoInfoScanner;
}

namespace MUSIC_INFO
{
  class CMusicInfoScanner;
}

class CApplication : public IWindowManagerCallback,
                     public IMsgTargetCallback,
                     public KODI::MESSAGING::IMessageTarget,
                     public CApplicationComponents,
                     public CApplicationPlayerCallback,
                     public CApplicationSettingsHandling
{
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
  const std::string& CurrentFile();
  CFileItem& CurrentFileItem();
  std::shared_ptr<CFileItem> CurrentFileItemPtr();
  const CFileItem& CurrentUnstackedItem();
  bool OnMessage(CGUIMessage& message) override;
  std::string GetCurrentPlayer();

  int  GetMessageMask() override;
  void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;

  bool PlayMedia(CFileItem& item, const std::string& player, PLAYLIST::Id playlistId);
  bool ProcessAndStartPlaylist(const std::string& strPlayList,
                               PLAYLIST::CPlayList& playlist,
                               PLAYLIST::Id playlistId,
                               int track = 0);
  bool PlayFile(CFileItem item,
                const std::string& player,
                bool bRestart = false,
                bool forceSelection = false);
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

  bool ExecuteXBMCAction(std::string action, const std::shared_ptr<CGUIListItem>& item = NULL);

#ifdef HAS_OPTICAL_DRIVE
  std::unique_ptr<MEDIA_DETECT::CAutorun> m_Autorun;
#endif

  std::string m_strPlayListFile;

  bool IsAppFocused() const { return m_AppFocused; }

  bool GetRenderGUI() const override;

  bool SetLanguage(const std::string &strLanguage);
  bool LoadLanguage(bool reload);

  void SetLoggingIn(bool switchingProfiles);

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
  void PlaybackCleanup();

  std::shared_ptr<ANNOUNCEMENT::CAnnouncementManager> m_pAnnouncementManager;
  std::unique_ptr<CGUIComponent> m_pGUI;
  std::unique_ptr<CWinSystemBase> m_pWinSystem;
  std::unique_ptr<ActiveAE::CActiveAE> m_pActiveAE;
  std::shared_ptr<CAppInboundProtocol> m_pAppPort;

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
  std::optional<bool> m_guiRenderLastState;

  std::unique_ptr<MUSIC_INFO::CMusicInfoScanner> m_musicInfoScanner;

  bool PlayStack(CFileItem& item, bool bRestart);

  std::unique_ptr<CInertialScrollingHandler> m_pInertialScrollingHandler;

  std::vector<std::shared_ptr<ADDON::CAddonInfo>>
      m_incompatibleAddons; /*!< Result of addon migration (incompatible addon infos) */

public:
  bool m_bStop{false};
  bool m_AppFocused{true};

private:
  void PrintStartupLog();
  void ResetCurrentItem();

  mutable CCriticalSection m_critSection; /*!< critical section for all changes to this class, except for changes to triggers */

  CCriticalSection m_frameMoveGuard;              /*!< critical section for synchronizing GUI actions from inside and outside (python) */
  std::atomic_uint m_WaitingExternalCalls;        /*!< counts threads which are waiting to be processed in FrameMove */
  unsigned int m_ProcessedExternalCalls = 0;      /*!< counts calls which are processed during one "door open" cycle in FrameMove */
  unsigned int m_ProcessedExternalDecay = 0;      /*!< counts to close door after a few frames of no python activity */
  int m_ExitCode{EXITCODE_QUIT};
  std::shared_ptr<CFileItem> m_itemCurrentFile; //!< Currently playing file
  CEvent m_playerEvent;
};

XBMC_GLOBAL_REF(CApplication,g_application);
#define g_application XBMC_GLOBAL_USE(CApplication)
