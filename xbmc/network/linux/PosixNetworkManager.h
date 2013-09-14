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

#include "xbmc/network/INetworkManager.h"

class CPosixNetworkManager : public INetworkManager
{
public:
  CPosixNetworkManager();
  virtual ~CPosixNetworkManager();

  virtual bool CanManageConnections();

  virtual ConnectionList GetConnections();

  virtual bool PumpNetworkEvents(INetworkEventsCallback *callback);
private:
  void RestoreSavedConnection();
  void RestoreSystemConnection();

  void FindNetworkInterfaces();
  bool FindWifiConnections(const char *interfaceName);

  int            m_socket;
  ConnectionList m_connections;
  bool           m_post_failed;
};
