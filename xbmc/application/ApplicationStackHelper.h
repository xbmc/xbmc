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
  CFileItem& SetNextStackPartAsCurrent();

  /*!
  \brief sets a given stack part as the current and returns a reference to it
  \param partNumber the number of the part that needs to become the current one
  */
  CFileItem& SetStackPartAsCurrent(int partNumber);

  /*!
  \brief Returns the FileItem currently playing back as part of a stack playback
  */
  CFileItem& GetCurrentStackPart();

  /*!
  \brief Returns the end time of a FileItem part of a stack playback
  \param partNumber the requested part number in the stack
  */
  std::chrono::milliseconds GetStackPartEndTime(int partNumber) const;

  /*!
  \brief Returns the start time of a FileItem part of a stack playback
  \param partNumber the requested part number in the stack
  */
  std::chrono::milliseconds GetStackPartStartTime(int partNumber) const
  {
    return (partNumber > 0) ? GetStackPartEndTime(partNumber - 1) : 0ms;
  }

  /*!
  \brief Returns the start time of the current FileItem part of a stack playback
  */
  std::chrono::milliseconds GetCurrentStackPartStartTime() const
  {
    return GetStackPartStartTime(m_currentStackPosition);
  }

  /*!
  \brief Returns the total time of a stack playback
  */
  std::chrono::milliseconds GetStackTotalTime() const;

  /*!
  \brief Returns the stack part number corresponding to the given timestamp in a stack playback
  \param msecs the requested timestamp in the stack (in milliseconds)
  */
  int GetStackPartNumberAtTime(std::chrono::milliseconds msecs) const;

  // Stack information registration methods

  /*!
  \brief Clear all entries in the item-stack map. To be called upon playback stopped.
  */
  void ClearAllStackInformation();

  /*!
  \brief Returns a smart pointer to the stack CFileItem.
  */
  std::shared_ptr<const CFileItem> GetStack(const CFileItem& item) const;

  /*!
  \brief Returns true if there is a stack for the given CFileItem part.
  \param item the reference to the item that is part of a stack
  */
  bool IsInStack(const CFileItem& item) const;

  /*!
  \brief Returns the part number of the part in the parameter
  \param item the reference to the item that is part of a stack
  */
  int GetStackPartNumber(const CFileItem& item);

  /*!
  \brief Returns the start time of the part in the parameter
  \param item the reference to the item that is part of a stack
  */
  std::chrono::milliseconds GetStackPartStartTime(const CFileItem& item) const;

  /*!
  \brief Stores the part start time in the item-stack map.
  \param item the reference to the item that is part of a stack
  \param startTime the start time of the part in other parameter
  */
  void SetStackPartStartTime(const CFileItem& item, std::chrono::milliseconds startTimeMs);

  /*!
  \brief Returns the total time of the stack associated to the part in the parameter
  \param item the reference to the item that is part of a stack
  */
  std::chrono::milliseconds GetStackTotalTime(const CFileItem& item) const;

  /*!
  \brief Updates the DynPath (which contains the entire stack://) of each part in the stack.
  \param newPath the updated stack:// path
  */
  void SetStackDynPaths(const std::string& newPath) const;

  /*!
  \brief Updates the stack:// with the DynPath of the given item and then updates all parts in the stack.
  \param item the FileItem in the stack that has an updated DynPath (eg. bluray://)
  \sa SetStackDynPaths
  */
  void SetStackPartDynPath(const CFileItem& item);

  /*!
  \brief Sets the total time of the stack in each stack part.
  \param item the FileItem in the stack that has an updated DynPath (eg. bluray://)
  */
  void SetStackTotalTime(std::chrono::milliseconds totalTime);

  /*!
  \brief Sets the starting andending offsets of a stack part.
  \param item the FileItem in the stack that has an updated DynPath (eg. bluray://)
  */
  void SetStackPartOffsets(const CFileItem& item,
                           const std::chrono::milliseconds startOffset,
                           const std::chrono::milliseconds endOffset);

  int GetKnownStackParts() const { return m_knownStackParts; }
  void IncreaseKnownStackParts() { m_knownStackParts += 1; }

  bool HasDiscParts() const;

  bool WasPlayingDiscStack() const { return m_wasDiscStack; }

  bool IsCurrentPartFinished() const { return m_partFinished; }
  void SetCurrentPartFinished(bool finished) { m_partFinished = finished; }

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
    std::chrono::milliseconds m_stackPartStartTimeMs{0ms};
    std::chrono::milliseconds m_stackTotalTimeMs{0ms};
    int m_stackPartNumber{0};
    std::shared_ptr<CFileItem> m_stackItem;
  };

  using StackMap = std::map<std::string, std::shared_ptr<StackPartInformation>, std::less<>>;
  StackMap m_stackMap;
  std::shared_ptr<StackPartInformation> GetStackPartInformation(const std::string& key);
  std::shared_ptr<StackPartInformation> GetStackPartInformation(const std::string& key) const;

  std::unique_ptr<CFileItemList> m_currentStack{std::make_unique<CFileItemList>()};
  int m_currentStackPosition{0};
  int m_knownStackParts{0};
  bool m_wasDiscStack{false};
  bool m_partFinished{false};
};
