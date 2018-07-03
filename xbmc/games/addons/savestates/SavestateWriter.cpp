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

#include "SavestateWriter.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "IMemoryStream.h"
#include "games/addons/savestates/SavestateUtils.h"
#include "pictures/Picture.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "Application.h"
#include "XBDateTime.h"

using namespace KODI;
using namespace GAME;

CSavestateWriter::~CSavestateWriter() = default;

bool CSavestateWriter::Initialize(const CGameClient* gameClient, uint64_t frameHistoryCount)
{
  //! @todo Handle savestates for standalone game clients
  if (gameClient->GetGamePath().empty())
  {
    CLog::Log(LOGERROR, "Savestates not implemented for standalone game clients");
    return false;
  }

  m_savestate.Reset();
  m_fps = 0.0;

  m_fps = gameClient->GetFrameRate();

  CDateTime now = CDateTime::GetCurrentDateTime();
  std::string label = now.GetAsLocalizedDateTime();

  m_savestate.SetType(SAVETYPE::AUTO);
  m_savestate.SetLabel(label);
  m_savestate.SetGameClient(gameClient->ID());
  m_savestate.SetGamePath(gameClient->GetGamePath());
  m_savestate.SetTimestamp(now);
  m_savestate.SetPlaytimeFrames(frameHistoryCount);
  m_savestate.SetPlaytimeWallClock(frameHistoryCount / m_fps); //! @todo Accumulate playtime instead of deriving it

  m_savestate.SetPath(CSavestateUtils::MakePath(m_savestate));

  if (m_fps == 0.0)
    return false; // Sanity check

  return !m_savestate.Path().empty();
}

bool CSavestateWriter::WriteSave(IMemoryStream* memoryStream)
{
  using namespace XFILE;

  if (memoryStream->CurrentFrame() == nullptr)
    return false;

  m_savestate.SetSize(memoryStream->FrameSize());

  CLog::Log(LOGDEBUG, "Saving savestate to %s", m_savestate.Path().c_str());

  bool bSuccess = false;

  CFile file;
  if (file.OpenForWrite(m_savestate.Path()))
  {
    ssize_t written = file.Write(memoryStream->CurrentFrame(), memoryStream->FrameSize());
    bSuccess = (written == static_cast<ssize_t>(memoryStream->FrameSize()));
  }

  if (!bSuccess)
    CLog::Log(LOGERROR, "Failed to write savestate to %s", m_savestate.Path().c_str());

  return bSuccess;
}

void CSavestateWriter::WriteThumb()
{
  //! @todo
}

bool CSavestateWriter::CommitToDatabase()
{
  bool bSuccess = m_db.AddSavestate(m_savestate);

  if (!bSuccess)
    CLog::Log(LOGERROR, "Failed to write savestate to database: %s", m_savestate.Path().c_str());

  return bSuccess;
}

void CSavestateWriter::CleanUpTransaction()
{
  using namespace XFILE;

  CFile::Delete(m_savestate.Path());
  if (CFile::Exists(m_savestate.Thumbnail()))
    CFile::Delete(m_savestate.Thumbnail());
}
