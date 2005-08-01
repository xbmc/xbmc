
#include "stdafx.h"
#include "GUIDialogMuteBug.h"
#include "guiWindowManager.h"
#include "settings.h"
#include "application.h"
#include "localizeStrings.h"


// the MuteBug is a true modeless dialog

#define MUTEBUG_IMAGE     901

CGUIDialogMuteBug::CGUIDialogMuteBug(void)
    : CGUIDialog(WINDOW_DIALOG_MUTE_BUG, "DialogMuteBug.xml")
{
  m_loadOnDemand = false;
}

CGUIDialogMuteBug::~CGUIDialogMuteBug(void)
{}

bool CGUIDialogMuteBug::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_MUTE_OFF:
    {
      Close();
      return true;
    }
    break;

  case GUI_MSG_MUTE_ON:
    {
      // this is handled in g_application
      // non-active modeless window can not get messages
      //Show(m_gWindowManager.GetActiveWindow());
      return true;
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}
