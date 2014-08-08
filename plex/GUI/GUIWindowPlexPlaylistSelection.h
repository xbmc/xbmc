//
//  GUIWindowPlexPlaylistSelection.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 06/08/14.
//
//

#ifndef __Plex_Home_Theater__GUIWindowPlexPlaylistSelection__
#define __Plex_Home_Theater__GUIWindowPlexPlaylistSelection__

#include "windows/GUIMediaWindow.h"

class CGUIWindowPlexPlaylistSelection : public CGUIMediaWindow
{
public:
  CGUIWindowPlexPlaylistSelection();
  
private:
  bool OnSelect(int item);
};

#endif /* defined(__Plex_Home_Theater__GUIWindowPlexPlaylistSelection__) */
