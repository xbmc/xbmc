#pragma once

#ifdef WITH_LINKS_BROWSER

#include "GUIWindow.h"
#include "GUIWebBrowserControl.h"

class CGUIWindowWebBrowser : public CGUIWindow
{
public:
  CGUIWindowWebBrowser(void);
  virtual ~CGUIWindowWebBrowser(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  virtual void AllocResources(bool forceLoad);
  virtual void FreeResources(bool forceLoad);

  virtual void Render();

protected:
  BOOL m_bKeepEngineRunning;
  bool m_bShowWebPageInfo;
  bool m_bShowPlayerInfo;
  CStdString m_strCurrentTitle;
  CStdString m_strCurrentURL;
  CStdString m_strCurrentStatus;
  CStdString m_strInitialURL;

};

#endif /* WITH_LINKS_BROWSER */