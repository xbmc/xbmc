#include "GUIDialogPlexUserSelect.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "dialogs/GUIDialogNumeric.h"
#include "guilib/GUIWindowManager.h"

#include "FileItem.h"
#include "PlexBusyIndicator.h"
#include "PlexJobs.h"
#include "Client/MyPlex/MyPlexManager.h"
#include "PlexApplication.h"
#include "Third-Party/hash-library/sha256.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIDialogPlexUserSelect::CGUIDialogPlexUserSelect()
  : CGUIDialogSelect(WINDOW_DIALOG_PLEX_USER_SELECT, "DialogPlexUserSelect.xml"), m_authed(false)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexUserSelect::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    m_authed = false;

    SetHeading("Switch to User");
    
    /* Add the old user, so it can work offline as well */
    if (g_plexApplication.myPlexManager)
    {
      CMyPlexUserInfo info = g_plexApplication.myPlexManager->GetCurrentUserInfo();
      if (info.id != -1)
      {
        CFileItemPtr oldUser = CFileItemPtr(new CFileItem);
        oldUser->SetLabel(info.username);
        oldUser->SetProperty("restricted", info.restricted);
        oldUser->SetProperty("protected", !info.pin.empty());
        oldUser->SetProperty("id", info.id);
        oldUser->SetArt("thumb", info.thumb);
        
        Add(oldUser.get());
        SetSelected(info.username);
      }
    }
    
    if (g_plexApplication.myPlexManager->IsSignedIn())
      fetchUsers();
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
//void CGUIDialogPlexUserSelect::OnJobComplete(unsigned int jobID, bool success, CJob *job)
//{
void CGUIDialogPlexUserSelect::fetchUsers()
{
  XFILE::CPlexDirectory dir;
  CFileItemList users;
  if (dir.GetDirectory(CURL("plexserver://myplex/api/home/users"), users))
  {
    std::string currentUsername = g_plexApplication.myPlexManager->GetCurrentUserInfo().username;

    Reset();
    for (int i = 0; i < users.Size(); i ++)
    {
      CFileItemPtr item = users.Get(i);
      if (item->GetProperty("restricted").asBoolean() == false)
        item->ClearProperty("restricted");
      if (item->GetProperty("protected").asBoolean() == false)
        item->ClearProperty("protected");
      if (item->GetProperty("admin").asBoolean() == false)
        item->ClearProperty("admin");
    }
    
    Add(users);
    SetSelected(currentUsername);
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexUserSelect::OnSelected()
{
  bool close = true;
  bool isAdmin = false;
  
  m_authed = false;
  
  if (GetSelectedItem())
    isAdmin = GetSelectedItem()->GetProperty("admin").asBoolean();
  
  CFileItemPtr item = m_vecList->Get(m_viewControl.GetSelectedItem());
  if (item && (item->GetProperty("protected").asBoolean() && !isAdmin))
  {
    bool firstTry = true;
    while (true)
    {
      CStdString pin;
      CGUIDialogNumeric* diag = (CGUIDialogNumeric*)g_windowManager.GetWindow(WINDOW_DIALOG_NUMERIC);
      if (diag)
      {
        CStdString initial = "";
        diag->SetMode(CGUIDialogNumeric::INPUT_PASSWORD, (void*)&initial);
        diag->SetHeading(firstTry ? "Enter PIN" : "Try again...");
        diag->DoModal();
          
        if (!diag->IsConfirmed() || diag->IsCanceled())
        {
          close = false;
          break;
        }
        else
        {
          diag->GetOutput(&pin);
          if (g_plexApplication.myPlexManager->VerifyPin(pin, item->GetProperty("id").asInteger()))
          {
            m_authed = true;
            if (g_plexApplication.myPlexManager->GetCurrentUserInfo().id != item->GetProperty("id").asInteger())
              g_plexApplication.myPlexManager->SwitchHomeUser(item->GetProperty("id").asInteger(), pin);
            break;
          }
          firstTry = false;
        }
      }
    }
  }
  else
  {
    // no PIN needed.
    m_authed = true;
    g_plexApplication.myPlexManager->SwitchHomeUser(item->GetProperty("id").asInteger(-1));
  }

  if (close)
    Close();
}
