//
//  MediaCenterLaunchHost.cpp
//  Plex
//
//  Created by Max Feingold on 10/27/2011.
//  Copyright 2011 Plex Inc. All rights reserved.
//

#include "MediaCenterLaunchHost.h"

void MediaCenterLaunchHost::OnStartup()
{
  // Nothing to do that we're aware of... yet
}

void MediaCenterLaunchHost::OnShutdown()
{
  // Restore the MCE window to its previous glory
  HWND hwndMCE = FindWindowW(L"eHome Render Window", NULL);
  if (hwndMCE)
    ShowWindow(hwndMCE, SW_RESTORE);
}