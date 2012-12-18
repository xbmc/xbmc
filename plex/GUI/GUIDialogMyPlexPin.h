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
#include "MyPlexManager.h"

class CGUIDialogMyPlexPin : public CGUIDialogBoxBase
{
  public:
    CGUIDialogMyPlexPin() : CGUIDialogBoxBase(WINDOW_DIALOG_MYPLEX_PIN, "DialogOK.xml"), m_done(false) {};

    virtual bool OnMessage(CGUIMessage &message);
    void SetButtonText(const CStdString& text);
    int GetDefaultLabelID(int controlId) const;

    static void ShowAndGetInput();
    MyPlexPinLogin m_pinLogin;


  private:
    bool m_done;
};

#endif // GUIDIALOGMYPLEXPIN_H
