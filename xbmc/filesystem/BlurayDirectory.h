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

  /*!
   \brief Resolve the underlying path and open the disc with libbluray.
   Only needed by callers that want the disc's own metadata (see GetBlurayTitle/GetBlurayID).
   GetDirectory resolves the path but leaves libbluray closed until something needs it.
   \return true if libbluray could open the disc, ie. this is a bluray
   */
  bool InitializeBluray(const std::string& root);
  static std::string GetBasePath(const CURL& url);
  std::string GetBlurayTitle();
  std::string GetBlurayID();

private:
  enum class DiscInfo : uint8_t
  {
    TITLE,
    ID
  };

  /*!
   \brief Resolve the disc's path through its directory handler, without touching the disc.
   */
  void SetRealPath(const std::string& root);

  /*!
   \brief Open the disc with libbluray, unless already open.
   Opening costs a dozen round trips to the disc (index.bdmv, the BDMV/META localisations,
   CERTIFICATE/id.bdmv and the AACS probe), which is why it is deferred until a caller needs
   something only libbluray can answer. SetRealPath must have been called first.
   \return true if libbluray has the disc open
   */
  bool EnsureBlurayOpen();

  /*!
   \brief Get whether this disc supports menus, opening it only if needed.
   */
  bool HasMenuSupport();

  void Dispose();
  std::string GetDiscInfoString(DiscInfo info);
  const BLURAY_DISC_INFO* GetDiscInfo() const;

  CURL m_url;
  std::string m_realPath;
  BLURAY* m_bd{nullptr};
  bool m_blurayInitialized{false};

  std::map<unsigned int, ClipInformation> m_clipCache;
};
} // namespace XFILE
