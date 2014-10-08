#include "GUIDialogPlexUserSelect.h"
#include "FileItem.h"
#include "PlexBusyIndicator.h"
#include "PlexJobs.h"
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
    g_plexApplication.busy.blockWaitingForJob(new CPlexDirectoryFetchJob(CURL("plexserver://staging2/api/home/users")), this);
 }

  return CGUIDialogSelect::OnMessage(message);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void CGUIDialogPlexUserSelect::OnJobComplete(unsigned int jobID, bool success, CJob *job)
{
  if (job && success)
  {
    CPlexDirectoryFetchJob* fjob = static_cast<CPlexDirectoryFetchJob*>(job);
    if (fjob)
      Add(fjob->m_items);
  }
}
