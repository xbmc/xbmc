//
//  PlexAutoUpdateMac.h
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-22.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#ifndef PLEXAUTOUPDATEMAC_H
#define PLEXAUTOUPDATEMAC_H

#include "AutoUpdate/PlexAutoUpdate.h"

#ifdef __OBJC__
#include <Cocoa/Cocoa.h>
#include <Sparkle/Sparkle.h>

@interface PlexUpdaterDelegate : NSObject
@end
#endif

class PlexAutoUpdateMac : public PlexAutoUpdate
{
  public:
    PlexAutoUpdateMac();
    virtual void checkForUpdate();
    ~PlexAutoUpdateMac();
#ifdef __OBJC__
  private:
    PlexUpdaterDelegate *updateDelegate;
#endif
};

#endif // PLEXAUTOUPDATEMAC_H
