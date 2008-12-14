/*!
 *	@file 	winEventLoop.c
 *  @brief 	Embed the AppWeb server in a windows single-threaded program 
 * 			using the windows message event mechanism. NOTE: this will 
 * 			work in a multi-threaded program as well.
 */
/*************************************************************************/
/*
 *	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
 *	The latest version of this code is available at http://www.mbedthis.com
 *
 *	This software is open source; you can redistribute it and/or modify it 
 *	under the terms of the GNU General Public License as published by the 
 *	Free Software Foundation; either version 2 of the License, or (at your 
 *	option) any later version.
 *
 *	This program is distributed WITHOUT ANY WARRANTY; without even the 
 *	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 *	See the GNU General Public License for more details at:
 *	http://www.mbedthis.com/downloads/gplLicense.html
 *	
 *	This General Public License does NOT permit incorporating this software 
 *	into proprietary programs. If you are unable to comply with the GPL, a 
 *	commercial license for this software and support services are available
 *	from Mbedthis Software at http://www.mbedthis.com
 */
/****************************** Includes ******************************/
#if WIN

#define		UNSAFE_FUNCTIONS_OK 1

#include	"appweb/appweb.h"

/******************************* Defines ******************************/

#define APP_NAME		"winEventLoop"
#define APP_TITLE		"Sample Windows Event Loop"
#define SOCKET_MESSAGE	WM_USER+32

/******************************** Locals ******************************/

static HINSTANCE	appInst;		/* Current application instance  */
static HWND			appHwnd;		/* Application window handle */

/************************** Forward Declarations **********************/

static int	findInstance();
static int	initWindow();
static void eventLoop();
static long msgProc(HWND hwnd, uint msg, uint wp, long lp);

/********************************* Code *******************************/

int APIENTRY WinMain(HINSTANCE inst, HINSTANCE junk, char *args, 
	int junk2)
{
	MaHttp		*http;		/* For the http service inside our app */
	MaServer	*server;	/* For a HTTP server */

	/*
	 *	Initialize the run-time and give our app a name 
	 *	"winEventLoop"
	 */
	mprCreateMpr("winEventLoop");

#if BLD_FEATURE_LOG
	/*
	 *	Do the following two statements only if you want debug trace
	 */
	mprAddLogFileListener();
	mprSetLogSpec("error.log:4");
#endif

	if (findInstance()) {
		mprError(MPR_L, MPR_LOG, "Application is already active");
		return FALSE;
	}

	/*
	 *	Create the window object
	 */ 
	if (initWindow() < 0) {
		mprError(MPR_L, MPR_ERROR, 
			"Can't initialize application Window");
		return FALSE;
	}

	/*
	 *	Use windows async select and message dispatcher rather than 
	 *	select()
	 */
	mprSetAsyncSelectMode(MPR_ASYNC_SELECT);

	/*
	 *	Start run-time services
	 */
	mprStartMpr(0);

	/*
	 *	Create the HTTP and server objects. Give the server a name 
	 *	"default" and define "." as the default serverRoot, ie. the 
	 *	directory with the server configuration files.
	 */
	http = maCreateHttp();
	server = maCreateServer(http, "default", ".");
	
	/*
	 *	Activate the handlers. Only needed when linking statically.
	 */
	mprCopyInit(0);

	/*
	 *	Configure the server based on the directives in 
	 *	winEventLoop.conf.
	 */
	if (maConfigureServer(server, "winEventLoop.conf") < 0) {
		fprintf(stderr, 
			"Can't configure the server. Error on line %d\n", 
			maGetConfigErrorLine(server));
		exit(2);
	}

	/*
	 *	Start serving pages. After this we are live.
	 */
	if (maStartServers(http) < 0) {
		fprintf(stderr, "Can't start the server\n");
		exit(2);
	}

	/*
	 *	Service events. This call will block until the server is exited
	 *	Call mprTerminate() at any time to instruct the server to exit.
	 */
	eventLoop();

	/*
	 *	Stop all HTTP services
	 */
	maStopServers(http);
	mprStopMpr();

	/*
	 *	Delete the server and http objects
	 */
	maDeleteServer(server);
	maDeleteHttp(http);

	/*
	 *	Stop and delete the run-time services
	 */
	mprDeleteMpr();

	return 0;
}

/**********************************************************************/
/*
 *	Sample main event loop using select. This demonstrates how to 
 *	integrate AppWeb with your applications event loop using select()
 */

static void eventLoop()
{
	MSG		msg;
	int		timeout, till;

	/*
	 *	We will nap for 50 milliseconds to avoid busy waiting
	 */
	timeout = 50;

	while (!mprIsExiting()) {

		if (mprRunTimers() > 0) {
			till = 0;
		} else {
			till = mprGetIdleTime();
		}

		/*
		 *	This will run tasks if maxThreads == 0 (single threaded). 
		 *	If multithreaded, the thread pool will run tasks
		 */
		if (mprRunTasks() > 0) {	/* Returns > 0 if more work to do */
			till = 0;				/* So don't block in select */
		}

		SetTimer(appHwnd, 0, till, NULL);

		/*
		 *	Socket events will be serviced in the msgProc
		 */
		if (GetMessage(&msg, NULL, 0, 0) == 0) {
			/*	WM_QUIT received */
			break;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

/**********************************************************************/
/*
 *	See if an instance of this product is already running
 */

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

/**********************************************************************/
/*
 *	Initialize the applications's window
 */ 

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
	mprSetHwnd(appHwnd);
	mprSetSocketHwnd(appHwnd);
	mprSetSocketMessage(SOCKET_MESSAGE);

	/*
	 *	Uncomment these lines to show the window (not much help)
	 *
	 * 		ShowWindow(appHwnd, SW_MINIMIZE);
	 * 		UpdateWindow(appHwnd);
	 */

	return 0;
}

/**********************************************************************/
/*
 *	Windows message processing loop
 */

static long msgProc(HWND hwnd, uint msg, uint wp, long lp)
{
	int		sock, winMask;

	switch (msg) {
	case WM_DESTROY:
	case WM_QUIT:
		mprTerminate(1);
		break;
	
	case SOCKET_MESSAGE:
		sock = wp;
		winMask = LOWORD(lp);
		mprServiceWinIO(sock, winMask);
		break;

	default:
		return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 0;
}

/**********************************************************************/
#endif /* WIN */

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
