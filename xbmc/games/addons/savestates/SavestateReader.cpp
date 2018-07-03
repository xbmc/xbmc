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

#include "SavestateReader.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "utils/log.h"
#include "IMemoryStream.h"

using namespace KODI;
using namespace GAME;

CSavestateReader::~CSavestateReader() = default;

bool CSavestateReader::Initialize(const std::string& path, const CGameClient* gameClient)
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

bool CSavestateReader::ReadSave(IMemoryStream* memoryStream)
{
  using namespace XFILE;

  bool bSuccess = false;

  CFile file;
  if (file.Open(m_savestate.Path()))
  {
    ssize_t read = file.Read(memoryStream->BeginFrame(), memoryStream->FrameSize());
    if (read == static_cast<ssize_t>(memoryStream->FrameSize()))
    {
      memoryStream->SubmitFrame();
      m_frameCount = m_savestate.PlaytimeFrames();
      bSuccess = true;
    }
  }

  if (!bSuccess)
    CLog::Log(LOGERROR, "Failed to read savestate %s", m_savestate.Path().c_str());

  return bSuccess;
}
