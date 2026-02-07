/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

struct SZipEntry;
template<typename TEntry>
class CFileSystemCache;

class CCacheComponent
{
public:
  CCacheComponent();
  ~CCacheComponent();

  void Init();
  void Deinit();

  CFileSystemCache<SZipEntry>& GetZipCache();

private:
  std::unique_ptr<CFileSystemCache<SZipEntry>> m_zipCache;
};
