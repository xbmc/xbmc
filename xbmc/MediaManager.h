#pragma once

#include "Settings.h" // for VECSOURCES

class CNetworkLocation
{
public:
  CNetworkLocation() { id = 0; };
  int id;
  CStdString path;
};

class CMediaManager
{
public:
  CMediaManager();

  bool LoadSources();
  bool SaveSources();

  void GetLocalDrives(VECSOURCES &localDrives, bool includeQ = true);
  void GetNetworkLocations(VECSOURCES &locations);

  bool AddNetworkLocation(const CStdString &path);
  bool HasLocation(const CStdString& path);
  bool RemoveLocation(const CStdString& path);
  bool SetLocationPath(const CStdString& oldPath, const CStdString& newPath);
protected:
  std::vector<CNetworkLocation> m_locations;
};

extern class CMediaManager g_mediaManager;

