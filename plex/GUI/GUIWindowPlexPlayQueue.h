#ifndef GUIWINDOWPLEXPLAYQUEUE_H
#define GUIWINDOWPLEXPLAYQUEUE_H

#include "GUIPlexMediaWindow.h"

class CGUIWindowPlexPlayQueue : public CGUIPlexMediaWindow
{
public:
  CGUIWindowPlexPlayQueue() : CGUIPlexMediaWindow(WINDOW_PLEX_PLAY_QUEUE, "PlexPlayQueue.xml")
  {
  }

  bool OnSelect(int iItem);
  bool OnMessage(CGUIMessage &message);
  void GetContextButtons(int itemNumber, CContextButtons &buttons);
  bool OnContextButton(int itemNumber, CONTEXT_BUTTON button);
};

#endif // GUIWINDOWPLEXPLAYQUEUE_H
