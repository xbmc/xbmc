/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/python/PythonInvoker.h"

class CAddonPythonInvoker : public CPythonInvoker
{
public:
  explicit CAddonPythonInvoker(ILanguageInvocationHandler *invocationHandler);
  ~CAddonPythonInvoker() override;

  static void GlobalInitializeModules(void);

protected:
  // overrides of CPythonInvoker
  const char* getInitializationScript() const override;
};
