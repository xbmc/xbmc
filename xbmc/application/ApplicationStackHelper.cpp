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
  if (!HasRegisteredStack(item))
    m_stackmap.clear();
  else
  {
    auto stack = GetRegisteredStack(item);
    std::erase_if(m_stackmap, [&](const auto& pair) { return pair.second->m_pStack != stack; });
  }
}

bool CApplicationStackHelper::InitializeStack(const CFileItem & item)
{
  if (!item.IsStack())
    return false;

  auto stack = std::make_shared<CFileItem>(item);

  Clear();
  // read and determine kind of stack
  CStackDirectory dir;
  CURL path{item.GetDynPath()};
  if (!dir.GetDirectory(path, *m_currentStack) || m_currentStack->IsEmpty())
    return false;
  for (int i = 0; i < m_currentStack->Size(); i++)
  {
    // keep cross-references between stack parts and the stack
    SetRegisteredStack(GetStackPartFileItem(i), stack);
    SetRegisteredStackPartNumber(GetStackPartFileItem(i), i);
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
      SetRegisteredStackPartStartTimeMs(part, i == 0 ? 0ms : times[i - 1]);
      // set total time in known parts
      SetRegisteredStackTotalTimeMs(part, totalTime);
    }
    m_knownStackParts = static_cast<int>(times.size());
  }

  auto msecs = std::chrono::milliseconds{item.GetStartOffset()};
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
  m_currentStackPosition = GetStackPartNumberAtTimeMs(msecs);

  // Remember if this was a disc stack
  m_wasDiscStack = HasDiscParts();

  return std::optional{(msecs - GetStackPartStartTimeMs(m_currentStackPosition)).count()};
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
  if (!m_currentStack->IsEmpty())
  {
    const int currentPart = GetCurrentPartNumber() < 1 ? 1 : GetCurrentPartNumber();
    for (int i = 0; i < currentPart; ++i)
    {
      const std::string path = GetStackPartFileItem(i).GetDynPath();
      if (URIUtils::IsDiscImage(path) || URIUtils::IsDVDFile(path) || URIUtils::IsBDFile(path))
      {
        return false;
      }
    }
    return true;
  }
  return false;
}

bool CApplicationStackHelper::HasDiscParts() const
{
  for (int i = 0; i < m_currentStack->Size(); ++i)
  {
    const std::string path = GetStackPartFileItem(i).GetDynPath();
    if (URIUtils::IsDiscImage(path) || URIUtils::IsDVDFile(path) || URIUtils::IsBDFile(path))
      return true;
  }
  return false;
}

bool CApplicationStackHelper::HasNextStackPartFileItem() const
{
  return m_currentStackPosition < m_currentStack->Size() - 1;
}

std::chrono::milliseconds CApplicationStackHelper::GetStackPartEndTimeMs(int partNumber) const
{
  return std::chrono::milliseconds(GetStackPartFileItem(partNumber).GetEndOffset());
}

std::chrono::milliseconds CApplicationStackHelper::GetStackTotalTimeMs() const
{
  for (int i = m_currentStack->Size() - 1; i >= 0; --i)
  {
    const auto endTime{GetStackPartEndTimeMs(i)};
    if (endTime > 0ms)
      return endTime;
  }
  return 0ms;
}

int CApplicationStackHelper::GetStackPartNumberAtTimeMs(std::chrono::milliseconds msecs) const
{
  if (msecs > 0ms)
  {
    // work out where to seek to
    for (int partNumber = 0; partNumber < m_currentStack->Size(); partNumber++)
    {
      if (msecs < GetStackPartEndTimeMs(partNumber))
        return partNumber;
    }
  }
  return 0;
}

void CApplicationStackHelper::ClearAllRegisteredStackInformation()
{
  m_stackmap.clear();
}

std::shared_ptr<const CFileItem> CApplicationStackHelper::GetRegisteredStack(
    const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->m_pStack;
}

