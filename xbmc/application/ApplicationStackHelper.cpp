/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationStackHelper.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "cores/IPlayer.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "filesystem/StackDirectory.h"
#include "settings/AdvancedSettings.h"
#include "settings/MediaSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoUtils.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

using namespace XFILE;
using namespace std::chrono_literals;

void CApplicationStackHelper::Clear()
{
  m_currentStackPosition = 0;
  m_knownStackParts = 0;
  m_stackPaths.clear();
  m_originalStackItems.Clear();
  m_oldStackPath.clear();
  m_stackMap.clear();
  m_partFinished = false;
  m_seekingParts = false;
}

void CApplicationStackHelper::OnPlayBackStarted()
{
  m_partFinished = false;
  m_seekingParts = false;
}

bool CApplicationStackHelper::InitializeStack(const CFileItem& item)
{
  std::unique_lock stackLock(m_critSection);

  Clear();

  // Get a FileItem for each part of the stack
  // (note this just populates m_strPath of the FileItem)
  // Assumes item.GetDynPath() is the stack:// path
  CStackDirectory dir;
  CURL stackPath{item.GetDynPath()};
  if (!dir.GetDirectory(stackPath, m_originalStackItems) || m_originalStackItems.IsEmpty())
    return false;

  // Populate stack map
  // Key is the path of each part (derived from stack:// above)
  const auto stack{std::make_shared<CFileItem>(item)};
  for (int i = 0; const auto& file : m_originalStackItems)
  {
    const std::string& path{file->GetPath()};
    const auto& part{GetOrCreateStackPartInformation(path)};
    part->stackItem = stack;
    part->partNumber = i++;
    m_stackPaths.emplace_back(path);
  }

  // Now see what times we have for each part
  CVideoDatabase db;
  std::vector<std::chrono::milliseconds> times;
  bool haveTimes{false};

  if (db.Open())
    haveTimes = db.GetStackTimes(item.GetDynPath(), times);

  // If no times and is a regular (file) stack then get times from files
  // Not possible for BD/DVD (folder) stacks due to playlist/title not known
  if (!haveTimes && !IsPlayingDiscStack())
  {
    std::chrono::milliseconds totalTime{0ms};
    for (int i = 0; i < m_originalStackItems.Size(); ++i)
    {
      int duration;
      const auto& part{m_originalStackItems.Get(i)};
      if (!CDVDFileInfo::GetFileDuration(part->GetDynPath(), duration))
      {
        Clear();
        return false;
      }
      totalTime += std::chrono::milliseconds{duration};
      times.emplace_back(totalTime);
    }
    if (db.IsOpen())
      db.SetStackTimes(item.GetDynPath(), times);
    haveTimes = true;
  }

  db.Close();

  // If now have times (saved or found above) then update stack items in stackmap
  if (haveTimes)
  {
    const std::chrono::milliseconds totalTime{times.back()};
    for (int i = 0; i < static_cast<int>(times.size()); ++i)
    {
      const auto& part{GetStackPartInformation(m_originalStackItems.Get(i)->GetPath())};
      part->stackItem->SetEndOffset(times[i].count());
      part->startTime = i == 0 ? 0ms : times[i - 1];
    }
    SetStackTotalTime(totalTime); // Set total time
    m_knownStackParts = static_cast<int>(times.size());
  }

  // Remember if this was a disc stack
  m_wasDiscStack = HasDiscParts();

  return true;
}

bool CApplicationStackHelper::ProcessNextPartInBookmark(CFileItem& item, CBookmark& bookmark)
{
  if (std::optional<int> nextPart{KODI::VIDEO::UTILS::GetNextPartFromBookmark(bookmark)}; nextPart)
  {
    // This updates the FileItem with the next part in the stack
    item = SetStackPartAsCurrent(*nextPart);
    bookmark.timeInSeconds = 0.0;
    bookmark.playerState.clear();
    return true;
  }
  return false;
}

static constexpr std::chrono::milliseconds STARTOFFSET_RESUME_MS = -1ms;

