/*
*  Breakpad.h
*  PlexMediaServer
*
*  Created by Max Feingold on 06/26/2013.
*  Copyright 2013 Plex Inc. All rights reserved.
*
*/

#ifndef __BREAKPAD_H__
#define __BREAKPAD_H__

#ifdef HAVE_BREAKPAD
#ifdef __linux__
#include "client/linux/handler/exception_handler.h"
#endif
#ifdef _WIN32
#include "client/windows/handler/exception_handler.h"

#ifdef _DEBUG
#pragma comment(lib, "breakpad-d.lib")
#else
#pragma comment(lib, "breakpad.lib")
#endif

#endif
#ifdef __APPLE__
#include "client/mac/handler/exception_handler.h"
#endif

class BreakpadScope
{
private:
#if defined(__linux__) || defined(__APPLE__)
  std::string m_processName;
#endif
#ifdef __linux__
  google_breakpad::MinidumpDescriptor* m_descriptor;
#endif
  google_breakpad::ExceptionHandler* m_eh;

public:
  BreakpadScope(const std::string& processName);
  ~BreakpadScope();
  void Dump();
};

#endif

#endif
