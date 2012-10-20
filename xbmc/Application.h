#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
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
#include "guilib/Key.h"
#include "threads/Condition.h"

#include <map>

class CFileItem;
class CFileItemList;
namespace ADDON
{
  class CSkinInfo;
  class IAddon;
  typedef boost::shared_ptr<IAddon> AddonPtr;
}

namespace MEDIA_DETECT
{
  class CAutorun;
}

#include "cores/IPlayer.h"
#include "cores/playercorefactory/PlayerCoreFactory.h"
#include "PlayListPlayer.h"
#if !defined(_WIN32) && defined(HAS_DVD_DRIVE)
#include "storage/DetectDVDType.h"
#endif
#ifdef _WIN32
#include "win32/WIN32Util.h"
#endif
#include "utils/Stopwatch.h"
#include "network/Network.h"
#include "utils/CharsetConverter.h"
#ifdef HAS_PERFORMANCE_SAMPLE
#include "utils/PerformanceStats.h"
#endif
#include "windowing/XBMC_events.h"
#include "threads/Thread.h"

class CSeekHandler;
class CKaraokeLyricsManager;
class CInertialScrollingHandler;
class DPMSSupport;
class CSplash;
class CBookmark;
class CWebServer;
#ifdef HAS_WEB_SERVER
class CWebServer;
class CHTTPImageHandler;
class CHTTPVfsHandler;
#ifdef HAS_JSONRPC
class CHTTPJsonRpcHandler;
#endif
#ifdef HAS_WEB_INTERFACE
class CHTTPWebinterfaceHandler;
class CHTTPWebinterfaceAddonsHandler;
#endif
#endif
namespace VIDEO
{
  class CVideoInfoScanner;
}

