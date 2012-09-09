#pragma once

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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


#include <vector>
#include <stdint.h>
#include "utils/StdString.h"

class CEdl
{
public:
  CEdl();
  virtual ~CEdl(void);

  typedef enum
  {
    CUT = 0,
    MUTE = 1,
    // SCENE = 2,
    COMM_BREAK = 3
  } Action;

  struct Cut
  {
    int64_t start; // ms
    int64_t end;   // ms
    Action action;
  };

  bool ReadEditDecisionLists(const CStdString& strMovie, const float fFramesPerSecond, const int iHeight);
  void Clear();

  bool HasCut();
  bool HasSceneMarker();
  CStdString GetInfo();
  int64_t GetTotalCutTime();
  int64_t RemoveCutTime(int64_t iSeek);
  int64_t RestoreCutTime(int64_t iClock);

  bool InCut(int64_t iSeek, Cut *pCut = NULL);

  bool GetNextSceneMarker(bool bPlus, const int64_t iClock, int64_t *iSceneMarker);

  static CStdString GetMPlayerEdl();

  static CStdString MillisecondsToTimeString(const int64_t iMilliseconds);

protected:
private:
  int64_t m_iTotalCutTime; // ms
  std::vector<Cut> m_vecCuts;
  std::vector<int64_t> m_vecSceneMarkers;

  bool ReadEdl(const CStdString& strMovie, const float fFramesPerSecond);
  bool ReadComskip(const CStdString& strMovie, const float fFramesPerSecond);
  bool ReadVideoReDo(const CStdString& strMovie);
  bool ReadBeyondTV(const CStdString& strMovie);
  bool ReadMythCommBreakList(const CStdString& strMovie, const float fFramesPerSecond);
  bool ReadMythCutList(const CStdString& strMovie, const float fFramesPerSecond);

  bool AddCut(Cut& NewCut);
  bool AddSceneMarker(const int64_t sceneMarker);

  bool WriteMPlayerEdl();

  void MergeShortCommBreaks();
};
