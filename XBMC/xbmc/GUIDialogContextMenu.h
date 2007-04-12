#pragma once
#include "GUIDialog.h"

class CGUIDialogContextMenu :
      public CGUIDialog
{
public:
  CGUIDialogContextMenu(void);
  virtual ~CGUIDialogContextMenu(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void DoModal(int iWindowID = WINDOW_INVALID);
  virtual void OnWindowUnload();
  void ClearButtons();
  int AddButton(int iLabel);
  int AddButton(const CStdString &strButton);
  int GetNumButtons();
  void EnableButton(int iButton, bool bEnable);
  int GetButton();
  float GetWidth();
  float GetHeight();

  static bool BookmarksMenu(const CStdString &strType, const CFileItem *item, float posX, float posY);
  static void SwitchMedia(const CStdString& strType, const CStdString& strPath, float posX, float posY);

protected:
  virtual void OnInitWindow();
  static CStdString GetDefaultShareNameByType(const CStdString &strType);
  static void SetDefault(const CStdString &strType, const CStdString &strDefault);
  static void ClearDefault(const CStdString &strType);

private:
  int m_iNumButtons;
  int m_iClickedButton;
};
