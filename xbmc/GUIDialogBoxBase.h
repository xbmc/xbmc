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
  void SetLine(int iLine, const wstring& strLine);
  void SetLine(int iLine, const string& strLine);
  void SetLine(int iLine, int iString);
  void SetHeading(const wstring& strLine);
  void SetHeading(const string& strLine);
  void SetHeading(int iString);
protected:
  bool m_bConfirmed;
};
