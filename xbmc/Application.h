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
#include "GUIDialogProgress.h"
#include "GUIWindowSystemInfo.h"
#include "GUIWindowSettingsGeneral.h"
#include "LocalizeStrings.h"
#include "utils/sntp.h"
#include "utils/delaycontroller.h"
#include "keyboard/virtualkeyboard.h"

class CApplication :
  public CXBApplicationEx
{
public:
  CApplication(void);
  virtual ~CApplication(void);
  virtual HRESULT Initialize();
  virtual void		FrameMove();
  virtual void		Render();
	void						Stop();
	void						LoadSkin(const CStdString& strSkin);
  CGUIWindowHome        m_guiHome;
  CGUIWindowPrograms    m_guiPrograms;
	CGUIWindowPictures		m_guiPictures;
	CGUIDialogYesNo				m_guiDialogYesNo;
	CGUIDialogProgress		m_guiDialogProgress;
	CGUIWindowMyFiles			m_guiMyFiles;
	CGUIWindowMusic				m_guiMyMusic;
	CGUIWindowVideo				m_guiMyVideo;
	CGUIWindowSettings		m_guiSettings;
	CGUIWindowSystemInfo	m_guiSystemInfo;
	CGUIWindowSettingsGeneral m_guiSettingsGeneral;
  CXBVirtualKeyboard    m_keyboard;
	CSNTPClient						m_sntpClient;
	CDelayController			m_ctrDpad;
	CDelayController			m_ctrIR;
};

extern CApplication g_application;