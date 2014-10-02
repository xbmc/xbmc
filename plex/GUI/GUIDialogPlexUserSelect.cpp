#include "GUIDialogPlexUserSelect.h"
#include "FileItem.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIDialogPlexUserSelect::CGUIDialogPlexUserSelect()
  : CGUIDialogSelect(WINDOW_DIALOG_PLEX_USER_SELECT, "DialogPlexUserSelect.xml")
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexUserSelect::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    SetHeading("Hello users");

    CFileItemList list;
    CFileItemPtr user = CFileItemPtr(new CFileItem);
    user->SetLabel("Maru");
    user->SetArt("thumb", "http://mousebreath.com/wp-content/uploads/2011/08/maru__02.jpg");

    list.Add(user);

    user = CFileItemPtr(new CFileItem);
    user->SetLabel("Malört");
    user->SetArt("thumb", "https://encrypted-tbn2.gstatic.com/images?q=tbn:ANd9GcTL6jvyGu9b1xJAnhgb9bqEWGFbM79j8neTjV-wik5PnRDD3W0wHQ");

    list.Add(user);

    Add(list);
  }

  return CGUIDialogSelect::OnMessage(message);
}
