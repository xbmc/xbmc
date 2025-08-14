/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "jobs/Job.h"

#include <utility>

template<typename F>
class CLambdaJob : public CJob
{
public:
  explicit CLambdaJob(F function) : m_function(std::move(function)) {}
  ~CLambdaJob() override = default;

  bool DoWork() override
  {
    m_function();
    return true;
  }

  bool Equals(const CJob* job) const override { return job == this; }

private:
  CLambdaJob() = delete;

  F m_function;
};
