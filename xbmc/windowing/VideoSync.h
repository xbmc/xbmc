/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Event.h"

class CVideoReferenceClock;
typedef void (*PUPDATECLOCK)(int NrVBlanks, uint64_t time, void *clock);

class CVideoSync
{
public:
  explicit CVideoSync(void* clock) { m_refClock = clock; }
  virtual ~CVideoSync() = default;
  virtual bool Setup(PUPDATECLOCK func) = 0;
  virtual void Run(CEvent& stop) = 0;
  virtual void Cleanup() = 0;
  virtual float GetFps() = 0;
  virtual void RefreshChanged() {}

protected:
  PUPDATECLOCK UpdateClock;
  float m_fps;
  void *m_refClock;
};
