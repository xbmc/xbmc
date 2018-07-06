/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateWriter.h"
#include "SavestateUtils.h"
#include "filesystem/File.h"
#include "games/addons/GameClient.h"
#include "pictures/Picture.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "Application.h"
#include "XBDateTime.h"

using namespace KODI;
using namespace RETRO;

CSavestateWriter::~CSavestateWriter() = default;

bool CSavestateWriter::Initialize(const GAME::CGameClient* gameClient, uint64_t frameHistoryCount)
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

bool CSavestateWriter::WriteSave(const uint8_t *data, size_t size)
{
  using namespace XFILE;

  if (data == nullptr)
    return false;

  m_savestate.SetSize(size);

  CLog::Log(LOGDEBUG, "Saving savestate to %s", m_savestate.Path().c_str());

  bool bSuccess = false;

  CFile file;
  if (file.OpenForWrite(m_savestate.Path()))
  {
    ssize_t written = file.Write(data, size);
    bSuccess = (written == static_cast<ssize_t>(size));
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
