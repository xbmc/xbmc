//
//  GUIDialogMyPlexPin.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-12-14.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef GUIDIALOGMYPLEXPIN_H
#define GUIDIALOGMYPLEXPIN_H

#include <dialogs/GUIDialogBoxBase.h>
#include "PlexTypes.h"
#include "Client/MyPlex/MyPlexManager.h"

class CGUIWindowMyPlex : public CGUIWindow
{
  public:
    CGUIWindowMyPlex() : CGUIWindow(WINDOW_MYPLEX_LOGIN, "MyPlexLogin.xml"), m_goHome(false), m_failed(false) {};

    virtual bool OnMessage(CGUIMessage &message);
    static void ShowAndGetInput();
    virtual bool OnAction(const CAction &action);

    void Setup();
    void ShowManualInput();
    void ShowPinInput();

    void ShowSuccess();
    void ShowFailure(int reason);

    void Close(bool forceClose = false, int nextWindowID = 0, bool enableSound = true, bool bWait = true);

    void ToggleInput()
    {
      if (m_manual)
        ShowPinInput();
      else
        ShowManualInput();
    }

    void HandleMyPlexState(int state, int errorCode);

  private:
    bool m_manual;
    bool m_goHome;
    bool m_failed;
};

#endif // GUIDIALOGMYPLEXPIN_H
