#ifndef CEDL_H
#define CEDL_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "StdString.h"
#include <vector>

class CEdl
{
public:
  CEdl();
  virtual ~CEdl(void);

  typedef enum 
  {
    CUT = 0,
    MUTE = 1,
    SCENE = 2
  } Action;  

  struct Cut
  {
    __int64 CutStart;
    __int64 CutEnd;
    Action CutAction;
  };

  bool ReadnCacheAny(const CStdString& strMovie);
  bool ReadEdl();
  bool ReadComskip();
  bool ReadVideoRedo();
  bool ReadBeyondTV();
  void Reset();
  
  bool AddCutpoint(const Cut& NewCut);
  bool AddScene(const Cut& NewCut);

  void SetMovie(const CStdString& strMovie);
  bool CacheEdl();
  CStdString GetCachedEdl();

  bool HaveCutpoints();
  bool HaveScenes();
  char GetEdlStatus();
  __int64 TotalCutTime();
  __int64 RemoveCutTime(__int64 iTime);
  __int64 RestoreCutTime(__int64 iTime);

  bool IsCached();
  bool InCutpoint(__int64 iAbsSeek, Cut *pCurCut = NULL);

  bool SeekScene(bool bPlus,__int64 *iScenemarker);

protected:
private:
  CStdString m_strCachedEdl;
  CStdString m_strMovie;
  CStdString m_strEdlFilename;
  bool m_bCutpoints;
  bool m_bCached;
  bool m_bScenes;
  __int64 m_iTotalCutTime; // msec
  char m_szBuffer[1024]; // Buffer for file reading
  std::vector<Cut> m_vecCutlist;
  std::vector<__int64> m_vecScenelist;
};

#endif // CEDL_H
