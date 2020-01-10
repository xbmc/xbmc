/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "tvosShared.h"

@implementation tvosShared

+ (NSString*)getSharedID
{
  return [@"group." stringByAppendingString:[self mainAppBundle].bundleIdentifier];
}

+ (NSURL*)getSharedURL
{
  NSString* sharedID = [self getSharedID];
  if ([self isJailbroken])
    return [[NSURL fileURLWithPath:@"/var/mobile/Library/Caches"]
        URLByAppendingPathComponent:sharedID];
  else
  {
    NSFileManager* fileManager = [NSFileManager defaultManager];
    NSURL* sharedUrl = [fileManager containerURLForSecurityApplicationGroupIdentifier:sharedID];
    sharedUrl = [sharedUrl URLByAppendingPathComponent:@"Library" isDirectory:YES];
    sharedUrl = [sharedUrl URLByAppendingPathComponent:@"Caches" isDirectory:YES];
    return sharedUrl;
  }
}

+ (BOOL)IsTVOSSandboxed
{
  // @todo merge with CDarwinUtils::IsIosSandboxed
  static BOOL ret;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    // we re NOT sandboxed if we are installed in /var/mobile/Applications with greeng0blin jailbreak
    ret = ![[self mainAppBundle].bundlePath containsString:@"/var/mobile/Applications/"];
  });
  return ret;
}

+ (BOOL)isJailbroken
{
  return ![self IsTVOSSandboxed];
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
