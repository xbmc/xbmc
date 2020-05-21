/*
 *  Copyright (C) 2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StaticLoggerBase.h"

#include "ServiceBroker.h"
#include "utils/log.h"

Logger CStaticLoggerBase::s_logger;

CStaticLoggerBase::CStaticLoggerBase(const std::string& loggerName)
{
  if (s_logger == nullptr)
    s_logger = CServiceBroker::GetLogging().GetLogger(loggerName);
}
