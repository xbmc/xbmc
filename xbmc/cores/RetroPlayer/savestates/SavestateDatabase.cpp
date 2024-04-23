/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateDatabase.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "SavestateFlatBuffer.h"
#include "URL.h"
#include "XBDateTime.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/IFileTypes.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/LocalizeStrings.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <memory>

namespace
{
constexpr auto SAVESTATE_EXTENSION = ".sav";
constexpr auto SAVESTATE_BASE_FOLDER = "special://home/saves/";
} // namespace

using namespace KODI;
using namespace RETRO;

CSavestateDatabase::CSavestateDatabase() = default;

std::unique_ptr<ISavestate> CSavestateDatabase::AllocateSavestate()
{
  std::unique_ptr<ISavestate> savestate;

  savestate = std::make_unique<CSavestateFlatBuffer>();

  return savestate;
}

bool CSavestateDatabase::AddSavestate(const std::string& savestatePath,
                                      const std::string& gamePath,
                                      const ISavestate& save)
{
  bool bSuccess = false;
  std::string path;

  if (savestatePath.empty())
    path = MakeSavestatePath(gamePath, save.Created());
  else
    path = savestatePath;

  CLog::Log(LOGDEBUG, "Saving savestate to {}", CURL::GetRedacted(path));

  const uint8_t* data = nullptr;
  size_t size = 0;
  if (save.Serialize(data, size))
  {
    XFILE::CFile file;
    if (file.OpenForWrite(path, true))
    {
      const ssize_t written = file.Write(data, size);
      if (written == static_cast<ssize_t>(size))
      {
        CLog::Log(LOGDEBUG, "Wrote savestate of {} bytes", size);
        bSuccess = true;
      }
    }
    else
      CLog::Log(LOGERROR, "Failed to open savestate for writing");
  }

  return bSuccess;
}

bool CSavestateDatabase::GetSavestate(const std::string& savestatePath, ISavestate& save)
{
  bool bSuccess = false;

  CLog::Log(LOGDEBUG, "Loading savestate from {}", CURL::GetRedacted(savestatePath));

  std::vector<uint8_t> savestateData;

  XFILE::CFile savestateFile;
  if (savestateFile.Open(savestatePath, XFILE::READ_TRUNCATED))
  {
    int64_t size = savestateFile.GetLength();
    if (size > 0)
    {
      savestateData.resize(static_cast<size_t>(size));

      const ssize_t readLength = savestateFile.Read(savestateData.data(), savestateData.size());
      if (readLength != static_cast<ssize_t>(savestateData.size()))
      {
        CLog::Log(LOGERROR, "Failed to read savestate {} of size {} bytes",
                  CURL::GetRedacted(savestatePath), size);
        savestateData.clear();
      }
    }
    else
      CLog::Log(LOGERROR, "Failed to get savestate length: {}", CURL::GetRedacted(savestatePath));
  }
  else
    CLog::Log(LOGERROR, "Failed to open savestate file {}", CURL::GetRedacted(savestatePath));

  if (!savestateData.empty())
    bSuccess = save.Deserialize(std::move(savestateData));

  return bSuccess;
}

bool CSavestateDatabase::GetSavestatesNav(CFileItemList& items,
                                          const std::string& gamePath,
                                          const std::string& gameClient /* = "" */)
{
  const std::string savesFolder = MakePath(gamePath);

  XFILE::CDirectory::CHints hints;
  hints.mask = SAVESTATE_EXTENSION;

  if (!XFILE::CDirectory::GetDirectory(savesFolder, items, hints))
    return false;

  if (!gameClient.empty())
  {
    for (int i = items.Size() - 1; i >= 0; i--)
    {
      std::unique_ptr<ISavestate> save = AllocateSavestate();
      GetSavestate(items[i]->GetPath(), *save);
      if (save->GameClientID() != gameClient)
        items.Remove(i);
    }
  }

  for (int i = 0; i < items.Size(); i++)
  {
    std::unique_ptr<ISavestate> savestate = AllocateSavestate();
    GetSavestate(items[i]->GetPath(), *savestate);

    GetSavestateItem(*savestate, items[i]->GetPath(), *items[i]);
  }

  return true;
}

