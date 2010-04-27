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

#include "Database.h"
#include "addons/Addon.h"
#include "addons/AddonManager.h"
#include "StdString.h"
#include "FileItem.h"

class CAddonDatabase : public CDatabase
{
public:
  CAddonDatabase();
  virtual ~CAddonDatabase();
  virtual bool Open();

  int AddAddon(const ADDON::AddonPtr& item, int idRepo);
  bool GetAddon(const CStdString& addonID, ADDON::AddonPtr& addon);
  bool GetAddons(ADDON::VECADDONS& addons);
  bool GetAddon(int id, ADDON::AddonPtr& addon);
  int AddRepository(const CStdString& id, const ADDON::VECADDONS& addons, const CStdString& checksum);
  void DeleteRepository(const CStdString& id);
  void DeleteRepository(int id);
  int GetRepoChecksum(const CStdString& id, CStdString& checksum);
  bool GetRepository(const CStdString& id, ADDON::VECADDONS& addons);
  bool GetRepository(int id, ADDON::VECADDONS& addons);
  bool SetRepoTimestamp(const CStdString& id, const CStdString& timestamp);
  int GetRepoTimestamp(const CStdString& id, CStdString& timestamp);
  bool Search(const CStdString& search, ADDON::VECADDONS& items);
  bool SearchTitle(const CStdString& strSearch, ADDON::VECADDONS& items);
  static void SetPropertiesFromAddon(const ADDON::AddonPtr& addon, CFileItemPtr& item); 
protected:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);
  virtual int GetMinVersion() const { return 3; }
  const char *GetDefaultDBName() const { return "Addons"; }
};

