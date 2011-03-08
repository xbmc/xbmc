#pragma once
/*
 *      Copyright (C) 2005-2011 Team XBMC
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

class CPVRRecordings : public std::vector<CPVRRecording *>
                     , private CThread
{
private:
  CCriticalSection  m_critSection;
  virtual void Process();
  virtual void UpdateFromClients(void);

public:
  CPVRRecordings(void) {};
  virtual ~CPVRRecordings(void) { Clear(); };

  bool Load() { return Update(); }
  void Unload();
  void Clear();
  void UpdateEntry(const CPVRRecording &tag);
  bool Update(bool bAsync = false);

  int GetNumRecordings();
  int GetRecordings(CFileItemList* results);
  bool DeleteRecording(const CFileItem &item);
  bool RenameRecording(CFileItem &item, CStdString &strNewName);

  bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  CPVRRecording *GetByPath(CStdString &path);
};

