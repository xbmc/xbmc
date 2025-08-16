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
#include "URL.h"
#include "Util.h"
#include "cores/VideoPlayer/DVDFileInfo.h"
#include "filesystem/StackDirectory.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <ranges>
#include <string>
#include <vector>
#include <utility>

using namespace XFILE;
using namespace std::chrono_literals;

void CApplicationStackHelper::Clear()
{
  m_currentStackPosition = 0;
  m_currentStack->Clear();
}

void CApplicationStackHelper::OnPlayBackStarted(const CFileItem& item)
{
  std::unique_lock lock(m_critSection);

  // time to clean up stack map
  if (!IsInStack(item))
    m_stackMap.clear();
  else
  {
    const auto stack{GetStack(item)};
    std::erase_if(m_stackMap, [&](const auto& pair) { return pair.second->m_stackItem != stack; });
  }
}

bool CApplicationStackHelper::InitializeStack(const CFileItem & item)
{
  if (!item.IsStack())
    return false;

  const auto stack{std::make_shared<CFileItem>(item)};
  Clear();

  // read and determine kind of stack
  CStackDirectory dir;
  CURL path{item.GetDynPath()};
  if (!dir.GetDirectory(path, *m_currentStack) || m_currentStack->IsEmpty())
    return false;
  for (int i = 0; i < m_currentStack->Size(); i++)
  {
    // keep cross-references between stack parts and the stack
    GetStackPartInformation(GetStackPartFileItem(i).GetPath())->m_stackItem = stack;
    GetStackPartInformation(GetStackPartFileItem(i).GetPath())->m_stackPartNumber = i;
  }

  return true;
}

static constexpr std::chrono::milliseconds STARTOFFSET_RESUME_MS = -1ms;

