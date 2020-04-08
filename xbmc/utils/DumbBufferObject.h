/*
 *  Copyright (C) 2005-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/BufferObject.h"

#include <memory>
#include <stdint.h>

class CDumbBufferObject : public CBufferObject
{
public:
  CDumbBufferObject();
  virtual ~CDumbBufferObject() override;

  // Registration
  static std::unique_ptr<CBufferObject> Create();
  static void Register();

  // IBufferObject overrides via CBufferObject
  bool CreateBufferObject(uint32_t format, uint32_t width, uint32_t height) override;
  void DestroyBufferObject() override;
  uint8_t* GetMemory() override;
  void ReleaseMemory() override;
  std::string GetName() const override { return "CDumbBufferObject"; }

private:
  int m_device{-1};
  uint64_t m_size{0};
  uint64_t m_offset{0};
  uint8_t* m_map{nullptr};
};
