/*CAsyncSocketEx by Tim Kosse (Tim.Kosse@gmx.de)
            Version 1.3 (2003-04-26)
--------------------------------------------------------

Introduction:
-------------

CAsyncSocketEx is a replacement for the MFC class CAsyncSocket.
This class was written because CAsyncSocket is not the fastest WinSock
wrapper and it's very hard to add new functionality to CAsyncSocket
derived classes. This class offers the same functionality as CAsyncSocket.
Also, CAsyncSocketEx offers some enhancements which were not possible with
CAsyncSocket without some tricks.

How do I use it?
----------------
Basically exactly like CAsyncSocket.
To use CAsyncSocketEx, just replace all occurrences of CAsyncSocket in your
code with CAsyncSocketEx. If you did not enhance CAsyncSocket yourself in
any way, you won't have to change anything else in your code.

Why is CAsyncSocketEx faster?
-----------------------------

CAsyncSocketEx is slightly faster when dispatching notification event messages.
First have a look at the way CAsyncSocket works. For each thread that uses
CAsyncSocket, a window is created. CAsyncSocket calls WSAAsyncSelect with
the handle of that window. Until here, CAsyncSocketEx works the same way.
But CAsyncSocket uses only one window message (WM_SOCKET_NOTIFY) for all
sockets within one thread. When the window recieve WM_SOCKET_NOTIFY, wParam
contains the socket handle and the window looks up an CAsyncSocket instance
using a map. CAsyncSocketEx works differently. It's helper window uses a
wide range of different window messages (WM_USER through 0xBFFF) and passes
a different message to WSAAsyncSelect for each socket. When a message in
the specified range is received, CAsyncSocketEx looks up the pointer to a
CAsyncSocketEx instance in an Array using the index of message - WM_USER.
As you can see, CAsyncSocketEx uses the helper window in a more efficient
way, as it don't have to use the slow maps to lookup it's own instance.
Still, speed increase is not very much, but it may be noticeable when using
a lot of sockets at the same time.
Please note that the changes do not affect the raw data throughput rate,
CAsyncSocketEx only dispatches the notification messages faster.

What else does CAsyncSocketEx offer?
------------------------------------

CAsyncSocketEx offers a flexible layer system. One example is the proxy layer.
Just create an instance of the proxy layer, configure it and add it to the layer
chain of your CAsyncSocketEx instance. After that, you can connect through
proxies.
Benefit: You don't have to change much to use the layer system.
Another layer that is currently in development is the SSL layer to establish
SSL encrypted connections.

License
-------

Feel free to use this class, as long as you don't claim that you wrote it
and this copyright notice stays intact in the source files.
If you use this class in commercial applications, please send a short message
to tim.kosse@gmx.de
*/

#if !defined(AFX_ASYNCSOCKETEX_H__AA9E4531_63B1_442F_9A71_09B2FEEDF34E__INCLUDED_)
#define AFX_ASYNCSOCKETEX_H__AA9E4531_63B1_442F_9A71_09B2FEEDF34E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define FD_FORCEREAD (1<<15)

#include "winsock.h"

class CAsyncSocketExHelperWindow;

#ifndef NOLAYERS
class CAsyncSocketExLayer;
#endif //NOLAYERS
class CCriticalSectionWrapper;
class CAsyncSocketEx
{
public:
	///////////////////////////////////////
	//Functions that imitate CAsyncSocket//
	///////////////////////////////////////

	//Construction
	//------------

	//Constructs a CAsyncSocketEx object.
	CAsyncSocketEx();
	virtual ~CAsyncSocketEx();

	//Creates a socket.
	BOOL Create(UINT nSocketPort = 0, int nSocketType = SOCK_STREAM,
				long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT |	FD_CONNECT | FD_CLOSE,
				LPCTSTR lpszSocketAddress = NULL );


	//Attributes
	//---------

	//Attaches a socket handle to a CAsyncSocketEx object.
	BOOL Attach( SOCKET hSocket,
				long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT |
				FD_CONNECT | FD_CLOSE );

	//Detaches a socket handle from a CAsyncSocketEx object.
	SOCKET Detach( );

	//Gets the error status for the last operation that failed.
	static int GetLastError();

	//Gets the address of the peer socket to which the socket is connected.
#ifdef _AFX
	BOOL GetPeerName( CString& rPeerAddress, UINT& rPeerPort );
#endif
	BOOL GetPeerName( SOCKADDR* lpSockAddr, int* lpSockAddrLen );

	//Gets the local name for a socket.
#ifdef _AFX
	BOOL GetSockName( CString& rSocketAddress, UINT& rSocketPort );
#endif
	BOOL GetSockName( SOCKADDR* lpSockAddr, int* lpSockAddrLen );

	//Retrieves a socket option.
	BOOL GetSockOpt(int nOptionName, void* lpOptionValue, int* lpOptionLen, int nLevel = SOL_SOCKET);

	//Sets a socket option.
	BOOL SetSockOpt(int nOptionName, const void* lpOptionValue, int nOptionLen, int nLevel = SOL_SOCKET);


	//Operations
	//----------

	//Accepts a connection on the socket.
	virtual BOOL Accept( CAsyncSocketEx& rConnectedSocket, SOCKADDR* lpSockAddr = NULL, int* lpSockAddrLen = NULL );

	//Requests event notification for the socket.
	BOOL AsyncSelect( long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE );

