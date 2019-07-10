/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "XBTF.h"

#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

class CXBTFReader : public CXBTFBase
{
public:
  CXBTFReader();
  ~CXBTFReader() override;

  bool Open(const std::string& path);
  bool IsOpen() const;
  void Close();

  time_t GetLastModificationTimestamp() const;

  bool Load(const CXBTFFrame& frame, unsigned char* buffer) const;

private:
  std::string m_path;
  FILE* m_file = nullptr;
};

typedef std::shared_ptr<CXBTFReader> CXBTFReaderPtr;
