#pragma once

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

#include "system.h" // for HAS_DVD_DRIVE et. al.
#include "XBApplicationEx.h"

#include "guilib/IMsgTargetCallback.h"
#include "utils/GlobalsHandling.h"
#include "utils/StdString.h"

#include <map>

class CAction;
class CFileItem;
class CFileItemList;
class CKey;
namespace ADDON
{
  class CSkinInfo;
  class IAddon;
  typedef std::shared_ptr<IAddon> AddonPtr;
}

namespace MEDIA_DETECT
{
  class CAutorun;
}
class CPlayerController;

#include "cores/IPlayerCallback.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "PlayListPlayer.h"
#include "settings/lib/ISettingsHandler.h"
#include "settings/lib/ISettingCallback.h"
#include "settings/lib/ISubSettings.h"
#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
#include "storage/DetectDVDType.h"
#endif
#ifdef TARGET_WINDOWS
#include "win32/WIN32Util.h"
#endif
#include "utils/Stopwatch.h"
#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceStats.h"
#endif
#include "windowing/XBMC_events.h"
#include "threads/Thread.h"

#include "ApplicationPlayer.h"

class CSeekHandler;
class CKaraokeLyricsManager;
class CInertialScrollingHandler;
class DPMSSupport;
class CSplash;
class CBookmark;
class CNetwork;

namespace VIDEO
{
  class CVideoInfoScanner;
}

namespace MUSIC_INFO
{
  class CMusicInfoScanner;
}

#define VOLUME_MINIMUM 0.0f        // -60dB
#define VOLUME_MAXIMUM 1.0f        // 0dB
#define VOLUME_DYNAMIC_RANGE 90.0f // 60dB
#define VOLUME_CONTROL_STEPS 90    // 90 steps

// replay gain settings struct for quick access by the player multiple
// times per second (saves doing settings lookup)
struct ReplayGainSettings
{
  int iPreAmp;
  int iNoGainPreAmp;
  int iType;
  bool bAvoidClipping;
};

