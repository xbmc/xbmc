/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ApplicationStackHelper.h"

#include "FileItem.h"
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
  if (!dir.GetDirectory(item.GetURL(), *m_currentStack) || m_currentStack->IsEmpty())
    return false;
  for (int i = 0; i < m_currentStack->Size(); i++)
  {
    // keep cross-references between stack parts and the stack
    SetRegisteredStack(GetStackPartFileItem(i), stack);
    SetRegisteredStackPartNumber(GetStackPartFileItem(i), i);
  }
  m_currentStackIsDiscImageStack = URIUtils::IsDiscImageStack(item.GetDynPath());

  return true;
}

int CApplicationStackHelper::InitializeStackStartPartAndOffset(const CFileItem& item)
{
  CVideoDatabase dbs;
  int64_t startoffset = 0;

  // case 1: stacked ISOs
  if (m_currentStackIsDiscImageStack)
  {
    // first assume values passed to the stack
    int selectedFile = item.m_lStartPartNumber;
    startoffset = item.GetStartOffset();

    // check if we instructed the stack to resume from default
    if (startoffset == STARTOFFSET_RESUME) // selected file is not specified, pick the 'last' resume point
    {
      if (dbs.Open())
      {
        CBookmark bookmark;
        std::string path = item.GetPath();
        if (item.HasProperty("original_listitem_url") && URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
          path = item.GetProperty("original_listitem_url").asString();
        if (dbs.GetResumeBookMark(path, bookmark))
        {
          startoffset = CUtil::ConvertSecsToMilliSecs(bookmark.timeInSeconds);
          selectedFile = bookmark.partNumber;
        }
        dbs.Close();
      }
      else
        CLog::LogF(LOGERROR, "Cannot open VideoDatabase");
    }

    // make sure that the selected part is within the boundaries
    if (selectedFile <= 0)
    {
      CLog::LogF(LOGWARNING, "Selected part {} out of range, playing part 1", selectedFile);
      selectedFile = 1;
    }
    else if (selectedFile > m_currentStack->Size())
    {
      CLog::LogF(LOGWARNING, "Selected part {} out of range, playing part {}", selectedFile,
                 m_currentStack->Size());
      selectedFile = m_currentStack->Size();
    }

    // set startoffset in selected item, track stack item for updating purposes, and finally play disc part
    m_currentStackPosition = selectedFile - 1;
    startoffset = startoffset > 0 ? STARTOFFSET_RESUME : 0;
  }
  // case 2: all other stacks
  else
  {
    // see if we have the info in the database
    //! @todo If user changes the time speed (FPS via framerate conversion stuff)
    //!       then these times will be wrong.
    //!       Also, this is really just a hack for the slow load up times we have
    //!       A much better solution is a fast reader of FPS and fileLength
    //!       that we can use on a file to get it's time.
    std::vector<uint64_t> times;
    bool haveTimes(false);

    if (dbs.Open())
    {
      haveTimes = dbs.GetStackTimes(item.GetPath(), times);
      dbs.Close();
    }

    // calculate the total time of the stack
    uint64_t totalTimeMs = 0;
    for (int i = 0; i < m_currentStack->Size(); i++)
    {
      if (haveTimes)
      {
        // set end time in every part
        GetStackPartFileItem(i).SetEndOffset(times[i]);
      }
      else
      {
        int duration;
        if (!CDVDFileInfo::GetFileDuration(GetStackPartFileItem(i).GetPath(), duration))
        {
          m_currentStack->Clear();
          return false;
        }
        totalTimeMs += duration;
        // set end time in every part
        GetStackPartFileItem(i).SetEndOffset(totalTimeMs);
        times.push_back(totalTimeMs);
      }
      // set start time in every part
      SetRegisteredStackPartStartTimeMs(GetStackPartFileItem(i), GetStackPartStartTimeMs(i));
    }
    // set total time in every part
    totalTimeMs = GetStackTotalTimeMs();
    for (int i = 0; i < m_currentStack->Size(); i++)
      SetRegisteredStackTotalTimeMs(GetStackPartFileItem(i), totalTimeMs);

    uint64_t msecs = item.GetStartOffset();

    if (!haveTimes || item.GetStartOffset() == STARTOFFSET_RESUME)
    {
      if (dbs.Open())
      {
        // have our times now, so update the dB
        if (!haveTimes && !times.empty())
          dbs.SetStackTimes(item.GetPath(), times);

        if (item.GetStartOffset() == STARTOFFSET_RESUME)
        {
          // can only resume seek here, not dvdstate
          CBookmark bookmark;
          std::string path = item.GetPath();
          if (item.HasProperty("original_listitem_url") && URIUtils::IsPlugin(item.GetProperty("original_listitem_url").asString()))
            path = item.GetProperty("original_listitem_url").asString();
          if (dbs.GetResumeBookMark(path, bookmark))
            msecs = static_cast<uint64_t>(bookmark.timeInSeconds * 1000);
          else
            msecs = 0;
        }
        dbs.Close();
      }
    }

    m_currentStackPosition = GetStackPartNumberAtTimeMs(msecs);
    startoffset = msecs - GetStackPartStartTimeMs(m_currentStackPosition);
  }
  return startoffset;
}

bool CApplicationStackHelper::IsPlayingISOStack() const
{
  return m_currentStack->Size() > 0 && m_currentStackIsDiscImageStack;
}

bool CApplicationStackHelper::IsPlayingRegularStack() const
{
  return m_currentStack->Size() > 0 && !m_currentStackIsDiscImageStack;
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
  return GetStackPartEndTimeMs(m_currentStack->Size() - 1);
}

int CApplicationStackHelper::GetStackPartNumberAtTimeMs(uint64_t msecs)
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
