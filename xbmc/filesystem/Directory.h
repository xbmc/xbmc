/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

#include <functional>
#include <memory>
#include <string>

class CFileItem;

namespace XFILE
{
/*!
 \ingroup filesystem
 \brief Wrappers for \e IDirectory
 */
class CDirectory
{
public:
  CDirectory(void);
  virtual ~CDirectory(void);

  class CHints
  {
  public:
    std::string mask;
    int flags = DIR_FLAG_DEFAULTS;
  };

  static bool GetDirectory(const CURL& url
                           , CFileItemList &items
                           , const std::string &strMask
                           , int flags);

  static bool GetDirectory(const CURL& url,
                           const std::shared_ptr<IDirectory>& pDirectory,
                           CFileItemList& items,
                           const CHints& hints);

  static bool GetDirectory(const CURL& url
                           , CFileItemList &items
                           , const CHints &hints);

  static bool Create(const CURL& url);
  static bool Exists(const CURL& url, bool bUseCache = true);
  static bool Remove(const CURL& url);
  static bool RemoveRecursive(const CURL& url);

  static bool GetDirectory(const std::string& strPath
                           , CFileItemList &items
                           , const std::string &strMask
                           , int flags);

  static bool GetDirectory(const std::string& strPath,
                           const std::shared_ptr<IDirectory>& pDirectory,
                           CFileItemList& items,
                           const std::string& strMask,
                           int flags);

  static bool GetDirectory(const std::string& strPath
                           , CFileItemList &items
                           , const CHints &hints);

  using DirectoryEnumerationCallback = std::function<void(const std::shared_ptr<CFileItem>& item)>;
  using DirectoryFilter = std::function<bool(const std::shared_ptr<CFileItem>& folder)>;

  /*!
   * \brief Enumerates files and folders in and below a directory. Every applicable gets passed to the callback.
   *
   * \param path Directory to enumerate
   * \param callback Files and folders matching the criteria are passed to this function
   * \param filter Only folders are passed to this function. If it return false the folder and everything below it will skipped from the enumeration
   * \param fileOnly If true only files are passed to \p callback. Doesn't affect \p filter
   * \param mask Only files matching this mask are passed to \p callback
   * \param flags See \ref DIR_FLAG enum
   */
  static bool EnumerateDirectory(
      const std::string& path,
      const DirectoryEnumerationCallback& callback,
      const DirectoryFilter& filter = [](const std::shared_ptr<CFileItem>&) { return true; },
      bool fileOnly = false,
      const std::string& mask = "",
      int flags = DIR_FLAG_DEFAULTS);

  static bool Create(const std::string& strPath);
  static bool Exists(const std::string& strPath, bool bUseCache = true);
  static bool Remove(const std::string& strPath);
  static bool RemoveRecursive(const std::string& strPath);

  /*! \brief Filter files that act like directories from the list, replacing them with their directory counterparts
   \param items The item list to filter
   \param mask  The mask to apply when filtering files
   \param expandImages True to include disc images in file directory expansion
  */
  static void FilterFileDirectories(CFileItemList &items, const std::string &mask,
                                    bool expandImages=false);
};
}
