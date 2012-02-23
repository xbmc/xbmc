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
#include "XBDateTime.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

namespace PVR
{
  class CPVRRecordings : public std::vector<CPVRRecording *>,
                         public Observable
  {
  private:
    CCriticalSection m_critSection;
    bool             m_bIsUpdating;
    CStdString       m_strDirectoryHistory;

    virtual void UpdateFromClients(void);
    virtual CStdString TrimSlashes(const CStdString &strOrig) const;
    virtual const CStdString GetDirectoryFromPath(const CStdString &strPath, const CStdString &strBase) const;
    virtual bool IsDirectoryMember(const CStdString &strDirectory, const CStdString &strEntryDirectory, bool bDirectMember = true) const;
    virtual void GetContents(const CStdString &strDirectory, CFileItemList *results) const;
    virtual void GetSubDirectories(const CStdString &strBase, CFileItemList *results, bool bAutoSkip = true);

  public:
    CPVRRecordings(void);
    virtual ~CPVRRecordings(void) { Clear(); };

    int Load();
    void Unload();
    void Clear();
    void UpdateEntry(const CPVRRecording &tag);
    void UpdateFromClient(const CPVRRecording &tag) { UpdateEntry(tag); }

    /**
     * @brief refresh the recordings list from the clients.
     */
    void Update(void);

    int GetNumRecordings();
    int GetRecordings(CFileItemList* results);
    bool DeleteRecording(const CFileItem &item);
    bool RenameRecording(CFileItem &item, CStdString &strNewName);

    bool GetDirectory(const CStdString& strPath, CFileItemList &items);
    CPVRRecording *GetByPath(const CStdString &path);
  };
}
