/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "tvosShared.h"

#include "platform/darwin/ios-common/DarwinEmbedUtils.h"

@implementation tvosShared

+ (NSString*)getSharedID
{
  return [@"group." stringByAppendingString:[self mainAppBundle].bundleIdentifier];
}

+ (NSURL*)getSharedURL
{
  NSString* sharedID = [self getSharedID];
  if (CDarwinEmbedUtils::IsIosSandboxed())
  {
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSURL* sharedUrl = [fileManager containerURLForSecurityApplicationGroupIdentifier:sharedID];
    // e.g. /private/var/mobile/Containers/Shared/AppGroup/32B9DA1F-3B1F-4DBC-8326-ABB08BF16EC9/
    sharedUrl = [sharedUrl URLByAppendingPathComponent:@"Library" isDirectory:YES];
    sharedUrl = [sharedUrl URLByAppendingPathComponent:@"Caches" isDirectory:YES];
    return sharedUrl;
  }
  else
  {
    return [[NSURL fileURLWithPath:@"/var/mobile/Library/Caches"]
        URLByAppendingPathComponent:sharedID];
  }
}

+ (NSBundle*)mainAppBundle
{
  NSBundle* bundle = NSBundle.mainBundle;
  if ([bundle.bundleURL.pathExtension isEqualToString:@"appex"])
  { // We're in a extension
    // Peel off two directory levels - Kodi.app/PlugIns/MY_APP_EXTENSION.appex
    bundle = [NSBundle bundleWithURL:bundle.bundleURL.URLByDeletingLastPathComponent
                                         .URLByDeletingLastPathComponent];
  }
  return bundle;
}

@end
