#pragma once
#include "guiwindow.h"
#include "guiwindowmanager.h"
#include "osd/osdmenu.h"
#include "osd/iexecutor.h"
#include "utils/CriticalSection.h"
using namespace OSD;
class CGUIWindowFullScreen :
  public CGUIWindow,
  public IExecutor
{
public:
  CGUIWindowFullScreen(void);
  virtual ~CGUIWindowFullScreen(void);
  virtual void    AllocResources();
  virtual void    FreeResources();
  virtual bool    OnMessage(CGUIMessage& message);
  virtual void    OnAction(const CAction &action);
  virtual void	  OnMouse();
  virtual void		Render();
	void				    RenderFullScreen();
  bool            NeedRenderFullScreen();
  bool            OSDVisible() const;
  void						ChangetheTimeCode(DWORD remote);  
  void						ChangetheSpeed(DWORD action);
  virtual void    OnExecute(int iAction, const IOSDOption* option);
  bool            m_bOSDVisible;
  void			  SetViewMode(int iViewMode);
	bool				m_bSmoothFFwdRewd;
	bool				m_bDiscreteFFwdRewd;

private:
	void				Update();
	void				ShowOSD();
	void				HideOSD();

	bool				m_bShowTime;
	bool				m_bShowCodecInfo;
	bool				m_bShowViewModeInfo;
	DWORD				m_dwShowViewModeTimeout;
	bool				m_bShowCurrentTime;
	DWORD				m_dwTimeCodeTimeout;
	DWORD				m_dwFPSTime;
	float				m_fFrameCounter;
	FLOAT				m_fFPS;
	COSDMenu			m_osdMenu;
	CCriticalSection	m_section;
	bool				m_bLastRender;
	int					m_strTimeStamp[5];
	int					m_iTimeCodePosition;
	int					m_iCurrentBookmark;
	DWORD				m_dwOSDTimeOut;
};
