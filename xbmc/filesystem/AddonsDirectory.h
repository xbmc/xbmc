/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"
#include "addons/AddonManager.h"

class CURL;
typedef std::shared_ptr<CFileItem> CFileItemPtr;

namespace XFILE
{

  /*!
  \ingroup windows
  \brief Get access to shares and it's directories.
  */
  class CAddonsDirectory : public IDirectory
  {
  public:
    CAddonsDirectory(void);
    ~CAddonsDirectory(void) override;
    bool GetDirectory(const CURL& url, CFileItemList &items) override;
    bool Create(const CURL& url) override { return true; }
    bool Exists(const CURL& url) override { return true; }
    bool AllowAll() const override { return true; }

    /*! \brief Fetch script and plugin addons of a given content type
     \param content the content type to fetch
     \param addons the list of addons to fill with scripts and plugin content
     \return true if content is valid, false if it's invalid.
     */
    static bool GetScriptsAndPlugins(const std::string &content, ADDON::VECADDONS &addons);

    /*! \brief Fetch scripts and plugins of a given content type
     \param content the content type to fetch
     \param items the list to fill with scripts and content
     \return true if more than one item is found, false otherwise.
     */
    static bool GetScriptsAndPlugins(const std::string &content, CFileItemList &items);

    static void GenerateAddonListing(const CURL &path, const ADDON::VECADDONS& addons, CFileItemList &items, const std::string label);
    static void GenerateAddonListingUpdates(const CURL& path,
                                            const ADDON::VECADDONS& addons,
                                            CFileItemList& items,
                                            const std::string label);
    static CFileItemPtr FileItemFromAddon(const ADDON::AddonPtr &addon, const std::string& path, bool folder = false);

    /*! \brief Returns true if `path` is a path or subpath of the repository directory, otherwise false */
    static bool IsRepoDirectory(const CURL& path);

  private:
    bool GetSearchResults(const CURL& path, CFileItemList &items);
    static void GenerateAddonListing(const CURL& path,
                                     const ADDON::VECADDONS& addons,
                                     CFileItemList& items,
                                     const std::string label,
                                     bool alwaysShowUpdateIcon);
  };
}
