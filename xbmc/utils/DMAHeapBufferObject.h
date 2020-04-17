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

class CDMAHeapBufferObject : public CBufferObject
{
public:
  CDMAHeapBufferObject() = default;
  virtual ~CDMAHeapBufferObject() override;

  // Registration
  static std::unique_ptr<CBufferObject> Create();
  static void Register();

  // IBufferObject overrides via CBufferObject
  bool CreateBufferObject(uint32_t format, uint32_t width, uint32_t height) override;
  bool CreateBufferObject(uint64_t size) override;
  void DestroyBufferObject() override;
  uint8_t* GetMemory() override;
  void ReleaseMemory() override;
  std::string GetName() const override { return "CDMAHeapBufferObject"; }

private:
  int m_dmaheapfd{-1};
  uint64_t m_size{0};
  uint8_t* m_map{nullptr};
};