void CSavestateDatabase::GetSavestateItem(const ISavestate& savestate,
                                          const std::string& savestatePath,
                                          CFileItem& item)
{
  CDateTime dateUTC = CDateTime::FromUTCDateTime(savestate.Created());

  std::string label;
  std::string label2;

  // Date has the lowest priority of being shown
  label = dateUTC.GetAsLocalizedDateTime(false, false);

  // Label has the next priority
  if (!savestate.Label().empty())
  {
    label2 = std::move(label);
    label = savestate.Label();
  }

  // "Autosave" has the highest priority
  if (savestate.Type() == SAVE_TYPE::AUTO)
  {
    label2 = std::move(label);
    label = g_localizeStrings.Get(15316); // "Autosave"
  }

  item.SetLabel(label);
  item.SetLabel2(label2);
  item.SetPath(savestatePath);
  item.SetArt("screenshot", MakeThumbnailPath(savestatePath));
  item.SetProperty(SAVESTATE_LABEL, savestate.Label());
  item.SetProperty(SAVESTATE_CAPTION, savestate.Caption());
  item.SetProperty(SAVESTATE_GAME_CLIENT, savestate.GameClientID());
  item.m_dateTime = dateUTC;
}

std::unique_ptr<ISavestate> CSavestateDatabase::RenameSavestate(const std::string& savestatePath,
                                                                const std::string& label)
{
  std::unique_ptr<ISavestate> savestate = AllocateSavestate();
  if (!GetSavestate(savestatePath, *savestate))
    return {};

  std::unique_ptr<ISavestate> newSavestate = AllocateSavestate();

  newSavestate->SetLabel(label);
  newSavestate->SetCaption(savestate->Caption());
  newSavestate->SetType(savestate->Type());
  newSavestate->SetCreated(savestate->Created());
  newSavestate->SetGameFileName(savestate->GameFileName());
  newSavestate->SetTimestampFrames(savestate->TimestampFrames());
  newSavestate->SetTimestampWallClock(savestate->TimestampWallClock());
  newSavestate->SetGameClientID(savestate->GameClientID());
  newSavestate->SetGameClientVersion(savestate->GameClientVersion());

  size_t memorySize = savestate->GetMemorySize();
  std::memcpy(newSavestate->GetMemoryBuffer(memorySize), savestate->GetMemoryData(), memorySize);

  newSavestate->Finalize();

  if (!AddSavestate(savestatePath, "", *newSavestate))
    return {};

  return newSavestate;
}

bool CSavestateDatabase::DeleteSavestate(const std::string& savestatePath)
{
  if (!XFILE::CFile::Delete(savestatePath))
  {
    CLog::Log(LOGERROR, "Failed to delete savestate file {}", CURL::GetRedacted(savestatePath));
    return false;
  }

  XFILE::CFile::Delete(MakeThumbnailPath(savestatePath));
  return true;
}

bool CSavestateDatabase::ClearSavestatesOfGame(const std::string& gamePath,
                                               const std::string& gameClient /* = "" */)
{
  //! @todo
  return false;
}

std::string CSavestateDatabase::MakeSavestatePath(const std::string& gamePath,
                                                  const CDateTime& creationTime)
{
  std::string path = MakePath(gamePath);
  return URIUtils::AddFileToFolder(path, creationTime.GetAsSaveString() + SAVESTATE_EXTENSION);
}

std::string CSavestateDatabase::MakeThumbnailPath(const std::string& savestatePath)
{
  return URIUtils::ReplaceExtension(savestatePath, ".jpg");
}

std::string CSavestateDatabase::MakePath(const std::string& gamePath)
{
  if (!CreateFolderIfNotExists(SAVESTATE_BASE_FOLDER))
    return "";

  std::string gameName = URIUtils::GetFileName(gamePath);
  std::string folderPath = URIUtils::AddFileToFolder(SAVESTATE_BASE_FOLDER, gameName);

  if (!CreateFolderIfNotExists(folderPath))
    return "";

  return folderPath;
}

bool CSavestateDatabase::CreateFolderIfNotExists(const std::string& path)
{
  if (!XFILE::CDirectory::Exists(path))
  {
    if (!XFILE::CDirectory::Create(path))
    {
      CLog::Log(LOGERROR, "Failed to create folder: {}", path);
      return false;
    }
  }

  return true;
}
