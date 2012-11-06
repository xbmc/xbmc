#include "GUIDialogRating.h"
#include "GUIWindowManager.h"
#include "GUIImage.h"
#include "Key.h"
#include "PlexTypes.h"
#include "threads/SingleLock.h"

CGUIDialogRating::CGUIDialogRating(void)
: CGUIDialog(WINDOW_DIALOG_RATING, "DialogRating.xml")
{
  m_loadType = LOAD_ON_GUI_INIT;
  m_iRating = 5;
  m_bConfirmed = false;
}

CGUIDialogRating::~CGUIDialogRating(void)
{
}

bool CGUIDialogRating::OnAction(const CAction &action)
{
  if (action.GetID() == ACTION_MOVE_LEFT && m_iRating > 0)
  {
    SetRating(m_iRating-1);
    return true;
  }
  else if (action.GetID() == ACTION_MOVE_RIGHT && m_iRating < 10)
  {
    SetRating(m_iRating+1);
    return true;
  }
  else if (action.GetID() == ACTION_SELECT_ITEM)
  {
    m_bConfirmed = true;
    Close(false);
  }
  return CGUIDialog::OnAction(action);
}

void CGUIDialogRating::SetHeading(int iHeading)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 1);
  msg.SetLabel(iHeading);
  
  CSingleTryLock tryLock(g_graphicsContext);
  if (tryLock.IsOwner())
    CGUIDialog::OnMessage(msg);
  else
    g_windowManager.SendThreadMessage(msg, GetID());
}

void CGUIDialogRating::SetTitle(const CStdString& strTitle)
{
  Initialize();
  CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), 2);
  msg.SetLabel(strTitle);

  CSingleTryLock tryLock(g_graphicsContext);
  if (tryLock.IsOwner())
    CGUIDialog::OnMessage(msg);
  else
    g_windowManager.SendThreadMessage(msg, GetID());
}

void CGUIDialogRating::SetRating(int iRating)
{
  Initialize();
  CGUIImage* image = (CGUIImage*)GetControl(10);
  CStdString fileName;
  fileName.Format("rating%d-big.png", iRating);
  image->SetFileName(fileName);
  m_iRating = iRating;
}

int CGUIDialogRating::ShowAndGetInput(int heading, const CStdString& title, int rating)
{
  CGUIDialogRating *dialog = (CGUIDialogRating *)g_windowManager.GetWindow(WINDOW_DIALOG_RATING);
  if (!dialog) return -1;  
  dialog->SetHeading(heading);
  dialog->SetTitle(title);
  dialog->SetRating(rating);
  dialog->m_bConfirmed = false;
  dialog->DoModal();
  if (dialog->m_bConfirmed)
  {
    return dialog->GetRating();
  }
  else
  {
    return -1;
  }

}
