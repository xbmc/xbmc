//
//  CocoaUtils.cpp
//  Plex
//
//  Created by Max Feingold on 10/21/2011.
//  Copyright 2011 Plex Inc. All rights reserved.
//
//
#include <string>

#include "CocoaUtils.h"

#include "config.h"

#ifndef __APPLE__

using namespace std;

const char* Cocoa_GetAppVersion()
{
  return APPLICATION_VERSION;
}

#ifdef _WIN32
string Cocoa_GetLanguage()
{
  string strRet = "en-US";

  static char ret[64];
  LANGID langID = GetUserDefaultUILanguage();

  // Stackoverflow reports LOCALE_SISO639LANGNAME as fitting in 8 characters.
  int ccBuf = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO639LANGNAME, ret, 9);
  if (ccBuf != 0)
  {
    ccBuf--;
    ret[ccBuf++] = '-';
    int ccBuf2 = GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SISO3166CTRYNAME, ret+ccBuf, 32);
    if (ccBuf2 != 0)
      strRet = ret;
  }

  return strRet;
}

#elif __linux__
std::string Cocoa_GetLanguage()
{
    return string("en-US");
}
#endif

#endif
