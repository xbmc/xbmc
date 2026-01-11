/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDirectory.h"

#include <string>
#include <vector>

namespace XFILE
{
  class CStackDirectory : public IDirectory
  {
    typedef struct StackPart
    {
      std::string title;
      std::string volume{};

      auto operator<=>(const StackPart&) const = default;
    } StackPart;

  public:
    bool GetDirectory(const CURL& url, CFileItemList& items) override;
    bool AllowAll() const override { return true; }

    /*!
    \brief Get the common base path/file from all the elements of the stack (for finding nfo files/metadata etc..)
    \param strPath The stack:// path
    \return The common base path/file
    */
    static std::string GetStackTitlePath(const std::string& strPath);

    /*!
    \brief Get the first element (file path) of a stack
    \param strPath The stack:// path
    \return The first element
    */
    static std::string GetFirstStackedFile(const std::string &strPath);

    /*!
    \brief Extract the indvidual paths from a stack:// path
    \param strPath The stack:// path
    \param vecPaths The vector to fill with the individual paths
    \return True if successful, false otherwise
    */
    static bool GetPaths(const std::string& strPath, std::vector<std::string>& vecPaths);

    /*!
    \brief Construct a stack:// path from a CFileItemList with the position in the stack determined by the entry in the stack vector
    \param items The CFileItemList containing the items to stack
    \param stack The vector containing the positions in the CFileItemList to stack (ie. if stack[1] == 3 then the 2nd item in the stack is items[3])
    Note that stack is 0 based.
    \return The constructed stack:// path
    */
    static std::string ConstructStackPath(const CFileItemList& items,
                                          const std::vector<int>& stack);

    /*!
    \brief Construct a stack:// path from a vector of paths and an optional additional path to append
    \param paths The vector of paths to stack
    \param stackedPath The constructed stack:// path
    \param newPath An optional additional path to append to the stack
    \return True if successful, false otherwise
    */
    static bool ConstructStackPath(const std::vector<std::string>& paths,
                                   std::string& stackedPath,
                                   const std::string& newPath = {});

    /*!
    \brief Get the base/parent path in common from all the parts of a stack:// path
    \param stackPath The stack:// path
    \return The base/parent path
    */
    static std::string GetBasePath(const std::string& stackPath);
    static std::string GetParentPath(const std::string& stackPath);
  };
}
