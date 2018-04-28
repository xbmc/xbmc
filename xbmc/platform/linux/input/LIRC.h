/*
*      Copyright (C) 2007-2018 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "threads/Thread.h"
#include "threads/CriticalSection.h"
#include "input/IRTranslator.h"
#include <string>

class CLirc : CThread
{
public:
  CLirc();
  ~CLirc() override;
  void Start();

protected:
  void Process() override;
  void ProcessCode(char *buf);
  bool CheckDaemon();

  int m_fd = -1;
  uint32_t m_firstClickTime = 0;
  CCriticalSection m_critSection;
  CIRTranslator m_irTranslator;
  int m_profileId;
};
