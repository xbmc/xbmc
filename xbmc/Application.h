#pragma once
#include "xbapplicationex.h"
#include "applicationmessenger.h"
#include "GUIWindowManager.h"
#include "guiwindow.h"
#include "GUIMessage.h"
#include "GUIButtonControl.h"
#include "GUIImage.h"
#include "GUIFontManager.h"
#include "key.h"
#include "GUIWindowHome.h"
#include "GUIWindowPrograms.h"
#include "GUIWindowSettingsPrograms.h"
#include "GUIWindowPictures.h"
#include "GUIWindowMyFiles.h"
#include "GUIWindowVideo.h"
#include "GUIWindowVideoGenre.h"
#include "GUIWindowVideoActors.h"
#include "GUIWindowVideoYear.h"
#include "GUIWindowSettings.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogSelect.h"
#include "GUIDialogFileStacking.h"
#include "GUIWindowSystemInfo.h"
#include "GUIWindowSettingsGeneral.h"
#include "GUIWindowSettingsScreen.h"
#include "GUIWindowSettingsUICalibration.h"
#include "GUIWindowSettingsScreenCalibration.h"
#include "GUIWindowSettingsSubtitles.h"
#include "GUIWindowMusicInfo.h" 
#include "GUIWindowVideoInfo.h"
#include "GUIWindowScriptsInfo.h"
#include "GUIWindowMusicOverlay.h"
#include "GUIWindowFullScreen.h"
#include "GUIWindowVideoOverlay.h"
#include "GUIWindowSettingsSlideShow.h"
#include "guiwindowsettingsfilter.h"
#include "guiwindowsettingsmusic.h"
#include "GUIWindowScripts.h"
#include "GUIWindowVisualisation.h"
#include "GUIWindowSlideshow.h"
#include "GUIWindowMusicPlaylist.h" 
#include "GUIWindowMusicSongs.h" 
#include "GUIWindowMusicAlbum.h"
#include "GUIWindowMusicArtists.h"
#include "GUIWindowMusicGenres.h" 
#include "GUIWindowMusicTop100.h" 
#include "LocalizeStrings.h"
#include "utils/sntp.h"
#include "utils/delaycontroller.h"
#include "keyboard/virtualkeyboard.h"
#include "lib/libPython/XBPython.h"
#include "lib/libGoAhead/webserver.h"
#include "lib/libfilezilla/xbfilezilla.h"
#include "cores/IPlayer.h"
#include "DetectDVDType.h"
#include "Autorun.h"
#include "IMsgTargetCallback.h"
#include "ButtonTranslator.h"

#include <vector>
#include <memory>

using namespace std;
using namespace MEDIA_DETECT;

class CApplication : public CXBApplicationEx, public IPlayerCallback, public IMsgTargetCallback
{
public:
  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT 			Initialize();
  virtual void					FrameMove();
  virtual void					Render();
	virtual HRESULT 			Create();

	void									Stop();
	void									LoadSkin(const CStdString& strSkin);

	virtual bool					OnMessage(CGUIMessage& message);

	virtual	void					OnPlayBackEnded();
	virtual	void					OnPlayBackStarted();
	bool									PlayFile(const CStdString& strFile,bool bRestart=false);
  void                  Restart(bool bSamePosition=true);
	void									EnableOverlay();
	void									DisableOverlay();
	bool									IsPlayingAudio() const ;
	bool									IsPlayingVideo() const ;
	void									OnKey(CKey& key);
	void									RenderFullScreen();
  bool                  NeedRenderFullScreen();
	void									SpinHD();
	void				CheckScreenSaver();		// CB: SCREENSAVER PATCH
	void									ResetAllControls();
  virtual void          Process();
	CGUIWindowHome									m_guiHome;
  CGUIWindowPrograms							m_guiPrograms;
  CGUIWindowSettingsPrograms      m_guiSettingsPrograms;
	CGUIWindowPictures							m_guiPictures;
	CGUIDialogYesNo									m_guiDialogYesNo;
	CGUIDialogProgress							m_guiDialogProgress;
	CGUIDialogOK										m_guiDialogOK;
	CGUIWindowMyFiles								m_guiMyFiles;
	CGUIWindowVideo									m_guiMyVideo;
	CGUIWindowSettings							m_guiSettings;
	CGUIWindowSystemInfo						m_guiSystemInfo;
	CGUIWindowSettingsGeneral				m_guiSettingsGeneral;
	CGUIWindowMusicInfo							m_guiMusicInfo;
	CGUIWindowVideoInfo							m_guiVideoInfo;
	CGUIWindowScriptsInfo						m_guiScriptsInfo;
	CGUIWindowSettingsScreen				m_guiSettingsScreen;
	CGUIWindowSettingsUICalibration	m_guiSettingsUICalibration;
	CGUIWindowSettingsScreenCalibration m_guiSettingsScreenCalibration;
	CGUIWindowSettingsSlideShow			m_guiSettingsSlideShow;
	CGUIWindowScripts								m_guiScripts;
	CGUIWindowSettingsFilter				m_guiSettingsFilter;
	CGUIDialogSelect								m_guiDialogSelect;
	CGUIDialogFileStacking					m_guiDialogFileStacking;
	CGUIWindowMusicOverlay					m_guiMusicOverlay;
	CGUIWindowFullScreen						m_guiWindowFullScreen;
	CGUIWindowVideoOverlay					m_guiWindowVideoOverlay;
	CGUIWindowVisualisation					m_guiWindowVisualisation;
	CGUIWindowSettingsMusic					m_guiSettingsMusic;
	CGUIWindowSlideShow							m_guiWindowSlideshow;
  CGUIWindowMusicPlayList					m_guiMyMusicPlayList;
	CGUIWindowMusicSongs						m_guiMyMusicSongs;
  CGUIWindowMusicAlbum						m_guiMyMusicAlbum;
	CGUIWindowMusicArtists					m_guiMyMusicArtists;
	CGUIWindowMusicGenres						m_guiMyMusicGenres;
	CGUIWindowMusicTop100						m_guiMyMusicTop100;
  CGUIWindowSettingsSubtitles     m_guiSettingsSubtitles;
  CGUIWindowVideoGenre            m_guiVideoGenre;
  CGUIWindowVideoActors           m_guiVideoActors;
	CGUIWindowVideoYear             m_guiVideoYear;
  CXBVirtualKeyboard   						m_keyboard;
	CSNTPClient											m_sntpClient;
	CDetectDVDMedia									m_DetectDVDType;
	CAutorun												m_Autorun;
	CDelayController								m_ctrDpad;
	CDelayController								m_ctrIR;
	CWebServer*											m_pWebServer;
	CXBFileZilla*										m_pFileZilla;
	IPlayer*												m_pPlayer;
	bool														m_bSpinDown;
	DWORD														m_dwSpinDownTime;
	DWORD														m_dwIdleTime;
	bool		m_bInactive;	// CB: SCREENSAVER PATCH
	bool		m_bScreenSave;	// CB: SCREENSAVER PATCH
	DWORD		m_dwSaverTick;	// CB: SCREENSAVER PATCH
protected:
	bool										m_bOverlayEnabled;
	CStdString							m_strCurrentPlayer;
  bool                    m_bSettingsLoaded;
  CStdString              m_strCurrentFile;
  D3DGAMMARAMP m_OldRamp;			// CB: SCREENSAVER PATCH
};

extern CApplication g_application;