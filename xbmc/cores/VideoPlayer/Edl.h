#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>

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

  bool ReadEditDecisionLists(const std::string& strMovie, const float fFramesPerSecond, const int iHeight);
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
  bool ReadPvr(const std::string& strMovie);

  bool AddCut(Cut& NewCut);
  bool AddSceneMarker(const int sceneMarker);

  void MergeShortCommBreaks();
};
