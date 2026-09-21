/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/Job.h"

#include <cstdint>
#include <string>

class CAutorunMediaJob : public CJob
{
public:
  CAutorunMediaJob(const std::string& label, const std::string& path, uint64_t generation);

  bool DoWork() override;
private:
  const char *GetWindowString(int selection);

  std::string m_path, m_label;
  uint64_t m_generation{0};
};