bool CApplicationStackHelper::HasRegisteredStack(const CFileItem& item) const
{
  const auto it = m_stackmap.find(item.GetPath());
  return it != m_stackmap.end() && it->second != nullptr;
}

void CApplicationStackHelper::SetRegisteredStack(const CFileItem& item,
                                                 std::shared_ptr<CFileItem> stackItem)
{
  GetStackPartInformation(item.GetPath())->m_pStack = std::move(stackItem);
}

CFileItem& CApplicationStackHelper::GetStackPartFileItem(int partNumber)
{
  return *(*m_currentStack)[partNumber];
}

const CFileItem& CApplicationStackHelper::GetStackPartFileItem(int partNumber) const
{
  return *(*m_currentStack)[partNumber];
}

int CApplicationStackHelper::GetRegisteredStackPartNumber(const CFileItem& item)
{
  return GetStackPartInformation(item.GetPath())->m_lStackPartNumber;
}

void CApplicationStackHelper::SetRegisteredStackPartNumber(const CFileItem& item, int partNumber)
{
  GetStackPartInformation(item.GetPath())->m_lStackPartNumber = partNumber;
}

std::chrono::milliseconds CApplicationStackHelper::GetRegisteredStackPartStartTimeMs(
    const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->m_lStackPartStartTimeMs;
}

void CApplicationStackHelper::SetRegisteredStackPartStartTimeMs(const CFileItem& item,
                                                                std::chrono::milliseconds startTime)
{
  GetStackPartInformation(item.GetPath())->m_lStackPartStartTimeMs = startTime;
}

std::chrono::milliseconds CApplicationStackHelper::GetRegisteredStackTotalTimeMs(
    const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->m_lStackTotalTimeMs;
}

void CApplicationStackHelper::SetRegisteredStackTotalTimeMs(const CFileItem& item,
                                                            std::chrono::milliseconds totalTime)
{
  GetStackPartInformation(item.GetPath())->m_lStackTotalTimeMs = totalTime;
}

void CApplicationStackHelper::SetRegisteredStackDynPaths(const std::string& newPath) const
{
  for (const auto& stackmapitem : m_stackmap)
    stackmapitem.second->m_pStack->SetDynPath(newPath);
}

void CApplicationStackHelper::SetRegisteredStackPartDynPath(const CFileItem& item,
                                                            const std::string& newPath)
{
  CFileItem& stackItem(GetStackPartFileItem(GetRegisteredStackPartNumber(item)));
  stackItem.SetDynPath(newPath);
}

void CApplicationStackHelper::SetStackEndTimeMs(const std::chrono::milliseconds totalTimeMs)
{
  for (int i = 0; i < m_currentStack->Size(); i++)
    SetRegisteredStackTotalTimeMs(GetStackPartFileItem(i), totalTimeMs);
}

void CApplicationStackHelper::SetStackPartOffsets(const CFileItem& item,
                                                  const std::chrono::milliseconds startOffset,
                                                  const std::chrono::milliseconds endOffset)
{
  CFileItem& stackItem(GetStackPartFileItem(GetRegisteredStackPartNumber(item)));
  stackItem.SetStartOffset(startOffset.count());
  stackItem.SetEndOffset(endOffset.count());
}

std::shared_ptr<CApplicationStackHelper::StackPartInformation> CApplicationStackHelper::
    GetStackPartInformation(const std::string& key)
{
  if (!m_stackmap.contains(key))
    m_stackmap[key] = std::make_shared<StackPartInformation>();
  return m_stackmap[key];
}

std::shared_ptr<CApplicationStackHelper::StackPartInformation> CApplicationStackHelper::
    GetStackPartInformation(const std::string& key) const
{
  const auto it = m_stackmap.find(key);
  if (it == m_stackmap.end())
    return std::make_shared<StackPartInformation>();
  return it->second;
}
