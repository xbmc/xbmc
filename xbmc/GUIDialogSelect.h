#pragma once
#include "GUIDialogBoxBase.h"
#include "GUIListItem.h"

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
  void Add(CFileItem* pItem);
  int GetSelectedLabel() const;
  const CStdString& GetSelectedLabelText();
  const CFileItem& GetSelectedItem();
  void EnableButton(bool bOnOff);
  void SetButtonLabel(int iString);
  bool IsButtonPressed();
  void Sort(bool bSortOrder = true);
  void SetSelected(int iSelected);
protected:
  bool m_bButtonEnabled;
  bool m_bButtonPressed;
  int m_iSelected;

  CFileItem m_selectedItem;
  vector<CFileItem*> m_vecList;
};
