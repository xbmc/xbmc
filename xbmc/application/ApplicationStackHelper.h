/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "FileItemList.h"
#include "application/IApplicationComponent.h"
#include "threads/CriticalSection.h"

#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <string>

using namespace std::chrono_literals;

class CFileItem;
class CFileItemList;

class CApplicationStackHelper : public IApplicationComponent
{
public:
  void Clear();
  void OnPlayBackStarted(const CFileItem& item);

  /*!
  \brief Initialize stack
  \param item the FileItem object that is the stack
  */
  bool InitializeStack(const CFileItem& item);

  /*!
  \brief Initialize stack times for each part, start & end, total time, and current part number if resume offset is specified.
  \param item the FileItem object that is the stack
  \returns the part offset if available, nullopt in case of errors
  */
  std::optional<int64_t> InitializeStackStartPartAndOffset(const CFileItem& item);

  /*!
  \brief returns the current part number
  */
  int GetCurrentPartNumber() const { return m_currentStackPosition; }

  /*!
  \brief returns the total number of parts
  */
  int GetTotalPartNumbers() const { return m_currentStack->Size(); }

  /*!
  \brief Returns true if Application is currently playing any stack
  \return true if Application is currently playing a stack, false otherwise
  */
  bool IsPlayingStack() const;

  /*!
  \brief Returns true if Application is currently playing an disc (ISO/BMDV/VIDEO_TS) stack
  */
  bool IsPlayingDiscStack() const;

  /*!
  \brief Returns true if Application is currently playing a Regular (non-ISO) stack
  */
  bool IsPlayingRegularStack() const;

  /*!
  \brief returns true if there is a next part available
  */
  bool HasNextStackPartFileItem() const;

  /*!
  \brief sets the next stack part as the current and returns a reference to it
  */
  const CFileItem& SetNextStackPartCurrentFileItem()
  {
    ++m_currentStackPosition;
    return GetStackPartFileItem(m_currentStackPosition);
  }

  /*!
  \brief sets a given stack part as the current and returns a reference to it
  \param partNumber the number of the part that needs to become the current one
  */
  const CFileItem& SetStackPartCurrentFileItem(int partNumber)
  {
    m_currentStackPosition = partNumber;
    return GetStackPartFileItem(m_currentStackPosition);
  }

  /*!
  \brief Returns the FileItem currently playing back as part of a stack playback
  */
  const CFileItem& GetCurrentStackPartFileItem() const
  {
    return GetStackPartFileItem(m_currentStackPosition);
  }

  /*!
  \brief Returns the end time of a FileItem part of a stack playback
  \param partNumber the requested part number in the stack
  */
  std::chrono::milliseconds GetStackPartEndTimeMs(int partNumber) const;

  /*!
  \brief Returns the start time of a FileItem part of a stack playback
  \param partNumber the requested part number in the stack
  */
  std::chrono::milliseconds GetStackPartStartTimeMs(int partNumber) const
  {
    return (partNumber > 0) ? GetStackPartEndTimeMs(partNumber - 1) : 0ms;
  }

  /*!
  \brief Returns the start time of the current FileItem part of a stack playback
  */
  std::chrono::milliseconds GetCurrentStackPartStartTimeMs() const
  {
    return GetStackPartStartTimeMs(m_currentStackPosition);
  }

  /*!
  \brief Returns the total time of a stack playback
  */
  std::chrono::milliseconds GetStackTotalTimeMs() const;

  /*!
  \brief Returns the stack part number corresponding to the given timestamp in a stack playback
  \param msecs the requested timestamp in the stack (in milliseconds)
  */
  int GetStackPartNumberAtTimeMs(std::chrono::milliseconds msecs) const;

  // Stack information registration methods

  /*!
  \brief Clear all entries in the item-stack map. To be called upon playback stopped.
  */
  void ClearAllRegisteredStackInformation();

  /*!
  \brief Returns a smart pointer to the stack CFileItem.
  */
  std::shared_ptr<const CFileItem> GetRegisteredStack(const CFileItem& item) const;

