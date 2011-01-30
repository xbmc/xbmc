#pragma once
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

/*
 * for DESCRIPTION see 'TVRecordInfoTag.cpp'
 */

#include "video/VideoInfoTag.h"
#include "threads/Thread.h"
#include "DateTime.h"
#include "settings/VideoSettings.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVRRecordingInfoTag : public CVideoInfoTag
{
private:
  int           m_clientID;           /// ID of the backend
  int           m_clientIndex;        /// Index number of the reecording on the client, given by the backend, -1 for unknown
  CStdString    m_strChannel;         /// Channel name where recording from
  CDateTime     m_recordingTime;      /// Recording start time
  CDateTimeSpan m_duration;           /// Duration
  int           m_Priority;
  int           m_Lifetime;
  CStdString    m_strStreamURL;       ///> Stream URL if empty use pvr client
  CStdString    m_strDirectory;       ///> Directory of this recording on the client
  CStdString    m_strFileNameAndPath; ///> Filename for PVRManager to open and read stream

  void DisplayError(PVR_ERROR err) const;

public:
  CPVRRecordingInfoTag();
  bool operator ==(const CPVRRecordingInfoTag& right) const;
  bool operator !=(const CPVRRecordingInfoTag& right) const;
  void Reset(void);

  const CStdString &Title(void) const { return m_strTitle; }
  void SetTitle(const CStdString &Title) { m_strTitle = Title; }
  const CStdString &Directory(void) const { return m_strDirectory; }
  void SetDirectory(const CStdString &path) { m_strDirectory = path; }
  const CStdString &PlotOutline(void) const { return m_strPlotOutline; }
  void SetPlotOutline(const CStdString &PlotOutline) { m_strPlotOutline = PlotOutline; }
  const CStdString &Plot(void) const { return m_strPlot; }
  void SetPlot(const CStdString &Plot) { m_strPlot = Plot; }
  const CStdString &ChannelName(void) const { return m_strChannel; }
  void SetChannelName(const CStdString &name) { m_strChannel = name; }
  const CDateTime &RecordingTime(void) const { return m_recordingTime; }
  void SetRecordingTime(const CDateTime &time) { m_recordingTime = time; }
  int GetDuration() const;
  void SetDuration(const CDateTimeSpan &duration) { m_duration = duration; }
  int Lifetime(void) const { return m_Lifetime; }
  void SetLifetime(int Lifetime) { m_Lifetime = Lifetime; }
  int Priority(void) const { return m_Priority; }
  void SetPriority(int Priority) { m_Priority = Priority; }
  const CStdString &Path(void) const { return m_strFileNameAndPath; }
  void SetPath(const CStdString &path) { m_strFileNameAndPath = path; }

  long ClientID(void) const { return m_clientID; }
  void SetClientID(int ClientId) { m_clientID = ClientId; }
  long ClientIndex(void) const { return m_clientIndex; }
  void SetClientIndex(int ClientIndex) { m_clientIndex = ClientIndex; }
  const CStdString StreamURL(void) const { return m_strStreamURL; }
  void SetStreamURL(const CStdString &stream) { m_strStreamURL = stream; }

  bool Delete(void) const;
  bool Rename(const CStdString &newName) const;

};


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

extern CPVRRecordings PVRRecordings;
