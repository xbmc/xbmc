/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateReader.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "utils/log.h"

using namespace KODI;
using namespace RETRO;

CSavestateReader::~CSavestateReader() = default;

bool CSavestateReader::Initialize(const std::string& path, const GAME::CGameClient* gameClient)
{
  bool bSuccess = false;

  CLog::Log(LOGDEBUG, "Loading savestate from %s", path.c_str());

  if (m_db.GetSavestate(path, m_savestate))
  {
    // Sanity checks
    if (m_savestate.GameClient() == gameClient->ID())
      bSuccess = true;
    else
      CLog::Log(LOGDEBUG, "Savestate game client %s doesn't match active %s", m_savestate.GameClient().c_str(), gameClient->ID().c_str());
  }
  else
    CLog::Log(LOGERROR, "Failed to query savestate %s", path.c_str());

  return bSuccess;
}

bool CSavestateReader::ReadSave(uint8_t *data, size_t size)
{
  using namespace XFILE;

  bool bSuccess = false;

  CFile file;
  if (file.Open(m_savestate.Path()))
  {
    ssize_t read = file.Read(data, size);
    if (read == static_cast<ssize_t>(size))
    {
      m_frameCount = m_savestate.PlaytimeFrames();
      bSuccess = true;
    }
  }

  if (!bSuccess)
    CLog::Log(LOGERROR, "Failed to read savestate %s", m_savestate.Path().c_str());

  return bSuccess;
}
