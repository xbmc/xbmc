#pragma once
#include "xbapplicationex.h"
#include "GUIWindowManager.h"
#include "guiwindow.h"
#include "GUIMessage.h"
#include "GUIButtonControl.h"
#include "GUIImage.h"
#include "GUIFontManager.h"
#include "key.h"
#include "GUIWindowHome.h"
#include "GUIWindowPrograms.h"
#include "GUIWindowPictures.h"
#include "GUIWindowMyFiles.h"
#include "GUIWindowMusic.h"
#include "GUIWindowVideo.h"
#include "GUIWindowSettings.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogOK.h"
#include "GUIDialogProgress.h"
#include "GUIDialogSelect.h"
#include "GUIWindowSystemInfo.h"
#include "GUIWindowSettingsGeneral.h"
#include "GUIWindowSettingsScreen.h"
#include "GUIWindowSettingsUICalibration.h"
#include "GUIWindowSettingsMovieCalibration.h"
#include "GUIWindowMusicInfo.h" 
#include "GUIWindowVideoInfo.h"
#include "GUIWindowMusicOverlay.h"
#include "GUIWindowFullScreen.h"
#include "GUIWindowVideoOverlay.h"
#include "LocalizeStrings.h"
#include "utils/sntp.h"
#include "utils/delaycontroller.h"
#include "keyboard/virtualkeyboard.h"
#include "lib/libPython/XboxPython.h"
#include "cores/IPlayer.h"
#include "DetectDVDType.h"

#include <vector>
using namespace std;

class CApplication : public CXBApplicationEx, public IPlayerCallback
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
	void									ExecuteScript(const CStdString& strScript);
	void									ProcessScripts();

	virtual	void					OnPlayBackEnded();
	virtual	void					OnPlayBackStarted();
	bool									PlayFile(const CStdString& strFile);
	void									EnableOverlay();
	void									DisableOverlay();
	bool									IsPlayingAudio() const ;
	bool									IsPlayingVideo() const ;
	void									OnKey(CKey& key);
	void									GetD3DParameters(D3DPRESENT_PARAMETERS& params);
	void									RenderFullScreen();
  CGUIWindowHome									m_guiHome;
  CGUIWindowPrograms							m_guiPrograms;
	CGUIWindowPictures							m_guiPictures;
	CGUIDialogYesNo									m_guiDialogYesNo;
	CGUIDialogProgress							m_guiDialogProgress;
	CGUIDialogOK										m_guiDialogOK;
	CGUIWindowMyFiles								m_guiMyFiles;
	CGUIWindowMusic									m_guiMyMusic;
	CGUIWindowVideo									m_guiMyVideo;
	CGUIWindowSettings							m_guiSettings;
	CGUIWindowSystemInfo						m_guiSystemInfo;
	CGUIWindowSettingsGeneral				m_guiSettingsGeneral;
	CGUIWindowMusicInfo							m_guiMusicInfo;
	CGUIWindowVideoInfo							m_guiVideoInfo;
	CGUIWindowSettingsScreen				m_guiSettingsScreen;
	CGUIWindowSettingsUICalibration	m_guiSettingsUICalibration;
	CGUIWindowSettingsMovieCalibration m_guiSettingsMovieCalibration;
	CGUIDialogSelect								m_guiDialogSelect;
	CGUIWindowMusicOverlay					m_guiMusicOverlay;
	CGUIWindowFullScreen						m_guiWindowFullScreen;
	CGUIWindowVideoOverlay					m_guiWindowVideoOverlay;
  CXBVirtualKeyboard   						m_keyboard;
	CSNTPClient											m_sntpClient;
	CDetectDVDMedia									m_DetectDVDType;
	CDelayController								m_ctrDpad;
	CDelayController								m_ctrIR;
	Python*													m_pPhytonParser;
	IPlayer*												m_pPlayer;
	bool														m_bSpinDown;
	DWORD														m_dwSpinDownTime;
protected:
	bool										m_bOverlayEnabled;
	vector<int>							m_vecScriptIds;
	typedef vector<int>::iterator ivecScriptIds;
};

extern CApplication g_application;