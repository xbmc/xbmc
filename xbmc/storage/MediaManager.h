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

#include "MediaSource.h" // for VECSOURCES
#include <map>
#include "utils/Job.h"
#include "IStorageProvider.h"
#include "threads/CriticalSection.h"

#ifdef HAS_DVD_DRIVE
using namespace MEDIA_DETECT;
#endif

#define TRAY_OPEN     16
#define TRAY_CLOSED_NO_MEDIA  64
#define TRAY_CLOSED_MEDIA_PRESENT 96

#define DRIVE_OPEN      0 // Open...
#define DRIVE_NOT_READY     1 // Opening.. Closing...
#define DRIVE_READY      2
#define DRIVE_CLOSED_NO_MEDIA   3 // CLOSED...but no media in drive
#define DRIVE_CLOSED_MEDIA_PRESENT  4 // Will be send once when the drive just have closed
#define DRIVE_NONE  5 // system doesn't have an optical drive

class CNetworkLocation
{
public:
  CNetworkLocation() { id = 0; };
  int id;
  CStdString path;
};

class CMediaManager : public IStorageEventsCallback, public IJobCallback
{
public:
  CMediaManager();

  void Initialize();
  void Stop();

  bool LoadSources();
  bool SaveSources();

  void GetLocalDrives(VECSOURCES &localDrives, bool includeQ = true);
  void GetRemovableDrives(VECSOURCES &removableDrives);
  void GetNetworkLocations(VECSOURCES &locations, bool autolocations = true);

  bool AddNetworkLocation(const CStdString &path);
  bool HasLocation(const CStdString& path) const;
  bool RemoveLocation(const CStdString& path);
  bool SetLocationPath(const CStdString& oldPath, const CStdString& newPath);

  void AddAutoSource(const CMediaSource &share, bool bAutorun=false);
  void RemoveAutoSource(const CMediaSource &share);
  bool IsDiscInDrive(const CStdString& devicePath="");
  bool IsAudio(const CStdString& devicePath="");
  bool HasOpticalDrive();
  CStdString TranslateDevicePath(const CStdString& devicePath, bool bReturnAsDevice=false);
  DWORD GetDriveStatus(const CStdString& devicePath="");
#ifdef HAS_DVD_DRIVE
  CCdInfo* GetCdInfo(const CStdString& devicePath="");
  bool RemoveCdInfo(const CStdString& devicePath="");
  CStdString GetDiskLabel(const CStdString& devicePath="");
  CStdString GetDiskUniqueId(const CStdString& devicePath="");
#endif
  CStdString GetDiscPath();
  void SetHasOpticalDrive(bool bstatus);

  bool Eject(CStdString mountpath);
  void EjectTray( const bool bEject=true, const char cDriveLetter='\0' );
  void CloseTray(const char cDriveLetter='\0');
  void ToggleTray(const char cDriveLetter='\0');

  void ProcessEvents();

  std::vector<CStdString> GetDiskUsage();

  virtual void OnStorageAdded(const CStdString &label, const CStdString &path);
  virtual void OnStorageSafelyRemoved(const CStdString &label);
  virtual void OnStorageUnsafelyRemoved(const CStdString &label);

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob *job) { }
protected:
  std::vector<CNetworkLocation> m_locations;

  CCriticalSection m_muAutoSource, m_CritSecStorageProvider;
#ifdef HAS_DVD_DRIVE
  std::map<CStdString,CCdInfo*> m_mapCdInfo;
  bool HashDVD(const CStdString& dvdpath, uint32_t& crc);
#endif
  bool m_bhasoptical;
  CStdString m_strFirstAvailDrive;

private:
  IStorageProvider *m_platformStorage;
};

extern class CMediaManager g_mediaManager;

