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
#include "GUIWindowMusicInfo.h" 
#include "GUIWindowVideoInfo.h"
#include "LocalizeStrings.h"
#include "utils/sntp.h"
#include "utils/delaycontroller.h"
#include "keyboard/virtualkeyboard.h"
#include "lib/libPython/XboxPython.h"

#include <vector>
using namespace std;

class CApplication :
  public CXBApplicationEx
{
public:
  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT Initialize();
  virtual void		FrameMove();
  virtual void		Render();
	virtual HRESULT Create();

	void						Stop();
	void						LoadSkin(const CStdString& strSkin);
	void						ExecuteScript(const CStdString& strScript);
	void						ProcessScripts();
  CGUIWindowHome        m_guiHome;
  CGUIWindowPrograms    m_guiPrograms;
	CGUIWindowPictures		m_guiPictures;
	CGUIDialogYesNo				m_guiDialogYesNo;
	CGUIDialogProgress		m_guiDialogProgress;
	CGUIDialogOK					m_guiDialogOK;
	CGUIWindowMyFiles			m_guiMyFiles;
	CGUIWindowMusic				m_guiMyMusic;
	CGUIWindowVideo				m_guiMyVideo;
	CGUIWindowSettings		m_guiSettings;
	CGUIWindowSystemInfo	m_guiSystemInfo;
	CGUIWindowSettingsGeneral m_guiSettingsGeneral;
	CGUIWindowMusicInfo		m_guiMusicInfo;
	CGUIWindowVideoInfo   m_guiVideoInfo;
	CGUIDialogSelect			m_guiDialogSelect;
  CXBVirtualKeyboard    m_keyboard;
	CSNTPClient						m_sntpClient;
	CDelayController			m_ctrDpad;
	CDelayController			m_ctrIR;
	Python*								m_pPhytonParser;
protected:
	vector<int>						m_vecScriptIds;
	typedef vector<int>::iterator ivecScriptIds;
};

extern CApplication g_application;