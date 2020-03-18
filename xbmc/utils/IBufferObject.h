/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

class IBufferObject
{
public:
  virtual ~IBufferObject() = default;

  virtual bool CreateBufferObject(uint32_t format, uint32_t width, uint32_t height) = 0;
  virtual void DestroyBufferObject() { }
  virtual uint8_t *GetMemory() = 0;
  virtual void ReleaseMemory() { }
  virtual int GetFd() { return -1; }
  virtual int GetStride() = 0;
};
