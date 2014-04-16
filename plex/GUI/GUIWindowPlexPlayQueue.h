#ifndef GUIWINDOWPLEXPLAYQUEUE_H
#define GUIWINDOWPLEXPLAYQUEUE_H

#include "GUIPlexMediaWindow.h"

class CGUIWindowPlexPlayQueue : public CGUIPlexMediaWindow
{
public:
  CGUIWindowPlexPlayQueue() : CGUIPlexMediaWindow(WINDOW_PLEX_PLAY_QUEUE, "PlexPlayQueue.xml")
  {
  }

};

#endif // GUIWINDOWPLEXPLAYQUEUE_H
