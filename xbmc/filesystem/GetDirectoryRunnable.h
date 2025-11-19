/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemList.h"
#include "VirtualDirectory.h"
#include "threads/IRunnable.h"

namespace XFILE
{
class CGetDirectoryRunnable : public IRunnable
{
public:
  CGetDirectoryRunnable(CVirtualDirectory& dir, const CURL& url, CFileItemList& items, bool useDir)
    : m_dir(dir), m_url(url), m_items(items), m_useDir(useDir)
  {
  }

  void Run() override
  {
    m_result = m_dir.GetDirectory(m_url, m_items, m_useDir, true);
  }

  void Cancel() override
  {
    m_dir.CancelDirectory();
  }

  bool GetResult() const { return m_result; }

protected:
  CVirtualDirectory& m_dir;
  CURL m_url;
  CFileItemList& m_items;
  bool m_useDir;
  bool m_result = false;
};
} // namespace XFILE
