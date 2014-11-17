#include "GUIDialogPlexUserSelect.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/GUIWindowManager.h"

#include "FileItem.h"
#include "PlexBusyIndicator.h"
#include "PlexJobs.h"
#include "Client/MyPlex/MyPlexManager.h"
#include "PlexApplication.h"

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
    SetHeading("Switch to User");
    g_plexApplication.busy.blockWaitingForJob(new CPlexDirectoryFetchJob(CURL("plexserver://myplex/api/home/users")), this);
  }

  return CGUIDialogSelect::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexUserSelect::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_SELECT_ITEM)
  {
    OnSelected();
    return true;
  }

  return CGUIDialogSelect::OnAction(action);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexUserSelect::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  std::string currentUsername = g_plexApplication.myPlexManager->GetCurrentUserInfo().username;

  if (job && success)
  {
    CPlexDirectoryFetchJob* fjob = static_cast<CPlexDirectoryFetchJob*>(job);
    if (fjob)
    {
      for (int i = 0; i < fjob->m_items.Size(); i ++)
      {
        CFileItemPtr item = fjob->m_items.Get(i);
        if (item->GetProperty("restricted").asInteger() == 0)
          item->ClearProperty("restricted");
        if (item->GetProperty("protected").asInteger() == 0)
          item->ClearProperty("protected");
        if (item->GetProperty("admin").asInteger() == 0)
          item->ClearProperty("admin");
      }

      Add(fjob->m_items);
      SetSelected(currentUsername);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexUserSelect::OnSelected()
{
  bool close = true;
  CFileItemPtr item = m_vecList->Get(m_viewControl.GetSelectedItem());
  if (item)
  {
    CStdString pin;
    if (item->GetProperty("protected").asInteger() == 1)
    {
      CGUIDialogNumeric* diag = (CGUIDialogNumeric*)g_windowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
      if (diag)
      {
        CStdString initial = "";
        diag->SetMode(CGUIDialogNumeric::INPUT_PASSWORD, (void*)&initial);
        diag->SetHeading("Enter PIN");
        diag->DoModal();

        if (diag->IsAutoClosed() && (!diag->IsConfirmed() || diag->IsCanceled()))
          close = false;
        else
          diag->GetOutput(&pin);
      }
    }
    g_plexApplication.myPlexManager->SwitchHomeUser(item->GetProperty("id").asInteger(-1), pin);
  }

  if (close)
    Close();
}
