#pragma once

#include "Settings.h" // for VECSHARES

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

  void GetLocalDrives(VECSHARES &localDrives);
  void GetNetworkLocations(VECSHARES &locations);

  bool AddNetworkLocation(const CStdString &path);
protected:
  vector<CNetworkLocation> m_locations;
};

extern class CMediaManager g_mediaManager;