#include "stdafx.h"
#include "xbserver.h"
#include "ServerThread.h"
#include "Options.h"
#include "ListenSocket.h"
#include "AdminListenSocket.h"


#pragma warning (disable:4244)
#pragma warning (disable:4800)

CAdminInterface* CXBServer::GetAdminInterface()
{
  return m_pAdminInterface;
}

COptions* CXBServer::GetOptions()
{
  return m_pOptions;
}


//////////////////////////////////////////////////
// server control methods
XFSTATUS CXBServer::SetThreadNum(int ThreadNum)
{
  if (ThreadNum == m_pOptions->GetOptionVal(OPTION_THREADNUM))
    return XFS_OK;

  m_pOptions->SetOption(OPTION_THREADNUM, ThreadNum);
	
	if (ThreadNum > m_ThreadArray.size())
	{
		int newthreads = ThreadNum - m_ThreadArray.size();
		for (int i = 0; i < newthreads; i++)
		{
			CServerThread *pThread = new CServerThread;
			if (pThread->Create(THREAD_PRIORITY_NORMAL, CREATE_SUSPENDED))
			{
				pThread->ResumeThread();
				m_ThreadArray.push_back(pThread);
			}
		}
		CStdString str;
		str.Format("Number of threads increased to %d.", ThreadNum);
		ShowStatus(str, 0);
	}
	else 
  if (ThreadNum < m_ThreadArray.size())
	{
		CStdString str;
		str.Format("Decreasing number of threads to %d.", ThreadNum);
		ShowStatus(str, 0);
		int removethreads = m_ThreadArray.size() - ThreadNum;
		int i = 0;
		for (std::list<CServerThread*>::iterator iter = m_ThreadArray.begin(); 
         iter != m_ThreadArray.end(); iter++, i++)
		{
			if (i >= ThreadNum)
				(*iter)->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_GOOFFLINE, 2);
		}
	}

  return XFS_OK;
}


XFSTATUS CXBServer::SetServerPort(int ServerPort)
{
  if (ServerPort == m_pOptions->GetOptionVal(OPTION_SERVERPORT))
    return XFS_OK;

  m_pOptions->SetOption(OPTION_SERVERPORT, ServerPort);

	if (m_pListenSocket)
	{
		CStdString str;
		str.Format("Closing listen socket on port %d", ServerPort);
		ShowStatus(str, 0);
		m_pListenSocket->Close();
		str.Format("Creating listen socket on port %d...", m_pOptions->GetOptionVal(OPTION_SERVERPORT));
		ShowStatus(str, 0);
		if (!m_pListenSocket->Create(m_pOptions->GetOptionVal(OPTION_SERVERPORT), SOCK_STREAM,FD_ACCEPT,0) || !m_pListenSocket->Listen())
		{
			delete m_pListenSocket;
			m_pListenSocket = 0;
			str.Format("Failed to create listen socket on port %d. Server is not online!", m_pOptions->GetOptionVal(OPTION_SERVERPORT));
			ShowStatus(str,1);
			m_nServerState = 0;
      return XFS_ERROR;
		}
		else
			ShowStatus("Listen socket port changed",0);
	}

  return XFS_OK;
}

