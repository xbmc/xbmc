#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "IDirectory.h"
#include "addons/AddonManager.h"

class CURL;
typedef boost::shared_ptr<CFileItem> CFileItemPtr;

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
    virtual ~CAddonsDirectory(void);
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    virtual bool Create(const char* strPath) { return true; }
    virtual bool Exists(const char* strPath) { return true; }
    virtual bool IsAllowed(const CStdString& strFile) const { return true; }

    /*! \brief Fetch script and plugin addons of a given content type
     \param content the content type to fetch
     \param addons the list of addons to fill with scripts and plugin content
     \return true if content is valid, false if it's invalid.
     */
    static bool GetScriptsAndPlugins(const CStdString &content, ADDON::VECADDONS &addons);

    /*! \brief Fetch scripts and plugins of a given content type
     \param content the content type to fetch
     \param items the list to fill with scripts and content
     \return true if more than one item is found, false otherwise.
     */
    static bool GetScriptsAndPlugins(const CStdString &content, CFileItemList &items);

    /*! \brief return the "Get More..." link item for the current content type
     \param content the content type for the link item
     \return a CFileItemPtr to a new item for the link.
     */
    static CFileItemPtr GetMoreItem(const CStdString &content);

    static void GenerateListing(CURL &path, ADDON::VECADDONS& addons, CFileItemList &items, bool reposAsFolders = true);
    static CFileItemPtr FileItemFromAddon(ADDON::AddonPtr &addon, const CStdString &basePath, bool folder = false);
  };
}
