#pragma once
#include "guiwindow.h"

#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
class CGUIConditionalButtonControl;
#endif

class CGUIWindowHome :
      public CGUIWindow
{
public:

  CGUIWindowHome(void);
  virtual ~CGUIWindowHome(void);

  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  virtual void Render();
#endif
protected:
#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  bool OnPollXLinkClient(CGUIConditionalButtonControl* pButton);
  void OnClickOnlineGaming(CGUIMessage& aMessage);
  void FadeBackgroundImages(int focusedControl, int messageSender, int associatedImage);
#endif

  int m_iLastControl;
  int m_iLastMenuOption;
  int m_iSelectedItem;
};