XFSTATUS CXBServer::SetAdminPort(int AdminPort)
{
  if (AdminPort == m_pOptions->GetOptionVal(OPTION_ADMINPORT))
    return XFS_OK;

  m_pOptions->SetOption(OPTION_ADMINPORT, AdminPort);

  CStdString adminIpBindings = m_pOptions->GetOption(OPTION_ADMINIPBINDINGS);
  if (AdminPort != m_pOptions->GetOptionVal(OPTION_ADMINPORT) || adminIpBindings!=m_pOptions->GetOption(OPTION_ADMINIPBINDINGS))
	{
		if (AdminPort == m_pOptions->GetOptionVal(OPTION_ADMINPORT))
		{
			for (std::list<CAdminListenSocket*>::iterator iter = m_AdminListenSocketList.begin(); iter!=m_AdminListenSocketList.end(); iter++)
			{
				(*iter)->Close();
				delete *iter;
			}
			m_AdminListenSocketList.clear();
		}
		CAdminListenSocket *pSocket = new CAdminListenSocket(m_pAdminInterface);
		if (!pSocket->Create(m_pOptions->GetOptionVal(OPTION_ADMINPORT), SOCK_STREAM, FD_ACCEPT, (m_pOptions->GetOption(OPTION_ADMINIPBINDINGS)!="*") ? _T("127.0.0.1") : NULL))
		{
			delete pSocket;
			CStdString str;
			str.Format(_T("Failed to change admin listen port to %d."), m_pOptions->GetOptionVal(OPTION_ADMINPORT));
			m_pOptions->SetOption(OPTION_ADMINPORT, AdminPort);
			ShowStatus(str, 1);
		}
		else
		{
			pSocket->Listen();
			for (std::list<CAdminListenSocket*>::iterator iter = m_AdminListenSocketList.begin(); iter!=m_AdminListenSocketList.end(); iter++)
			{
				(*iter)->Close();
				delete *iter;
			}
			m_AdminListenSocketList.clear();

			m_AdminListenSocketList.push_back(pSocket);
			if (AdminPort != m_pOptions->GetOptionVal(OPTION_ADMINPORT))
			{
				CStdString str;
				str.Format(_T("Admin listen port changed to %d."), m_pOptions->GetOptionVal(OPTION_ADMINPORT));
				ShowStatus(str, 0);
			}

			if (m_pOptions->GetOption(OPTION_ADMINIPBINDINGS) != "*")
			{
				BOOL bError = FALSE;
				CStdString str = _T("Failed to bind the admin interface to the following IPs:");
				CStdString ipBindings = m_pOptions->GetOption(OPTION_ADMINIPBINDINGS);

				if (ipBindings != "")
					ipBindings += " ";
				while (ipBindings != "")
				{
					int pos = ipBindings.Find(" ");
					if (pos == -1)
						break;
					CStdString ip = ipBindings.Left(pos);
					ipBindings = ipBindings.Mid(pos+1);
					CAdminListenSocket *pAdminListenSocket = new CAdminListenSocket(m_pAdminInterface);
					if (!pAdminListenSocket->Create(m_pOptions->GetOptionVal(OPTION_ADMINPORT), SOCK_STREAM, FD_ACCEPT, ip) || !pAdminListenSocket->Listen())
					{
						bError = TRUE;
						str += _T(" ") + ip;
						delete pAdminListenSocket;
					}
					else
						m_AdminListenSocketList.push_back(pAdminListenSocket);
				}
				if (bError)
					ShowStatus(str, 1);
			}
			if (adminIpBindings!=m_pOptions->GetOption(OPTION_ADMINIPBINDINGS))
				ShowStatus(_T("Admin interface IP bindings changed"), 0);
		}

	}

  return XFS_OK;
}


void CXBServer::ShowStatus(LPCTSTR msg, int nType)
{
  _int64 eventDate;
  SYSTEMTIME systemtime;
  GetLocalTime(&systemtime);
  FILETIME filetime;
  SystemTimeToFileTime(&systemtime, &filetime);
  eventDate = ((_int64)filetime.dwHighDateTime << 32) + (_int64)filetime.dwLowDateTime;
  ShowStatus(eventDate, msg, nType);
}

void CXBServer::ShowStatus(_int64 eventDate, LPCTSTR msg, int nType)
{
  enumStatusSource Source;
  switch (nType) {
    case 0: Source = serverOk; break;
    case 1: Source = serverError; break;
    case 2: Source = userReceive; break;
    case 3: Source = userSend; break;
    default: Source = serverOk; break;
  };
  for (std::list<CXFNotificationClient*>::iterator clientIt = mClientList.begin();
         clientIt != mClientList.end(); ++clientIt)
    (*clientIt)->OnShowStatus(eventDate, msg, Source);

  CServer::ShowStatus(eventDate, msg, nType);
}