	//Associates a local address with the socket.
	BOOL Bind(UINT nSocketPort, LPCTSTR lpszSocketAddress);
	BOOL Bind(const SOCKADDR* lpSockAddr, int nSockAddrLen);

	//Closes the socket.
	virtual void Close();

	//Establishes a connection to a peer socket.
	virtual BOOL Connect(LPCTSTR lpszHostAddress, UINT nHostPort);
	virtual BOOL Connect( const SOCKADDR* lpSockAddr, int nSockAddrLen );

	//Controls the mode of the socket.
	BOOL IOCtl( long lCommand, DWORD* lpArgument );

	//Establishes a socket to listen for incoming connection requests.
	BOOL Listen( int nConnectionBacklog = 5 );

	//Receives data from the socket.
	virtual int Receive(void* lpBuf, int nBufLen, int nFlags = 0);

	//Sends data to a connected socket.
	virtual int Send(const void* lpBuf, int nBufLen, int nFlags = 0);

	//Disables Send and/or Receive calls on the socket.
	BOOL ShutDown( int nHow = sends );
	enum { receives = 0, sends = 1, both = 2 };

	//Overridable Notification Functions
	//----------------------------------

	//Notifies a listening socket that it can accept pending connection requests by calling Accept.
	virtual void OnAccept(int nErrorCode);

	//Notifies a socket that the socket connected to it has closed.
	virtual void OnClose(int nErrorCode);

	//Notifies a connecting socket that the connection attempt is complete, whether successfully or in error.
	virtual void OnConnect(int nErrorCode);

	//Notifies a listening socket that there is data to be retrieved by calling Receive.
	virtual void OnReceive(int nErrorCode);

	//Notifies a socket that it can send data by calling Send.
	virtual void OnSend(int nErrorCode);

	////////////////////////
	//Additional functions//
	////////////////////////

#ifndef NOLAYERS
	//Resets layer chain.
	void RemoveAllLayers();

	//Attaches a new layer to the socket.
	BOOL AddLayer(CAsyncSocketExLayer *pLayer);

	//Is a layer attached to the socket?
	BOOL IsLayerAttached() const;
#endif //NOLAYERS

	//Returns the handle of the socket.
	SOCKET GetSocketHandle();

	//Trigers an event on the socket
	// Any combination of FD_READ, FD_WRITE, FD_CLOSE, FD_ACCEPT, FD_CONNECT and FD_FORCEREAD is valid for lEvent.
	BOOL TriggerEvent(long lEvent);

protected:
	//Strucure to hold the socket data
	struct t_AsyncSocketExData
	{
		SOCKET hSocket; //Socket handle
		int nSocketIndex; //Index of socket, required by CAsyncSocketExHelperWindow
	} m_SocketData;

	//If using layers, only the events specified with m_lEvent will send to the event handlers.
	long m_lEvent;

	//AsyncGetHostByName
	char *m_pAsyncGetHostByNameBuffer; //Buffer for hostend structure
	HANDLE m_hAsyncGetHostByNameHandle; //TaskHandle
	int m_nAsyncGetHostByNamePort; //Port to connect to

	//Returns the handle of the helper window
	HWND GetHelperWindowHandle();

	//Attaches socket handle to helper window
	void AttachHandle(SOCKET hSocket);

	//Detaches socket handle to helper window
	void DetachHandle(SOCKET hSocket);

	//Critical section for thread synchronization
	static CCriticalSectionWrapper m_sGlobalCriticalSection;

	//Pointer to the data of the local thread
	struct t_AsyncSocketExThreadData
	{
		CAsyncSocketExHelperWindow *m_pHelperWindow;
		int nInstanceCount;
		DWORD nThreadId;
	} *m_pLocalAsyncSocketExThreadData;

	//List of the data structures for all threads
	static struct t_AsyncSocketExThreadDataList
	{
		t_AsyncSocketExThreadDataList *pNext;
		t_AsyncSocketExThreadData *pThreadData;
	} *m_spAsyncSocketExThreadDataList;

	//Initializes Thread data and helper window, fills m_pLocalAsyncSocketExThreadData
	BOOL InitAsyncSocketExInstance();

	//Destroys helper window after last instance of CAsyncSocketEx in current thread has been closed
	void FreeAsyncSocketExInstance();

#ifndef NOSOCKETSTATES
	int m_nPendingEvents;

	int GetState() const;
	void SetState(int nState);

	int m_nState;
#endif //NOSOCKETSTATES

#ifndef NOLAYERS
	//Layer chain
	CAsyncSocketExLayer *m_pFirstLayer;
	CAsyncSocketExLayer *m_pLastLayer;

	friend CAsyncSocketExLayer;

	//Called by the layers to notify application of some events
	virtual int OnLayerCallback(const CAsyncSocketExLayer *pLayer, int nType, int nParam1, int nParam2);
#endif //NOLAYERS

	friend CAsyncSocketExHelperWindow;
};

#ifndef NOLAYERS
#define LAYERCALLBACK_STATECHANGE 0
#define LAYERCALLBACK_LAYERSPECIFIC 1
#endif //NOLAYERS

enum SocketState
{
	notsock,
	unconnected,
	connecting,
	listening,
	connected,
	closed,
	aborted,
	attached
};

#endif // !defined(AFX_ASYNCSOCKETEX_H__AA9E4531_63B1_442F_9A71_09B2FEEDF34E__INCLUDED_)