void CApplicationStackHelper::GetStackPartAndOptions(CFileItem& item,
                                                     CPlayerOptions& options,
                                                     bool restart)
{
  std::unique_lock stackLock(m_critSection);

  std::chrono::milliseconds start{0ms};
  bool updated{false};

  std::string path{item.GetDynPath()}; // stack:// path
  if (item.HasProperty("original_listitem_url") &&
      URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
    path = item.GetProperty("original_listitem_url").asString();

  if (restart)
  {
    m_currentStackPosition = 0;
    options = {};
  }
  else
  {
    // If resume then find start time in bookmark
    start = std::chrono::milliseconds(item.GetStartOffset());
    if (start == STARTOFFSET_RESUME_MS)
    {
      CVideoDatabase db;
      if (db.Open())
      {
        CBookmark bookmark;
        if (db.GetResumeBookMark(path, bookmark))
        {
          updated = ProcessNextPartInBookmark(item, bookmark);
          start = std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::duration<double>(bookmark.timeInSeconds));
          options.starttime = bookmark.timeInSeconds;
          options.state = bookmark.playerState;
          item.SetStartOffset(STARTOFFSET_RESUME);
        }
        db.Close();
      }
    }
  }

  // Find stack part at this absolute time
  if (!updated)
  {
    m_currentStackPosition = GetStackPartNumberAtTime(start);
    item = *m_originalStackItems.Get(m_currentStackPosition);
    options.starttime =
        std::chrono::duration<double>(start - GetStackPartStartTime(m_currentStackPosition))
            .count(); // Relative time in seconds
  }

  options.fullscreen =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_fullScreenOnMovieStart &&
      !CMediaSettings::GetInstance().DoesMediaStartWindowed();
}

bool CApplicationStackHelper::UpdateDiscStackAndItemTimes(const CFileItem& playedFile)
{
  // Build stacktimes for disc image stacks after each disc is played
  // Assumes the stack dynpaths have already been updated
  if (m_stackMap.empty())
    return false;

  CVideoDatabase db;
  if (!db.Open())
    return false;

  std::unique_lock stackLock(m_critSection);

  // Get existing stacktimes (if any)
  const std::string newStackPath{m_stackMap.begin()->second->stackItem->GetDynPath()};
  const std::string path{!m_oldStackPath.empty() ? m_oldStackPath : newStackPath};
  CLog::LogF(LOGDEBUG, "Looking for existing stacktimes ({})", path);

  std::vector<std::chrono::milliseconds> times;
  const bool haveTimes{db.GetStackTimes(path, times)};

  // See if new part played
  if (GetKnownStackParts() - 1 == static_cast<int>(times.size()))
  {
    std::chrono::milliseconds thisPartTime{
        playedFile.GetVideoInfoTag()->m_streamDetails.GetVideoDuration() * 1000ms};
    const std::chrono::milliseconds startPartTime{haveTimes ? times.back() : 0ms};
    const std::chrono::milliseconds totalTime{startPartTime + thisPartTime};
    CLog::LogF(LOGDEBUG, "Updating stack total time to {}ms (was {}ms)", totalTime.count(),
               thisPartTime.count());

    // Add this part's time to end of stacktimes
    times.emplace_back(totalTime);
    db.SetStackTimes(newStackPath, times);
    db.Close();

    // Update stack times (for bookmark and % played)
    SetStackPartStartTime(playedFile, startPartTime);
    SetStackTotalTime(totalTime);

    return true;
  }

  db.Close();
  return true;
}

void CApplicationStackHelper::SetNextPartBookmark(const std::string& path)
{

  CBookmark bookmark;
  bookmark.timeInSeconds = static_cast<double>(GetCurrentStackPartStartTime().count()) / 1000.0;
  bookmark.totalTimeInSeconds = static_cast<double>(GetStackTotalTime().count()) / 1000.0;
  bookmark.playerState = StringUtils::Format("<nextpart>{}</nextpart>", GetCurrentPartNumber());

  CVideoDatabase db;
  if (db.Open())
  {
    db.AddBookMarkToFile(path, bookmark, CBookmark::RESUME);
    db.Close();
  }
}

bool CApplicationStackHelper::IsPlayingStack() const
{
  return !m_stackMap.empty();
}

bool CApplicationStackHelper::IsPlayingDiscStack() const
{
  return !m_stackMap.empty() && !IsPlayingRegularStack();
}

bool CApplicationStackHelper::IsPlayingRegularStack() const
{
  return !m_stackMap.empty() && !HasDiscParts();
}

bool CApplicationStackHelper::HasDiscParts() const
{
  return std::ranges::any_of(m_stackPaths,
                             [](const std::string& path) {
                               return URIUtils::IsDiscImage(path) || URIUtils::IsDVDFile(path) ||
                                      URIUtils::IsBDFile(path);
                             });
}

bool CApplicationStackHelper::HasNextStackPartFileItem() const
{
  return m_currentStackPosition < static_cast<int>(m_stackMap.size()) - 1;
}

bool CApplicationStackHelper::IsPlayingLastStackPart() const
{
  return m_currentStackPosition == static_cast<int>(m_stackMap.size()) - 1;
}

CFileItem& CApplicationStackHelper::SetNextStackPartAsCurrent()
{
  ++m_currentStackPosition;
  return GetCurrentStackPart();
}

CFileItem& CApplicationStackHelper::SetStackPartAsCurrent(int partNumber)
{
  m_currentStackPosition = partNumber;
  return GetCurrentStackPart();
}

CFileItem& CApplicationStackHelper::GetCurrentStackPart() const
{
  return *m_originalStackItems.Get(m_currentStackPosition);
}

