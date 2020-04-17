/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/logtypes.h"

#include <string>

class CStaticLoggerBase
{
protected:
  explicit CStaticLoggerBase(const std::string& loggerName);

  static Logger s_logger;
};
