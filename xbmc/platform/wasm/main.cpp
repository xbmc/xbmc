/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "application/AppEnvironment.h"
#include "application/AppParamParser.h"
#include "application/AppParams.h"
#include "platform/xbmc.h"

#include <locale.h>
#include <stdlib.h>

int main(int argc, char* argv[])
{
  setlocale(LC_NUMERIC, "C");

  // Emscripten VFS: data is preloaded under /kodi via --preload-file
  setenv("KODI_HOME", "/kodi", 0);

  CAppParamParser appParamParser;
  appParamParser.Parse(argv, argc);
  appParamParser.GetAppParams()->SetLogTarget("console");

  CAppEnvironment::SetUp(appParamParser.GetAppParams());
  const int status = XBMC_Run(true);
  // Only reached when startup failed: emscripten_set_main_loop(..., 1) never returns.
  CAppEnvironment::TearDown();
  return status;
}
