/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <map>
#include <memory>

class CEventLog;

class CEventLogManager
{
public:
  CEventLog &GetEventLog(unsigned int profileId);

private:
  std::map<unsigned int, std::unique_ptr<CEventLog>> m_eventLogs;
  CCriticalSection m_eventMutex;
};
