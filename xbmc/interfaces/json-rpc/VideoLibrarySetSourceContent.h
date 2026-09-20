/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JSONRPCUtils.h"
#include "addons/Scraper.h"
#include "video/VideoInfoScanner.h"

#include <string>

class CVariant;

namespace JSONRPC
{

/*! \brief What clearing a path's content binding should do to the items already scraped from it.
 */
enum class SourceContentClearMode
{
  CLEAR,
  EXCLUDE,
  REMOVE,
};

struct ParsedSetSourceContent
{
  std::string path;
  ADDON::ContentType content{ADDON::ContentType::NONE};
  std::string scraperId;
  std::string scraperSettings;
  SourceContentClearMode clearMode{SourceContentClearMode::CLEAR};
  bool refresh{false};

  /*! m_allExtAudio has no parameter and is left default constructed; the handler carries the
      path's stored value across. */
  KODI::VIDEO::SScanSettings settings;
};

/*! \brief Parse and validate the parameters of VideoLibrary.SetSourceContent.

 The scan settings produced are those CGUIDialogContentSettings::Show() writes for the same
 choices. The caller's three flags do not map onto SScanSettings one for one, because
 parent_name_root has no column and is derived on read from parent_name and recurse.

 \param parameterObject the JSON-RPC parameters, with schema defaults already applied
 \param parsed filled in on success
 \return OK, or InvalidParams
 */
JSONRPC_STATUS ParseSetSourceContentParams(const CVariant& parameterObject,
                                           ParsedSetSourceContent& parsed);

} // namespace JSONRPC
