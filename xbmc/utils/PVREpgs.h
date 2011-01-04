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

#include "DateTime.h"
#include "utils/Thread.h"
#include "utils/PVREpg.h"
#include "utils/PVREpgInfoTag.h"
#include "utils/PVRChannels.h"
#include "../addons/include/xbmc_pvr_types.h"

class CPVRChannel;
struct PVREpgSearchFilter;

class CPVREpgs : public std::vector<CPVREpg*>
{
  friend class CPVREpg;

private:
  CCriticalSection m_critSection;
  bool  m_bInihibitUpdate;

public:
  CPVREpgs(void);

  CPVREpg *AddEPG(long ChannelID);
  const CPVREpg *GetEPG(long ChannelID) const;
  const CPVREpg *GetEPG(const CPVRChannel *Channel, bool AddIfMissing = false) const;
  void Add(CPVREpg *entry);

  void Cleanup(void);
  bool ClearAll(void);
  bool ClearChannel(long ChannelID);
  void Load();
  void Unload();
  void Update(bool Scan = false);
  void InihibitUpdate(bool yesNo) { m_bInihibitUpdate = yesNo; }
  int GetEPGSearch(CFileItemList* results, const PVREpgSearchFilter &filter);
  int GetEPGAll(CFileItemList* results, bool radio = false);
  int GetEPGChannel(unsigned int number, CFileItemList* results, bool radio = false);
  int GetEPGNow(CFileItemList* results, bool radio = false);
  int GetEPGNext(CFileItemList* results, bool radio = false);
  CDateTime GetFirstEPGDate(bool radio = false);
  CDateTime GetLastEPGDate(bool radio = false);
  void SetVariableData(CFileItemList* results);
  void AssignChangedChannelTags(bool radio = false);
};

extern CPVREpgs PVREpgs;
