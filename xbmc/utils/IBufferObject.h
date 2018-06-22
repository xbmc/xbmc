/*
 *      Copyright (C) 2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <stdint.h>

class IBufferObject
{
public:
  virtual ~IBufferObject() = default;

  virtual bool CreateBufferObject(int width, int height) = 0;
  virtual void DestroyBufferObject() { }
  virtual uint8_t *GetMemory() = 0;
  virtual void ReleaseMemory() { }
  virtual int GetFd() { return -1; }
  virtual int GetStride() = 0;

protected:
  int m_width = 0;
  int m_height = 0;
};
