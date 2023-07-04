/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "IListProvider.h"

#include "DirectoryProvider.h"
#include "MultiProvider.h"
#include "StaticProvider.h"

#include <tinyxml2.h>

std::unique_ptr<IListProvider> IListProvider::Create(const tinyxml2::XMLNode* node, int parentID)
{
  const auto* root = node->FirstChildElement("content");
  if (root)
  {
    const auto* next = root->NextSiblingElement("content");
    if (next)
      return std::make_unique<CMultiProvider>(root, parentID);

    return CreateSingle(root, parentID);
  }
  return std::unique_ptr<IListProvider>{};
}

std::unique_ptr<IListProvider> IListProvider::CreateSingle(const tinyxml2::XMLNode* content,
                                                           int parentID)
{
  const auto* item = content->FirstChildElement("item");
  if (item)
    return std::make_unique<CStaticListProvider>(content->ToElement(), parentID);

  if (!content->NoChildren())
    return std::make_unique<CDirectoryProvider>(content->ToElement(), parentID);

  return std::unique_ptr<IListProvider>{};
}
