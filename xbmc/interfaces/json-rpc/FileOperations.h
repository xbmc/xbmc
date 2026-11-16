/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemHandler.h"
#include "JSONRPC.h"
#include "media/MediaSection.h"

#include <memory>
#include <optional>

class CFileItem;
class CVariant;

namespace JSONRPC
{
  class CFileOperations : public CFileItemHandler
  {
  public:
    static JSONRPC_STATUS GetRootDirectory(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetDirectory(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS GetFileDetails(const CVariant& parameterObject, CVariant& result);
    static JSONRPC_STATUS SetFileDetails(const CVariant& parameterObject, CVariant& result);

    static JSONRPC_STATUS PrepareDownload(ITransportLayer* transport,
                                          IClient* client,
                                          const CVariant& parameterObject,
                                          CVariant& result);
    static JSONRPC_STATUS Download(ITransportLayer* transport,
                                   IClient* client,
                                   const CVariant& parameterObject,
                                   CVariant& result);

    static bool FillFileItem(
        const std::shared_ptr<CFileItem>& originalItem,
        std::shared_ptr<CFileItem>& item,
        std::optional<KODI::MEDIA::MediaSection> section = {},
        const CVariant& parameterObject = CVariant(CVariant::VariantTypeArray));
    static bool FillFileItemList(const CVariant& parameterObject, CFileItemList& list);
    static bool NeedsLibraryLookup(const CVariant& parameterObject);
  };
}
