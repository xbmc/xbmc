/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <time.h>

#include <sys/resource.h>
#include <sys/time.h>
#include <sys/times.h>

class CPosixResourceCounter
{
public:
  CPosixResourceCounter();
  virtual ~CPosixResourceCounter();

  double GetCPUUsage();
  void   Reset();

protected:
  struct rusage  m_usage;
  struct timeval m_tmLastCheck;
  double m_dLastUsage;
};

