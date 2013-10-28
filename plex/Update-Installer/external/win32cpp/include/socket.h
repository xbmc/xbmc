// Win32++   Version 7.2
// Released: 5th AUgust 2011
//
//      David Nash
//      email: dnash@bigpond.net.au
//      url: https://sourceforge.net/projects/win32-framework
//
//
// Copyright (c) 2005-2011  David Nash
//
// Permission is hereby granted, free of charge, to
// any person obtaining a copy of this software and
// associated documentation files (the "Software"),
// to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify,
// merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom
// the Software is furnished to do so, subject to the
// following conditions:
//
// The above copyright notice and this permission notice
// shall be included in all copies or substantial portions
// of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
//
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
// socket.h
//  Declaration of the CSocket class
//
// The CSocket class represents a network socket. It encapsualtes many of
// the Windows Socket SPI fuctions, providing an object-oriented approach
// to network programming. After StartEvents is called, CSocket monitors
// the socket and responds automatically to network events. This event
// monitoring, for example, automatically calls OnReceive when there is
// data on the socket to be read, and OnAccept when a server should accept
// a connection from a client.

// Users of this class should be aware that functions like OnReceive,
// OnAccept, etc. are called on a different thread from the one CSocket is
// instanciated on. The thread for these functions needs to respond quickly
// to other network events, so it shouldn't be delayed. It also doesn't run
// a message loop, so it can't be used to create windows. For these reasons
// it might be best to use PostMessage in response to these functions in a
// windows environment.

// Refer to the network samples for an example of how to use this class to
// create a TCP client & server, and a UDP client and server.

// To compile programs with CSocket, link with ws3_32.lib for Win32,
// and ws2.lib for Windows CE. Windows 95 systems will need to install the
// "Windows Sockets 2.0 for Windows 95". It's available from:
// http://support.microsoft.com/kb/182108/EN-US/

// For a TCP server, inherit a class from CSocket and override OnAccept, OnDisconnect
// and OnRecieve. Create one instance of this class and use it as a listening socket.
// The purpose of the listening socket is to detect connections from clients and accept them.
// For the listening socket, we do the following:
// 1) Create the socket.
// 2) Bind an IP address to the socket.
// 3) Listen on the socket for incoming connection requests.
// 4) Use StartNotifyRevents to receive notification of network events.
// 5) Override OnAccept to accept requests on a newly created data CSocket object.
// 6) Create a new data socket for each client connection accepted.
// 7) The server socket uses the 'accept' function to accept an incoming connection
//     from this new data socket.

// The purpose of the data socket is to send data to, and recieve data from the client.
// There will be one data socket for each client accepted by the server.
// To use it we do the following:
// * To recieve data from the client, override OnReceive and use Receive.
// * To send data to use Send.
// * OnDisconnect can be used to detect when the client is disconnected.

// For a TCP client, inherit from CSocket and override OnReceive and OnDisconnect.
// Create an instance of this inherited class, and  perform the following steps:
// 1) Create the socket.
// 2) Connect to the server.
// 3) Use StartNotifyRevents to receive notification of network events.
//    We are now ready to send and recieve data from the server.
// * Use Send to send data to the server.
// * Override OnReceive and use Recieve to receive data from the server
// * OnDisconnect can be used to detect when the client is disconnected from the server.

// Notes regarding IPv6 support
// * IPv6 is supported on Windows Vista and above. Windows XP with SP2 provides
//    "experimental" support, which can be enabled by entering "ipv6 install"
//    at a command prompt.
// * IPv6 is not supported by all compilters and devlopment environments. In
//    particular, it is not supported by Dev-C++ or Borland 5.5. A modern
//    Platform SDK needs to be added to Visual Studio 6 for it to support IPv6.
// * IsIPV6Supported returns false if either the operating system or the
//    development environment fails to support IPv6.
//

#ifndef _WIN32XX_SOCKET_H_
#define _WIN32XX_SOCKET_H_


#include "wincore.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>


#define THREAD_TIMEOUT 100


namespace Win32xx
{

	typedef int  WINAPI GETADDRINFO(LPCSTR, LPCSTR, const struct addrinfo*, struct addrinfo**);
	typedef void WINAPI FREEADDRINFO(struct addrinfo*);

	class CSocket
	{
	public:
		CSocket();
		virtual ~CSocket();

