
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

	// Create a UDP socket
	client_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (client_socket == SOCKET_ERROR)
	{
		CLog::Log("UDPCLIENT: Unable to create socket.");	
		return false;
	}

	unsigned int value =  1;
	if( setsockopt( client_socket, SOL_SOCKET, SO_BROADCAST, (char*) &value, sizeof( unsigned int ) ) == SOCKET_ERROR)
	{
		CLog::Log("UDPCLIENT: Unable to set socket option.");
		return false;
	}

	unsigned long nonblocking=1;
	ioctlsocket(client_socket, FIONBIO, &nonblocking);

	CThread::Create(false);
	return true;
}

bool CUdpClient::Broadcast(int aPort, CStdString& aMessage)
{
	EnterCriticalSection(&critical_section);

	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(aPort);
	addr.sin_addr.s_addr = INADDR_BROADCAST;

	UdpCommand broadcast = {addr,aMessage};
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

	UdpCommand transmit = {addr,aMessage};
	commands.push_back(transmit);

	LeaveCriticalSection(&critical_section);
	return true;
}

bool CUdpClient::Send(SOCKADDR_IN aAddress, CStdString& aMessage)
{
	EnterCriticalSection(&critical_section);

	UdpCommand transmit = {aAddress,aMessage};
	commands.push_back(transmit);

	LeaveCriticalSection(&critical_section);
    return true;
}


void CUdpClient::Process() 
{
	CLog::Log("UDPCLIENT: Listening.");	

	SOCKADDR_IN remoteAddress;
	char messageBuffer[513];
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
				OnMessage(remoteAddress,message);
				g_graphicsContext.Unlock();
			}
			else
			{
				CStdString debug;
				debug.Format("UDPCLIENT: Socket error %u",WSAGetLastError());
				CLog::Log(debug);
			}

			// is there any more data to read?
			dataAvailable = 0;
			ioctlsocket(client_socket, FIONREAD, &dataAvailable);
		}

		// dispatch a single command if any pending
		DispatchNextCommand();
	}

	CLog::Log("UDPCLIENT: Stopped listening.");	
}


void CUdpClient::DispatchNextCommand()
{
	EnterCriticalSection(&critical_section);

	if (commands.size()<=0)
	{
		LeaveCriticalSection(&critical_section);

		// wait for 1/2 second - not strictly necessary but gives some cpu back
		// and allows several kai engine responses to queue up so we can read them
		// in on go, as opposed to in real time (which isn't all that visually appealing
		// in the current usage model).
		Sleep(500);
		return;
	}

	COMMANDITERATOR it = commands.begin();
	UdpCommand command = *it;	
	commands.erase(it);
	LeaveCriticalSection(&critical_section);

	int ret = sendto(client_socket, command.message, command.message.GetLength(), 0, (struct sockaddr *) &command.address, sizeof(command.address));
	if(ret == SOCKET_ERROR)
	{
		CLog::Log("UDPCLIENT: Unable to transmit data to host.");	
	}
}