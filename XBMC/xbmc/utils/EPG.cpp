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

#include "EPG.h"

CTVChannel::CTVChannel(DWORD clientID, long idBouquet, long idChannel, int number, CStdString name, CStdString callsign, CStdString iconPath)
{
  m_clientID = clientID;
  m_number = number;
  m_name = name;
  m_callsign = callsign;
  m_iconPath = iconPath;
  m_idBouquet = idBouquet; // can only be set by TVDatabase
  m_idChannel = idChannel; // can only be set by TVDatabase
  m_programmes = NULL;
}
