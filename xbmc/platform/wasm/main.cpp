/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "application/AppEnvironment.h"
#include "application/AppParamParser.h"
#include "application/AppParams.h"
#include "platform/xbmc.h"

#include <locale.h>
#include <new>
#include <stdlib.h>

#include <emscripten/heap.h>
#include <emscripten/threading.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
  setlocale(LC_NUMERIC, "C");

  // Emscripten VFS: data is preloaded under /kodi via --preload-file
  setenv("KODI_HOME", "/kodi", 0);

  CAppParamParser appParamParser;
  appParamParser.Parse(argv, argc);
  appParamParser.GetAppParams()->SetLogTarget("console");

  CAppEnvironment::SetUp(appParamParser.GetAppParams());
  (void)XBMC_Run(true);
  return 0;
}
