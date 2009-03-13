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
  void GetRemovableDrives(VECSOURCES &removableDrives);
  void GetNetworkLocations(VECSOURCES &locations);

  bool AddNetworkLocation(const CStdString &path);
  bool HasLocation(const CStdString& path) const;
  bool RemoveLocation(const CStdString& path);
  bool SetLocationPath(const CStdString& oldPath, const CStdString& newPath);
protected:
  std::vector<CNetworkLocation> m_locations;
};

extern class CMediaManager g_mediaManager;

