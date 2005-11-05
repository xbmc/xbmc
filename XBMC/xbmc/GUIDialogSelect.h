#pragma once
#include "GUIDialogBoxBase.h"
#include "guilistitem.h"

class CGUIDialogSelect :
      public CGUIDialogBoxBase
{
public:
  CGUIDialogSelect(void);
  virtual ~CGUIDialogSelect(void);
  virtual bool OnMessage(CGUIMessage& message);

  virtual void Close(bool forceClose = false);
  void Reset();
  void Add(const CStdString& strLabel);
  int GetSelectedLabel() const;
  const CStdString& GetSelectedLabelText();
  void EnableButton(bool bOnOff);
  void SetButtonLabel(int iString);
  bool IsButtonPressed();
  void Sort(bool bSortAscending = true);
  void SetSelected(int iSelected);
protected:
  bool m_bButtonEnabled;
  bool m_bButtonPressed;
  int m_iSelected;
  CStdString m_strSelected;
  vector<CGUIListItem*> m_vecList;
};
