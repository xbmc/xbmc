/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>

namespace EDL
{
struct Edit;
}

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

  bool HasSceneMarker() const;
  std::string GetInfo() const;
  int GetTotalCutTime() const;
  int RemoveCutTime(int iSeek) const;
  double RestoreCutTime(double dClock) const;

  /*!
   * @brief Get the EDL edit list.
   * @return The EDL edits or an empty vector if no edits exist.
  */
  const std::vector<EDL::Edit>& GetEditList() const { return m_vecEdits; }

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
   * @brief Get the nearest edit provided a given seek time
   * @param forward if searching should be done forward (true) or backwards (false)
   * @param seekTime the seek time
   * @param[in,out] nearestEdit a pointer to the nearest edit
   * @return true if the nearest EDL edit was found, false otherwise
  */
  bool GetNearestEdit(bool forward, int seekTime, EDL::Edit* nearestEdit) const;

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

  // FIXME: remove const modifier for iClock as it makes no sense as it means nothing
  // for the reader of the interface, but limits the implementation
  // to not modify the parameter on stack
  bool GetNextSceneMarker(bool bPlus, const int iClock, int *iSceneMarker);

  // FIXME: remove const modifier as it makes no sense as it means nothing
  // for the reader of the interface, but limits the implementation
  // to not modify the parameter on stack
  static std::string MillisecondsToTimeString(const int iMilliseconds);

private:
  int m_iTotalCutTime; // ms
  std::vector<EDL::Edit> m_vecEdits;
  std::vector<int> m_vecSceneMarkers;
  int m_lastEditTime;

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
};
