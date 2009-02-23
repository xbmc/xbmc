#pragma once

/*
*      Copyright (C) 2005-2009 Team XBMC
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

#include "FileItem.h"

class CTVChannel
{
  
public:
  CTVChannel(long clientID, int number, int clientnumber, CStdString name, CStdString callsign, CStdString iconPath);

  CTVChannel(CTVChannel &oldChannel);
  ~CTVChannel();

  bool GetEPGNowInfo(CTVEPGInfoTag *result);
  bool GetEPGNextInfo(CTVEPGInfoTag *result);

  const CFileItemList* const GetItems() { return m_programmes; };
  const int Number()    { return m_number; };
  const int ClientNumber()    { return m_clientNumber; };
  const long Client() { return m_client; };
  const CStdString Name() { return m_name; };
  const CStdString Callsign() { return m_callsign; };
  const CStdString IconPath() { return m_iconPath; };

protected:
  friend class CEPG;
  void SetChannelItems(CFileItemList* items) { m_programmes = items; };

private:
  long m_client;
  int m_number;
  int m_clientNumber;
  CStdString m_name;
  CStdString m_callsign;
  CStdString m_iconPath;

  CFileItemList* m_programmes;
};

typedef std::vector< CTVChannel* > VECCHANNELS;
