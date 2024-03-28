/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "application/IApplicationComponent.h"
#include "threads/CriticalSection.h"

#include <map>
#include <memory>
#include <optional>
#include <string>

class CFileItem;
class CFileItemList;

class CApplicationStackHelper : public IApplicationComponent
{
public:
  CApplicationStackHelper(void);
  ~CApplicationStackHelper() = default;

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
  \brief Returns true if Application is currently playing an ISO stack
  */
  bool IsPlayingISOStack() const;

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
    return GetStackPartFileItem(++m_currentStackPosition);
  }

  /*!
  \brief sets a given stack part as the current and returns a reference to it
  \param partNumber the number of the part that needs to become the current one
  */
  const CFileItem& SetStackPartCurrentFileItem(int partNumber)
  {
    return GetStackPartFileItem(m_currentStackPosition = partNumber);
  }

  /*!
  \brief Returns the FileItem currently playing back as part of a (non-ISO) stack playback
  */
  const CFileItem& GetCurrentStackPartFileItem() const
  {
    return GetStackPartFileItem(m_currentStackPosition);
  }

  /*!
  \brief Returns the end time of a FileItem part of a (non-ISO) stack playback
  \param partNumber the requested part number in the stack
  */
  uint64_t GetStackPartEndTimeMs(int partNumber) const;

  /*!
  \brief Returns the start time of a FileItem part of a (non-ISO) stack playback
  \param partNumber the requested part number in the stack
  */
  uint64_t GetStackPartStartTimeMs(int partNumber) const { return (partNumber > 0) ? GetStackPartEndTimeMs(partNumber - 1) : 0; }

  /*!
  \brief Returns the start time of the current FileItem part of a (non-ISO) stack playback
  */
  uint64_t GetCurrentStackPartStartTimeMs() const { return GetStackPartStartTimeMs(m_currentStackPosition); }

  /*!
  \brief Returns the total time of a (non-ISO) stack playback
  */
  uint64_t GetStackTotalTimeMs() const;

  /*!
  \brief Returns the stack part number corresponding to the given timestamp in a (non-ISO) stack playback
  \param msecs the requested timestamp in the stack (in milliseconds)
  */
  int GetStackPartNumberAtTimeMs(uint64_t msecs);

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
  uint64_t GetRegisteredStackPartStartTimeMs(const CFileItem& item) const;

  /*!
  \brief Stores the part start time in the item-stack map.
  \param item the reference to the item that is part of a stack
  \param startTime the start time of the part in other parameter
  */
  void SetRegisteredStackPartStartTimeMs(const CFileItem& item, uint64_t startTimeMs);

  /*!
  \brief Returns the total time of the stack associated to the part in the parameter
  \param item the reference to the item that is part of a stack
  */
  uint64_t GetRegisteredStackTotalTimeMs(const CFileItem& item) const;

  /*!
  \brief Stores the stack's total time associated to the part in the item-stack map.
  \param item the reference to the item that is part of a stack
  \param totalTime the total time of the stack
  */
  void SetRegisteredStackTotalTimeMs(const CFileItem& item, uint64_t totalTimeMs);

  CCriticalSection m_critSection;

protected:
  /*!
  \brief Returns a FileItem part of a (non-ISO) stack playback
  \param partNumber the requested part number in the stack
  */
  CFileItem& GetStackPartFileItem(int partNumber);
  const CFileItem& GetStackPartFileItem(int partNumber) const;

  class StackPartInformation
  {
  public:
    StackPartInformation()
    {
      m_lStackPartNumber = 0;
      m_lStackPartStartTimeMs = 0;
      m_lStackTotalTimeMs = 0;
    };
    uint64_t m_lStackPartStartTimeMs;
    uint64_t m_lStackTotalTimeMs;
    int m_lStackPartNumber;
    std::shared_ptr<CFileItem> m_pStack;
  };

  typedef std::shared_ptr<StackPartInformation> StackPartInformationPtr;
  typedef std::map<std::string, StackPartInformationPtr> Stackmap;
  Stackmap m_stackmap;
  StackPartInformationPtr GetStackPartInformation(const std::string& key);
  StackPartInformationPtr GetStackPartInformation(const std::string& key) const;

  std::unique_ptr<CFileItemList> m_currentStack;
  int m_currentStackPosition = 0;
  bool m_currentStackIsDiscImageStack = false;
};
