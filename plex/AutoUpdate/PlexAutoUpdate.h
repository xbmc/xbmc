//
//  PlexAutoUpdate.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-24.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXAUTOUPDATE_H
#define PLEXAUTOUPDATE_H

class PlexAutoUpdate
{
  public:
    PlexAutoUpdate() {};
    static PlexAutoUpdate GetAutoUpdater();

    virtual void checkForUpdate() {};
};

#endif // PLEXAUTOUPDATE_H
