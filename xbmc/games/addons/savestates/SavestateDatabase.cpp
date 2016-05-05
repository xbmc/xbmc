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

CSavestateDatabase::CSavestateDatabase() :
  CDenormalizedDatabase(SAVESTATE_OBJECT)
{
  BeginDeclarations();
  DeclareIndex(SAVESTATE_FIELD_PATH, "VARCHAR(512)");
  DeclareOneToMany(SAVESTATE_FIELD_GAMECLIENT, "VARCHAR(64)");
  DeclareOneToMany(SAVESTATE_FIELD_GAME_PATH, "VARCHAR(1024)");
  DeclareOneToMany(SAVESTATE_FIELD_GAME_CRC, "CHAR(8)");
}

bool CSavestateDatabase::Open()
{
  return CDatabase::Open(g_advancedSettings.m_databaseSavestates);
}

void CSavestateDatabase::UpdateTables(int version)
{
  if (version < 1)
  {
    BeginDeclarations();
    DeclareIndex(SAVESTATE_FIELD_PATH, "VARCHAR(512)");
    DeclareOneToMany(SAVESTATE_FIELD_GAMECLIENT, "VARCHAR(64)");
    DeclareOneToMany(SAVESTATE_FIELD_GAME_PATH, "VARCHAR(1024)");
    DeclareOneToMany(SAVESTATE_FIELD_GAME_CRC, "CHAR(8)");
  }
}

bool CSavestateDatabase::AddSavestate(const CSavestate& save)
{
  if (!AddObject(&save))
  {
    CLog::Log(LOGERROR, "Failed to update the database with savestate information");
    return false;
  }
  return true;
}

bool CSavestateDatabase::GetSavestate(const std::string& path, CSavestate& save)
{
  return GetObjectByIndex(SAVESTATE_FIELD_PATH, path, &save);
}

bool CSavestateDatabase::GetSavestatesNav(CFileItemList& items, const std::string& gamePath, const std::string& gameClient /* = "" */)
{
  std::map<std::string, int> predicates;
  if (GetItemID(SAVESTATE_FIELD_GAME_PATH, gamePath, predicates[SAVESTATE_FIELD_GAME_PATH]))
  {
    if (!gameClient.empty())
      GetItemID(SAVESTATE_FIELD_GAMECLIENT, gameClient, predicates[SAVESTATE_FIELD_GAMECLIENT]);

    return GetObjectsNav(items, predicates);
  }
  return false;
}

bool CSavestateDatabase::RenameSavestate(const std::string& path, const std::string& label)
{
  CSavestate save;
  if (GetObjectByIndex(SAVESTATE_FIELD_PATH, path, &save))
  {
    if (save.Label() != label)
    {
      save.SetLabel(label);
      return AddObject(&save) != -1;
    }
  }
  return false;
}

bool CSavestateDatabase::DeleteSavestate(const std::string& path)
{
  using namespace XFILE;

  bool bSuccess = false;

  if (DeleteObjectByIndex(SAVESTATE_FIELD_PATH, path))
  {
    if (!CFile::Delete(path))
      CLog::Log(LOGERROR, "Failed to delete savestate %s", path.c_str());
    else
    {
      std::string thumbPath = CSavestateUtils::MakeThumbPath(path);
      if (CFile::Exists(thumbPath))
      {
        if (!CFile::Delete(thumbPath))
          CLog::Log(LOGERROR, "Failed to delete thumbnail %s", thumbPath.c_str());
      }

      bSuccess = true;
    }
  }

  return bSuccess;
}

bool CSavestateDatabase::ClearSavestatesOfGame(const std::string& gamePath, const std::string& gameClient /* = "" */)
{
  CFileItemList items;
  if (GetSavestatesNav(items, gamePath, gameClient))
  {
    bool bSuccess = true;

    for (int i = 0; i < items.Size(); i++)
      bSuccess &= DeleteSavestate(items[i]->GetPath());

    return bSuccess;
  }
  return false;
}

bool CSavestateDatabase::Exists(const CVariant& object, int& idObject)
{
  if (!IsValid(object))
    return false;

  CSavestate dummy;
  return GetObjectByIndex(SAVESTATE_FIELD_PATH, object[SAVESTATE_FIELD_PATH], &dummy);
}

bool CSavestateDatabase::IsValid(const CVariant& object) const
{
  return !object[SAVESTATE_FIELD_PATH].asString().empty();
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
