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

#include "stdafx.h"
#include "Thread.h"
#include "xbnetwork.h"

#include <xtl.h>
#include <stdio.h>
#include <algorithm>


#pragma warning (disable:4244)
#pragma warning (disable:4800)
unsigned long GetLocalIPAddress()
{
  static unsigned long ipaddress = 0;
  
  if (ipaddress == 0)
  {
    char szIP[33];

    // get local ip address
    XNADDR xna;
	  DWORD dwState;
	  do
	  {
		  dwState = XNetGetTitleXnAddr(&xna);
		  Sleep(1000);
	  } while (dwState==XNET_GET_XNADDR_PENDING);

  
	  XNetInAddrToString(xna.ina,szIP,32);
	  OutputDebugString("Local address is ");
	  OutputDebugString(szIP);
	  OutputDebugString("\n");
    ipaddress = inet_addr(szIP);
  }

  return ipaddress;
}


char* inet_ntoa (struct in_addr in)
{
  static char _inetaddress[32];
  sprintf(_inetaddress, "%d.%d.%d.%d", in.S_un.S_un_b.s_b1, in.S_un.S_un_b.s_b2, in.S_un.S_un_b.s_b3, in.S_un.S_un_b.s_b4);
  return _inetaddress;
}



struct hostent FAR * gethostbyname(const char FAR * name)
{
  return NULL;
}

// make sure the synchronised socket functions are used
// 'using namespace SyncSocket' can give ambiguous call compiler errors
#undef accept 
#undef bind
#undef closesocket
#undef connect
#undef ioctlsocket
#undef getpeername
#undef getsockname
#undef getsockopt
#undef listen
#undef recv
#undef recvfrom
#undef select
#undef send
#undef sendto
#undef setsockopt
#undef shutdown



CAsyncSelectHelper::CAsyncSelectHelper(SOCKET s, HWND hWnd, unsigned int wMsg, long lEvent)
: mSocket(s)
, mIsConnected(false)
, mSendEnabled(true)
, mReceiveEnabled(true)
, mLockCount(0)
{
  SetParams(hWnd, wMsg, lEvent);

  ASSERT(s != INVALID_SOCKET);
  InitializeCriticalSection(&mCS);
}

CAsyncSelectHelper::~CAsyncSelectHelper()
{
  DeleteCriticalSection(&mCS);
}

void CAsyncSelectHelper::Reset()
{
  mAcceptEnabled = false;
  mIsConnecting = false;
  mIsConnected = false;
  mSendEnabled = false;
  mReceiveEnabled = false;
  mIsListening = false;
}

void CAsyncSelectHelper::SetParams(HWND hWnd, unsigned int wMsg, long lEvent)
{
  if (lEvent == 0)
  {
    // this will be deleted in CAsyncSelectManager::Run() if this->mSocket == INVALID_SOCKET 
    mSocket = INVALID_SOCKET;
  }

  mMsg = wMsg;
  mWnd = hWnd;

  mEvent = lEvent;
  
  mIsConnecting = false; // only set if connect() is called
  mIsListening = false; // only set if listen() is called
  mAcceptEnabled = mEvent & FD_ACCEPT;
  mReceiveEnabled = mEvent & FD_READ;
  mSendEnabled = mEvent & FD_WRITE;
}

void CAsyncSelectHelper::Lock()
{
  EnterCriticalSection(&mCS);
  mLockCount++;
  ASSERT(mLockCount == 1);
}


void CAsyncSelectHelper::Unlock()
{
  mLockCount--;
  LeaveCriticalSection(&mCS);
}

/////////////////////////////////////////////////////////////////
// CAsyncSelectManager

CAsyncSelectManager gAsyncSelectManager;

CAsyncSelectManager::CAsyncSelectManager()
{
  mEventStop = CreateEvent(NULL, FALSE, TRUE, NULL);

  // create suspended as there are no sockets yet to be polled
  mIsSuspended = true;
  Create(THREAD_PRIORITY_NORMAL, CREATE_SUSPENDED);
}


