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

#include "utils/sntp.h"
#include "utils/delaycontroller.h"
#include "lib/libGoAhead/webserver.h"
#include "lib/libfilezilla/xbfilezilla.h"
#include "cores/IPlayer.h"
#include "cores/playercorefactory.h"
#include "DetectDVDType.h"
#include "Autorun.h"
#include "CdgParser.h"
#include "utils/Splash.h"
#include "PlaylistPlayer.h"
#include "utils/IMDB.h"

using namespace MEDIA_DETECT;
using namespace MUSIC_INFO;

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
  const CFileItem& CurrentFileItem();
  const EPLAYERCORES GetCurrentPlayer();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnPlayBackEnded();
  virtual void OnPlayBackStarted();
  virtual void OnPlayBackStopped();
  virtual void OnQueueNextItem();
  bool PlayMedia(const CFileItem& item, int iPlaylist = PLAYLIST_MUSIC_TEMP);
  bool ProcessAndStartPlaylist(const CStdString& strPlayList, CPlayList& playlist, int iPlaylist);
  bool PlayFile(const CFileItem& item, bool bRestart = false);
  void StopPlaying();
  void Restart(bool bSamePosition = true);
  bool IsPlaying() const ;
  bool IsPlayingAudio() const ;
  bool IsPlayingVideo() const ;
  bool OnKey(CKey& key);
  void RenderFullScreen();
  bool NeedRenderFullScreen();
  bool MustBlockHDSpinDown(bool bCheckThisForNormalSpinDown = true);
  void CheckNetworkHDSpinDown(bool playbackStarted = false);
  void CheckHDSpindown();
  void CheckScreenSaver();   // CB: SCREENSAVER PATCH
  void CheckAudioScrobblerStatus();
  void ActivateScreenSaver();
  void CheckShutdown();
  CMusicInfoTag* GetCurrentSong();
  CIMDBMovie* GetCurrentMovie();

  void ResetAllControls();
  virtual void Process();
  void ResetScreenSaver();
  int GetVolume() const;
  void SetVolume(int iPercent);
  void Mute(void);
  void UnMute(void);
  int GetPlaySpeed() const;
  void SetPlaySpeed(int iSpeed);
  bool IsButtonDown(DWORD code);
  bool AnyButtonDown();
  bool ResetScreenSaverWindow();
  void SetKaiNotification(const CStdString& aCaption, const CStdString& aDescription, CGUIImage* aIcon=NULL);
  double GetTotalTime() const;
  double GetTime() const;
  void SeekTime( double dTime = 0.0 );

  void SaveMusicScanSettings();
  void RestoreMusicScanSettings();

  CGUIDialogVolumeBar m_guiDialogVolumeBar;
  CGUIDialogSeekBar m_guiDialogSeekBar;
  CGUIDialogKaiToast m_guiDialogKaiToast;
  CGUIDialogMuteBug m_guiDialogMuteBug;
  CGUIWindowMusicOverlay m_guiMusicOverlay;
  CGUIWindowVideoOverlay m_guiVideoOverlay;
  CGUIWindowPointer m_guiPointer;

  CSNTPClient m_sntpClient;
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
  DWORD m_dwSaverTick;  // CB: SCREENSAVER PATCH
  DWORD m_dwSkinTime;

  char m_CurrDAAPHost[64];
  void *m_DAAPPtr;
  int m_DAAPDBID;
  void *m_DAAPSong;
  UINT64 m_DAAPSongSize;
  void *m_DAAPArtistPtr;
  CCdgParser m_CdgParser;
  bool m_bMasterLockPreviouslyEntered;
  int m_iMasterLockRetriesRemaining;
  bool m_bMasterLockOverridesLocalPasswords;
  int m_MasterUserModeCounter;  
  EPLAYERCORES m_eForcedNextPlayer;

  CStdString m_strPlayListFile;
  
  int GlobalIdleTime();
  bool SetControllerRumble(FLOAT m_fLeftMotorSpeed, FLOAT m_fRightMotorSpeed,int iDuration);
  

protected:
  EPLAYERCORES m_eCurrentPlayer;
  void UpdateLCD();
  bool SwitchToFullScreen();
  void FatalErrorHandler(bool InitD3D, bool MapDrives, bool InitNetwork);
  void InitBasicD3D();
  bool m_bXboxMediacenterLoaded;
  bool m_bSettingsLoaded;
  int m_iPlaySpeed;
  bool m_bAllSettingsLoaded;
  bool m_bInitializing;
  CFileItem m_itemCurrentFile;
  int m_nextPlaylistItem;
  D3DGAMMARAMP m_OldRamp;  // CB: SCREENSAVER PATCH
  CSplash* m_splash;

  DWORD m_threadID;       // application thread ID.  Used in applicationMessanger to know where
                          // we are firing a thread with delay from.
  static LONG WINAPI UnhandledExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo);

  // timer information
  CXBStopWatch m_idleTimer;
};

extern CApplication g_application;
extern CStdString g_LoadErrorStr;
