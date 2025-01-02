/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CFavouritesURL;
class CFileItem;
class CFileItemList;

namespace FAVOURITES_UTILS
{
bool ChooseAndSetNewName(CFileItem& item);
bool ChooseAndSetNewThumbnail(CFileItem& item);
bool MoveItem(CFileItemList& items, const std::shared_ptr<CFileItem>& item, int amount);
bool RemoveItem(CFileItemList& items, const std::shared_ptr<CFileItem>& item);
bool ShouldEnableMoveItems();

bool ExecuteAction(const CFavouritesURL& favURL, const std::shared_ptr<CFileItem>& item);

} // namespace FAVOURITES_UTILS
