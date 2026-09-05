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
#include <optional>
#include <set>

class CFileItem;
class CFileItemList;
class CThumbLoader;
class CVariant;
class ISerializable;

namespace PVR
{
class CPVRRecording;
class CPVRTimerInfoTag;
} // namespace PVR

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

    /*!
     \brief Diagnoses an item FillFileItemList dropped without saying why

     Bypasses the caches so a cached hit cannot mask storage that has gone away.

     \param item A single item as given by the client
     \return NotFound, or InvalidParams when nothing better applies
     */
    static JSONRPC_STATUS DiagnoseUnresolvedItem(const CVariant& item);

  private:
    static void Sort(CFileItemList &items, const CVariant& parameterObject);
    /*!
     \param epgRecording The item's EPG recording, looked up once; engaged and null when none
     \param epgTimer The item's EPG timer, looked up once; engaged and null when none
     */
    static bool GetField(const std::string& field,
                         const CVariant& info,
                         const std::shared_ptr<CFileItem>& item,
                         CVariant& result,
                         bool& fetchedArt,
                         std::optional<std::shared_ptr<PVR::CPVRRecording>>& epgRecording,
                         std::optional<std::shared_ptr<PVR::CPVRTimerInfoTag>>& epgTimer,
                         CThumbLoader* thumbLoader = nullptr);
  };
}