std::chrono::milliseconds CApplicationStackHelper::GetStackPartEndTime(int partNumber) const
{
  return std::chrono::milliseconds(
      GetStackPartInformation(m_stackPaths[partNumber])->stackItem->GetEndOffset());
}

std::chrono::milliseconds CApplicationStackHelper::GetStackPartStartTime(int partNumber) const
{
  return GetStackPartInformation(m_stackPaths[partNumber])->startTime;
}

std::chrono::milliseconds CApplicationStackHelper::GetCurrentStackPartStartTime() const
{
  return GetStackPartInformation(m_stackPaths[m_currentStackPosition])->startTime;
}

std::chrono::milliseconds CApplicationStackHelper::GetStackTotalTime() const
{
  return m_stackTotalTime;
}

int CApplicationStackHelper::GetStackPartNumberAtTime(std::chrono::milliseconds msecs) const
{
  if (msecs > 0ms)
  {
    for (int i = static_cast<int>(m_stackMap.size()) - 1; i >= 0; --i)
    {
      const auto startTime{GetStackPartStartTime(i)};
      const auto endTime{GetStackPartEndTime(i)};
      if (endTime > 0ms && msecs >= startTime && msecs < endTime)
        return i;
    }
  }
  return 0;
}

std::shared_ptr<const CFileItem> CApplicationStackHelper::GetStack(const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->stackItem;
}

bool CApplicationStackHelper::IsInStack(const CFileItem& item) const
{
  const auto it = m_stackMap.find(item.GetPath());
  return it != m_stackMap.end() && it->second != nullptr;
}

int CApplicationStackHelper::GetStackPartNumber(const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->partNumber;
}

std::chrono::milliseconds CApplicationStackHelper::GetStackPartStartTime(
    const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->startTime;
}

void CApplicationStackHelper::SetStackPartStartTime(const CFileItem& item,
                                                    std::chrono::milliseconds startTime) const
{
  GetStackPartInformation(item.GetPath())->startTime = startTime;
}

void CApplicationStackHelper::SetStackDynPaths(const std::string& newPath) const
{
  for (const auto& stackMapItem : m_stackMap | std::views::values)
    stackMapItem->stackItem->SetDynPath(newPath);
}

void CApplicationStackHelper::SetStackFileIds(int fileId)
{
  for (const auto& stackMapItem : m_stackMap | std::views::values)
    stackMapItem->stackItem->GetVideoInfoTag()->m_iFileId = fileId;
}

void CApplicationStackHelper::SetStackPartDynPath(const CFileItem& item)
{
  if (m_stackMap.empty())
    return;

  // Need to build stack://
  std::vector<std::string> paths{};

  // Get DynPath of first item in stack
  std::string stackedPath{m_stackMap.begin()->second->stackItem->GetDynPath()};
  CStackDirectory::GetPaths(stackedPath, paths);
  const int partNumber{GetStackPartNumber(item)};

  // Save last stack path
  m_oldStackPath = stackedPath;

  // Update this part with new DynPath
  paths[partNumber] = item.GetDynPath();

  // Generate new stack:// path
  CStackDirectory::ConstructStackPath(paths, stackedPath);
  SetStackDynPaths(stackedPath);
}

std::string CApplicationStackHelper::GetStackDynPath() const
{
  return !m_stackMap.empty() ? m_stackMap.begin()->second->stackItem->GetDynPath() : std::string{};
}

std::string CApplicationStackHelper::GetOldStackDynPath() const
{
  return m_oldStackPath;
}

void CApplicationStackHelper::SetStackTotalTime(const std::chrono::milliseconds totalTime)
{
  m_stackTotalTime = totalTime;
}

void CApplicationStackHelper::SetStackPartOffsets(const CFileItem& item,
                                                  const std::chrono::milliseconds startOffset,
                                                  const std::chrono::milliseconds endOffset) const
{
  if (const auto it{m_stackMap.find(item.GetPath())}; it != m_stackMap.end())
  {
    auto stackItem{it->second->stackItem};
    stackItem->SetStartOffset(startOffset.count());
    stackItem->SetEndOffset(endOffset.count());
  }
}

void CApplicationStackHelper::IncreaseKnownStackParts()
{
  m_knownStackParts += 1;
  m_oldStackPath.clear();
}

std::shared_ptr<CApplicationStackHelper::StackPartInformation> CApplicationStackHelper::
    GetOrCreateStackPartInformation(const std::string& key)
{
  if (!m_stackMap.contains(key))
    m_stackMap[key] = std::make_shared<StackPartInformation>();
  return m_stackMap[key];
}

std::shared_ptr<CApplicationStackHelper::StackPartInformation> CApplicationStackHelper::
    GetStackPartInformation(const std::string& key) const
{
  if (const auto it{m_stackMap.find(key)}; it != m_stackMap.end())
    return it->second;
  return std::make_shared<StackPartInformation>();
}
