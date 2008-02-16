#pragma once
#include "../guilib/GUIDialog.h"

class CGUIDialogBoxBase :
      public CGUIDialog
{
public:
  CGUIDialogBoxBase(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIDialogBoxBase(void);
  virtual bool OnMessage(CGUIMessage& message);
  bool IsConfirmed() const;
  void SetLine(int iLine, const std::string& strLine);
  void SetLine(int iLine, int iString);
  void SetHeading(const std::string& strLine);
  void SetHeading(int iString);
  void SetChoice(int iButton, int iString);
  void SetChoice(int iButton, const std::string& strString);
protected:
  virtual void OnInitWindow();
  bool m_bConfirmed;
};