		// Operations
		virtual void Accept(CSocket& rClientSock, struct sockaddr* addr, int* addrlen);
		virtual int  Bind(LPCTSTR addr, LPCTSTR port);
		virtual int  Bind(const struct sockaddr* name, int namelen);
		virtual int  Connect(LPCTSTR addr, LPCTSTR port);
		virtual int  Connect(const struct sockaddr* name, int namelen);
		virtual BOOL Create( int family, int type, int protocol = IPPROTO_IP);
		virtual void Disconnect();
		virtual void FreeAddrInfo( struct addrinfo* ai );
		virtual int  GetAddrInfo( LPCTSTR nodename, LPCTSTR servname, const struct addrinfo* hints, struct addrinfo** res);
		virtual LPCTSTR GetLastError();
		virtual int  ioCtlSocket(long cmd, u_long* argp);
		virtual BOOL IsIPV6Supported();
		virtual int  Listen(int backlog = SOMAXCONN);
		virtual int  Receive(TCHAR* buf, int len, int flags);
		virtual int  ReceiveFrom(TCHAR* buf, int len, int flags, struct sockaddr* from, int* fromlen);
		virtual int  Send(LPCTSTR buf, int len, int flags);
		virtual int  SendTo(LPCTSTR send, int len, int flags, LPCTSTR addr, LPCTSTR port);
		virtual int  SendTo(LPCTSTR buf, int len, int flags, const struct sockaddr* to, int tolen);

		virtual void StartEvents();
		virtual void StopEvents();

		// Attributes
		virtual int  GetPeerName(struct sockaddr* name, int* namelen);
		virtual int  GetSockName(struct sockaddr* name, int* namelen);
		SOCKET& GetSocket() { return m_Socket; }
		virtual int  GetSockOpt(int level, int optname, char* optval, int* optlen);
		virtual int  SetSockOpt(int level, int optname, const char* optval, int optlen);

		// Override these functions to monitor events
		virtual void OnAccept()		{}
		virtual void OnAddresListChange() {}
		virtual void OnDisconnect()	{}
		virtual void OnConnect()	{}
		virtual void OnOutOfBand()	{}
		virtual void OnQualityOfService() {}
		virtual void OnReceive()	{}
		virtual void OnRoutingChange() {}
		virtual void OnSend()		{}



		// Allow CSocket to be used as a SOCKET
		operator SOCKET() const {return m_Socket;}

	private:
		CSocket(const CSocket&);				// Disable copy construction
		CSocket& operator = (const CSocket&);	// Disable assignment operator
		static UINT WINAPI EventThread(LPVOID thread_data);

		tString m_tsErrorMessage;
		SOCKET m_Socket;
		HMODULE m_hWS2_32;
		HANDLE m_hEventThread;	// Handle to the thread
		HANDLE m_StopRequest;	// An event to signal the event thread should stop
		HANDLE m_Stopped;		// An event to signal the event thread is stopped