CAsyncSelectManager::~CAsyncSelectManager()
{
  SetEvent(mEventStop);
  
  // resume thread so that it can react to mEventStop
  if (mIsSuspended)
  {
    mIsSuspended = false;
    OutputDebugString(_T("CAsyncSelectManager resumed for closing\n"));
    ResumeThread();
  }

  WaitForSingleObject(m_hEventStarted, INFINITE);

  CAsyncSelectHelperList::iterator it; 
  for (it = mAsyncSelectHelperList.begin(); it != mAsyncSelectHelperList.end(); ++it)
    delete *it;

  CloseHandle(mEventStop);
}


CAsyncSelectHelper* CAsyncSelectManager::GetHelper(SOCKET s)
{
  mAsyncSelectHelperListCS.Lock();

  CAsyncSelectHelper* result = NULL;

  CAsyncSelectHelperList::iterator it = std::find_if(mAsyncSelectHelperList.begin(), mAsyncSelectHelperList.end(), CAsyncSelectFindFunctor(s));
  if (it != mAsyncSelectHelperList.end())
    result = *it;

  mAsyncSelectHelperListCS.Unlock();

  return result;
}

int CAsyncSelectManager::WSAAsyncSelect(SOCKET s, HWND hWnd, unsigned int wMsg, long lEvent)
{
  mAsyncSelectHelperListCS.Lock();
  
  CAsyncSelectHelperList::iterator it = std::find_if(mAsyncSelectHelperList.begin(), mAsyncSelectHelperList.end(), CAsyncSelectFindFunctor(s));
  if (it != mAsyncSelectHelperList.end())
    (*it)->SetParams(hWnd, wMsg, lEvent);
  else
  {
    // set socket in non blocking mode
    DWORD dummy = 1;
    ioctlsocket(s, FIONBIO, &dummy);
    // create helper for event notification
    CAsyncSelectHelper* helper = new CAsyncSelectHelper(s, hWnd, wMsg, lEvent);

    AddHelper(helper);
  }

  mAsyncSelectHelperListCS.Unlock();
  return 0;
}


// similar to WSAAsyncSelect, but copy all settings from srcHelper
int CAsyncSelectManager::Accept(SOCKET s, CAsyncSelectHelper* srcHelper)
{
  // set socket in non blocking mode
  DWORD enableNonBlocking = 1;
  ioctlsocket(s, FIONBIO, &enableNonBlocking);
  // create helper for event notification
  
  CAsyncSelectHelper* helper = new CAsyncSelectHelper(s, srcHelper->mWnd, srcHelper->mMsg, srcHelper->mEvent);
  helper->mAcceptEnabled = srcHelper->mAcceptEnabled;
  helper->mIsConnected = srcHelper->mIsConnected;
  helper->mIsConnecting = srcHelper->mIsConnecting;
  helper->mIsListening = srcHelper->mIsListening;
  helper->mReceiveEnabled = srcHelper->mReceiveEnabled;
  helper->mSendEnabled = srcHelper->mSendEnabled;
  
  mAsyncSelectHelperListCS.Lock();
  
  AddHelper(helper);
  
  mAsyncSelectHelperListCS.Unlock();

  return 0;
}

void CAsyncSelectManager::AddHelper(CAsyncSelectHelper* Helper)
{
  mAsyncSelectHelperList.push_back(Helper);

  if (mIsSuspended)
  {
    mIsSuspended = false;
    OutputDebugString(_T("CAsyncSelectManager resumed\n"));
	  SetEvent(m_hEventStarted);
    ResumeThread();
  }
}


