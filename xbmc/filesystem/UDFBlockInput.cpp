/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "UDFBlockInput.h"

#include "filesystem/File.h"

#include <mutex>

#include <udfread/udfread.h>

int CUDFBlockInput::Close(udfread_block_input* bi)
{
  auto m_bi = reinterpret_cast<UDF_BI*>(bi);

  m_bi->fp->Close();

  return 0;
}

uint32_t CUDFBlockInput::Size(udfread_block_input* bi)
{
  auto m_bi = reinterpret_cast<UDF_BI*>(bi);

  return static_cast<uint32_t>(m_bi->fp->GetLength() / UDF_BLOCK_SIZE);
}

int CUDFBlockInput::Read(
    udfread_block_input* bi, uint32_t lba, void* buf, uint32_t blocks, int flags)
{
  auto m_bi = reinterpret_cast<UDF_BI*>(bi);
  std::unique_lock<CCriticalSection> lock(m_bi->lock);

  int64_t pos = static_cast<int64_t>(lba) * UDF_BLOCK_SIZE;

  if (m_bi->fp->Seek(pos, SEEK_SET) != pos)
    return -1;

  ssize_t size = blocks * UDF_BLOCK_SIZE;
  ssize_t read = m_bi->fp->Read(buf, size);
  if (read > 0)
    return static_cast<int>(read / UDF_BLOCK_SIZE);

  return static_cast<int>(read);
}

udfread_block_input* CUDFBlockInput::GetBlockInput(const std::string& file)
{
  auto fp = std::make_shared<XFILE::CFile>();

  if (fp->Open(file))
  {
    m_bi = std::make_unique<UDF_BI>();
    if (m_bi)
    {
      m_bi->fp = fp;
      m_bi->bi.close = CUDFBlockInput::Close;
      m_bi->bi.read = CUDFBlockInput::Read;
      m_bi->bi.size = CUDFBlockInput::Size;

      return &m_bi->bi;
    }

    fp->Close();
  }

  return nullptr;
}
