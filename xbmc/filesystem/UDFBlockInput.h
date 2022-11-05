/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/CriticalSection.h"

#include <memory>

#include <udfread/blockinput.h>

namespace XFILE
{
class CFile;
}

class CUDFBlockInput
{
public:
  CUDFBlockInput() = default;
  ~CUDFBlockInput() = default;

  udfread_block_input* GetBlockInput(const std::string& file);

private:
  static int Close(udfread_block_input* bi);
  static uint32_t Size(udfread_block_input* bi);
  static int Read(udfread_block_input* bi, uint32_t lba, void* buf, uint32_t nblocks, int flags);

  struct UDF_BI
  {
    struct udfread_block_input bi;
    std::shared_ptr<XFILE::CFile> fp{nullptr};
    CCriticalSection lock;
  };

  std::unique_ptr<UDF_BI> m_bi{nullptr};
};
