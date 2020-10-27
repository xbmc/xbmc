/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IBufferObject.h"

#include <memory>
#include <stdint.h>

/**
 * @brief base class for using the IBufferObject interface. Derived classes
 *        should be based on this class.
 *
 */
class CBufferObject : public IBufferObject
{
public:
  /**
   * @brief Get a BufferObject from CBufferObjectFactory
   *
   * @return std::unique_ptr<CBufferObject>
   */
  static std::unique_ptr<CBufferObject> GetBufferObject(bool needsCreateBySize);

  bool CreateBufferObject(uint64_t size) override { return false; }

  int GetFd() override;
  uint32_t GetStride() override;
  uint64_t GetModifier() override;

  void SyncStart() override;
  void SyncEnd() override;

protected:
  int m_fd{-1};
  uint32_t m_stride{0};
};
