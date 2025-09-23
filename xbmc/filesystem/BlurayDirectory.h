/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DiscDirectoryHelper.h"
#include "IDirectory.h"
#include "URL.h"
#include "bluray/MPLSParser.h"

#include <map>
#include <string>

#include <libbluray/bluray.h>

class CFileItem;
class CFileItemList;

namespace XFILE
{
using namespace std::chrono_literals;

class CBlurayDirectory : public IDirectory
{
public:
  CBlurayDirectory();
  ~CBlurayDirectory() override;
  CBlurayDirectory(const CBlurayDirectory&) = delete;
  CBlurayDirectory& operator=(const CBlurayDirectory&) = delete;
  CBlurayDirectory(CBlurayDirectory&&) noexcept = default;
  CBlurayDirectory& operator=(CBlurayDirectory&&) noexcept = default;

  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  bool Resolve(CFileItem& item) const override;

  bool InitializeBluray(const std::string &root);
  static std::string GetBasePath(const CURL& url);
  std::string GetBlurayTitle() const;
  std::string GetBlurayID() const;

private:
  enum class DiscInfo : uint8_t
  {
    TITLE,
    ID
  };

  void Dispose();
  std::string GetDiscInfoString(DiscInfo info) const;
  const BLURAY_DISC_INFO* GetDiscInfo() const;

  CURL m_url;
  std::string m_realPath;
  BLURAY* m_bd{nullptr};
  bool m_blurayInitialized{false};
  bool m_blurayMenuSupport{false};

  std::map<unsigned int, ClipInformation> m_clipCache;
};
} // namespace XFILE
