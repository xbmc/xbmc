
#include "stdafx.h"
#include "UdpClient.h"
#include "Log.h"
#include "graphiccontext.h"

CUdpClient::CUdpClient(void) : CThread()
{
}

CUdpClient::~CUdpClient(void)
{
	closesocket(client_socket);
	DeleteCriticalSection(&critical_section);
}

bool CUdpClient::Create(void)
{
	m_bStop = false;

	InitializeCriticalSection(&critical_section);

	CLog::Log(LOGINFO, "UDPCLIENT: Creating UDP socket...");	

	// Create a UDP socket
	client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_socket == SOCKET_ERROR)
	{
		CLog::Log(LOGINFO, "UDPCLIENT: Unable to create socket.");	
		return false;
	}

	CLog::Log(LOGINFO, "UDPCLIENT: Setting broadcast socket option...");	

	unsigned int value =  1;
	if( setsockopt( client_socket, SOL_SOCKET, SO_BROADCAST, (char*) &value, sizeof( unsigned int ) ) == SOCKET_ERROR)
	{
		CLog::Log(LOGINFO, "UDPCLIENT: Unable to set socket option.");
		return false;
	}

	CLog::Log(LOGINFO, "UDPCLIENT: Setting non-blocking socket options...");	

	unsigned long nonblocking=1;
	ioctlsocket(client_socket, FIONBIO, &nonblocking);

	CLog::Log(LOGINFO, "UDPCLIENT: Spawning listener thread...");	
	CThread::Create(false);

	CLog::Log(LOGINFO, "UDPCLIENT: Ready.");	

	return true;
}

void CUdpClient::OnStartup()
{
	SetPriority( THREAD_PRIORITY_LOWEST );
}

bool CUdpClient::Broadcast(int aPort, CStdString& aMessage)
{
	EnterCriticalSection(&critical_section);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(aPort);
	addr.sin_addr.s_addr = INADDR_BROADCAST;

	UdpCommand broadcast = {addr,aMessage,NULL,NULL};
	commands.push_back(broadcast);

	LeaveCriticalSection(&critical_section);
    return true;
}


bool CUdpClient::Send(CStdString aIpAddress, int aPort, CStdString& aMessage)
{
	EnterCriticalSection(&critical_section);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(aPort);
	addr.sin_addr.s_addr = inet_addr(aIpAddress);

	UdpCommand transmit = {addr,aMessage,NULL,NULL};
	commands.push_back(transmit);

	LeaveCriticalSection(&critical_section);
	return true;
}

bool CUdpClient::Send(SOCKADDR_IN aAddress, CStdString& aMessage)
{
	EnterCriticalSection(&critical_section);

	UdpCommand transmit = {aAddress,aMessage,NULL,NULL};
	commands.push_back(transmit);

	LeaveCriticalSection(&critical_section);
    return true;
}

bool CUdpClient::Send(SOCKADDR_IN aAddress, LPBYTE pMessage, DWORD dwSize)
{
	EnterCriticalSection(&critical_section);

	UdpCommand transmit = {aAddress,"",pMessage,dwSize};
	commands.push_back(transmit);

	LeaveCriticalSection(&critical_section);
    return true;
}


void CUdpClient::Process() 
{
	Sleep(2000);

	CLog::Log(LOGINFO, "UDPCLIENT: Listening.");	

	SOCKADDR_IN remoteAddress;
	char messageBuffer[1024];
	DWORD dataAvailable;

	while ( !m_bStop ) 
	{		
		// is there any data to read
		dataAvailable = 0;
		ioctlsocket(client_socket, FIONREAD, &dataAvailable);

		// while there is data to read
		while (dataAvailable>0)
		{
			// read data
			int messageLength = sizeof(messageBuffer) -1 ;
			int remoteAddressSize = sizeof(remoteAddress);

			int ret = recvfrom(client_socket, messageBuffer, messageLength, 0, (struct sockaddr *) &remoteAddress, &remoteAddressSize);
			if(ret!=SOCKET_ERROR)
			{
				// Packet received
				messageLength = ret;
				messageBuffer[messageLength] = '\0';

				CStdString message = messageBuffer;

				// NOTE: You should consider locking access to the screen device
				// or at least wait until after vertical refresh before firing off events
				// to protect access to graphics resources.  

				g_graphicsContext.Lock();
				OnMessage(remoteAddress,message,(LPBYTE)messageBuffer,messageLength);
				g_graphicsContext.Unlock();
			}
			else
			{
				CStdString debug;
				debug.Format("UDPCLIENT: Socket error %u",WSAGetLastError());
				CLog::Log(LOGDEBUG, debug);
			}

			// is there any more data to read?
			dataAvailable = 0;
			ioctlsocket(client_socket, FIONREAD, &dataAvailable);
		}

		// dispatch a single command if any pending
		DispatchNextCommand();
	}

	closesocket(client_socket);

	CLog::Log(LOGNOTICE, "UDPCLIENT: Stopped listening.");	
}


void CUdpClient::DispatchNextCommand()
{
	EnterCriticalSection(&critical_section);

	if (commands.size()<=0)
	{
		LeaveCriticalSection(&critical_section);

		// relinquish the remainder of this threads time slice
		Sleep(0);
		return;
	}

	COMMANDITERATOR it = commands.begin();
	UdpCommand command = *it;	
	commands.erase(it);
	LeaveCriticalSection(&critical_section);

	int ret;
	if (command.binarySize>0)
	{
		ret = sendto(client_socket, (LPCSTR) command.binary, command.binarySize, 0, (struct sockaddr *) &command.address, sizeof(command.address));
		delete[] command.binary;
	}
	else
	{
		ret = sendto(client_socket, command.message, command.message.GetLength(), 0, (struct sockaddr *) &command.address, sizeof(command.address));
	}
	if(ret == SOCKET_ERROR)
	{
		CLog::Log(LOGERROR, "UDPCLIENT: Unable to transmit data to host.");	
	}
}