DWORD CAsyncSelectManager::Run()
{
  TIMEVAL timeval;
  timeval.tv_sec = 0;
  timeval.tv_usec = 100; 

  SetEvent(m_hEventStarted);
  ResetEvent(mEventStop);
  
  CAsyncSelectHelperList::iterator it;

  while (WaitForSingleObject(mEventStop, 0) != WAIT_OBJECT_0)
  {
    if (mAsyncSelectHelperList.size() == 0 && !mIsSuspended)
    {
      mIsSuspended = true;
      OutputDebugString(_T("CAsyncSelectManager suspended\n"));
      SuspendThread();
    }

    for (it = mAsyncSelectHelperList.begin(); it != mAsyncSelectHelperList.end(); )
    {
      // one socket at a time
      // TODO: eliminate loop by using select() on more sockets.

      CAsyncSelectHelper* helper = *it;
      
      SOCKET s = helper->mSocket;
      if (s == INVALID_SOCKET)
      {
        CAsyncSelectHelperList::iterator tempIt = it;
        ++it;
        delete helper;
        mAsyncSelectHelperList.erase(tempIt);
      }
      else
      {
        // lock the socket for the duration of the loop
        // otherwise ioctlsocket() can falsely report that 0 bytes can be read
        // indicating that the socket has been closed, while in fact, the socket
        // was just emptied using recv() in another thread
        CSocketLock lock(s);

        ++it;
        fd_set readfds;
        FD_ZERO(&readfds);
        if (helper->mEvent & (FD_ACCEPT | FD_READ | FD_CLOSE))
          FD_SET(s, &readfds);

        fd_set writefds;
        FD_ZERO(&writefds);
        if (helper->mEvent & (FD_WRITE | FD_CONNECT))
          FD_SET(s, &writefds);

        fd_set exceptfds;
        FD_ZERO(&exceptfds);
        FD_SET(s, &exceptfds);

        int result = select(0, &readfds, &writefds, &exceptfds, &timeval);
     
        if (result == 0)
        {
          // time out
          // nothing happened to the socket
        }
        else
        if (result == SOCKET_ERROR)
        {
          CStdString str;
          str.Format(_T("0x%X : Socket 0x%X select() result == SOCKET_ERROR\n"), GetCurrentThreadId(), helper->mSocket);
          OutputDebugString(str);
        }
        else
        {
          // handle one event at a time
          if (FD_ISSET(s, &exceptfds))
          {
            if (helper->mEvent & FD_CONNECT && helper->mIsConnecting)
            {
              CStdString str;
              str.Format(_T("0x%X : Socket 0x%X select() exception received, FD_CONNECT\n"), GetCurrentThreadId(), helper->mSocket);
              OutputDebugString(str);

              PostMessage(helper->mWnd, helper->mMsg, s, MAKELPARAM(FD_CONNECT, SOCKET_ERROR));
            }
            else
            if (helper->mEvent & FD_CLOSE && helper->mIsConnected)
            {
              CStdString str;
              str.Format(_T("0x%X : Socket 0x%X select() exception received, FD_CLOSE\n"), GetCurrentThreadId(), helper->mSocket);
              OutputDebugString(str);

              PostMessage(helper->mWnd, helper->mMsg, s, MAKELPARAM(FD_CLOSE, SOCKET_ERROR));
            }
            else
            {
              CStdString str;
              str.Format(_T("0x%X : Socket 0x%X select() exception received, but not handled\n"), GetCurrentThreadId(), helper->mSocket);
              OutputDebugString(str);
              
              PostMessage(helper->mWnd, helper->mMsg, s, MAKELPARAM(FD_CLOSE, SOCKET_ERROR));
            }

            helper->Reset();
          }
          else
          if (FD_ISSET(s, &readfds))
          {
            if (helper->mIsListening)
            {
              if ( (helper->mEvent & FD_ACCEPT) && helper->mAcceptEnabled)
              {
                helper->mAcceptEnabled = false;
                helper->mIsConnected = true;
                helper->mSendEnabled = true;
                helper->mReceiveEnabled = true;

                PostMessage(helper->mWnd, helper->mMsg, s, MAKELPARAM(FD_ACCEPT, 0));
                PostMessage(helper->mWnd, helper->mMsg, s, MAKELPARAM(FD_WRITE, 0));
              }
              else
              {
                CStdString str;
                str.Format(_T("0x%X : Socket 0x%X select() FD_ACCEPT received, but not handled\n"), GetCurrentThreadId(), helper->mSocket);
                OutputDebugString(str);
              }
            }
            else
            {
              if (helper->mIsConnected)
              {
                DWORD nBytes;
                
                if (ioctlsocket(s, FIONREAD, &nBytes) == 0)
                  if (nBytes == 0)
                  {
                    if (helper->mEvent & FD_CLOSE)
                    {
                      if (helper->mIsConnected == true)
                      {
                        helper->Reset();

                        PostMessage(helper->mWnd, helper->mMsg, s, MAKELPARAM(FD_CLOSE, 0));
                      }
                      else
                      {
                        CStdString str;
                        str.Format(_T("0x%X : Socket 0x%X select() FD_CLOSE received, but mIsConnected == false\n"), GetCurrentThreadId(), helper->mSocket);
                        OutputDebugString(str);
                      }
                    }
                  }
                  else
                  if (helper->mEvent & FD_READ)
                    if (helper->mReceiveEnabled)
                    {
                      helper->mReceiveEnabled = false;

                      PostMessage(helper->mWnd, helper->mMsg, s, MAKELPARAM(FD_READ, 0));
                    }
                    else
                    {
                      CStdString str;
                      str.Format(_T("0x%X : Socket 0x%X select() FD_READ received, but not handled\n"), GetCurrentThreadId(), helper->mSocket);
                      //OutputDebugString(str);
                    }
              }
              else
              {
                Sleep(1);
              }
            }
          }
          else
          if (FD_ISSET(s, &writefds))
          {
            if (helper->mIsConnecting)
            {
              if (helper->mEvent & FD_CONNECT)  
              {
                helper->mIsListening = false;
                helper->mIsConnecting = false;
                helper->mSendEnabled = true;
                helper->mReceiveEnabled = true;
                helper->mIsConnected = true;

                PostMessage(helper->mWnd, helper->mMsg, s, MAKELPARAM(FD_CONNECT, 0));
                PostMessage(helper->mWnd, helper->mMsg, s, MAKELPARAM(FD_WRITE, 0));
              }
              else
              {
                CStdString str;
                str.Format(_T("0x%X : Socket 0x%X select() FD_CONNECT received, but not handled\n"), GetCurrentThreadId(), helper->mSocket);
                OutputDebugString(str);
              }
            }
            else
            if (helper->mIsConnected)
            if (helper->mEvent & FD_WRITE)
              if (helper->mSendEnabled)
              {
                // TODO: helper->mSendEnabled should be set to false here,
                // however, this causes passive mode to fail
                helper->mSendEnabled = false;

                PostMessage(helper->mWnd, helper->mMsg, s, MAKELPARAM(FD_WRITE, 0));
              }
              else
              {
                /*
                CStdString str;
                str.Format(_T("0x%X : Socket 0x%X select() FD_WRITE received, but not handled\n"), GetCurrentThreadId(), helper->mSocket);
                OutputDebugString(str);
                */

                Sleep(1);
              }
          }
          else
          {
            CStdString str;
            str.Format(_T("0x%X : Socket 0x%X select() unknown event\n"), GetCurrentThreadId(), helper->mSocket);
            OutputDebugString(str);
          }
        }
      }
 
    } // for
  } // while

  ResetEvent(m_hEventStarted);

  return 0;
}