LRESULT CXBServer::OnServerMessage(WPARAM wParam, LPARAM lParam)
{
  switch (wParam)
  {
    case FSM_STATUSMESSAGE:
      // handled by base class, which calls ShowStatus()
      break;
    case FSM_CONNECTIONDATA:
      {
        t_connop *pConnOp = reinterpret_cast<t_connop*>(lParam);
        if (pConnOp)
        {
          int connectionId = pConnOp->data->userid;
          enumConnectionStatus connectionStatus = (pConnOp->op == USERCONTROL_CONNOP_ADD ? connectionAdd : 
                         (pConnOp->op == USERCONTROL_CONNOP_MODIFY ? connectionModify : connectionRemove));

          mConnectionMapCS.Lock();
		      if (pConnOp->op != USERCONTROL_CONNOP_REMOVE)
          {
			      mConnectionMap[pConnOp->data->userid].mId = pConnOp->data->userid;
            mConnectionMap[pConnOp->data->userid].mIPAddress = pConnOp->data->ip;
            mConnectionMap[pConnOp->data->userid].mPort = pConnOp->data->port;
            if (pConnOp->data->user)
              mConnectionMap[pConnOp->data->userid].mUsername = pConnOp->data->user;
            else
              mConnectionMap[pConnOp->data->userid].mUsername = _T("");
          }
		      else
		      {
			      std::map<int, SXFConnection>::iterator iter=mConnectionMap.find(pConnOp->data->userid);
			      if (iter!=mConnectionMap.end())
				      mConnectionMap.erase(iter);
		      }
          mConnectionMapCS.Unlock();

          for (std::list<CXFNotificationClient*>::iterator clientIt = mClientList.begin();
               clientIt != mClientList.end(); ++clientIt)
           (*clientIt)->OnConnection(connectionId, connectionStatus);
        }
      }
      break;
    case FSM_THREADCANQUIT:
      for (std::list<CXFNotificationClient*>::iterator clientIt = mClientList.begin();
           clientIt != mClientList.end(); ++clientIt)
       (*clientIt)->OnServerWillQuit();
      break;
    case FSM_SEND:
      for (std::list<CXFNotificationClient*>::iterator clientIt = mClientList.begin();
           clientIt != mClientList.end(); ++clientIt)
       (*clientIt)->OnIncSendCount(lParam);
      break;
    case FSM_RECV:
      for (std::list<CXFNotificationClient*>::iterator clientIt = mClientList.begin();
           clientIt != mClientList.end(); ++clientIt)
       (*clientIt)->OnIncRecvCount(lParam);
      break;
    case FSM_FILETRANSFER:
      {
        SXFTransferInfo* transferInfo = (SXFTransferInfo*)lParam;
        for (std::list<CXFNotificationClient*>::iterator clientIt = mClientList.begin();
             clientIt != mClientList.end(); ++clientIt)
          (*clientIt)->OnFileTransfer(transferInfo);

        delete transferInfo;
      }
      break;
    default:
      break;
  };

  LRESULT result = CServer::OnServerMessage(wParam, lParam);
  return result;
}


XFSTATUS CXBServer::AddNotificationClient(CXFNotificationClient* Client)
{
  mClientListCS.Lock();
  XFSTATUS result = XFS_ALREADY_EXISTS;
  if (std::find(mClientList.begin(), mClientList.end(), Client) == mClientList.end())
  {
    mClientList.push_back(Client);
    result = XFS_OK;
  }

  mClientListCS.Unlock();
  return result;
}

XFSTATUS CXBServer::RemoveNotificationClient(CXFNotificationClient* Client)
{
  mClientListCS.Lock();
  XFSTATUS result = XFS_NOT_FOUND;
  if (std::find(mClientList.begin(), mClientList.end(), Client) != mClientList.end())
  {
    mClientList.remove(Client);
    result = XFS_OK;
  }
  
  mClientListCS.Unlock();
  return result;
}



XFSTATUS CXBServer::CloseConnection(int ConnectionId)
{
  std::map<int, t_connectiondata>::iterator iter = m_UsersList.find(ConnectionId);
	if (iter!=m_UsersList.end())
	{
		t_controlmessage *msg=new t_controlmessage;
		msg->command=USERCONTROL_KICK;
		msg->socketid=ConnectionId;
		(*iter).second.pThread->PostThreadMessage(WM_FILEZILLA_THREADMSG, FTM_CONTROL, (LPARAM)msg);
    return XFS_OK;
  }

  return XFS_NOT_FOUND;
}

XFSTATUS CXBServer::GetAllConnections(std::vector<SXFConnection>& ConnectionVector)
{
  ConnectionVector.clear();

  std::map<int, SXFConnection>::iterator it;
  
  mConnectionMapCS.Lock();
  for (it = mConnectionMap.begin(); it != mConnectionMap.end(); ++it)
    ConnectionVector.push_back((*it).second);

  mConnectionMapCS.Unlock();
  return XFS_OK;
}


XFSTATUS CXBServer::GetConnection(int ConnectionId, SXFConnection& Connection)
{
  mConnectionMapCS.Lock();
  std::map<int, SXFConnection>::iterator it = mConnectionMap.find(ConnectionId);
  XFSTATUS result = XFS_NOT_FOUND;

  if (it != mConnectionMap.end())
  {
    Connection = (*it).second;
    result = XFS_OK;
  }

  mConnectionMapCS.Unlock();
  return result;
}

