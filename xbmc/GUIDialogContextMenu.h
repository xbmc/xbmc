#pragma once
#include "GUIDialog.h"

class CGUIDialogContextMenu :
      public CGUIDialog
{
public:
  CGUIDialogContextMenu(void);
  virtual ~CGUIDialogContextMenu(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void DoModal(DWORD dwParentId);
  void ClearButtons();
  int AddButton(int iLabel);
  int AddButton(const wstring &strButton);
  int GetNumButtons();
  void EnableButton(int iButton, bool bEnable);
  int GetButton();
  DWORD GetWidth();
  DWORD GetHeight();

  static bool BookmarksMenu(const CStdString &strType, const CStdString &strLabel, const CStdString &strPath, int iLockMode, bool bMaxRetryExceeded, int iPosX, int iPosY);

protected:
  virtual void OnInitWindow();
  static bool CheckMasterCode(int iLockMode);
private:
  int m_iNumButtons;
  int m_iClickedButton;
};
