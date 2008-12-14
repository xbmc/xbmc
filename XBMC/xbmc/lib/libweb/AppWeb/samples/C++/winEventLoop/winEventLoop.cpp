///
///	@file 	winEventLoop.cpp
/// @brief 	Embed the AppWeb server in a windows single-threaded 
///			program using the windows message event mechanism. 
///			NOTE: this will work in a multi-threaded program as well.
///
////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	The latest version of this code is available at http://www.mbedthis.com
//
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version.
//
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	See the GNU General Public License for more details at:
//	http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This General Public License does NOT permit incorporating this software 
//	into proprietary programs. If you are unable to comply with the GPL, a 
//	commercial license for this software and support services are available
//	from Mbedthis Software at http://www.mbedthis.com
//
/////////////////////////////// Includes ///////////////////////////////
#if WIN

#include	"appweb/appweb.h"

/////////////////////////////// Defines ////////////////////////////////

#define APP_NAME		"winEventLoop"
#define APP_TITLE		"Sample Windows Event Loop"
#define SOCKET_MESSAGE	WM_USER+32

//////////////////////////////// Locals ////////////////////////////////

static HINSTANCE	appInst;			// Current application instance 
static HWND			appHwnd;			// Application window handle 
static Mpr			*mp;

//////////////////////////// Forward Declarations //////////////////////

static void eventLoop();
static int	findInstance();
static int	initWindow();
static long msgProc(HWND hwnd, uint msg, uint wp, long lp);

/////////////////////////////////// Code ///////////////////////////////

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE junk, char *args, 
	int junk2)
{
	MaHttp		*http;					// Http service inside our app
	MaServer	*server;				// For the HTTP server
	Mpr			mpr("winEventLoop");	// Initialize the run time

	//
	//	Do the following two statements only if you want debug trace
	//
	mp = &mpr;

#if BLD_FEATURE_LOG
	mpr.addListener(new MprLogToFile());
	mpr.setLogSpec("error.log:4");
#endif

	if (findInstance()) {
		mprError(MPR_L, MPR_LOG, "Application is already active");
		return FALSE;
	}

	//
	//	Create the window object
	// 
	if (initWindow() < 0) {
		mprError(MPR_L, MPR_ERROR, 
			"Can't initialize application Window");
		return FALSE;
	}

	//
	//	Use windows async select and message dispatcher rather than 
	//	select()
	//
	mp->setAsyncSelectMode(MPR_ASYNC_SELECT);

	//
	//	Start the Mbedthis Portable Runtime and request single threading
	//	NOTE: this program can be compiled multi-threaded. If so, change
	//	the parameter to a "1".
	//
	mpr.start();

	//
	//	Create Http and Server objects for this application. We set the
	//	ServerName to be "default" and the initial serverRoot to be ".".
	//	This will be overridden in winEventLoop.conf.
	//
	http = new MaHttp();
	server = new MaServer(http, "default", ".");
	
	//
	//	Activate the copy module and handler
	//
	new MaCopyModule(0);

	//
	//	Configure the server with the configuration directive file
	//
	if (server->configure("winEventLoop.conf") < 0) {
		mprFprintf(MPR_STDERR, 
			"Can't configure the server. Error on line %d\n", 
			server->getLine());
		exit(2);
	}
	
	//
	//	Start the server
	//
	if (http->start() < 0) {
		mprFprintf(MPR_STDERR, "Can't start the server\n");
		exit(2);
	}

	//
	//	Service incoming requests until time to exit.
	//
	eventLoop();

	//
	//	Orderly shutdown
	//
	http->stop();
	mpr.stop(0);
	delete server;
	delete http;

	//
	//	MPR run-time will automatically stop and be cleaned up
	//
	return 0;
}

////////////////////////////////////////////////////////////////////////
//
//	Sample main event loop. This demonstrates how to integrate Mpr with 
//	your applications event loop using select()
//

void eventLoop()
{
	MSG		msg;
	int		till;

	//
	//	If single threaded or if you desire control over the event 
	//	loop, you should code an event loop similar to that below:
	//
	while (!mp->isExiting()) {

		if (mp->runTimers() > 0) {
			till = 0;
		} else {
			till = mp->getIdleTime();
		}

		//
		//	This will run tasks if poolThreads == 0 (single threaded). 
		//	If multithreaded, the thread pool will run tasks
		//
		if (mp->runTasks() > 0) {	// Returns > 0 if more work to do
			till = 0;				// So don't block in select
		}
		SetTimer(appHwnd, 0, till, NULL);

		//
		//	Socket events will be serviced in the msgProc
		//
		if (GetMessage(&msg, NULL, 0, 0) == 0) {
			//	WM_QUIT received
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

////////////////////////////////////////////////////////////////////////
//
//	See if an instance of this product is already running
//

static int findInstance()
{
	HWND	hwnd;

	hwnd = FindWindow(APP_NAME, APP_TITLE);
	if (hwnd) {
		if (IsIconic(hwnd)) {
			ShowWindow(hwnd, SW_RESTORE);
		}
		SetForegroundWindow(hwnd);
		return 1;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////
//
//	Initialize the applications's window
// 

static int initWindow()
{
	WNDCLASS 	wc;
	int			rc;

	wc.style			= CS_HREDRAW | CS_VREDRAW;
	wc.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	wc.cbClsExtra		= 0;
	wc.cbWndExtra		= 0;
	wc.hInstance		= (HINSTANCE) appInst;
	wc.hIcon			= NULL;
	wc.lpfnWndProc		= (WNDPROC) msgProc;
	wc.lpszMenuName		= wc.lpszClassName = APP_NAME;

	rc = RegisterClass(&wc);
	if (rc == 0) {
		mprError(MPR_L, MPR_ERROR, "Can't register windows class");
		return -1;
	}

	appHwnd = CreateWindow(APP_NAME, APP_TITLE, WS_OVERLAPPED,
		CW_USEDEFAULT, 0, 0, 0, NULL, NULL, appInst, NULL);

	if (! appHwnd) {
		mprError(MPR_L, MPR_ERROR, "Can't create window");
		return -1;
	}
	mp->setHwnd(appHwnd);
	mp->setSocketHwnd(appHwnd);
	mp->setSocketMessage(SOCKET_MESSAGE);

	//
	//	Uncomment these lines if you want to show the window 
	//		(not much help)
	//
	// 		ShowWindow(appHwnd, SW_MINIMIZE);
	// 		UpdateWindow(appHwnd);
	//
	return 0;
}

////////////////////////////////////////////////////////////////////////
//
//	Windows message processing loop
//

static long msgProc(HWND hwnd, uint msg, uint wp, long lp)
{
	int		sock, winMask;

	switch (msg) {
	case WM_DESTROY:
	case WM_QUIT:
		mprGetMpr()->terminate(1);
		break;
	
	case SOCKET_MESSAGE:
		sock = wp;
		winMask = LOWORD(lp);
		mprGetMpr()->serviceIO(sock, winMask);
		break;

	default:
		return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////
#endif // WIN

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
