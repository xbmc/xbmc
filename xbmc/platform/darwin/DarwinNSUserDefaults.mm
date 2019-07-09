/*
 *  Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "platform/darwin/DarwinNSUserDefaults.h"

#import "filesystem/SpecialProtocol.h"
#import "utils/log.h"

#import <Foundation/NSString.h>
#import <Foundation/NSUserDefaults.h>


static bool translatePathIntoKey(const std::string &path, std::string &key)
{
  size_t pos;
  std::string translated_key = CSpecialProtocol::TranslatePath(path);
  if ((pos = translated_key.find("Caches/userdata")) != std::string::npos)
  {
    key = translated_key.erase(0, pos);
    return true;
  }

  return false;
}

bool CDarwinNSUserDefaults::Synchronize()
{
  return [[NSUserDefaults standardUserDefaults] synchronize] == YES;
}

bool CDarwinNSUserDefaults::GetKey(const std::string &key, std::string &value)
{
  if (!key.empty())
  {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *nsstring_key = [NSString stringWithUTF8String: key.c_str()];
    NSString *nsstring_value = [defaults stringForKey:nsstring_key];
    if (nsstring_value)
    {
      value = [nsstring_value UTF8String];
      if (!value.empty())
        return true;
    }
  }

  return false;
}

bool CDarwinNSUserDefaults::SetKey(const std::string &key, const std::string &value, bool synchronize)
{
  if (!key.empty() && !value.empty())
  {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];

    NSString *nsstring_key = [NSString stringWithUTF8String: key.c_str()];
    NSString *nsstring_value = [NSString stringWithUTF8String: value.c_str()];

    [defaults setObject:nsstring_value forKey:nsstring_key];
    if (synchronize)
      return [defaults synchronize] == YES;
    else
      return true;
  }

  return false;
}

bool CDarwinNSUserDefaults::DeleteKey(const std::string &key, bool synchronize)
{
  if (!key.empty())
  {
    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSString *nsstring_key = [NSString stringWithUTF8String: key.c_str()];
    [defaults removeObjectForKey:nsstring_key];
    if (synchronize)
      return [defaults synchronize] == YES;
    else
      return true;
  }

  return false;
}

bool CDarwinNSUserDefaults::KeyExists(const std::string &key)
{
  if (!key.empty())
  {
    NSString *nsstring_key = [NSString stringWithUTF8String: key.c_str()];
    if ([[NSUserDefaults standardUserDefaults] objectForKey:nsstring_key])
      return true;
  }

  return false;
}

bool CDarwinNSUserDefaults::IsKeyFromPath(const std::string &path)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
  {
    CLog::Log(LOGDEBUG, "found key %s", translated_key.c_str());
    return true;
  }

  return false;
}

bool CDarwinNSUserDefaults::GetKeyFromPath(const std::string &path, std::string &value)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CDarwinNSUserDefaults::GetKey(translated_key, value);

  return false;
}

bool CDarwinNSUserDefaults::SetKeyFromPath(const std::string &path, const std::string &value, bool synchronize)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty() && !value.empty())
    return CDarwinNSUserDefaults::SetKey(translated_key, value, synchronize);

  return false;
}

bool CDarwinNSUserDefaults::DeleteKeyFromPath(const std::string &path, bool synchronize)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CDarwinNSUserDefaults::DeleteKey(translated_key, synchronize);

  return false;
}

bool CDarwinNSUserDefaults::KeyFromPathExists(const std::string &path)
{
  std::string translated_key;
  if (translatePathIntoKey(path, translated_key) && !translated_key.empty())
    return CDarwinNSUserDefaults::KeyExists(translated_key);

  return false;
}
