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

class CFileItem;

class CEdl
{
public:
  CEdl();

  typedef enum
  {
    CUT = 0,
    MUTE = 1,
    // SCENE = 2,
    COMM_BREAK = 3
  } Action;

  struct Cut
  {
    int start; // ms
    int end;   // ms
    Action action;
  };

  bool ReadEditDecisionLists(const CFileItem& fileItem, const float fFramesPerSecond);
  void Clear();

  bool HasCut() const;
  bool HasSceneMarker() const;
  std::string GetInfo() const;
  int GetTotalCutTime() const;
  int RemoveCutTime(int iSeek) const;
  double RestoreCutTime(double dClock) const;

  bool InCut(int iSeek, Cut *pCut = NULL);
  bool GetNearestCut(bool bPlus, const int iSeek, Cut *pCut) const;

  int GetLastCutTime() const;
  void SetLastCutTime(const int iCutTime);

  bool GetNextSceneMarker(bool bPlus, const int iClock, int *iSceneMarker);

  static std::string MillisecondsToTimeString(const int iMilliseconds);

private:
  int m_iTotalCutTime; // ms
  std::vector<Cut> m_vecCuts;
  std::vector<int> m_vecSceneMarkers;
  int m_lastCutTime;

  bool ReadEdl(const std::string& strMovie, const float fFramesPerSecond);
  bool ReadComskip(const std::string& strMovie, const float fFramesPerSecond);
  bool ReadVideoReDo(const std::string& strMovie);
  bool ReadBeyondTV(const std::string& strMovie);
  bool ReadPvr(const CFileItem& fileItem);

  bool AddCut(Cut& NewCut);
  bool AddSceneMarker(const int sceneMarker);

  void MergeShortCommBreaks();
};
