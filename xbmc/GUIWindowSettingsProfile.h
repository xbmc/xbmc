#pragma once
#include "GUIWindow.h"

class CGUIWindowSettingsProfile :
      public CGUIWindow
{
public:
  CGUIWindowSettingsProfile(void);
  virtual ~CGUIWindowSettingsProfile(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

protected:
  virtual void OnInitWindow();
  std::vector<CFileItem*> m_vecListItems;

  void OnPopupMenu(int iItem);
  void DoRename(int iItem);
  void DoOverwrite(int iItem);
  int GetSelectedItem();
  void LoadList();
  void SetLastLoaded();
  void ClearListItems();
};
