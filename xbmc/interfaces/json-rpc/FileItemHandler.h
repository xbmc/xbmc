/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JSONRPC.h"
#include "JSONUtils.h"

#include <memory>
#include <set>

class CFileItem;
class CFileItemList;
class CThumbLoader;
class CVariant;
class ISerializable;

namespace JSONRPC
{
  class CFileItemHandler : public CJSONUtils
  {
  protected:
    static void FillDetails(const ISerializable* info,
                            const std::shared_ptr<CFileItem>& item,
                            std::set<std::string>& fields,
                            CVariant& result,
                            CThumbLoader* thumbLoader = nullptr);
    static void HandleFileItemList(const char *ID, bool allowFile, const char *resultname, CFileItemList &items, const CVariant &parameterObject, CVariant &result, bool sortLimit = true);
    static void HandleFileItemList(const char *ID, bool allowFile, const char *resultname, CFileItemList &items, const CVariant &parameterObject, CVariant &result, int size, bool sortLimit = true);
    static void HandleFileItem(const char* ID,
                               bool allowFile,
                               const char* resultname,
                               const std::shared_ptr<CFileItem>& item,
                               const CVariant& parameterObject,
                               const CVariant& validFields,
                               CVariant& result,
                               bool append = true,
                               CThumbLoader* thumbLoader = nullptr);
    static void HandleFileItem(const char* ID,
                               bool allowFile,
                               const char* resultname,
                               const std::shared_ptr<CFileItem>& item,
                               const CVariant& parameterObject,
                               const std::set<std::string>& validFields,
                               CVariant& result,
                               bool append = true,
                               CThumbLoader* thumbLoader = nullptr);

    static bool FillFileItemList(const CVariant &parameterObject, CFileItemList &list);
  private:
    static void Sort(CFileItemList &items, const CVariant& parameterObject);
    static bool GetField(const std::string& field,
                         const CVariant& info,
                         const std::shared_ptr<CFileItem>& item,
                         CVariant& result,
                         bool& fetchedArt,
                         CThumbLoader* thumbLoader = nullptr);
  };
}
