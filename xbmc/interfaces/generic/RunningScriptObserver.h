/*
 *  Copyright (C) 2017-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Event.h"
#include "threads/IRunnable.h"
#include "threads/Thread.h"

#include <atomic>

class CRunningScriptObserver : public IRunnable
{
public:
  CRunningScriptObserver(int scriptId, CEvent& evt);
  ~CRunningScriptObserver();

  void Abort();

protected:
  // implementation of IRunnable
  void Run() override;

  int m_scriptId;
  CEvent& m_event;

  CEvent m_stopEvent;
  CThread m_thread;
};