  /*!
  \brief Returns true if there is a registered stack for the given CFileItem part.
  \param item the reference to the item that is part of a stack
  */
  bool HasRegisteredStack(const CFileItem& item) const;

  /*!
  \brief Stores a smart pointer to the stack CFileItem in the item-stack map.
  \param item the reference to the item that is part of a stack
  \param stackItem the smart pointer to the stack CFileItem
  */
  void SetRegisteredStack(const CFileItem& item, std::shared_ptr<CFileItem> stackItem);

  /*!
  \brief Returns the part number of the part in the parameter
  \param item the reference to the item that is part of a stack
  */
  int GetRegisteredStackPartNumber(const CFileItem& item);

  /*!
  \brief Stores the part number in the item-stack map.
  \param item the reference to the item that is part of a stack
  \param partNumber the part number of the part in other parameter
  */
  void SetRegisteredStackPartNumber(const CFileItem& item, int partNumber);

  /*!
  \brief Returns the start time of the part in the parameter
  \param item the reference to the item that is part of a stack
  */
  std::chrono::milliseconds GetRegisteredStackPartStartTimeMs(const CFileItem& item) const;

  /*!
  \brief Stores the part start time in the item-stack map.
  \param item the reference to the item that is part of a stack
  \param startTime the start time of the part in other parameter
  */
  void SetRegisteredStackPartStartTimeMs(const CFileItem& item,
                                         std::chrono::milliseconds startTimeMs);

  /*!
  \brief Returns the total time of the stack associated to the part in the parameter
  \param item the reference to the item that is part of a stack
  */
  std::chrono::milliseconds GetRegisteredStackTotalTimeMs(const CFileItem& item) const;

  /*!
  \brief Stores the stack's total time associated to the part in the item-stack map.
  \param item the reference to the item that is part of a stack
  \param totalTime the total time of the stack
  */
  void SetRegisteredStackTotalTimeMs(const CFileItem& item, std::chrono::milliseconds totalTimeMs);

  /*!
  \brief Updates the DynPath (which contains the entire stack://) as each element is played
  \param newPath the updated stack:// path
  */
  void SetRegisteredStackDynPaths(const std::string& newPath) const;

  void SetRegisteredStackPartDynPath(const CFileItem& item, const std::string& newPath);

  void SetStackEndTimeMs(const std::chrono::milliseconds totalTimeMs);
  void SetStackPartOffsets(const CFileItem& item,
                           const std::chrono::milliseconds startOffset,
                           const std::chrono::milliseconds endOffset);
  int GetKnownStackParts() const { return m_knownStackParts; }
  void IncreaseKnownStackParts() { m_knownStackParts += 1; }

  void SetStackPartStopped(const bool value) { m_stackpartstopped = value; }
  bool GetStackPartStopped() const { return m_stackpartstopped; }

  bool HasDiscParts() const;

  bool WasPlayingDiscStack() const { return m_wasDiscStack; }

  CCriticalSection m_critSection;

private:
  /*!
  \brief Returns a FileItem part of a stack playback
  \param partNumber the requested part number in the stack
  */
  CFileItem& GetStackPartFileItem(int partNumber);
  const CFileItem& GetStackPartFileItem(int partNumber) const;

  class StackPartInformation
  {
  public:
    std::chrono::milliseconds m_lStackPartStartTimeMs{0ms};
    std::chrono::milliseconds m_lStackTotalTimeMs{0ms};
    int m_lStackPartNumber{0};
    std::shared_ptr<CFileItem> m_pStack;
  };

  using Stackmap = std::map<std::string, std::shared_ptr<StackPartInformation>, std::less<>>;
  Stackmap m_stackmap;
  std::shared_ptr<StackPartInformation> GetStackPartInformation(const std::string& key);
  std::shared_ptr<StackPartInformation> GetStackPartInformation(const std::string& key) const;

  std::unique_ptr<CFileItemList> m_currentStack{new CFileItemList};
  int m_currentStackPosition = 0;
  int m_knownStackParts{0};
  bool m_wasDiscStack{false};
  bool m_stackpartstopped{false};
};