		GETADDRINFO* m_pfnGetAddrInfo;		// pointer for the GetAddrInfo function
		FREEADDRINFO* m_pfnFreeAddrInfo;	// pointer for the FreeAddrInfo function
	};
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace Win32xx
{

	inline CSocket::CSocket() : m_Socket(INVALID_SOCKET), m_hEventThread(0)
	{
		// Initialise the Windows Socket services
		WSADATA wsaData;

		if (0 != ::WSAStartup(MAKEWORD(2,2), &wsaData))
			throw CWinException(_T("WSAStartup failed"));

		m_hWS2_32 = ::LoadLibrary(_T("WS2_32.dll"));
		if (0 == m_hWS2_32)
			throw CWinException(_T("Failed to load WS2_2.dll"));

		m_pfnGetAddrInfo = (GETADDRINFO*) GetProcAddress(m_hWS2_32, "getaddrinfo");
		m_pfnFreeAddrInfo = (FREEADDRINFO*) GetProcAddress(m_hWS2_32, "freeaddrinfo");

		m_StopRequest = ::CreateEvent(0, TRUE, FALSE, 0);
		m_Stopped = ::CreateEvent(0, TRUE, FALSE, 0);
	}

	inline CSocket::~CSocket()
	{
		Disconnect();

		// Close handles
		::CloseHandle(m_StopRequest);
		::CloseHandle(m_Stopped);

		// Terminate the  Windows Socket services
		::WSACleanup();

		::FreeLibrary(m_hWS2_32);
	}

	inline void CSocket::Accept(CSocket& rClientSock, struct sockaddr* addr, int* addrlen)
	{
		// The accept function permits an incoming connection attempt on the socket.

		rClientSock.m_Socket = ::accept(m_Socket, addr, addrlen);
		if (INVALID_SOCKET == rClientSock.GetSocket())
			TRACE(_T("Accept failed\n"));
	}

	inline int CSocket::Bind(LPCTSTR addr, LPCTSTR port)
	// The bind function associates a local address with the socket.
	{
		int RetVal = 0;

		if (IsIPV6Supported())
		{

#ifdef GetAddrInfo	// Skip the following code block for older development environments

			ADDRINFO Hints= {0};
			Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
			ADDRINFO *AddrInfo;

			RetVal = GetAddrInfo(addr, port, &Hints, &AddrInfo);
			if (RetVal != 0)
			{
				TRACE( _T("GetAddrInfo failed\n"));
				return RetVal;
			}

			// Bind the IP address to the listening socket
			RetVal =  ::bind( m_Socket, AddrInfo->ai_addr, (int)AddrInfo->ai_addrlen );
			if ( RetVal == SOCKET_ERROR )
			{
				TRACE(_T("Bind failed\n"));
				return RetVal;
			}

			// Free the address information allocated by GetAddrInfo
			FreeAddrInfo(AddrInfo);

#endif

		}
		else
		{
			sockaddr_in clientService;
			clientService.sin_family = AF_INET;
			clientService.sin_addr.s_addr = inet_addr( T2A(addr) );
			int nPort = -1;
            nPort = atoi( T2A(port) );
			if (-1 == nPort)
			{
				TRACE(_T("Invalid port number\n"));
				return SOCKET_ERROR;
			}
			clientService.sin_port = htons( (u_short)nPort );

			RetVal = ::bind( m_Socket, (SOCKADDR*) &clientService, sizeof(clientService) );
			if ( 0 != RetVal )
				TRACE(_T("Bind failed\n"));
		}

		return RetVal;
	}

	inline int CSocket::Bind(const struct sockaddr* name, int namelen)
	{
		// The bind function associates a local address with the socket.

		int Result = ::bind (m_Socket, name, namelen);
		if ( 0 != Result )
			TRACE(_T("Bind failed\n"));
		return Result;
	}

	inline int CSocket::Connect(LPCTSTR addr, LPCTSTR port)
	// The Connect function establishes a connection to the socket.
	{
		int RetVal = 0;

		if (IsIPV6Supported())
		{

#ifdef GetAddrInfo	// Skip the following code block for older development environments

			ADDRINFO Hints= {0};
			Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
			ADDRINFO *AddrInfo;

			RetVal = GetAddrInfo(addr, port, &Hints, &AddrInfo);
			if (RetVal != 0)
			{
				TRACE( _T("getaddrinfo failed\n"));
				return SOCKET_ERROR;
			}

			// Bind the IP address to the listening socket
			RetVal = Connect( AddrInfo->ai_addr, (int)AddrInfo->ai_addrlen );
			if ( RetVal == SOCKET_ERROR )
			{
				TRACE(_T("Connect failed\n"));
				return RetVal;
			}

			// Free the address information allocatied by GetAddrInfo
			FreeAddrInfo(AddrInfo);

#endif

		}
		else
		{
			sockaddr_in clientService;
			clientService.sin_family = AF_INET;
			clientService.sin_addr.s_addr = inet_addr( T2A(addr) );
			int nPort = -1;
			nPort = atoi( T2A(port) );
			if (-1 == nPort)
			{
				TRACE(_T("Invalid port number\n"));
				return SOCKET_ERROR;
			}
			clientService.sin_port = htons( (u_short)nPort );

			RetVal = ::connect( m_Socket, (SOCKADDR*) &clientService, sizeof(clientService) );
			if ( 0 != RetVal )
				TRACE(_T("Connect failed\n"));
		}

		return RetVal;
	}

	inline int CSocket::Connect(const struct sockaddr* name, int namelen)
	{
		// The Connect function establishes a connection to the socket.

		int Result = ::connect( m_Socket, name, namelen );
		if ( 0 != Result )
			TRACE(_T("Connect failed\n"));

		return Result;
	}

	inline BOOL CSocket::Create( int family, int type, int protocol /*= IPPROTO_IP*/)
	{
		// Creates the socket

		// Valid values:
		//  family:		AF_INET or AF_INET6
		//	type:		SOCK_DGRAM, SOCK_SEQPACKET, SOCK_STREAM, SOCK_RAW
		//	protocol:	IPPROTO_IP, IPPROTO_TCP, IPPROTO_UDP, IPPROTO_RAW, IPPROTO_ICMP, IPPROTO_ICMPV6

		m_Socket = socket(family, type, protocol);
		if(m_Socket == INVALID_SOCKET)
		{
			TRACE(_T("Failed to create socket\n"));
			return FALSE;
		}

		return TRUE;
	}

	inline void CSocket::Disconnect()
	{
		::shutdown(m_Socket, SD_BOTH);
		StopEvents();
		::closesocket(m_Socket);
		m_Socket = INVALID_SOCKET;
	}

	inline UINT WINAPI CSocket::EventThread(LPVOID thread_data)
	{
		// These are the possible network event notifications:
		//	FD_READ 	Notification of readiness for reading.
		//	FD_WRITE 	Motification of readiness for writing.
		//	FD_OOB 		Notification of the arrival of Out Of Band data.
		//	FD_ACCEPT 	Notification of incoming connections.
		//	FD_CONNECT 	Notification of completed connection or multipoint join operation.
		//	FD_CLOSE 	Notification of socket closure.
		//	FD_QOS		Notification of socket Quality Of Service changes
		//	FD_ROUTING_INTERFACE_CHANGE	Notification of routing interface changes for the specified destination.
		//	FD_ADDRESS_LIST_CHANGE		Notification of local address list changes for the address family of the socket.

		WSANETWORKEVENTS NetworkEvents;
		CSocket* pSocket = (CSocket*)thread_data;
		SOCKET sClient = pSocket->m_Socket;

        WSAEVENT AllEvents[2];
		AllEvents[0] = ::WSACreateEvent();
		AllEvents[1] = (WSAEVENT)pSocket->m_StopRequest;
		long Events = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE |
			          FD_QOS | FD_ROUTING_INTERFACE_CHANGE | FD_ADDRESS_LIST_CHANGE;

		// Associate the network event object (hNetworkEvents) with the
		// specified network events (Events) on socket sClient.
		if(	SOCKET_ERROR == WSAEventSelect(sClient, AllEvents[0], Events))
		{
			TRACE(_T("Error in Event Select\n"));
			::SetEvent(pSocket->m_Stopped);
			::WSACloseEvent(AllEvents[0]);
			return 0;
		}

		// loop until the stop event is set
		for (;;) // infinite loop
		{
			// Wait 100 ms for a network event
			DWORD dwResult = ::WSAWaitForMultipleEvents(2, AllEvents, FALSE, THREAD_TIMEOUT, FALSE);

			// Check event for stop thread
			if(::WaitForSingleObject(pSocket->m_StopRequest, 0) == WAIT_OBJECT_0)
			{
				::WSACloseEvent(AllEvents[0]);
				::SetEvent(pSocket->m_Stopped);
				return 0;
			}

			if (WSA_WAIT_FAILED == dwResult)
			{
				TRACE(_T("WSAWaitForMultipleEvents failed\n"));
				::WSACloseEvent(AllEvents[0]);
				::SetEvent(pSocket->m_Stopped);
				return 0;
			}

			// Proceed if a network event occurred
			if (WSA_WAIT_TIMEOUT != dwResult)
			{

				if ( SOCKET_ERROR == ::WSAEnumNetworkEvents(sClient, AllEvents[0], &NetworkEvents) )
				{
					TRACE(_T("WSAEnumNetworkEvents failed\n"));
					::WSACloseEvent(AllEvents[0]);
					::SetEvent(pSocket->m_Stopped);
					return 0;
				}

				if (NetworkEvents.lNetworkEvents & FD_ACCEPT)
					pSocket->OnAccept();

				if (NetworkEvents.lNetworkEvents & FD_READ)
					pSocket->OnReceive();

				if (NetworkEvents.lNetworkEvents & FD_WRITE)
					pSocket->OnSend();

				if (NetworkEvents.lNetworkEvents & FD_OOB)
					pSocket->OnOutOfBand();

				if (NetworkEvents.lNetworkEvents & FD_QOS)
					pSocket->OnQualityOfService();

				if (NetworkEvents.lNetworkEvents & FD_CONNECT)
					pSocket->OnConnect();

				if (NetworkEvents.lNetworkEvents & FD_ROUTING_INTERFACE_CHANGE)
					pSocket->OnRoutingChange();

				if (NetworkEvents.lNetworkEvents & FD_ADDRESS_LIST_CHANGE)
					pSocket->OnAddresListChange();

				if (NetworkEvents.lNetworkEvents & FD_CLOSE)
				{
					::shutdown(sClient, SD_BOTH);
					::closesocket(sClient);
					pSocket->OnDisconnect();
					::WSACloseEvent(AllEvents[0]);
					::SetEvent(pSocket->m_Stopped);
					return 0;
				}
			}
		}
	}

	inline int CSocket::GetAddrInfo( LPCTSTR nodename, LPCTSTR servname, const struct addrinfo* hints, struct addrinfo** res)
	{

#ifdef GetAddrInfo

		std::string sNodeName = T2A(nodename);
		std::string sServName = T2A(servname);
		return (*m_pfnGetAddrInfo)(sNodeName.c_str(), sServName.c_str(), hints, res);

#else

		UNREFERENCED_PARAMETER(nodename);
		UNREFERENCED_PARAMETER(servname);
		UNREFERENCED_PARAMETER(hints);
		UNREFERENCED_PARAMETER(res);

		throw CWinException(_T("getaddrinfo is not supported"));

#endif

	}

	inline LPCTSTR CSocket::GetLastError()
	{
		// Retrieves the most recent network error.

		int ErrorCode = WSAGetLastError();
		LPTSTR Message = NULL;
		m_tsErrorMessage = _T("");

		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
					  FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_MAX_WIDTH_MASK,
					  NULL, ErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
					  (LPTSTR)&Message, 1024, NULL);

		if (Message)
		{
			m_tsErrorMessage = Message;
			::LocalFree(Message);
		}

		return m_tsErrorMessage.c_str();
	}

