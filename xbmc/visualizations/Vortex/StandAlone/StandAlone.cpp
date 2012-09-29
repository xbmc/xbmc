/*
 *  Copyright © 2010-2012 Team XBMC
 *  http://xbmc.org
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <windows.h>
#include <d3d9.h>
#include "Vortex.h"
#include <math.h>

IDirect3D9*	pD3D9 = NULL;
IDirect3DDevice9* pD3DDevice = NULL;
HWND gHWND = NULL;

Vortex g_Vortex;

void CreateDevice( int iWidth, int iHeight )
{
	pD3D9 = Direct3DCreate9( D3D_SDK_VERSION );
	D3DDISPLAYMODE mode;
	pD3D9->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &mode );

//  	iWidth = mode.Width;
//  	iHeight = mode.Height;
//  	bool bWindowed = TRUE;

	D3DPRESENT_PARAMETERS PresentParameters;
	memset( &PresentParameters, 0, sizeof( PresentParameters ) );

	PresentParameters.BackBufferCount			=	1;
	PresentParameters.BackBufferFormat			=	mode.Format;
	PresentParameters.BackBufferWidth			=	iWidth;
	PresentParameters.BackBufferHeight			=	iHeight;
	PresentParameters.SwapEffect				=	D3DSWAPEFFECT_COPY;
	PresentParameters.Flags						=	0;
	PresentParameters.EnableAutoDepthStencil	=	TRUE;
	PresentParameters.AutoDepthStencilFormat	=	D3DFMT_D24X8;
	PresentParameters.Windowed					=	true;
	PresentParameters.PresentationInterval		=	D3DPRESENT_INTERVAL_ONE;
	PresentParameters.hDeviceWindow				=	( HWND )gHWND;

	pD3D9->CreateDevice( D3DADAPTER_DEFAULT,
						 D3DDEVTYPE_HAL,
						 ( HWND )gHWND,
						 D3DCREATE_HARDWARE_VERTEXPROCESSING,
						 &PresentParameters,
						 &pD3DDevice );
}

LRESULT CALLBACK StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uMsg )
	{
	case WM_CLOSE:
		{
			HMENU hMenu;
			hMenu = GetMenu( hWnd );
			if( hMenu != NULL )
				DestroyMenu( hMenu );
			DestroyWindow( hWnd );
			UnregisterClass( "Direct3DWindowClass", NULL );
			return 0;
		}

	case WM_DESTROY:
		PostQuitMessage( 0 );
		break;
	}

	return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

void RenderFrame()
{

	pD3DDevice->BeginScene();

	short waves[2][576];
	static float sin1 = 0;
	static float sin2 = 0;

	sin1 += 10;
	sin2 += 20;

	static short iCurrent = 0;
	for ( int i=0; i < 576; i++)
	{
// 		if ( ( rand() % 10) > 4)
// 			iCurrent += (short)(rand() % (255));
// 		else
// 			iCurrent -= (short)(rand() % (255));
//		iCurrent = sinf(sin1) * (1 << 11);
//		iCurrent += sinf(sin2) * (1 << 11);
		waves[0][i] = (rand() % 128 ) << 10;//iCurrent;//iCurrent;
		waves[1][i] = (rand() % 128 ) << 10;//iCurrent;//iCurrent;
	}
	sin1 += 0.2f;
	sin2 += 0.3f;

	g_Vortex.AudioData(&waves[0][0], 576, NULL, 0);

	g_Vortex.Render();

	pD3DDevice->EndScene();

	pD3DDevice->Present( NULL, NULL, 0, NULL );
}

void MainLoop()
{
	bool bGotMsg;
	MSG msg;
	msg.message = WM_NULL;
	PeekMessage( &msg, NULL, 0U, 0U, PM_NOREMOVE );

	while( WM_QUIT != msg.message )
	{
		// Use PeekMessage() so we can use idle time to render the scene. 
		bGotMsg = ( PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE ) != 0 );

		if( bGotMsg )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
		else
		{
			// Render a frame during idle time (no messages are waiting)
			RenderFrame();
		}
	}
}


int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	// Register the windows class
	WNDCLASS wndClass;
	wndClass.style = CS_DBLCLKS;
	wndClass.lpfnWndProc = StaticWndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInstance;
	wndClass.hIcon = NULL;
	wndClass.hCursor = LoadCursor( NULL, IDC_ARROW );
	wndClass.hbrBackground = ( HBRUSH )GetStockObject( BLACK_BRUSH );
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = "Direct3DWindowClass";

	if( !RegisterClass( &wndClass ) )
	{
		DWORD dwError = GetLastError();
		if( dwError != ERROR_CLASS_ALREADY_EXISTS )
			return -1;
	}

	// Find the window's initial size, but it might be changed later
	int nDefaultWidth = 640;
	int nDefaultHeight = 360;

	RECT rc;
	SetRect( &rc, 0, 0, nDefaultWidth, nDefaultHeight );
	AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, false );

	// Create the render window
	HWND hWnd = CreateWindow( "Direct3DWindowClass", "Vortex", WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, ( rc.right - rc.left ), ( rc.bottom - rc.top ), 0,
		NULL, hInstance, 0 );
	if( hWnd == NULL )
	{
		DWORD dwError = GetLastError();
		return -1;
	}
	gHWND = hWnd;

	ShowWindow( hWnd, SW_SHOW );

	CreateDevice( nDefaultWidth, nDefaultHeight );

	extern char g_TexturePath[];
	extern char g_PresetPath[];

	strcpy_s( g_TexturePath, 511, "Textures//" );
	strcpy_s( g_PresetPath, 511, "Presets//" );

	g_Vortex.Init( pD3DDevice, 0, 0, nDefaultWidth, nDefaultHeight, 1.0f);

	MainLoop();

	g_Vortex.Shutdown();

	pD3DDevice->Release();
	pD3D9->Release();

	_CrtDumpMemoryLeaks();

	return 0;
}