std::optional<int64_t> CApplicationStackHelper::InitializeStackStartPartAndOffset(
    const CFileItem& item)
{
  CVideoDatabase dbs;
  std::vector<std::chrono::milliseconds> times;
  std::chrono::milliseconds totalTime{0ms};
  bool haveTimes{false};

  if (dbs.Open())
    haveTimes = dbs.GetStackTimes(item.GetDynPath(), times);

  // If not times and is a regular (file) stack then get times from files
  // Not possible for BD/DVD (folder) stacks due to playlist/title not known
  if (!haveTimes && !IsPlayingDiscStack())
  {
    for (int i = 0; i < m_currentStack->Size(); ++i)
    {
      int duration;
      if (!CDVDFileInfo::GetFileDuration(GetStackPartFileItem(i).GetDynPath(), duration))
      {
        m_currentStack->Clear();
        return std::nullopt;
      }
      totalTime += std::chrono::milliseconds{duration};
      times.emplace_back(totalTime);
    }
    dbs.SetStackTimes(item.GetDynPath(), times);
    haveTimes = true;
  }

  // If have times (saved or found above) then update stack
  if (haveTimes)
  {
    totalTime = times.back();
    for (int i = 0; i < static_cast<int>(times.size()); ++i)
    {
      CFileItem& part{GetStackPartFileItem(i)};
      // set end time in known parts
      part.SetEndOffset(times[i].count());
      // set start time in known parts
      SetStackPartStartTime(part, i == 0 ? 0ms : times[i - 1]);
    }
    // set total time
    SetStackTotalTime(totalTime);
    m_knownStackParts = static_cast<int>(times.size());
  }

  auto msecs{std::chrono::milliseconds{item.GetStartOffset()}};
  if (msecs == STARTOFFSET_RESUME_MS)
  {
    CBookmark bookmark;
    std::string path = item.GetDynPath();
    if (item.HasProperty("original_listitem_url") &&
        URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
      path = item.GetProperty("original_listitem_url").asString();
    if (dbs.GetResumeBookMark(path, bookmark))
      msecs = std::chrono::milliseconds{static_cast<int64_t>(bookmark.timeInSeconds * 1000)};
    else
      msecs = 0ms;
  }

  dbs.Close();

  // Adjust absolute position to stack
  m_currentStackPosition = GetStackPartNumberAtTime(msecs);

  // Remember if this was a disc stack
  m_wasDiscStack = HasDiscParts();

  return std::optional{(msecs - GetStackPartStartTime(m_currentStackPosition)).count()};
}

bool CApplicationStackHelper::IsPlayingStack() const
{
  return m_currentStack->Size() > 0;
}

bool CApplicationStackHelper::IsPlayingDiscStack() const
{
  return !m_currentStack->IsEmpty() && !IsPlayingRegularStack();
}

bool CApplicationStackHelper::IsPlayingRegularStack() const
{
  return !m_currentStack->IsEmpty() && !HasDiscParts();
}

bool CApplicationStackHelper::HasDiscParts() const
{
  return std::any_of(m_currentStack->begin(), m_currentStack->end(),
                     [](const std::shared_ptr<CFileItem>& item)
                     {
                       const std::string& path{item->GetDynPath()};
                       return URIUtils::IsDiscImage(path) || URIUtils::IsDVDFile(path) ||
                              URIUtils::IsBDFile(path);
                     });
}

bool CApplicationStackHelper::HasNextStackPartFileItem() const
{
  return m_currentStackPosition < m_currentStack->Size() - 1;
}

CFileItem& CApplicationStackHelper::SetNextStackPartAsCurrent()
{
  ++m_currentStackPosition;
  return GetStackPartFileItem(m_currentStackPosition);
}

CFileItem& CApplicationStackHelper::SetStackPartAsCurrent(int partNumber)
{
  m_currentStackPosition = partNumber;
  return GetStackPartFileItem(m_currentStackPosition);
}

CFileItem& CApplicationStackHelper::GetCurrentStackPart()
{
  return GetStackPartFileItem(m_currentStackPosition);
}

std::chrono::milliseconds CApplicationStackHelper::GetStackPartEndTime(int partNumber) const
{
  return std::chrono::milliseconds(GetStackPartFileItem(partNumber).GetEndOffset());
}

std::chrono::milliseconds CApplicationStackHelper::GetStackTotalTime() const
{
  for (int i = m_currentStack->Size() - 1; i >= 0; --i)
  {
    const auto endTime{GetStackPartEndTime(i)};
    if (endTime > 0ms)
      return endTime;
  }
  return 0ms;
}

int CApplicationStackHelper::GetStackPartNumberAtTime(std::chrono::milliseconds msecs) const
{
  if (msecs > 0ms)
  {
    // work out where to seek to
    for (int partNumber = 0; partNumber < m_currentStack->Size(); partNumber++)
    {
      if (msecs < GetStackPartEndTime(partNumber))
        return partNumber;
    }
  }
  return 0;
}

void CApplicationStackHelper::ClearAllStackInformation()
{
  m_stackMap.clear();
}

std::shared_ptr<const CFileItem> CApplicationStackHelper::GetStack(
    const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->m_stackItem;
}

bool CApplicationStackHelper::IsInStack(const CFileItem& item) const
{
  const auto it = m_stackMap.find(item.GetPath());
  return it != m_stackMap.end() && it->second != nullptr;
}

CFileItem& CApplicationStackHelper::GetStackPartFileItem(int partNumber)
{
  return *(*m_currentStack)[partNumber];
}

const CFileItem& CApplicationStackHelper::GetStackPartFileItem(int partNumber) const
{
  return *(*m_currentStack)[partNumber];
}

int CApplicationStackHelper::GetStackPartNumber(const CFileItem& item)
{
  return GetStackPartInformation(item.GetPath())->m_stackPartNumber;
}

std::chrono::milliseconds CApplicationStackHelper::GetStackPartStartTime(
    const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->m_stackPartStartTimeMs;
}

void CApplicationStackHelper::SetStackPartStartTime(const CFileItem& item,
                                                                std::chrono::milliseconds startTime)
{
  GetStackPartInformation(item.GetPath())->m_stackPartStartTimeMs = startTime;
}

std::chrono::milliseconds CApplicationStackHelper::GetStackTotalTime(
    const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->m_stackTotalTimeMs;
}

void CApplicationStackHelper::SetStackDynPaths(const std::string& newPath) const
{
  for (const auto& stackmapitem : m_stackMap | std::views::values)
    stackmapitem->m_stackItem->SetDynPath(newPath);
}

void CApplicationStackHelper::SetStackPartDynPath(const CFileItem& item)
{
  // Need to build stack://
  std::vector<std::string> paths{};
  CStackDirectory::GetPaths(item.GetDynPath(), paths);
  const int partNumber{GetStackPartNumber(item)};
  paths[partNumber] = item.GetDynPath();
  std::string stackedPath;
  CStackDirectory::ConstructStackPath(paths, stackedPath);
  SetStackDynPaths(stackedPath);
}

void CApplicationStackHelper::SetStackTotalTime(const std::chrono::milliseconds totalTime)
{
  std::ranges::for_each(m_stackMap | std::views::values,
                        [totalTime](const auto& part) { part->m_stackTotalTimeMs = totalTime; });
}

void CApplicationStackHelper::SetStackPartOffsets(const CFileItem& item,
                                                  const std::chrono::milliseconds startOffset,
                                                  const std::chrono::milliseconds endOffset)
{
  CFileItem& stackItem(GetStackPartFileItem(GetStackPartNumber(item)));
  stackItem.SetStartOffset(startOffset.count());
  stackItem.SetEndOffset(endOffset.count());
}

std::shared_ptr<CApplicationStackHelper::StackPartInformation> CApplicationStackHelper::
    GetStackPartInformation(const std::string& key)
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
