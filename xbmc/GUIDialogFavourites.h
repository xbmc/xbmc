#pragma once
#include "GUIDialog.h"

class CGUIDialogFavourites :
      public CGUIDialog
{
public:
  CGUIDialogFavourites(void);
  virtual ~CGUIDialogFavourites(void);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void OnInitWindow();

  virtual CFileItem *GetCurrentListItem(int offset = 0);

  virtual bool HasListItems() const { return true; };

protected:
  int GetSelectedItem();
  void OnClick(int item);
  void OnPopupMenu(int item);
  void OnMoveItem(int item, int amount);
  void OnDelete(int item);
  void OnRename(int item);
  void UpdateList();

  CFileItemList m_favourites;
};