	inline int  CSocket::GetPeerName(struct sockaddr* name, int* namelen)
	{
		int Result = ::getpeername(m_Socket, name, namelen);
		if (0 != Result)
			TRACE(_T("GetPeerName failed\n"));

		return Result;
	}

	inline int  CSocket::GetSockName(struct sockaddr* name, int* namelen)
	{
		int Result = ::getsockname(m_Socket, name, namelen);
		if (0 != Result)
			TRACE(_T("GetSockName Failed\n"));

		return Result;
	}

	inline int  CSocket::GetSockOpt(int level, int optname, char* optval, int* optlen)
	{
		int Result = ::getsockopt(m_Socket, level, optname, optval, optlen);
		if (0 != Result)
			TRACE(_T("GetSockOpt Failed\n"));

		return Result;
	}

	inline void CSocket::FreeAddrInfo( struct addrinfo* ai )
	{

#ifdef GetAddrInfo

		(*m_pfnFreeAddrInfo)(ai);

#else

		UNREFERENCED_PARAMETER(ai);

		throw CWinException(_T("getaddrinfo is not supported"));

#endif

	}

	inline int CSocket::ioCtlSocket(long cmd, u_long* argp)
	{
		int Result = ::ioctlsocket(m_Socket, cmd, argp);
		if (0 != Result)
			TRACE(_T("ioCtlSocket Failed\n"));

		return Result;
	}

