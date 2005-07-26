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
  virtual bool OnAction(const CAction &action);
  virtual void Render();
protected:
  bool OnPollXLinkClient(CGUIConditionalButtonControl* pButton);
  void OnPopupContextMenu();
  void OnClickOnlineGaming(CGUIMessage& aMessage);
  void UpdateButtonScroller();

  IDirect3DTexture8* m_pTexture;
  int m_iLastControl;
  int m_iLastMenuOption;
  int m_iSelectedItem;
};
