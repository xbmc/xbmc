/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AE.h"
#include "threads/IRunnable.h"

class IThreadedAE : public IAE, public IRunnable
{
public:
  virtual void Run () = 0;
  virtual void Stop() = 0;
};

