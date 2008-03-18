/*
 * XBFileZilla
 * Copyright (c) 2003 MrDoubleYou
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// DebugTestApplication.cpp: implementation of the CXBFileZilla class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XBFileZilla.h"
#include "XBFileZillaImp.h"
#include "XBServer.h"


CXBFileZilla::CXBFileZilla(LPCTSTR Path)
{
  SetConfigurationPath(Path);
}

CXBFileZilla::~CXBFileZilla()
{
  XBFILEZILLA(DestructInstance());
}

bool CXBFileZilla::Start(bool Wait)
{
  return XBFILEZILLA(Start(Wait));
}

bool CXBFileZilla::Stop()
{
  return XBFILEZILLA(Stop());
}


void CXBFileZilla::SetConfigurationPath(LPCTSTR Path)
{
  XBFILEZILLA(SetConfigurationPath(Path));
} 

LPCTSTR CXBFileZilla::GetConfigurationPath()
{
  return XBFILEZILLA(GetConfigurationPath());
} 


XFSTATUS CXBFileZilla::AddNotificationClient(CXFNotificationClient* Client)
{
  return XBFILEZILLA(GetServer())->AddNotificationClient(Client);
}

XFSTATUS CXBFileZilla::RemoveNotificationClient(CXFNotificationClient* Client)
{
  return XBFILEZILLA(GetServer())->RemoveNotificationClient(Client);
}


XFSTATUS CXBFileZilla::AddUser(LPCTSTR Name, CXFUser*& User)
{
  return XBFILEZILLA(AddUser(Name, User));
}

XFSTATUS CXBFileZilla::RemoveUser(LPCTSTR Name)
{
  return XBFILEZILLA(RemoveUser(Name));
}

XFSTATUS CXBFileZilla::GetUser(LPCTSTR Name, CXFUser*& User)
{
  return XBFILEZILLA(GetUser(Name, User));
}

XFSTATUS CXBFileZilla::GetAllUsers(std::vector<CXFUser*>& UserVector)
{
  return XBFILEZILLA(GetAllUsers(UserVector));
}

XFSTATUS CXBFileZilla::CloseConnection(int ConnectionId)
{
  return XBFILEZILLA(GetServer())->CloseConnection(ConnectionId);
}

XFSTATUS CXBFileZilla::GetAllConnections(std::vector<SXFConnection>& ConnectionVector)
{
  return XBFILEZILLA(GetServer())->GetAllConnections(ConnectionVector);
}

XFSTATUS CXBFileZilla::GetConnection(int ConnectionId, SXFConnection& Connection)
{
  return XBFILEZILLA(GetServer())->GetConnection(ConnectionId, Connection);
}

XFSTATUS CXBFileZilla::GetNoConnections()
{
	int intConnCount = 0;
	std::vector<SXFConnection> Connections;

	GetAllConnections(Connections);

	for (std::vector<SXFConnection>::const_iterator ConnIter = Connections.begin(); ConnIter!=Connections.end(); ConnIter++)
	{
		intConnCount ++;
	}

	return(intConnCount);
}

void CXBFileZilla::SetCriticalOperationCallback(CriticalOperationCallback Callback)
{
  XBFILEZILLA(SetCriticalOperationCallback(Callback));
}


