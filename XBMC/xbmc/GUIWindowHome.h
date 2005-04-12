#pragma once
#include "guiwindow.h"

class CGUIConditionalButtonControl;

class CGUIWindowHome :
      public CGUIWindow
{
public:

  CGUIWindowHome(void);
  virtual ~CGUIWindowHome(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual void OnAction(const CAction &action);
  virtual void Render();
  virtual void OnWindowLoaded();

protected:
  bool OnPollXLinkClient(CGUIConditionalButtonControl* pButton);
  void OnPopupContextMenu();
  void OnClickShutdown(CGUIMessage& aMessage);
  void OnClickDashboard(CGUIMessage& aMessage);
  void OnClickReboot(CGUIMessage& aMessage);
  void OnClickCredits(CGUIMessage& aMessage);
  void OnClickOnlineGaming(CGUIMessage& aMessage);
  void UpdateButtonScroller();

  IDirect3DTexture8* m_pTexture;
  int m_iLastControl;
  int m_iLastMenuOption;
};
