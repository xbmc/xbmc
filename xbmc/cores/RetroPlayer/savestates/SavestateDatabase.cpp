/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SavestateDatabase.h"

#include "FileItem.h"
#include "SavestateFlatBuffer.h"
#include "URL.h"
#include "XBDateTime.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/IFileTypes.h"
#include "games/dialogs/DialogGameDefines.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

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

  savestate.reset(new CSavestateFlatBuffer);

  return savestate;
}

bool CSavestateDatabase::AddSavestate(std::string& savestatePath,
                                      const std::string& gamePath,
                                      const ISavestate& save)
{
  bool bSuccess = false;
  std::string path;

  if (savestatePath.empty())
  {
    path = MakePath(gamePath);
    path = URIUtils::AddFileToFolder(path, save.Created().GetAsSaveString() + SAVESTATE_EXTENSION);

    // return the path to the new savestate
    savestatePath = path;
  }
  else
  {
    path = savestatePath;
  }

  CLog::Log(LOGDEBUG, "Saving savestate to {}", CURL::GetRedacted(path));

  const uint8_t* data = nullptr;
  size_t size = 0;
  if (save.Serialize(data, size))
  {
    XFILE::CFile file;
    if (file.OpenForWrite(path))
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

    const std::string label = savestate->Label();

    CDateTime dateUTC = CDateTime::FromUTCDateTime(savestate->Created());
    if (label.empty())
      items[i]->SetLabel(dateUTC.GetAsLocalizedDateTime(false, false));
    else
    {
      items[i]->SetLabel(label);
      items[i]->SetLabel2(dateUTC.GetAsLocalizedDateTime(false, false));
    }

    items[i]->SetArt("icon", MakeThumbnailPath(items[i]->GetPath()));
    items[i]->SetProperty(SAVESTATE_CAPTION, savestate->Caption());
    items[i]->m_dateTime = dateUTC;
  }

  return true;
}

bool CSavestateDatabase::RenameSavestate(const std::string& savestatePath, const std::string& label)
{
  std::unique_ptr<ISavestate> savestate = AllocateSavestate();
  if (!GetSavestate(savestatePath, *savestate))
    return false;

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

  std::string path = savestatePath;
  if (!AddSavestate(path, "", *newSavestate))
    return false;

  return true;
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