namespace MUSIC_INFO
{
  class CMusicInfoScanner;
}

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

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback
{
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

  bool StartWebServer();
  void StopWebServer();
  bool StartAirplayServer();
  void StopAirplayServer(bool bWait);
  bool StartJSONRPCServer();
  void StopJSONRPCServer(bool bWait);
  void StartUPnP();
  void StopUPnP(bool bWait);
  void StartUPnPRenderer();
  void StopUPnPRenderer();
  void StartUPnPServer();
  void StopUPnPServer();
  void StartPVRManager();
  void StopPVRManager();
  bool StartEventServer();
  bool StopEventServer(bool bWait, bool promptuser);
  void RefreshEventServer();
  void StartZeroconf();
  void StopZeroconf();
  void DimLCDOnPlayback(bool dim);
  bool IsCurrentThread() const;
  void Stop(int exitCode);
  void RestartApp();
  void UnloadSkin(bool forReload = false);
  bool LoadUserWindows();
  void ReloadSkin();
  const CStdString& CurrentFile();
  CFileItem& CurrentFileItem();
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
  bool PlayFile(const CFileItem& item, bool bRestart = false);
  void SaveFileState(bool bForeground = false);
  void UpdateFileState();
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  bool IsPlaying() const;
  bool IsPaused() const;
  bool IsPlayingAudio() const;
  bool IsPlayingVideo() const;
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
  void CheckAudioScrobblerStatus();
  void ActivateScreenSaver(bool forceType = false);

  virtual void Process();
  void ProcessSlow();
  void ResetScreenSaver();
  int GetVolume() const;
  void SetVolume(float iValue, bool isPercentage = true);
  bool IsMuted() const;
  void ToggleMute(void);
  void ShowVolumeBar(const CAction *action = NULL);
  int GetPlaySpeed() const;
  int GetSubtitleDelay() const;
  int GetAudioDelay() const;
  void SetPlaySpeed(int iSpeed);
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

  void StartVideoCleanup();

  void StartVideoScan(const CStdString &path, bool scanAll = false);
  void StartMusicScan(const CStdString &path, int flags = 0);
  void StartMusicAlbumScan(const CStdString& strDirectory, bool refresh=false);
  void StartMusicArtistScan(const CStdString& strDirectory, bool refresh=false);

  void UpdateLibraries();
  void CheckMusicPlaylist();

  bool ExecuteXBMCAction(std::string action);

  static bool OnEvent(XBMC_Event& newEvent);

#if defined(HAS_LINUX_NETWORK)
  CNetworkLinux& getNetwork();
#elif defined(HAS_WIN32_NETWORK)
  CNetworkWin32& getNetwork();
#else
  CNetwork& getNetwork();
#endif
#ifdef HAS_PERFORMANCE_SAMPLE
  CPerformanceStats &GetPerformanceStats();
#endif

#ifdef HAS_DVD_DRIVE
  MEDIA_DETECT::CAutorun* m_Autorun;
#endif

#if !defined(_WIN32) && defined(HAS_DVD_DRIVE)
  MEDIA_DETECT::CDetectDVDMedia m_DetectDVDType;
#endif

  IPlayer* m_pPlayer;

#ifdef HAS_WEB_SERVER
  CWebServer& m_WebServer;
  CHTTPImageHandler& m_httpImageHandler;
  CHTTPVfsHandler& m_httpVfsHandler;
#ifdef HAS_JSONRPC
  CHTTPJsonRpcHandler& m_httpJsonRpcHandler;
#endif
#ifdef HAS_WEB_INTERFACE
  CHTTPWebinterfaceHandler& m_httpWebinterfaceHandler;
  CHTTPWebinterfaceAddonsHandler& m_httpWebinterfaceAddonsHandler;
#endif
#endif

  inline bool IsInScreenSaver() { return m_bScreenSave; };
  int m_iScreenSaveLock; // spiff: are we checking for a lock? if so, ignore the screensaver state, if -1 we have failed to input locks

  bool m_bIsPaused;
  bool m_bPlaybackStarting;

  CKaraokeLyricsManager* m_pKaraokeMgr;

  PLAYERCOREID m_eForcedNextPlayer;
  CStdString m_strPlayListFile;

  int GlobalIdleTime();
  void NewFrame();
  bool WaitFrame(unsigned int timeout);

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

  bool IsPresentFrame();

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
protected:
  bool LoadSkin(const CStdString& skinID);
  void LoadSkin(const boost::shared_ptr<ADDON::CSkinInfo>& skin);

  bool m_skinReloading; // if true we disallow LoadSkin until ReloadSkin is called

#if defined(TARGET_DARWIN_IOS)
  friend class CWinEventsIOS;
#endif
  // screensaver
  bool m_bScreenSave;
  ADDON::AddonPtr m_screenSaver;

  // timer information
#ifdef _WIN32
  CWinIdleTimer m_idleTimer;
#else
  CStopWatch m_idleTimer;
#endif
  CStopWatch m_restartPlayerTimer;
  CStopWatch m_frameTime;
  CStopWatch m_navigationTimer;
  CStopWatch m_slowTimer;
  CStopWatch m_screenSaverTimer;
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
  PLAYERCOREID m_eCurrentPlayer;
  bool m_bInitializing;
  bool m_bPlatformDirectories;

  CBookmark& m_progressTrackingVideoResumeBookmark;
  CFileItemPtr m_progressTrackingItem;
  bool m_progressTrackingPlayCountUpdate;

  int m_iPlaySpeed;
  int m_currentStackPosition;
  int m_nextPlaylistItem;

  bool m_bPresentFrame;
  unsigned int m_lastFrameTime;
  unsigned int m_lastRenderTime;

  bool m_bStandalone;
  bool m_bEnableLegacyRes;
  bool m_bTestMode;
  bool m_bSystemScreenSaverEnable;

  int        m_frameCount;
  CCriticalSection m_frameMutex;
  XbmcThreads::ConditionVariable  m_frameCond;

  VIDEO::CVideoInfoScanner *m_videoInfoScanner;
  MUSIC_INFO::CMusicInfoScanner *m_musicInfoScanner;

  void Mute();
  void UnMute();

  void SetHardwareVolume(float hardwareVolume);
  void UpdateLCD();

  void VolumeChanged() const;

  bool PlayStack(const CFileItem& item, bool bRestart);
  bool ProcessMouse();
  bool ProcessRemote(float frameTime);
  bool ProcessGamepad(float frameTime);
  bool ProcessEventServer(float frameTime);
  bool ProcessPeripherals(float frameTime);
  bool ProcessJoystickEvent(const std::string& joystickName, int button, bool isAxis, float fAmount, unsigned int holdTime = 0);
  int  GetActiveWindowID(void);

  float NavigationIdleTime();
  static bool AlwaysProcess(const CAction& action);

  void SaveCurrentFileSettings();

  bool InitDirectoriesLinux();
  bool InitDirectoriesOSX();
  bool InitDirectoriesWin32();
  void CreateUserDirs();

  CSeekHandler *m_seekHandler;
  CInertialScrollingHandler *m_pInertialScrollingHandler;
#if defined(HAS_LINUX_NETWORK)
  CNetworkLinux m_network;
#elif defined(HAS_WIN32_NETWORK)
  CNetworkWin32 m_network;
#else
  CNetwork    m_network;
#endif
#ifdef HAS_PERFORMANCE_SAMPLE
  CPerformanceStats m_perfStats;
#endif

#ifdef HAS_EVENT_SERVER
  std::map<std::string, std::map<int, float> > m_lastAxisMap;
#endif

};

extern CApplication g_application;
