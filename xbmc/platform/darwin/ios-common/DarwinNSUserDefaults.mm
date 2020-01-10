/*
 *  Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "DarwinNSUserDefaults.h"

#import "filesystem/SpecialProtocol.h"
#import "utils/URIUtils.h"
#import "utils/log.h"

#import "platform/darwin/ios-common/NSData+GZIP.h"
#import "platform/darwin/tvos/filesystem/TVOSFileUtils.h"

#import <string>

#import <Foundation/NSData.h>
#import <Foundation/NSDictionary.h>
#import <Foundation/NSString.h>
#import <Foundation/NSUserDefaults.h>

static bool firstLookup = true;

static bool translatePathIntoKey(const std::string& path, std::string& key)
{
  if (firstLookup)
  {
    NSDictionary<NSString*, id>* dict =
        [[NSUserDefaults standardUserDefaults] dictionaryRepresentation];
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    for (NSString* aKey in dict.allKeys)
    {
      // do something like a log:
      if (![aKey hasPrefix:@"/userdata/"])
        continue;

      NSData* nsdata = [defaults dataForKey:aKey];
      size_t size = nsdata.length;
      CLog::Log(LOGNOTICE, "nsuserdefaults: %s with size %ld", aKey.UTF8String, size);
    }
    firstLookup = false;
  }
  size_t pos;
  std::string translated_key = CSpecialProtocol::TranslatePath(path);
  std::string userDataDir =
      URIUtils::AddFileToFolder(CTVOSFileUtils::GetUserHomeDirectory(), "userdata");
  if (translated_key.find(userDataDir) != std::string::npos)
  {
    if ((pos = translated_key.find("/userdata")) != std::string::npos)
    {
      key = translated_key.erase(0, pos);
      return true;
    }
  }

  return false;
}

bool CDarwinNSUserDefaults::Synchronize()
{
  return [[NSUserDefaults standardUserDefaults] synchronize] == YES;
}

void CDarwinNSUserDefaults::GetDirectoryContents(const std::string& path,
                                                 std::vector<std::string>& contents)
{
  // tvos path adds /private/../..
  // We need to strip this as GetUserHomeDirectory() doesnt have private in the path
  std::string subpath = path;
  const std::string& str_private = "/private";
  size_t pos = subpath.find(str_private.c_str(), 0, str_private.length());

  if (pos != std::string::npos)
    subpath.erase(pos, str_private.length());

  std::string userDataDir =
      URIUtils::AddFileToFolder(CTVOSFileUtils::GetUserHomeDirectory(), "userdata");

  if (subpath.find(userDataDir) == std::string::npos)
    return;

  NSDictionary<NSString*, id>* dict =
      [[NSUserDefaults standardUserDefaults] dictionaryRepresentation];
  for (NSString* aKey in dict.allKeys)
  {
    // do something like a log:
    if (![aKey hasPrefix:@"/userdata/"])
      continue;

    std::string keypath = aKey.UTF8String;
    std::string fullKeyPath =
        URIUtils::AddFileToFolder(CTVOSFileUtils::GetUserHomeDirectory(), keypath);
    std::string endingDirectory = URIUtils::GetDirectory(fullKeyPath);
    if (subpath == endingDirectory)
    {
      contents.push_back(fullKeyPath);
    }
  }
}

bool CDarwinNSUserDefaults::GetKey(const std::string& key, std::string& value)
{
  if (!key.empty())
  {
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSString* nsstring_key = @(key.c_str());
    NSString* nsstring_value = [defaults stringForKey:nsstring_key];
    if (nsstring_value)
    {
      value = nsstring_value.UTF8String;
      if (!value.empty())
        return true;
    }
  }

  return false;
}

bool CDarwinNSUserDefaults::GetKeyData(const std::string& key, void* lpBuf, size_t& uiBufSize)
{
  if (key.empty())
  {
    uiBufSize = 0;
    return false;
  }

  NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
  NSString* nsstring_key = @(key.c_str());
  NSData* nsdata = [defaults dataForKey:nsstring_key];
  if (!nsdata)
  {
    uiBufSize = 0;
    return false;
  }

  NSData* decompressed = nsdata;
  if ([nsdata isGzippedData])
    decompressed = [nsdata gunzippedData];

  uiBufSize = decompressed.length;

  // call was to get size of file
  if (lpBuf == nullptr)
    return true;

  memcpy(lpBuf, decompressed.bytes, decompressed.length);
  return true;
}

bool CDarwinNSUserDefaults::SetKey(const std::string& key,
                                   const std::string& value,
                                   bool synchronize)
{
  if (!key.empty() && !value.empty())
  {
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];

    NSString* nsstring_key = @(key.c_str());
    NSString* nsstring_value = @(value.c_str());

    [defaults setObject:nsstring_value forKey:nsstring_key];
    if (synchronize)
      return [defaults synchronize] == YES;
    else
      return true;
  }

  return false;
}

bool CDarwinNSUserDefaults::SetKeyData(const std::string& key,
                                       const void* lpBuf,
                                       size_t uiBufSize,
                                       bool synchronize)
{
  if (!key.empty() && lpBuf != nullptr && uiBufSize > 0)
  {
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSString* nsstring_key = @(key.c_str());
    NSData* nsdata_value = [NSData dataWithBytes:lpBuf length:uiBufSize];

    NSData* compressed = [nsdata_value gzippedData];
    CLog::Log(LOGDEBUG, "NSUSerDefaults: compressed %s from %ld to %ld", key.c_str(), uiBufSize,
              compressed.length);

    [defaults setObject:compressed forKey:nsstring_key];
    if (synchronize)
      return [defaults synchronize] == YES;
    else
      return true;
  }

  return false;
}

bool CDarwinNSUserDefaults::DeleteKey(const std::string& key, bool synchronize)
{
  if (!key.empty())
  {
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSString* nsstring_key = @(key.c_str());
    [defaults removeObjectForKey:nsstring_key];
    if (synchronize)
      return [defaults synchronize] == YES;
    else
      return true;
  }

  return false;
}

bool CDarwinNSUserDefaults::KeyExists(const std::string& key)
{
  if (!key.empty())
  {
    NSString* nsstring_key = @(key.c_str());
    if ([[NSUserDefaults standardUserDefaults] objectForKey:nsstring_key])
      return true;
  }

  return false;
}

bool CDarwinNSUserDefaults::IsKeyFromPath(const std::string& path)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
  {
    CLog::Log(LOGDEBUG, "found key %s", translated_key.c_str());
    return true;
  }

  return false;
}

bool CDarwinNSUserDefaults::GetKeyFromPath(const std::string& path, std::string& value)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CDarwinNSUserDefaults::GetKey(translated_key, value);

  return false;
}

bool CDarwinNSUserDefaults::GetKeyDataFromPath(const std::string& path,
                                               void* lpBuf,
                                               size_t& uiBufSize)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CDarwinNSUserDefaults::GetKeyData(translated_key, lpBuf, uiBufSize);

  return false;
}

bool CDarwinNSUserDefaults::SetKeyFromPath(const std::string& path,
                                           const std::string& value,
                                           bool synchronize)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty() && !value.empty())
    return CDarwinNSUserDefaults::SetKey(translated_key, value, synchronize);

  return false;
}

bool CDarwinNSUserDefaults::SetKeyDataFromPath(const std::string& path,
                                               const void* lpBuf,
                                               size_t uiBufSize,
                                               bool synchronize)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CDarwinNSUserDefaults::SetKeyData(translated_key, lpBuf, uiBufSize, synchronize);

  return false;
}

bool CDarwinNSUserDefaults::DeleteKeyFromPath(const std::string& path, bool synchronize)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CDarwinNSUserDefaults::DeleteKey(translated_key, synchronize);

  return false;
}

bool CDarwinNSUserDefaults::KeyFromPathExists(const std::string& path)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CDarwinNSUserDefaults::KeyExists(translated_key);

  return false;
}
