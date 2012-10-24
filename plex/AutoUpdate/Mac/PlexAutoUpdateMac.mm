//
//  PlexAutoUpdateMac.mm
//  Plex
//
//  Created by Tobias Hieta <tobias@plexapp.com> on 2012-10-22.
//  Copyright 2012 Plex Inc. All rights reserved.
//

#include "PlexAutoUpdateMac.h"
#include <Sparkle/Sparkle.h>
#include "log.h"

PlexAutoUpdateMac::PlexAutoUpdateMac() : PlexAutoUpdate()
{
  updateDelegate = [[PlexUpdaterDelegate alloc] init];
  [updateDelegate retain];
  
  SUUpdater *updater = [SUUpdater sharedUpdater];
  [updater setDelegate:updateDelegate];
  [updater setAutomaticallyChecksForUpdates:YES];
  [updater setAutomaticallyDownloadsUpdates:YES];
  
  CLog::Log(LOGDEBUG, "PlexAutoUpdate is inited..");
  
  checkForUpdate();
}

void
PlexAutoUpdateMac::checkForUpdate()
{
  CLog::Log(LOGDEBUG, "Looking for a update");
  [[SUUpdater sharedUpdater] checkForUpdatesInBackground];
}

PlexAutoUpdateMac::~PlexAutoUpdateMac()
{
  [updateDelegate release];
}

@implementation PlexUpdaterDelegate

- (BOOL)updaterShouldPromptForPermissionToCheckForUpdates:(SUUpdater *)bundle
{
  return NO;
}

- (void)updater:(SUUpdater *)updater didFindValidUpdate:(SUAppcastItem *)update
{
  CLog::Log(LOGINFO, "PlexAutoUpdate: Found a valid update");
}

- (void)updaterDidNotFindUpdate:(SUUpdater *)update
{
  CLog::Log(LOGDEBUG, "PlexAutoUpdate: Didn't find a update when polling");
}

@end