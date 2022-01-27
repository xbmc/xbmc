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
  /*!
   * @brief EDL class constructor
  */
  CEdl();

  /*!
   * @brief Searches and reads all possible EDL sources for a given fileItem
   * until a matching one is found. In case it finds a match, it parses the
   * file using the appropriate parser (e.g. ReadVideoReDo, ReadComskip, etc).
   * @param fileItem the fileItem to look for EDL files @note Kodi will use the dynpath
   * @param fps the frames per second of the playing file
   * @return true if a matching EDL file was found and parsed correctly, false otherwise
   */
  bool ReadEditDecisionLists(const CFileItem& fileItem, float fps);

  /*!
   * @brief Reset the CEdl member variables (clear all entries)
  */
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

  /*!
   * @brief Check if EDL has scene markers
   * @return true if EDL has scene markers, false otherwise
   */
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
  const std::vector<EDL::Edit>& GetRawEditList() const { return m_edits; }

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
   * @param seekTime The seek time (on the original timeline)
   * @param[in,out] edit The edit pointer (or nullptr if seekTime not within an edit)
   * @return true if seekTime is within an edit, false otherwise
  */
  bool InEdit(int seekTime, EDL::Edit* edit = nullptr);

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

  /*!
   * @brief Get the closest scenemarker by providing the given clock time and the search
   * direction (forward vs backwards)
   * @param forward If the search should be performed forward (true) with respect to the provided
   * clock time or backwards (false)
   * @param clock The clock time
   * @param[in,out] sceneMarker The closest scene marker
   * @return true if it could find a next scene marker, false otherwise
  */
  bool GetNextSceneMarker(bool forward, int clock, int* sceneMarker);

  /*!
   * @brief Provided the time in msec, returns a timestring (\sa TIME_FORMAT_HH_MM_SS)
   * return the timestring correspondent to the provided time in msec
  */
  static std::string MillisecondsToTimeString(int msec);

private:
  /*!
   * @brief total cut time (EDL cuts) in ms
  */
  int m_totalCutTime;
  /*!
   * @brief the list of EDL edits
  */
  std::vector<EDL::Edit> m_edits;
  /*!
   * @brief the list of EDL scene markers
  */
  std::vector<int> m_sceneMarkers;
  /*!
   * @brief Last processed EDL edit time (ms)
  */
  int m_lastEditTime;
  /*!
   * @brief Last processed EDL edit action type
  */
  EDL::Action m_lastEditActionType{EDL::EDL_ACTION_NONE};

  /*!
   * @brief Read .edl files (MPlayer format)
   * @param path the path of the media item being played
   * @param fps frames per second of the item being played
   * @return true if the file is correctly parsed and edits added, false otherwise
  */
  bool ReadEdl(const std::string& path, float fps);

  /*!
   * @brief Read edl files (Comskip format)
   * @param path the path of the media item being played
   * @param fps frames per second of the item being played
   * @return true if the file is correctly parsed and edits added, false otherwise
  */
  bool ReadComskip(const std::string& path, float fps);

  /*!
   * @brief Read edl files (VideoReDo format)
   * @param path the path of the media item being played
   * @return true if the file is correctly parsed and edits added, false otherwise
  */
  bool ReadVideoReDo(const std::string& path);

  /*!
   * @brief Read edl files (SnapStream BeyondTV format)
   * @param path the path of the media item being played
   * @return true if the file is correctly parsed and edits added, false otherwise
  */
  bool ReadBeyondTV(const std::string& path);

  /*!
   * @brief Read edl edits provided by a PVR backend for a given fileitem
   * @param fileItem the item being played
   * @return true if the file has EDL edits provided by PVR, false otherwise
  */
  bool ReadPvr(const CFileItem& fileItem);

  /*!
   * @brief Adds an edit to the list of EDL edits
   * @note Edits are stored in ascending order
   * @param newEdit the edit to add
   * @return true if the operation succeeds, false otherwise
  */
  bool AddEdit(const EDL::Edit& newEdit);

  /*!
   * @brief Adds a scene marker to the EDL list
   * @param sceneMarker the scene marker
   * @return true if the operation succeeds, false otherwise
  */
  bool AddSceneMarker(int sceneMarker);

  /*!
   * @brief Merge all short commbreaks, depending on EDL advanced settings
  */
  void MergeShortCommBreaks();

  /*!
   * @brief Adds scene markers at the start and end of some edits
   * (currently only for commercial breaks)
  */
  void AddSceneMarkersAtStartAndEndOfEdits();
};