class CBackgroundPlayer : public CThread
{
public:
  CBackgroundPlayer(const CFileItem &item, int iPlayList);
  virtual ~CBackgroundPlayer();
  virtual void Process();
protected:
  CFileItem *m_item;
  int       m_iPlayList;
};

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback,
                     public ISettingCallback, public ISettingsHandler, public ISubSettings
{
  friend class CApplicationPlayer;
public:

  enum ESERVERS
  {
    ES_WEBSERVER = 1,
    ES_AIRPLAYSERVER,
    ES_JSONRPCSERVER,
    ES_UPNPRENDERER,
    ES_UPNPSERVER,
    ES_EVENTSERVER,
    ES_ZEROCONF
  };

  CApplication(void);
  virtual ~CApplication(void);
  virtual bool Initialize();
  virtual void FrameMove(bool processEvents, bool processGUI = true);
  virtual void Render();
  virtual bool RenderNoPresent();
  virtual void Preflight();
  virtual bool Create();
  virtual bool Cleanup();

  bool CreateGUI();
  bool InitWindow();
  bool DestroyWindow();
  void StartServices();
  void StopServices();

  bool StartServer(enum ESERVERS eServer, bool bStart, bool bWait = false);

  /*!
   * @brief Starts the PVR manager and decide if the manager should handle the startup window activation.
   * @return true, if the startup window activation is handled by the pvr manager, otherwise false
   */
  bool StartPVRManager();
  void StopPVRManager();
  bool IsCurrentThread() const;
  void Stop(int exitCode);
  void RestartApp();
  void UnloadSkin(bool forReload = false);
  bool LoadUserWindows();
  void ReloadSkin(bool confirm = false);
  const std::string& CurrentFile();
  CFileItem& CurrentFileItem();
  CFileItem& CurrentUnstackedItem();
  virtual bool OnMessage(CGUIMessage& message);
  PLAYERCOREID GetCurrentPlayer();
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackPaused();
  virtual void OnPlayBackResumed();
  virtual void OnPlayBackStopped();
  virtual void OnQueueNextItem();
  virtual void OnPlayBackSeek(int iTime, int seekOffset);
  virtual void OnPlayBackSeekChapter(int iChapter);
  virtual void OnPlayBackSpeedChanged(int iSpeed);
  bool PlayMedia(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC);
  bool PlayMediaSync(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC);
  bool ProcessAndStartPlaylist(const CStdString& strPlayList, PLAYLIST::CPlayList& playlist, int iPlaylist, int track=0);
  PlayBackRet PlayFile(const CFileItem& item, bool bRestart = false);
  void SaveFileState(bool bForeground = false);
  void UpdateFileState();
  void LoadVideoSettings(const std::string &path);
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  bool IsPlayingFullScreenVideo() const;
  bool IsStartingPlayback() const { return m_bPlaybackStarting; }
  bool IsFullScreen();
  bool OnKey(const CKey& key);
  bool OnAppCommand(const CAction &action);
  bool OnAction(const CAction &action);
  void CheckShutdown();
  void InhibitIdleShutdown(bool inhibit);
  bool IsIdleShutdownInhibited() const;
  // Checks whether the screensaver and / or DPMS should become active.
  void CheckScreenSaverAndDPMS();
  void CheckPlayingProgress();
  void ActivateScreenSaver(bool forceType = false);
  void CloseNetworkShares();

  void ShowAppMigrationMessage();
  virtual void Process();
  void ProcessSlow();
  void ResetScreenSaver();
  float GetVolume(bool percentage = true) const;
  void SetVolume(float iValue, bool isPercentage = true);
  bool IsMuted() const;
  bool IsMutedInternal() const { return m_muted; }
  void ToggleMute(void);
  void SetMute(bool mute);
  void ShowVolumeBar(const CAction *action = NULL);
  int GetSubtitleDelay() const;
  int GetAudioDelay() const;
  void ResetSystemIdleTimer();
  void ResetScreenSaverTimer();
  void StopScreenSaverTimer();
  // Wakes up from the screensaver and / or DPMS. Returns true if woken up.
  bool WakeUpScreenSaverAndDPMS(bool bPowerOffKeyPressed = false);
  bool WakeUpScreenSaver(bool bPowerOffKeyPressed = false);
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

  void StopShutdownTimer();
  void ResetShutdownTimers();

  void StopVideoScan();
  void StopMusicScan();
  bool IsMusicScanning() const;
  bool IsVideoScanning() const;

  /*!
   \brief Starts a video library cleanup.
   \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
   */
  void StartVideoCleanup(bool userInitiated = true);

  /*!
   \brief Starts a video library update.
   \param path The path to scan or "" (empty string) for a global scan.
   \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
   \param scanAll Whether to scan everything not already scanned (regardless of whether the user normally doesn't want a folder scanned).
   */
  void StartVideoScan(const CStdString &path, bool userInitiated = true, bool scanAll = false);

  /*!
  \brief Starts a music library cleanup.
  \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
  */
  void StartMusicCleanup(bool userInitiated = true);

  /*!
   \brief Starts a music library update.
   \param path The path to scan or "" (empty string) for a global scan.
   \param userInitiated Whether the action was initiated by the user (either via GUI or any other method) or not.  It is meant to hide or show dialogs.
   \param flags Flags for controlling the scanning process.  See xbmc/music/infoscanner/MusicInfoScanner.h for possible values.
   */
  void StartMusicScan(const CStdString &path, bool userInitiated = true, int flags = 0);
  void StartMusicAlbumScan(const CStdString& strDirectory, bool refresh=false);
  void StartMusicArtistScan(const CStdString& strDirectory, bool refresh=false);

  void UpdateLibraries();
  void CheckMusicPlaylist();

  bool ExecuteXBMCAction(std::string action);

  static bool OnEvent(XBMC_Event& newEvent);

  CNetwork& getNetwork();
#ifdef HAS_PERFORMANCE_SAMPLE
  CPerformanceStats &GetPerformanceStats();
#endif

#ifdef HAS_DVD_DRIVE
  MEDIA_DETECT::CAutorun* m_Autorun;
#endif

#if !defined(TARGET_WINDOWS) && defined(HAS_DVD_DRIVE)
  MEDIA_DETECT::CDetectDVDMedia m_DetectDVDType;
#endif

  CApplicationPlayer* m_pPlayer;

  inline bool IsInScreenSaver() { return m_bScreenSave; };
  inline bool IsDPMSActive() { return m_dpmsIsActive; };
  int m_iScreenSaveLock; // spiff: are we checking for a lock? if so, ignore the screensaver state, if -1 we have failed to input locks

  bool m_bPlaybackStarting;
  typedef enum
  {
    PLAY_STATE_NONE = 0,
    PLAY_STATE_STARTING,
    PLAY_STATE_PLAYING,
    PLAY_STATE_STOPPED,
    PLAY_STATE_ENDED,
  } PlayState;
  PlayState m_ePlayState;
  CCriticalSection m_playStateMutex;

  CKaraokeLyricsManager* m_pKaraokeMgr;

  PLAYERCOREID m_eForcedNextPlayer;
  CStdString m_strPlayListFile;

  int GlobalIdleTime();

  void EnablePlatformDirectories(bool enable=true)
  {
    m_bPlatformDirectories = enable;
  }

  bool PlatformDirectoriesEnabled()
  {
    return m_bPlatformDirectories;
  }

  void SetStandAlone(bool value);

  bool IsStandAlone()
  {
    return m_bStandalone;
  }

  void SetEnableLegacyRes(bool value)
  {
    m_bEnableLegacyRes = value;
  }

  bool IsEnableLegacyRes()
  {
    return m_bEnableLegacyRes;
  }

  void SetEnableTestMode(bool value)
  {
    m_bTestMode = value;
  }

  bool IsEnableTestMode()
  {
    return m_bTestMode;
  }

  void Minimize();
  bool ToggleDPMS(bool manual);

  float GetDimScreenSaverLevel() const;

  /*! \brief Retrieve the applications seek handler.
   \return a constant pointer to the seek handler.
   \sa CSeekHandler
   */
  const CSeekHandler *GetSeekHandler() const { return m_seekHandler; };

  bool SwitchToFullScreen();

  CSplash* GetSplash() { return m_splash; }
  void SetRenderGUI(bool renderGUI);
  bool GetRenderGUI() const { return m_renderGUI; };

  bool SetLanguage(const CStdString &strLanguage);

  ReplayGainSettings& GetReplayGainSettings() { return m_replayGainSettings; }

  void SetLoggingIn(bool loggingIn) { m_loggingIn = loggingIn; }

protected:
  virtual bool OnSettingsSaving() const;

  virtual bool Load(const TiXmlNode *settings);
  virtual bool Save(TiXmlNode *settings) const;

  virtual void OnSettingChanged(const CSetting *setting);
  virtual void OnSettingAction(const CSetting *setting);
  virtual bool OnSettingUpdate(CSetting* &setting, const char *oldSettingId, const TiXmlNode *oldSettingNode);

  bool LoadSkin(const CStdString& skinID);
  bool LoadSkin(const std::shared_ptr<ADDON::CSkinInfo>& skin);

  bool m_skinReverting;

  bool m_loggingIn;

#if defined(TARGET_DARWIN_IOS)
  friend class CWinEventsIOS;
#endif
#if defined(TARGET_ANDROID)
  friend class CWinEventsAndroid;
#endif
  // screensaver
  bool m_bScreenSave;
  ADDON::AddonPtr m_screenSaver;

  // timer information
#ifdef TARGET_WINDOWS
  CWinIdleTimer m_idleTimer;
  CWinIdleTimer m_screenSaverTimer;
#else
  CStopWatch m_idleTimer;
  CStopWatch m_screenSaverTimer;
#endif
  CStopWatch m_restartPlayerTimer;
  CStopWatch m_frameTime;
  CStopWatch m_navigationTimer;
  CStopWatch m_slowTimer;
  CStopWatch m_shutdownTimer;

  bool m_bInhibitIdleShutdown;

  DPMSSupport* m_dpms;
  bool m_dpmsIsActive;
  bool m_dpmsIsManual;

  CFileItemPtr m_itemCurrentFile;
  CFileItemList* m_currentStack;
  CFileItemPtr m_stackFileItemToUpdate;

  CStdString m_prevMedia;
  CSplash* m_splash;
  ThreadIdentifier m_threadID;       // application thread ID.  Used in applicationMessanger to know where we are firing a thread with delay from.
  bool m_bInitializing;
  bool m_bPlatformDirectories;

  CBookmark& m_progressTrackingVideoResumeBookmark;
  CFileItemPtr m_progressTrackingItem;
  bool m_progressTrackingPlayCountUpdate;

  int m_currentStackPosition;
  int m_nextPlaylistItem;

  bool m_bPresentFrame;
  unsigned int m_lastFrameTime;
  unsigned int m_lastRenderTime;

  bool m_bStandalone;
  bool m_bEnableLegacyRes;
  bool m_bTestMode;
  bool m_bSystemScreenSaverEnable;

  VIDEO::CVideoInfoScanner *m_videoInfoScanner;
  MUSIC_INFO::CMusicInfoScanner *m_musicInfoScanner;

  bool m_muted;
  float m_volumeLevel;

  void Mute();
  void UnMute();

  void SetHardwareVolume(float hardwareVolume);

  void VolumeChanged() const;

  PlayBackRet PlayStack(const CFileItem& item, bool bRestart);
  bool ProcessMouse();
  bool ProcessRemote(float frameTime);
  bool ProcessGamepad(float frameTime);
  bool ProcessEventServer(float frameTime);
  bool ProcessPeripherals(float frameTime);
  bool ProcessJoystickEvent(const std::string& joystickName, int button, short inputType, float fAmount, unsigned int holdTime = 0);
  bool ExecuteInputAction(const CAction &action);
  int  GetActiveWindowID(void);

  float NavigationIdleTime();
  static bool AlwaysProcess(const CAction& action);

  bool InitDirectoriesLinux();
  bool InitDirectoriesOSX();
  bool InitDirectoriesWin32();
  void CreateUserDirs();

  CSeekHandler *m_seekHandler;
  CPlayerController *m_playerController;
  CInertialScrollingHandler *m_pInertialScrollingHandler;
  CNetwork    *m_network;
#ifdef HAS_PERFORMANCE_SAMPLE
  CPerformanceStats m_perfStats;
#endif

#ifdef HAS_EVENT_SERVER
  std::map<std::string, std::map<int, float> > m_lastAxisMap;
#endif

  ReplayGainSettings m_replayGainSettings;
};

XBMC_GLOBAL_REF(CApplication,g_application);
#define g_application XBMC_GLOBAL_USE(CApplication)