	inline BOOL CSocket::IsIPV6Supported()
	{
		BOOL IsIPV6Supported = FALSE;

#ifdef GetAddrInfo

		if (m_pfnGetAddrInfo != 0 && m_pfnFreeAddrInfo != 0)
			IsIPV6Supported = TRUE;

#endif

		return IsIPV6Supported;
	}

	inline int CSocket::Listen(int backlog /*= SOMAXCONN*/)
	{
		int Result = ::listen(m_Socket, backlog);
		if (0 != Result)
			TRACE(_T("Listen Failed\n"));

		return Result;
	}

	inline int CSocket::Receive(TCHAR* buf, int len, int flags)
	{
		std::vector<char> vChar(len+1, '\0');
		char* pCharArray = &vChar.front();
		int Result = ::recv(m_Socket, pCharArray, len, flags);
		if (SOCKET_ERROR == Result)
			TRACE(_T("Receive failed\n"));

		lstrcpyn(buf, A2T(pCharArray), len);

		return Result;
	}

	inline int CSocket::ReceiveFrom(TCHAR* buf, int len, int flags, struct sockaddr* from, int* fromlen)
	//The ReceiveFrom function receives a datagram and stores the source address.
	{
		std::vector<char> vChar(len+1, '\0');
		char* pCharArray = &vChar.front();
		int Result = ::recvfrom(m_Socket, pCharArray, len, flags, from, fromlen);
		if (SOCKET_ERROR == Result)
			TRACE(_T("ReceiveFrom failed\n"));

		lstrcpyn(buf, A2T(pCharArray), len);

		return Result;
	}

