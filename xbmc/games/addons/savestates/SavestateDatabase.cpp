 /*
 *      Copyright (C) 2012-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "SavestateDatabase.h"
#include "Savestate.h"
#include "SavestateDefines.h"
#include "SavestateUtils.h"
#include "addons/AddonManager.h"
#include "dbwrappers/dataset.h"
#include "filesystem/File.h"
#include "games/GameTypes.h"
#include "games/tags/GameInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "FileItem.h"

#include "utils/log.h"

using namespace KODI;
using namespace GAME;

#define SAVESTATE_OBJECT  "savestate"

CSavestateDatabase::CSavestateDatabase() = default;

bool CSavestateDatabase::AddSavestate(const CSavestate& save)
{
  //! @todo
  return false;
}

bool CSavestateDatabase::GetSavestate(const std::string& path, CSavestate& save)
{
  //! @todo
  return false;
}

bool CSavestateDatabase::GetSavestatesNav(CFileItemList& items, const std::string& gamePath, const std::string& gameClient /* = "" */)
{
  //! @todo
  return false;
}

bool CSavestateDatabase::RenameSavestate(const std::string& path, const std::string& label)
{
  //! @todo
  return false;
}

bool CSavestateDatabase::DeleteSavestate(const std::string& path)
{
  //! @todo
  return false;
}

bool CSavestateDatabase::ClearSavestatesOfGame(const std::string& gamePath, const std::string& gameClient /* = "" */)
{
  //! @todo
  return false;
}

CFileItem* CSavestateDatabase::CreateFileItem(const CVariant& object) const
{
  using namespace ADDON;

  CSavestate save;
  save.Deserialize(object);
  CFileItem* item = new CFileItem(save.Label());

  item->SetPath(save.Path());
  if (!save.Thumbnail().empty())
    item->SetArt("thumb", save.Thumbnail());
  else
  {
    AddonPtr addon;
    if (CAddonMgr::GetInstance().GetAddon(save.GameClient(), addon, ADDON_GAMEDLL))
      item->SetArt("thumb", addon->Icon());
  }

  // Use the slot number as the second label
  if (save.Type() == SAVETYPE::SLOT)
    item->SetLabel2(StringUtils::Format("%u", save.Slot()));

  item->m_dateTime = save.Timestamp();
  item->SetProperty(FILEITEM_PROPERTY_SAVESTATE_DURATION, static_cast<uint64_t>(save.PlaytimeWallClock()));
  item->GetGameInfoTag()->SetGameClient(save.GameClient());
  item->m_dwSize = save.Size();
  item->m_bIsFolder = false;

  return item;
}
