#pragma once
#include "GUIDialog.h"
#include "guilistitem.h"

class CGUIDialogSelect :
      public CGUIDialog
{
public:
  CGUIDialogSelect(void);
  virtual ~CGUIDialogSelect(void);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void Close();
  void Reset();
  void Add(const CStdString& strLabel);
  int GetSelectedLabel() const;
  const CStdString& GetSelectedLabelText();
  void SetHeading(const wstring& strLine);
  void SetHeading(const string& strLine);
  void SetHeading(int iString);
  void EnableButton(bool bOnOff);
  void SetButtonLabel(int iString);
  bool IsButtonPressed();
  void Sort(bool bSortAscending = true);
protected:
  bool m_bButtonEnabled;
  bool m_bButtonPressed;
  int m_iSelected;
  CStdString m_strSelected;
  vector<CGUIListItem*> m_vecList;
};
