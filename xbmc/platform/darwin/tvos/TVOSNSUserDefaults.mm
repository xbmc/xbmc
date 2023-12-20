/*
 *  Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "TVOSNSUserDefaults.h"

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
      CLog::Log(LOGINFO, "nsuserdefaults: {} with size {}", aKey.UTF8String, size);
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

bool CTVOSNSUserDefaults::Synchronize()
{
  return [[NSUserDefaults standardUserDefaults] synchronize] == YES;
}

void CTVOSNSUserDefaults::GetDirectoryContents(const std::string& path,
                                               std::vector<std::string>& contents)
{
  std::string userDataDir =
      URIUtils::AddFileToFolder(CTVOSFileUtils::GetUserHomeDirectory(), "userdata");

  if (path.find(userDataDir) == std::string::npos)
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
    if (path == endingDirectory)
    {
      contents.push_back(fullKeyPath);
    }
  }
}

bool CTVOSNSUserDefaults::GetKey(const std::string& key, std::string& value)
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

bool CTVOSNSUserDefaults::GetKeyData(const std::string& key, void* lpBuf, size_t& uiBufSize)
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

bool CTVOSNSUserDefaults::SetKey(const std::string& key, const std::string& value, bool synchronize)
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

bool CTVOSNSUserDefaults::SetKeyData(const std::string& key,
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
    CLog::Log(LOGDEBUG, "NSUSerDefaults: compressed {} from {} to {}", key, uiBufSize,
              compressed.length);

    [defaults setObject:compressed forKey:nsstring_key];
    if (synchronize)
      return [defaults synchronize] == YES;
    else
      return true;
  }

  return false;
}

bool CTVOSNSUserDefaults::DeleteKey(const std::string& key, bool synchronize)
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

bool CTVOSNSUserDefaults::KeyExists(const std::string& key)
{
  if (!key.empty())
  {
    NSString* nsstring_key = @(key.c_str());
    if ([[NSUserDefaults standardUserDefaults] objectForKey:nsstring_key])
      return true;
  }

  return false;
}

bool CTVOSNSUserDefaults::IsKeyFromPath(const std::string& path)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
  {
    CLog::Log(LOGDEBUG, "found key {}", translated_key);
    return true;
  }

  return false;
}

bool CTVOSNSUserDefaults::GetKeyFromPath(const std::string& path, std::string& value)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CTVOSNSUserDefaults::GetKey(translated_key, value);

  return false;
}

bool CTVOSNSUserDefaults::GetKeyDataFromPath(const std::string& path,
                                             void* lpBuf,
                                             size_t& uiBufSize)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CTVOSNSUserDefaults::GetKeyData(translated_key, lpBuf, uiBufSize);

  return false;
}

bool CTVOSNSUserDefaults::SetKeyFromPath(const std::string& path,
                                         const std::string& value,
                                         bool synchronize)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty() && !value.empty())
    return CTVOSNSUserDefaults::SetKey(translated_key, value, synchronize);

  return false;
}

bool CTVOSNSUserDefaults::SetKeyDataFromPath(const std::string& path,
                                             const void* lpBuf,
                                             size_t uiBufSize,
                                             bool synchronize)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CTVOSNSUserDefaults::SetKeyData(translated_key, lpBuf, uiBufSize, synchronize);

  return false;
}

bool CTVOSNSUserDefaults::DeleteKeyFromPath(const std::string& path, bool synchronize)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CTVOSNSUserDefaults::DeleteKey(translated_key, synchronize);

  return false;
}

bool CTVOSNSUserDefaults::KeyFromPathExists(const std::string& path)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CTVOSNSUserDefaults::KeyExists(translated_key);

  return false;
}
