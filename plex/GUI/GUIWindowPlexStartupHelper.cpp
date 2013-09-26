//
//  GUIWindowPlexStartupHelper.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-09-25.
//
//

#include "GUIWindowPlexStartupHelper.h"
#include "FileItem.h"
#include "PlexTypes.h"
#include "StdString.h"
#include "Variant.h"
#include "ApplicationMessenger.h"

CGUIWindowPlexStartupHelper::CGUIWindowPlexStartupHelper() :
  CGUIWindow(WINDOW_PLEX_STARTUP_HELPER, "PlexStartupHelper.xml")
{
  m_loadType = LOAD_EVERY_TIME;
  m_item = CFileItemPtr(new CFileItem);
  SetPage(WIZARD_PAGE_WELCOME);
}

bool CGUIWindowPlexStartupHelper::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetSenderId() == 121)
    {
      switch (m_page) {
        case WIZARD_PAGE_WELCOME:
          SetPage(WIZARD_PAGE_MYPLEX);
          break;
        case WIZARD_PAGE_MYPLEX:
          SetPage(WIZARD_PAGE_AUDIO);
          break;
        case WIZARD_PAGE_AUDIO:
          CApplicationMessenger::Get().ExecBuiltIn("XBMC.ActivateWindow(Home)");
          break;
        default:
          break;
      }
    }
  }
  return CGUIWindow::OnMessage(message);
}

void CGUIWindowPlexStartupHelper::SetPage(WizardPage page)
{
  m_page = page;
  m_item->SetProperty("WizardWelcome", CVariant());
  m_item->SetProperty("WizardMyPlex", CVariant());
  m_item->SetProperty("WizardAudio", CVariant());
  
  if (page == WIZARD_PAGE_WELCOME)
    m_item->SetProperty("WizardWelcome", 1);
  else if (page == WIZARD_PAGE_MYPLEX)
    m_item->SetProperty("WizardMyPlex", 1);
  else if (page == WIZARD_PAGE_AUDIO)
    m_item->SetProperty("WizardAudio", 1);
}