/*
*      Copyright (C) 2007-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
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
