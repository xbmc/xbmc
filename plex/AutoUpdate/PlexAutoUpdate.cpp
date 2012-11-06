//
//  PlexAutoUpdate.cpp
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-24.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "PlexAutoUpdate.h"

#ifdef __APPLE__
#include "Mac/PlexAutoUpdateMac.h"
#endif

PlexAutoUpdate
PlexAutoUpdate::GetAutoUpdater()
{
#ifdef __APPLE__
  return PlexAutoUpdateMac();
#endif
  return PlexAutoUpdate();
}
