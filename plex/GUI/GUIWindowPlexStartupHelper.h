//
//  GUIWindowPlexStartupHelper.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-09-25.
//
//

#ifndef __Plex_Home_Theater__GUIWindowPlexStartupHelper__
#define __Plex_Home_Theater__GUIWindowPlexStartupHelper__

#include <iostream>
#include "guilib/GUIDialog.h"
#include "guilib/GUIMessage.h"

typedef enum {
  WIZARD_PAGE_WELCOME,
  WIZARD_PAGE_MYPLEX,
  WIZARD_PAGE_AUDIO
} WizardPage;

class CGUIWindowPlexStartupHelper : public CGUIWindow {
  
public:
  CGUIWindowPlexStartupHelper();
  
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool HasListItems() const { return true; }
  virtual CFileItemPtr GetCurrentListItem(int offset = 0) { return m_item; }

  void AudioControlSelected(int id);
  
  void SetupAudioStuff();

  static int GetNumberOfHDMIChannels();

private:
  
  void SetPage(WizardPage page);
  
  CFileItemPtr m_item;
  WizardPage m_page;
  
};

#endif /* defined(__Plex_Home_Theater__GUIWindowPlexStartupHelper__) */
