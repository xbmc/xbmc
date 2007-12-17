#ifndef GUI_DIALOG_ACCES_POINTS
#define GUI_DIALOG_ACCES_POINTS

#pragma once

#include <vector>
#include "GUIDialog.h"
#include "Network.h"

class CGUIDialogAccessPoints : public CGUIDialog
{
public:
  CGUIDialogAccessPoints(void);
  virtual ~CGUIDialogAccessPoints(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);
  void SetInterfaceName(CStdString interfaceName);
  CStdString GetSelectedAccessPointEssId();
  EncMode GetSelectedAccessPointEncMode();
  bool WasItemSelected();
  
private:
  std::vector<NetworkAccessPoint> m_aps;
  CStdString m_interfaceName;
  CStdString m_selectedAPEssId;
  EncMode m_selectedAPEncMode;
  bool m_wasItemSelected;
};

#endif
