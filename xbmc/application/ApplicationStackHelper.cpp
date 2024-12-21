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

#include <utility>

using namespace XFILE;

CApplicationStackHelper::CApplicationStackHelper(void)
  : m_currentStack(new CFileItemList)
{
}

void CApplicationStackHelper::Clear()
{
  m_currentStackPosition = 0;
  m_currentStack->Clear();
}

void CApplicationStackHelper::OnPlayBackStarted(const CFileItem& item)
{
  std::unique_lock<CCriticalSection> lock(m_critSection);

  // time to clean up stack map
  if (!HasRegisteredStack(item))
    m_stackmap.clear();
  else
  {
    auto stack = GetRegisteredStack(item);
    Stackmap::iterator itr = m_stackmap.begin();
    while (itr != m_stackmap.end())
    {
      if (itr->second->m_pStack != stack)
      {
        itr = m_stackmap.erase(itr);
      }
      else
      {
        ++itr;
      }
    }
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

std::optional<int64_t> CApplicationStackHelper::InitializeStackStartPartAndOffset(
    const CFileItem& item)
{
  // see if we have the info in the database
  //! @todo If user changes the time speed (FPS via framerate conversion stuff)
  //!       then these times will be wrong.
  //!       Also, this is really just a hack for the slow load up times we have
  //!       A much better solution is a fast reader of FPS and fileLength
  //!       that we can use on a file to get it's time.
  CVideoDatabase dbs;

  std::vector<uint64_t> times;
  uint64_t totalTimeMs{0};
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
      totalTimeMs += duration;
      times.emplace_back(totalTimeMs);
    }
    dbs.SetStackTimes(item.GetDynPath(), times);
    haveTimes = true;
  }

  // If have times (saved or found above) then update stack
  if (haveTimes)
  {
    totalTimeMs = times.back();
    int i;
    for (i = 0; i < static_cast<int>(times.size()); ++i)
    {
      CFileItem& part{GetStackPartFileItem(i)};
      // set end time in known parts
      part.SetEndOffset(times[i]);
      // set start time in known parts
      SetRegisteredStackPartStartTimeMs(part, i == 0 ? 0 : times[i - 1]);
      // set total time in known parts
      SetRegisteredStackTotalTimeMs(part, totalTimeMs);
    }
    m_knownStackParts = times.size();
  }

  int64_t msecs = item.GetStartOffset();

  if (msecs == STARTOFFSET_RESUME)
  {
    CBookmark bookmark;
    std::string path = item.GetDynPath();
    if (item.HasProperty("original_listitem_url") &&
        URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
      path = item.GetProperty("original_listitem_url").asString();
    if (dbs.GetResumeBookMark(path, bookmark))
      msecs = static_cast<int64_t>(bookmark.timeInSeconds * 1000);
    else
      msecs = 0;
  }

  dbs.Close();

  // Adjust absolute position to stack
  m_currentStackPosition = GetStackPartNumberAtTimeMs(msecs);

  // Remember if this was a disc stack
  m_wasDiscStack = HasDiscParts();

  return msecs - static_cast<int64_t>(GetStackPartStartTimeMs(m_currentStackPosition));
}

// IsPlayingDiscStack - means we can't move back and forth between items

bool CApplicationStackHelper::IsPlayingDiscStack() const
{
  return m_currentStack->Size() > 0 && !IsPlayingRegularStack();
}

// Is PlayingRegularStack - means we can move back and forth between items

bool CApplicationStackHelper::IsPlayingRegularStack() const
{
  if (m_currentStack->Size() > 0)
  {
    const int currentPart = GetCurrentPartNumber() < 1 ? 1 : GetCurrentPartNumber();
    for (int i = 0; i < currentPart; ++i)
    {
      const std::string path = GetStackPartFileItem(i).GetDynPath();
      if (URIUtils::IsDiscImage(path) || URIUtils::IsDVDFile(path) || URIUtils::IsBDFile(path))
        return false;
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

uint64_t CApplicationStackHelper::GetStackPartEndTimeMs(int partNumber) const
{
  return GetStackPartFileItem(partNumber).GetEndOffset();
}

uint64_t CApplicationStackHelper::GetStackTotalTimeMs() const
{
  for (int i = m_currentStack->Size() - 1; i >= 0; --i)
  {
    const uint64_t endTime = GetStackPartEndTimeMs(i);
    if (endTime > 0)
      return endTime;
  }
  return 0;
}

int CApplicationStackHelper::GetStackPartNumberAtTimeMs(uint64_t msecs) const
{
  if (msecs > 0)
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
  GetStackPartInformation(item.GetDynPath())->m_pStack = std::move(stackItem);
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

uint64_t CApplicationStackHelper::GetRegisteredStackPartStartTimeMs(const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->m_lStackPartStartTimeMs;
}

void CApplicationStackHelper::SetRegisteredStackPartStartTimeMs(const CFileItem& item, uint64_t startTime)
{
  GetStackPartInformation(item.GetPath())->m_lStackPartStartTimeMs = startTime;
}

uint64_t CApplicationStackHelper::GetRegisteredStackTotalTimeMs(const CFileItem& item) const
{
  return GetStackPartInformation(item.GetPath())->m_lStackTotalTimeMs;
}

void CApplicationStackHelper::SetRegisteredStackTotalTimeMs(const CFileItem& item, uint64_t totalTime)
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

void CApplicationStackHelper::SetStackEndTimeMs(const uint64_t totalTimeMs)
{
  for (int i = 0; i < m_currentStack->Size(); i++)
    SetRegisteredStackTotalTimeMs(GetStackPartFileItem(i), totalTimeMs);
}

void CApplicationStackHelper::SetStackPartOffsets(const CFileItem& item,
                                                  const int64_t startOffset,
                                                  const int64_t endOffset)
{
  CFileItem& stackItem(GetStackPartFileItem(GetRegisteredStackPartNumber(item)));
  stackItem.SetStartOffset(startOffset);
  stackItem.SetEndOffset(endOffset);
}

CApplicationStackHelper::StackPartInformationPtr CApplicationStackHelper::GetStackPartInformation(
    const std::string& key)
{
  if (m_stackmap.count(key) == 0)
  {
    StackPartInformationPtr value(new StackPartInformation());
    m_stackmap[key] = value;
  }
  return m_stackmap[key];
}

CApplicationStackHelper::StackPartInformationPtr CApplicationStackHelper::GetStackPartInformation(
    const std::string& key) const
{
  const auto it = m_stackmap.find(key);
  if (it == m_stackmap.end())
    return std::make_shared<StackPartInformation>();
  return it->second;
}