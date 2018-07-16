/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/IBufferObject.h"

#include <stdint.h>

struct gbm_bo;
struct gbm_device;

class CGBMBufferObject : public IBufferObject
{
public:
  CGBMBufferObject(int format);
  virtual ~CGBMBufferObject() override;

  bool CreateBufferObject(int width, int height) override;
  void DestroyBufferObject() override;
  uint8_t* GetMemory() override;
  void ReleaseMemory() override;
  int GetFd() override;
  int GetStride() override;
  uint64_t GetModifier();

private:
  gbm_device *m_device = nullptr;

  int m_format = 0;
  int m_fd = -1;
  uint32_t m_stride = 0;
  uint8_t *m_map = nullptr;
  void *m_map_data = nullptr;
  gbm_bo *m_bo = nullptr;
};
