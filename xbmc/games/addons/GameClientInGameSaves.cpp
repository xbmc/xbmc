/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GameClientInGameSaves.h"

#include "GameClient.h"
#include "GameClientTranslator.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "games/GameUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <assert.h>

using namespace KODI;
using namespace GAME;

// The names RetroArch uses, so saves can be copied between the two
#define INGAME_SAVES_SAVE_RAM "savedata.srm"
#define INGAME_SAVES_RTC "savedata.rtc"

CGameClientInGameSaves::CGameClientInGameSaves(CGameClient* addon,
                                               const AddonInstance_Game* dllStruct)
  : m_gameClient(addon),
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
  std::string fileName;
  switch (memoryType)
  {
    case GAME_MEMORY_SAVE_RAM:
      fileName = INGAME_SAVES_SAVE_RAM;
      break;
    case GAME_MEMORY_RTC:
      fileName = INGAME_SAVES_RTC;
      break;
    default:
      return std::string();
  }

  // A standalone game has no file, so its add-on stands in for one
  const std::string& gamePath = m_gameClient->GetGamePath();
  const std::string folder =
      CGameUtils::GetGameFolder(gamePath.empty() ? m_gameClient->ID() : gamePath);
  if (!XFILE::CDirectory::Exists(folder))
    XFILE::CDirectory::Create(folder);

  return URIUtils::AddFileToFolder(folder, fileName);
}

void CGameClientInGameSaves::Load(GAME_MEMORY memoryType)
{
  uint8_t* gameMemory = nullptr;
  size_t size = 0;

  try
  {
    m_dllStruct->toAddon->GetMemory(m_dllStruct, memoryType, &gameMemory, &size);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: {}: Exception caught in GetMemory()", m_gameClient->ID());
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
        CLog::Log(LOGINFO, "GAME: In-game saves ({}) loaded from {}",
                  CGameClientTranslator::ToString(memoryType), path);
      }
      else
      {
        CLog::Log(LOGERROR, "GAME: Failed to read in-game saves ({}): {}/{} bytes read",
                  CGameClientTranslator::ToString(memoryType), read, size);
      }
    }
    else
    {
      CLog::Log(LOGERROR, "GAME: Unable to open in-game saves ({}) from file {}",
                CGameClientTranslator::ToString(memoryType), path);
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "GAME: No in-game saves ({}) to load",
              CGameClientTranslator::ToString(memoryType));
  }
}

void CGameClientInGameSaves::Save(GAME_MEMORY memoryType)
{
  uint8_t* gameMemory = nullptr;
  size_t size = 0;

  try
  {
    m_dllStruct->toAddon->GetMemory(m_dllStruct, memoryType, &gameMemory, &size);
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "GAME: {}: Exception caught in GetMemory()", m_gameClient->ID());
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
        CLog::Log(LOGINFO, "GAME: In-game saves ({}) written to {}",
                  CGameClientTranslator::ToString(memoryType), path);
      }
      else
      {
        CLog::Log(LOGERROR, "GAME: Failed to write in-game saves ({}): {}/{} bytes written",
                  CGameClientTranslator::ToString(memoryType), written, size);
      }
    }
    else
    {
      CLog::Log(LOGERROR, "GAME: Unable to open in-game saves ({}) from file {}",
                CGameClientTranslator::ToString(memoryType), path);
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "GAME: No in-game saves ({}) to save",
              CGameClientTranslator::ToString(memoryType));
  }
}