///////////////////////////////////////////////////////////////////////
// CSocketLock

CSocketLock::CSocketLock(SOCKET psocket)
{
  mHelper = gAsyncSelectManager.GetHelper(psocket);

  if (mHelper)
    mHelper->Lock();
}

CSocketLock::~CSocketLock()
{
  if (mHelper)
    mHelper->Unlock();
}


///////////////////////////////////////////////////////////////////////
// synchronised socket methods


namespace SyncSocket {

SOCKET fz_accept(IN SOCKET s, OUT struct sockaddr FAR * addr, IN OUT int FAR * addrlen)
{
  CSocketLock lock(s);

  if (lock.mHelper)
    lock.mHelper->mAcceptEnabled = true;

  SOCKET temp = ::accept(s, addr, addrlen);

  if (temp != INVALID_SOCKET)
    if (lock.mHelper)
      gAsyncSelectManager.Accept(temp, lock.mHelper);

  return temp;
}

int fz_bind(IN SOCKET s, IN const struct sockaddr FAR * name, IN int namelen)
{
  CSocketLock lock(s);
  
  // Bind to INADDR_ANY
  SOCKADDR_IN sa = *((SOCKADDR_IN*)name);

  sa.sin_addr.s_addr = INADDR_ANY;

  return ::bind(s, (const struct sockaddr FAR *)&sa, namelen);
}

int fz_closesocket(IN SOCKET s)
{
  CSocketLock lock(s);

  int result = ::closesocket(s);

  if (lock.mHelper)
    lock.mHelper->mSocket = INVALID_SOCKET;

  return result;
}

int fz_connect(IN SOCKET s, IN const struct sockaddr FAR * name, IN int namelen)
{
  CSocketLock lock(s);

  int result = ::connect(s, name, namelen);

  if (lock.mHelper)
    lock.mHelper->mIsConnecting = true;

  return result;
}

int fz_ioctlsocket(IN SOCKET s, IN long cmd, IN OUT u_long FAR * argp)
{
  CSocketLock lock(s);
  int result = ::ioctlsocket(s, cmd, argp);
  return result;
}

int fz_getpeername(IN SOCKET s, OUT struct sockaddr FAR * name, IN OUT int FAR * namelen)
{
  CSocketLock lock(s);
  return ::getpeername(s, name, namelen);
}

int fz_getsockname(IN SOCKET s, OUT struct sockaddr FAR * name, IN OUT int FAR * namelen)
{
  CSocketLock lock(s);
  int result = ::getsockname(s, name, namelen);
  if (((sockaddr_in*)name)->sin_addr.S_un.S_addr == 0)
    ((sockaddr_in*)name)->sin_addr.S_un.S_addr = GetLocalIPAddress();
  return result;
}

int fz_getsockopt(IN SOCKET s, IN int level, IN int optname, OUT char FAR * optval, IN OUT int FAR * optlen)
{
  CSocketLock lock(s);
  return ::getsockopt(s, level, optname, optval, optlen);
}

int fz_listen(IN SOCKET s, IN int backlog)
{
  CSocketLock lock(s);

  int result = ::listen(s, backlog);
  
  if (lock.mHelper)
    lock.mHelper->mIsListening = true;
  
  return result; 
}

int fz_recv(IN SOCKET s, OUT char FAR * buf, IN int len, IN int flags)
{
  CSocketLock lock(s);

  int result = ::recv(s, buf, len, flags);

  if (lock.mHelper)
    lock.mHelper->mReceiveEnabled = true;

  return result;
}

int fz_recvfrom(IN SOCKET s, OUT char FAR * buf, IN int len, IN int flags, OUT struct sockaddr FAR * from, IN OUT int FAR * fromlen)
{
  CSocketLock lock(s);

  int result = ::recvfrom(s, buf, len, flags, from, fromlen);

  if (lock.mHelper)
    lock.mHelper->mReceiveEnabled = true;

  return result;
}


// synchronisation of select() is being done in another way, 
// see CAsyncSelectManager::Run()
int fz_select(IN int nfds, IN OUT fd_set FAR * readfds, IN OUT fd_set FAR * writefds, IN OUT fd_set FAR *exceptfds, IN const struct timeval FAR * timeout)
{
//  CSocketLock lock(s);
  return ::select(nfds, readfds, writefds, exceptfds, timeout);
}


int fz_send(IN SOCKET s, IN const char FAR * buf, IN int len, IN int flags)
{
  CSocketLock lock(s);

  int result = ::send(s, buf, len, flags);

  if (lock.mHelper)
    lock.mHelper->mSendEnabled = true;

  return result;
}

int fz_sendto(IN SOCKET s, IN const char FAR * buf, IN int len, IN int flags, IN const struct sockaddr FAR * to, IN int tolen)
{
  CSocketLock lock(s);

  int result = ::sendto(s, buf, len, flags, to, tolen);

  if (lock.mHelper)
    lock.mHelper->mSendEnabled = true;

  return result;
}

int fz_setsockopt(IN SOCKET s, IN int level, IN int optname, IN const char FAR * optval, IN int optlen)
{
  CSocketLock lock(s);
  return ::setsockopt(s, level, optname, optval, optlen);
}

int fz_shutdown(IN SOCKET s, IN int how)
{
  CSocketLock lock(s);
  return ::shutdown(s, how);
}

/////////////////////////////////////////////////////////////
// async socket helper functions

int fz_WSAAsyncSelect(SOCKET s, HWND hWnd, unsigned int wMsg, long lEvent)
{
  return gAsyncSelectManager.WSAAsyncSelect(s, hWnd, wMsg, lEvent);
}


}; // namespace SyncSocket

