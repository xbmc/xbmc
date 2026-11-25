/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LibraryQueryFileItemListModifier.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "URL.h"
#include "media/LibraryQuery.h"
#include "utils/ItemProperties.h"
#include "utils/StringUtils.h"

#include <string>

#define URL_OPTION_XSP              "xsp"

namespace KODI::MEDIA
{

bool CLibraryQueryFileItemListModifier::CanModify(const CFileItemList& items) const
{
  return !GetUrlOption(items.GetPath(), URL_OPTION_XSP).empty();
}

bool CLibraryQueryFileItemListModifier::Modify(CFileItemList& items) const
{
  if (items.HasProperty(ITEM::PROPERTY::SORT_ORDER))
  {
    return false;
  }

  std::string xspOption = GetUrlOption(items.GetPath(), URL_OPTION_XSP);
  if (xspOption.empty())
    return false;

  // check for smartplaylist-specific sorting information
  CLibraryQuery xsp;
  if (!xsp.LoadFromJson(xspOption))
    return false;

  items.SetProperty(ITEM::PROPERTY::SORT_ORDER, (int)xsp.GetOrder());
  items.SetProperty(ITEM::PROPERTY::SORT_ASCENDING,
                    xsp.GetOrderDirection() == SortOrder::ASCENDING);

  return true;
}

std::string CLibraryQueryFileItemListModifier::GetUrlOption(const std::string& path,
                                                            const std::string& option)
{
  if (path.empty() || option.empty())
    return StringUtils::Empty;

  CURL url(path);
  return url.GetOption(option);
}

} // namespace KODI::MEDIA
