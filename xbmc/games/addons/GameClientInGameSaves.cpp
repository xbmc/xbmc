/*
 *      Copyright (C) 2016-2017 Team Kodi
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

#include "GameClientInGameSaves.h"

#include "GameClient.h"
#include "GameClientTranslator.h"
#include "filesystem/File.h"
#include "filesystem/Directory.h"
#include "profiles/ProfilesManager.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <assert.h>

using namespace KODI;
using namespace GAME;

#define INGAME_SAVES_DIRECTORY          "InGameSaves"
#define INGAME_SAVES_EXTENSION_SAVE_RAM ".sav"
#define INGAME_SAVES_EXTENSION_RTC      ".rtc"

CGameClientInGameSaves::CGameClientInGameSaves(CGameClient* addon, const KodiToAddonFuncTable_Game* dllStruct) :
  m_gameClient(addon),
  m_dllStruct(dllStruct)
{
  assert(m_gameClient != nullptr);
  assert(m_dllStruct != nullptr);
}

void CGameClientInGameSaves::Load()
{
  Load(GAME_MEMORY_SAVE_RAM);
  Load(GAME_MEMORY_RTC);
}

void CGameClientInGameSaves::Save()
{
  Save(GAME_MEMORY_SAVE_RAM);
  Save(GAME_MEMORY_RTC);
}

std::string CGameClientInGameSaves::GetPath(GAME_MEMORY memoryType)
{
  std::string path = URIUtils::AddFileToFolder(CProfilesManager::GetInstance().GetSavestatesFolder(), INGAME_SAVES_DIRECTORY);
  if (!XFILE::CDirectory::Exists(path))
    XFILE::CDirectory::Create(path);

  // Append save game filename
  std::string gamePath = URIUtils::GetFileName(m_gameClient->GetGamePath());
  path = URIUtils::AddFileToFolder(path, gamePath.empty() ? m_gameClient->ID() : gamePath);

  // Append file extension
  switch (memoryType)
  {
  case GAME_MEMORY_SAVE_RAM: return path + INGAME_SAVES_EXTENSION_SAVE_RAM;
  case GAME_MEMORY_RTC:      return path + INGAME_SAVES_EXTENSION_RTC;
  default:
    break;
  }
  return std::string();
}

void CGameClientInGameSaves::Load(GAME_MEMORY memoryType)
{
  uint8_t *gameMemory = nullptr;
  size_t size = 0;

  try
  {
    m_dllStruct->GetMemory(memoryType, &gameMemory, &size);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: Exception caught in GetMemory()", m_gameClient->ID().c_str());
  }

  const std::string path = GetPath(memoryType);
  if (size > 0 && XFILE::CFile::Exists(path))
  {
    XFILE::CFile file;
    if (file.Open(path))
    {
      ssize_t read = file.Read(gameMemory, size);
      if (read == static_cast<ssize_t>(size))
      {
        CLog::Log(LOGINFO, "GAME: In-game saves (%s) loaded from %s", CGameClientTranslator::ToString(memoryType), path.c_str());
      }
      else
      {
        CLog::Log(LOGERROR, "GAME: Failed to read in-game saves (%s): %ld/%ld bytes read", CGameClientTranslator::ToString(memoryType), read, size);
      }
    }
    else
    {
      CLog::Log(LOGERROR, "GAME: Unable to open in-game saves (%s) from file %s", CGameClientTranslator::ToString(memoryType), path.c_str());
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "GAME: No in-game saves (%s) to load", CGameClientTranslator::ToString(memoryType));
  }
}

void CGameClientInGameSaves::Save(GAME_MEMORY memoryType)
{
  uint8_t *gameMemory = nullptr;
  size_t size = 0;

  try
  {
    m_dllStruct->GetMemory(memoryType, &gameMemory, &size);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: %s: Exception caught in GetMemory()", m_gameClient->ID().c_str());
  }

  if (size > 0)
  {
    const std::string path = GetPath(memoryType);
    XFILE::CFile file;
    if (file.OpenForWrite(path, true))
    {
      ssize_t written = 0;
      written = file.Write(gameMemory, size);
      file.Close();
      if (written == static_cast<ssize_t>(size))
      {
        CLog::Log(LOGINFO, "GAME: In-game saves (%s) written to %s", CGameClientTranslator::ToString(memoryType), path.c_str());
      }
      else
      {
        CLog::Log(LOGERROR, "GAME: Failed to write in-game saves (%s): %ld/%ld bytes written", CGameClientTranslator::ToString(memoryType), written, size);
      }
    }
    else
    {
      CLog::Log(LOGERROR, "GAME: Unable to open in-game saves (%s) from file %s", CGameClientTranslator::ToString(memoryType), path.c_str());
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "GAME: No in-game saves (%s) to save", CGameClientTranslator::ToString(memoryType));
  }
}
