#ifndef __XBSERVER_H
#define __XBSERVER_H

#pragma once

#include "xbfilezilla.h"
#include "Server.h"
#include "AdminSocket.h"

class CAdminInterface;
class COptions;

// server states for CServer::ToggleActive:
// ST_GOOFFLINE can be combined with ST_WAIT_USERS and ST_WAIT_TRANSFERS
#define ST_GOONLINE  0x0001
#define ST_GOOFFLINE 0x0002
#define ST_NOT_USED 0x0004 // this one is not used 
#define ST_WAIT_USERS 0x0008 // Wait until all users have logged out
#define ST_WAIT_TRANSFERS 0x0010 // Wait until all transfers are finished


class CXBServer : public CServer
{
public:
  CAdminInterface* GetAdminInterface();
  COptions* GetOptions();

  //////////////////////////////////////////////////
  // server control methods

  XFSTATUS SetThreadNum(int ThreadNum);
  XFSTATUS SetServerPort(int ServerPort);
  XFSTATUS SetAdminPort(int AdminPort);

  //////////////////////////////////////////////////
  // connection methods
  XFSTATUS CloseConnection(int ConnectionId); // i.e. kick user
  XFSTATUS GetAllConnections(std::vector<SXFConnection>& ConnectionVector);
  XFSTATUS GetConnection(int ConnectionId, SXFConnection& Connection);
  
  //////////////////////////////////////////////////
  // notifications

  XFSTATUS AddNotificationClient(CXFNotificationClient* Client);
  XFSTATUS RemoveNotificationClient(CXFNotificationClient* Client);


  // overload ShowStatus() to allow notifications
	virtual void ShowStatus(LPCTSTR msg, int nType);
	virtual void ShowStatus(_int64 eventDate, LPCTSTR msg, int nType);
protected:
  // overload OnServerMessage() to allow notifications
  virtual LRESULT OnServerMessage(WPARAM wParam, LPARAM lParam);

  std::list<CXFNotificationClient*> mClientList;
  CCriticalSectionWrapper mClientListCS;
  std::map<int, SXFConnection> mConnectionMap;
  CCriticalSectionWrapper mConnectionMapCS;
};

class CXBAdminSocket : public CAdminSocket
{
};


#endif

