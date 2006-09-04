#pragma once
#include "xbapplicationex.h"

#include "MusicInfoTag.h"
#include "FileItem.h"

#include "GUIDialogSeekBar.h"
#include "GUIDialogKaiToast.h"
#include "GUIDialogVolumeBar.h"
#include "GUIDialogMuteBug.h"
#include "GUIWindowPointer.h"   // Mouse pointer
#include "GUIWindowMusicOverlay.h"
#include "GUIWindowVideoOverlay.h"

#include "utils/delaycontroller.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory.h"
#include "DetectDVDType.h"
#include "Autorun.h"
#include "utils/Splash.h"
#include "PlaylistPlayer.h"
#include "utils/IMDB.h"
#include "xbstopwatch.h"

using namespace MEDIA_DETECT;
using namespace MUSIC_INFO;

class CWebServer;
class CXBFileZilla;
class CCdgParser;
class CSNTPClient;

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback
{
public:
  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT Initialize();
  virtual void FrameMove();
  virtual void Render();
  virtual HRESULT Create();

  void StartServices();
  void StopServices();
  void StartKai();
  void StopKai();
  void StartWebServer();
  void StopWebServer();
  void StartFtpServer();
  void StopFtpServer();
  void StartTimeServer();
  void StopTimeServer();
  void StartUPnP();
  void StopUPnP();
  void StartLEDControl(bool switchoff = false);
  void DimLCDOnPlayback(bool dim);
  void PrintXBEToLCD(const char* xbePath);
  void CheckDate();		//GeminiServer CheckDate
  DWORD GetThreadId() const { return m_threadID; };
  void Stop();
  void RestartApp();
  void LoadSkin(const CStdString& strSkin);
  void UnloadSkin();
  bool LoadUserWindows(const CStdString& strSkinPath);
  void DelayLoadSkin();
  void CancelDelayLoadSkin();
  const CStdString& CurrentFile();
  CFileItem& CurrentFileItem();
  const EPLAYERCORES GetCurrentPlayer();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackPaused();
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackStopped();
  virtual void OnQueueNextItem();
  bool PlayMedia(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC_TEMP);
  bool ProcessAndStartPlaylist(const CStdString& strPlayList, CPlayList& playlist, int iPlaylist);
  bool PlayFile(const CFileItem& item, bool bRestart = false);
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  void DelayedPlayerRestart();
  void CheckDelayedPlayerRestart();
  bool IsPlaying() const ;
  bool IsPaused() const;
  bool IsPlayingAudio() const ;
  bool IsPlayingVideo() const ;
  bool OnKey(CKey& key);
  bool OnAction(const CAction &action);
  void RenderFullScreen();
  void RenderMemoryStatus();
  bool NeedRenderFullScreen();
  bool MustBlockHDSpinDown(bool bCheckThisForNormalSpinDown = true);
  void CheckNetworkHDSpinDown(bool playbackStarted = false);
  void CheckHDSpindown();
  void CheckScreenSaver();   // CB: SCREENSAVER PATCH
  void CheckPlayingProgress();
  void CheckAudioScrobblerStatus();
  void ActivateScreenSaver(bool forceType = false);
  void CheckShutdown();
  CMusicInfoTag* GetCurrentSong();
  CIMDBMovie* GetCurrentMovie();

  void ResetAllControls();
  virtual void Process();
  void ProcessSlow();
  void ResetScreenSaver();
  int GetVolume() const;
  void SetVolume(int iPercent);
  void Mute(void);
  int GetPlaySpeed() const;
  void SetPlaySpeed(int iSpeed);
  bool IsButtonDown(DWORD code);
  bool AnyButtonDown();
  bool ResetScreenSaverWindow();
  void SetKaiNotification(const CStdString& aCaption, const CStdString& aDescription, CGUIImage* aIcon=NULL);
  double GetTotalTime() const;
  double GetTime() const;
  float GetPercentage() const;
  void SeekPercentage(float percent);
  void SeekTime( double dTime = 0.0 );

  void SaveMusicScanSettings();
  void RestoreMusicScanSettings();
  void CheckMusicPlaylist();

  CGUIDialogVolumeBar m_guiDialogVolumeBar;
  CGUIDialogSeekBar m_guiDialogSeekBar;
  CGUIDialogKaiToast m_guiDialogKaiToast;
  CGUIDialogMuteBug m_guiDialogMuteBug;
  CGUIWindowMusicOverlay m_guiMusicOverlay;
  CGUIWindowVideoOverlay m_guiVideoOverlay;
  CGUIWindowPointer m_guiPointer;

  CSNTPClient *m_psntpClient;
  CDetectDVDMedia m_DetectDVDType;
  CAutorun m_Autorun;
  CDelayController m_ctrDpad;
  CWebServer* m_pWebServer;
  CXBFileZilla* m_pFileZilla;
  IPlayer* m_pPlayer;

  bool m_bSpinDown;
  bool m_bNetworkSpinDown;
  DWORD m_dwSpinDownTime;
  bool m_bInactive;    // CB: SCREENSAVER PATCH
  bool m_bScreenSave;  // CB: SCREENSAVER PATCH
  int m_iScreenSaveLock; // spiff: are we checking for a lock? if so, ignore the screensaver state, if -1 we have failed to input locks
  DWORD m_dwSaverTick;  // CB: SCREENSAVER PATCH
  DWORD m_dwSkinTime;

  CCdgParser* m_pCdgParser;

  EPLAYERCORES m_eForcedNextPlayer;
  CStdString m_strPlayListFile;
  
  int GlobalIdleTime();
  bool SetControllerRumble(FLOAT m_fLeftMotorSpeed, FLOAT m_fRightMotorSpeed,int iDuration);

  
protected:
  // timer information
  CXBStopWatch m_idleTimer;
  CXBStopWatch m_restartPlayerTimer;
  CXBStopWatch m_frameTime;
  CXBStopWatch m_navigationTimer;
  CXBStopWatch m_slowTimer;

  CFileItem m_itemCurrentFile;
  CFileItemList m_currentStack;
  D3DGAMMARAMP m_OldRamp;  // CB: SCREENSAVER PATCH
  CSplash* m_splash;
  DWORD m_threadID;       // application thread ID.  Used in applicationMessanger to know where we are firing a thread with delay from.
  EPLAYERCORES m_eCurrentPlayer;
  
  bool m_bXboxMediacenterLoaded;
  bool m_bSettingsLoaded;
  bool m_switchingToFullScreen;
  bool m_bAllSettingsLoaded;
  bool m_bInitializing;
  bool m_playCountUpdated;
  
  int m_iPlaySpeed;
  int m_currentStackPosition;
  int m_nextPlaylistItem;

  static LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);

  void SetHardwareVolume(long hardwareVolume);
  void UpdateLCD();
  void FatalErrorHandler(bool InitD3D, bool MapDrives, bool InitNetwork);
  void CheckForDebugButtonCombo();
  void InitBasicD3D();
  
  
  bool PlayStack(const CFileItem& item, bool bRestart);
  bool SwitchToFullScreen();
  bool ProcessMouse();
  bool ProcessHTTPApiButtons();
  bool ProcessKeyboard();
  bool ProcessRemote(float frameTime);
  bool ProcessGamepad(float frameTime);
 
  
  float NavigationIdleTime();
 
};

extern CApplication g_application;
extern CStdString g_LoadErrorStr;