	inline int CSocket::Send(LPCTSTR buf, int len, int flags)
	{
		int Result = ::send(m_Socket, T2A(buf), len, flags);
		if (SOCKET_ERROR == Result)
			TRACE(_T("Send failed\n"));

		return Result;
	}

	inline int CSocket::SendTo(LPCTSTR send, int len, int flags, LPCTSTR addr, LPCTSTR port)
	// The sendto function sends data to a specific destination.
	{
		int RetVal = 0;

		if (IsIPV6Supported())
		{

#ifdef GetAddrInfo	// Skip the following code block for older development environments

			ADDRINFO Hints= {0};
			Hints.ai_flags = AI_NUMERICHOST | AI_PASSIVE;
			ADDRINFO *AddrInfo;

			RetVal = GetAddrInfo(addr, port, &Hints, &AddrInfo);
			if (RetVal != 0)
			{
				TRACE( _T("GetAddrInfo failed\n"));
				return SOCKET_ERROR;
			}

			RetVal = ::sendto(m_Socket, T2A(send), len, flags, AddrInfo->ai_addr, (int)AddrInfo->ai_addrlen );
			if ( RetVal == SOCKET_ERROR )
			{
				TRACE(_T("SendTo failed\n"));
				return RetVal;
			}

			// Free the address information allocatied by GetAddrInfo
			FreeAddrInfo(AddrInfo);

#endif

		}
		else
		{
			sockaddr_in clientService;
			clientService.sin_family = AF_INET;
			clientService.sin_addr.s_addr = inet_addr( T2A(addr) );
			int nPort = -1;
            nPort = atoi( T2A(port));
			if (-1 == nPort)
			{
				TRACE(_T("Invalid port number\n"));
				return SOCKET_ERROR;
			}
			clientService.sin_port = htons( (u_short)nPort );

			RetVal = ::sendto( m_Socket, T2A(send), len, flags, (SOCKADDR*) &clientService, sizeof(clientService) );
			if ( SOCKET_ERROR != RetVal )
				TRACE(_T("SendTo failed\n"));
		}

		return RetVal;
	}

	inline int CSocket::SendTo(LPCTSTR buf, int len, int flags, const struct sockaddr* to, int tolen)
	// The sendto function sends data to a specific destination.
	{
		int Result =  ::sendto(m_Socket, T2A(buf), len, flags, to, tolen);
		if (SOCKET_ERROR == Result)
			TRACE(_T("SendTo failed\n"));

		return Result;
	}

	inline int CSocket::SetSockOpt(int level, int optname, const char* optval, int optlen)
	{
		int Result = ::setsockopt(m_Socket, level, optname, optval, optlen);
		if (0 != Result)
			TRACE(_T("SetSockOpt failed\n"));

		return Result;
	}

	inline void CSocket::StartEvents()
	{
		// This function starts the thread which monitors the socket for events.
		StopEvents();	// Ensure the thread isn't already running
		UINT ThreadID;	// a return variable required for Win95, Win98, WinME
		m_hEventThread = (HANDLE)::_beginthreadex(NULL, 0, CSocket::EventThread, (LPVOID) this, 0, &ThreadID);
	}

	inline void CSocket::StopEvents()
	{
		// Terminates the event thread gracefully (if possible)
		if (m_hEventThread)
		{
			::SetThreadPriority(m_hEventThread, THREAD_PRIORITY_HIGHEST);
			::SetEvent(m_StopRequest);

			for (;;)	// infinite loop
			{
				// wait for the Thread stopping event to be set
				if ( WAIT_TIMEOUT == ::WaitForSingleObject(m_Stopped, THREAD_TIMEOUT * 10) )
				{
					// Note: An excessive delay in processing any of the notification functions
					// can cause us to get here. (Yes one second is an excessive delay. Its a bug!)
					TRACE(_T("*** Error: Event Thread won't die ***\n") );
				}
				else break;
			}

			::CloseHandle(m_hEventThread);
			m_hEventThread = 0;
		}

		::ResetEvent(m_StopRequest);
		::ResetEvent(m_Stopped);
	}
}


#endif // #ifndef _WIN32XX_SOCKET_H_

