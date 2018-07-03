/*
*      Copyright (C) 2017 Team Kodi
*      http://kodi.tv
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this Program; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#include "FileItem.h"
#include "Util.h"

class CApplicationStackHelper
{
public:
  CApplicationStackHelper(void);
  ~CApplicationStackHelper(void);

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
  */
  int InitializeStackStartPartAndOffset(const CFileItem& item);

  /*!
  \brief returns the current part number
  */
  int GetCurrentPartNumber() const { return m_currentStackPosition; }

  /*!
  \brief Returns true if Application is currently playing an ISO stack
  */
  bool IsPlayingISOStack() const { return m_currentStack->Size() > 0 && m_currentStackIsDiscImageStack; }

  /*!
  \brief Returns true if Application is currently playing a Regular (non-ISO) stack
  */
  bool IsPlayingRegularStack() const { return m_currentStack->Size() > 0 && !m_currentStackIsDiscImageStack; }

  /*!
  \brief Returns a FileItem part of a (non-ISO) stack playback
  \param partNumber the requested part number in the stack
  */
  CFileItem& GetStackPartFileItem(int partNumber) const { return  *(*m_currentStack)[partNumber]; }

  /*!
  \brief returns true if there is a next part available
  */
  bool HasNextStackPartFileItem() const { return m_currentStackPosition < m_currentStack->Size() - 1; }

  /*!
  \brief sets the next stack part as the current and returns a reference to it
  */
  CFileItem& SetNextStackPartCurrentFileItem() { return  GetStackPartFileItem(++m_currentStackPosition); }

  /*!
  \brief sets a given stack part as the current and returns a reference to it
  \param partNumber the number of the part that needs to become the current one
  */
  CFileItem& SetStackPartCurrentFileItem(int partNumber) { return  GetStackPartFileItem(m_currentStackPosition = partNumber); }

  /*!
  \brief Returns the FileItem currently playing back as part of a (non-ISO) stack playback
  */
  CFileItem& GetCurrentStackPartFileItem() const { return GetStackPartFileItem(m_currentStackPosition); }

  /*!
  \brief Returns the end time of a FileItem part of a (non-ISO) stack playback
  \param partNumber the requested part number in the stack
  */
  uint64_t GetStackPartEndTimeMs(int partNumber) const { return GetStackPartFileItem(partNumber).m_lEndOffset; }

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
  uint64_t GetStackTotalTimeMs() const { return GetStackPartEndTimeMs(m_currentStack->Size() - 1); }

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
  CFileItemPtr GetRegisteredStack(const CFileItem& item);

  /*!
  \brief Returns true if there is a registered stack for the given CFileItem part.
  \param item the reference to the item that is part of a stack
  */
  bool HasRegisteredStack(const CFileItem& item);

  /*!
  \brief Stores a smart pointer to the stack CFileItem in the item-stack map.
  \param item the reference to the item that is part of a stack
  \param stackItem the smart pointer to the stack CFileItem
  */
  void SetRegisteredStack(const CFileItem& item, CFileItemPtr stackItem);

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
  uint64_t GetRegisteredStackPartStartTimeMs(const CFileItem& item);

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
  uint64_t GetRegisteredStackTotalTimeMs(const CFileItem& item);

  /*!
  \brief Stores the stack's total time associated to the part in the item-stack map.
  \param item the reference to the item that is part of a stack
  \param totalTime the total time of the stack
  */
  void SetRegisteredStackTotalTimeMs(const CFileItem& item, uint64_t totalTimeMs);

  CCriticalSection m_critSection;

protected:

  class StackPartInformation
  {
  public:
    StackPartInformation()
    {
      m_lStackPartNumber = 0;
      m_lStackPartStartTimeMs = 0;
      m_lStackTotalTimeMs = 0;
      m_pStack = nullptr;
    };
    uint64_t m_lStackPartStartTimeMs;
    uint64_t m_lStackTotalTimeMs;
    int m_lStackPartNumber;
    CFileItemPtr m_pStack;
  };

  typedef std::shared_ptr<StackPartInformation> StackPartInformationPtr;
  typedef std::map<std::string, StackPartInformationPtr> Stackmap;
  Stackmap m_stackmap;
  StackPartInformationPtr GetStackPartInformation(std::string key);

  std::unique_ptr<CFileItemList> m_currentStack;
  int m_currentStackPosition = 0;
  bool m_currentStackIsDiscImageStack = false;
};
