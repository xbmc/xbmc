#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include "PVRRecording.h"
#include "threads/Thread.h"
#include "DateTime.h"

class CPVRRecordings : public std::vector<CPVRRecordingInfoTag>
                     , private CThread
{
private:
  CCriticalSection  m_critSection;
  virtual void Process();

public:
  CPVRRecordings(void);
  bool Load() { return Update(true); }
  void Unload();
  bool Update(bool Wait = false);
  int GetNumRecordings();
  int GetRecordings(CFileItemList* results);
  static bool DeleteRecording(const CFileItem &item);
  static bool RenameRecording(CFileItem &item, CStdString &newname);
  bool RemoveRecording(const CFileItem &item);
  bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  CPVRRecordingInfoTag *GetByPath(CStdString &path);
  void Clear();
};

