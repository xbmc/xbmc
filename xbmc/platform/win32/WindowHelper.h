/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

class CWHelper: public CThread
{
public:
  CWHelper(void);
  virtual ~CWHelper(void);

  virtual void OnStartup();
  virtual void OnExit();
  virtual void Process();

  void SetHWND(HWND hwnd);
  void SetHANDLE(HANDLE hProcess);

private:
  HWND  m_hwnd;
  HANDLE m_hProcess;

};

extern CWHelper g_windowHelper;
