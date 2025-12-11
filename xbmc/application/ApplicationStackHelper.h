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
#include <string>

class CPlayerOptions;
class CFileItem;
class CFileItemList;

class CApplicationStackHelper : public IApplicationComponent
{
public:
  void Clear();

  void OnPlayBackStarted();

  /*!
  \brief Initialize stack and times for each part.
  \param item the FileItem object that is the stack
  */
  bool InitializeStack(const CFileItem& item);

  /*!
  \brief Initialize stack times for each part, start & end, total time, and current part number if resume offset is specified.
  \param item the FileItem object that is the stack
  \param options player options to update
  \param restart true if playback is a restart, false otherwise
  */
  void GetStackPartAndOptions(CFileItem& item, CPlayerOptions& options, bool restart);

  /*!
  \brief Updates the stack, fileItem and database stacktimes with new times.
  The stack should have already been updated with the new dynpath.
  \param playedFile The FileItem of the actual file played (updated in InputStream).
  \return true if successful, false otherwise.
  */
  bool UpdateDiscStackAndTimes(const CFileItem& playedFile);

  /*!
  \brief If a disc stack is stopped between parts when the next part has not been determined (ie. playlist not selected),
  then we need to save the bookmark for the next part before exiting playback.
  \param path the stack:// path
  */
  void SetNextPartBookmark(const std::string& path);

  /*!
  \brief returns the current part number
  */
  int GetCurrentPartNumber() const { return m_currentStackPosition; }

  /*!
  \brief returns the total number of parts
  */
  int GetTotalPartNumbers() const { return static_cast<int>(m_stackMap.size()); }

  /*!
  \brief Returns true if Application is currently playing any stack
  */
  bool IsPlayingStack() const;

  /*!
  \brief Returns true if Application is currently playing a disc (ISO/BMDV/VIDEO_TS) stack
  */
  bool IsPlayingDiscStack() const;

  /*!
  \brief Returns true if Application is currently playing a regular (non-disc) stack
  */
  bool IsPlayingRegularStack() const;

  /*!
  \brief Returns true if Application is currently playing a disc stack where all parts up to the current one have been resolved
  */
  bool IsPlayingResolvedDiscStack() const;

  /*!
  \brief Returns true if there is another stack part available
  */
  bool HasNextStackPartFileItem() const;

  /*!
  \brief Returns true if playing the last part of the stack
  */
  bool IsPlayingLastStackPart() const;

  /*!
  \brief Sets the next stack part as the current and returns a reference to it
  */
  CFileItem& SetNextStackPartAsCurrent();

  /*!
  \brief Sets a given stack part as the current and returns a reference to it
  \param partNumber the number of the part that needs to become the current one
  */
  CFileItem& SetStackPartAsCurrent(int partNumber);

  /*!
  \brief Returns the FileItem currently playing back as part of a stack playback
  */
  CFileItem& GetCurrentStackPart() const;

  /*!
  \brief Returns the end time of a FileItem part of a stack playback
  \param partNumber the requested part number in the stack
  */
  std::chrono::milliseconds GetStackPartEndTime(int partNumber) const;

  /*!
  \brief Returns the start time of a FileItem part of a stack playback
  \param partNumber the requested part number in the stack
  */
  std::chrono::milliseconds GetStackPartStartTime(int partNumber) const;

  /*!
  \brief Returns the start time of the current FileItem part of a stack playback
  */
  std::chrono::milliseconds GetCurrentStackPartStartTime() const;

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
  int GetStackPartNumber(const CFileItem& item) const;

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
  void SetStackPartStartTime(const CFileItem& item, std::chrono::milliseconds startTime) const;

  /*!
  \brief Sets the file id of the VideoInfoTag of each part in the stack.
  \param fileId the file id
  */
  void SetStackFileIds(int fileId);

  /*!
  \brief Sets the stream details of the VideoInfoTag of the given part of the stack.
  \param item the reference to the item that is part of a stack
  */
  void SetStackPartStreamDetails(const CFileItem& item);

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
  void SetStackPartPath(const CFileItem& item);

  /*!
  \brief Returns the stack:// path of the stack.
  */
  std::string GetStackDynPath() const;

  /*!
  \brief Returns the stack:// path of the stack prior to the last resolved part being updated.
  */
  std::string GetOldStackDynPath() const;

  /*!
  \brief Sets the total time of the stack in each stack part.
  \param totalTime the total time of the stack (in ms)
  */
  void SetStackTotalTime(std::chrono::milliseconds totalTime);

  /*!
  \brief Sets the starting and ending offsets of a stack part.
  \param item the FileItem in the stack that has an updated DynPath (eg. bluray://)
  \param startOffset the start offset in ms
  \param endOffset the end offset in ms
  */
  void SetStackPartOffsets(const CFileItem& item,
                           const std::chrono::milliseconds startOffset,
                           const std::chrono::milliseconds endOffset) const;

  /*!
  \brief Returns the number of parts in the stack that are currently resolved (ie. a playlist has been selected and path is bluray://)
  */
  int GetKnownStackParts() const { return m_knownStackParts; }

  /*!
  \brief Increases the number of known (resolved) stack parts by one
  */
  void IncreaseKnownStackParts();

  /*!
  \brief Returns true if any part of the stack are disc parts (ISO/BMDV/VIDEO_TS)
  */
  bool HasDiscParts() const;

  /*!
  \brief Returns true if any part of the stack was a disc part (ISO/BMDV/VIDEO_TS)
  \ (prior to being resolved to a playlist bluray:// path)
  */
  bool WasPlayingDiscStack() const { return m_wasDiscStack; }

  /*!
  \brief Returns true if the current part has finished playing
  */
  bool IsCurrentPartFinished() const { return m_partFinished; }

  /*!
  \brief Set the status of the current playing part
  \param finished true if the current part has finished playing, false otherwise
  */
  void SetCurrentPartFinished(bool finished) { m_partFinished = finished; }

  /*!
  \brief Returns true if currently seeking between parts
  */
  bool IsSeekingParts() const { return m_seekingParts; }

  /*!
  \brief Flag if currently seeking between parts
  \param seeking true if currently seeking between parts, false if not
  */
  void SetSeekingParts(bool seeking) { m_seekingParts = seeking; }

private:
  mutable CCriticalSection m_critSection;

  bool ProcessNextPartInBookmark(CFileItem& item, CBookmark& bookmark);

  class StackPartInformation
  {
  public:
    std::shared_ptr<CFileItem> stackItem;
    std::chrono::milliseconds startTime{std::chrono::milliseconds(0)};
    int partNumber{0};
  };

  using StackMap = std::map<std::string, std::shared_ptr<StackPartInformation>, std::less<>>;
  StackMap m_stackMap;
  std::shared_ptr<StackPartInformation> GetOrCreateStackPartInformation(const std::string& key);
  std::shared_ptr<StackPartInformation> GetStackPartInformation(const std::string& key) const;
  std::shared_ptr<StackPartInformation> GetStackPartInformation(const CFileItem& item) const;

  CFileItemList m_originalStackItems;
  std::vector<std::string> m_stackPaths;

  std::chrono::milliseconds m_stackTotalTime{std::chrono::milliseconds(0)};

  int m_currentStackPosition{0};
  int m_knownStackParts{0};
  std::string m_oldStackPath{};
  bool m_wasDiscStack{false};
  bool m_partFinished{false};
  bool m_seekingParts{false};
};
