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

#include "DateTime.h"
#include "FileItem.h"

class CTVChannel
{
public:
  CTVChannel(DWORD clientID, long idBouquet, long idChannel, int number, CStdString name, 
             CStdString callsign, CStdString iconPath);

  const CFileItemList* GetItems() { return m_programmes; };
  void SetItems(CFileItemList* items) { m_programmes = items; };

  const long  GetChannelID() { return m_idChannel; };
  void  SetChannelID(long id) { m_idChannel = id; };
  const int Number()    { return m_number; };
  const long GetBouquetID() { return m_idBouquet; };
  const long GetClientID() { return m_clientID; };
  const CStdString Name() { return m_name; };
  const CStdString Callsign() { return m_callsign; };
  const CStdString IconPath() { return m_iconPath; };

private:
  DWORD m_clientID;
  int m_number;
  long m_idBouquet;
  long m_idChannel;
  CStdString m_name;
  CStdString m_callsign;
  CStdString m_iconPath;

  CFileItemList* m_programmes;
};

typedef std::vector< CTVChannel* > EPGData;

class CEPG
{
public:
  CEPG(DWORD sourceID, CDateTime start, CDateTime end);
  CEPG(const CEPG& rhs);

  void AddChannel(DWORD sourceID, CTVChannel channel);

  CDateTime m_start;
  CDateTime m_end;

  const EPGData* GetGrid() { return m_grid; };
  const CDateTime GetStart() { return m_start; };
  const CDateTime GetEnd() { return m_end; };

  bool Save();
  bool Sort();

private:
  EPGData* m_grid;
};
