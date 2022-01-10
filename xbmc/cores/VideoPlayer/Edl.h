/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/EdlEdit.h"

#include <string>
#include <vector>

class CFileItem;

class CEdl
{
public:
  CEdl();

  // FIXME: remove const modifier for fFramesPerSecond as it makes no sense as it means nothing
  // for the reader of the interface, but limits the implementation
  // to not modify the parameter on stack
  bool ReadEditDecisionLists(const CFileItem& fileItem, const float fFramesPerSecond);
  void Clear();

  /*!
   * @brief Check if there are any parsed edits in EDL for the current item
   * @return true if EDL has edits, false otherwise
   */
  bool HasEdits() const;

  /*!
   * @brief Check if the edit list has EDL cuts (edits with action CUT)
   * @return true if EDL has cuts, false otherwise
   */
  bool HasCuts() const;

  bool HasSceneMarker() const;

  /*!
   * @brief Get the total cut time removed from the original item
   * because of EDL cuts
   * @return the total cut time
  */
  int GetTotalCutTime() const;

  /*!
   * @brief Providing a given seek time, return the actual time without
   * considering cut ranges removed from the file
   * @note VideoPlayer always displays/returns the playback time considering
   * cut blocks are not part of the playable file
   * @param seek the desired seek time
   * @return the seek time without considering EDL cut blocks
  */
  int GetTimeWithoutCuts(int seek) const;

  /*!
   * @brief Provided a given seek time, return the time after correction with
   * the addition of the already surpassed EDL cut ranges
   * @note VideoPlayer uses it to restore the correct time after seek since cut blocks
   * are not part of the playable file
   * @param seek the desired seek time
   * @return the seek time after applying the cut blocks already surpassed by the
   * provided seek time
  */
  double GetTimeAfterRestoringCuts(double seek) const;

  /*!
   * @brief Get the raw EDL edit list.
   * @return The EDL edits or an empty vector if no edits exist. Edits are
   * provided with respect to the original media item timeline.
  */
  const std::vector<EDL::Edit>& GetRawEditList() const { return m_vecEdits; }

  /*!
   * @brief Get the EDL edit list.
   * @return The EDL edits or an empty vector if no edits exist. Edits are
   * provided with respect to the actual timeline, i.e. considering EDL cuts
   * are not part of the media item.
  */
  const std::vector<EDL::Edit> GetEditList() const;

  /*!
   * @brief Get the list of EDL cut markers.
   * @return The list of EDL cut markers or an empty vector if no EDL cuts exist.
   * The returned values are accurate with respect to cut durations. I.e. if the file
   * has multiple cuts, the positions of subsquent cuts are automatically corrected by
   * substracting the previous cut durations.
  */
  const std::vector<int64_t> GetCutMarkers() const;

  /*!
   * @brief Get the list of EDL scene markers.
   * @return The list of EDL scene markers or an empty vector if no EDL scene exist.
   * The returned values are accurate with respect to cut durations. I.e. if the file
   * has multiple cuts, the positions of scene markers are automatically corrected by
   * substracting the surpassed cut durations until the scene marker point.
  */
  const std::vector<int64_t> GetSceneMarkers() const;

  /*!
   * @brief Check if for the provided seek time is contained within an EDL
   * edit and fill pEdit with the respective edit struct.
   * @note seek time refers to the time in the original file timeline (i.e. without
   * considering cut blocks)
   * @param iSeek The seek time (on the original timeline)
   * @param[in,out] pEdit The edit pointer (or nullptr if iSeek not within an edit)
   * @return true if iSeek is within an edit, false otherwise
  */
  bool InEdit(int iSeek, EDL::Edit* pEdit = nullptr);

  /*!
   * @brief Get the last processed edit time (set during playback when a given
   * edit is surpassed)
   * @return The last processed edit time (ms) or -1 if not any
  */
  int GetLastEditTime() const;

  /*!
   * @brief Set the last processed edit time (set during playback when a given
   * edit is surpassed)
   * @param editTime The last processed EDL edit time (ms)
  */
  void SetLastEditTime(int editTime);

  /*!
   * @brief Reset the last recorded edit time (-1)
  */
  void ResetLastEditTime();

  /*!
   * @brief Set the last processed edit action type
   * @param action The action type (e.g. COMM_BREAK)
  */
  void SetLastEditActionType(EDL::Action action);

  /*!
   * @brief Get the last processed edit action type (set during playback when a given
   * edit is surpassed)
   * @return The last processed edit action type or -1 if not any
  */
  EDL::Action GetLastEditActionType() const;

  // FIXME: remove const modifier for iClock as it makes no sense as it means nothing
  // for the reader of the interface, but limits the implementation
  // to not modify the parameter on stack
  bool GetNextSceneMarker(bool bPlus, const int iClock, int *iSceneMarker);

  // FIXME: remove const modifier as it makes no sense as it means nothing
  // for the reader of the interface, but limits the implementation
  // to not modify the parameter on stack
  static std::string MillisecondsToTimeString(const int iMilliseconds);

private:
  // total cut time (edl cuts) in ms
  int m_totalCutTime;
  std::vector<EDL::Edit> m_vecEdits;
  std::vector<int> m_vecSceneMarkers;

  /*!
   * @brief Last processed EDL edit time (ms)
  */
  int m_lastEditTime;

  /*!
   * @brief Last processed EDL edit action type
  */
  EDL::Action m_lastEditActionType{EDL::EDL_ACTION_NONE};

  // FIXME: remove const modifier for fFramesPerSecond as it makes no sense as it means nothing
  // for the reader of the interface, but limits the implementation
  // to not modify the parameter on stack
  bool ReadEdl(const std::string& strMovie, const float fFramesPerSecond);
  // FIXME: remove const modifier for fFramesPerSecond as it makes no sense as it means nothing
  // for the reader of the interface, but limits the implementation
  // to not modify the parameter on stack
  bool ReadComskip(const std::string& strMovie, const float fFramesPerSecond);
  // FIXME: remove const modifier for strMovie as it makes no sense as it means nothing
  // for the reader of the interface, but limits the implementation
  // to not modify the parameter on stack
  bool ReadVideoReDo(const std::string& strMovie);
  // FIXME: remove const modifier for strMovie as it makes no sense as it means nothing
  // for the reader of the interface, but limits the implementation
  // to not modify the parameter on stack
  bool ReadBeyondTV(const std::string& strMovie);
  bool ReadPvr(const CFileItem& fileItem);

  /*!
   * @brief Adds an edit to the list of EDL edits
   * @param newEdit the edit to add
   * @return true if the operation succeeds, false otherwise
  */
  bool AddEdit(const EDL::Edit& newEdit);

  // FIXME: remove const modifier for strMovie as it makes no sense as it means nothing
  // for the reader of the interface, but limits the implementation
  // to not modify the parameter on stack
  bool AddSceneMarker(const int sceneMarker);

  void MergeShortCommBreaks();

  /*!
   * @brief Adds scene markers at the start and end of some edits
   * (currently only for commercial breaks)
  */
  void AddSceneMarkersAtStartAndEndOfEdits();
};
