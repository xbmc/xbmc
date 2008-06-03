#ifndef _TESTXBS_H_
#define _TESTXBS_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

//DirectX8 Includes
#pragma comment(lib, "D3d8.lib") //directX 8
#pragma comment(lib, "D3dx8.lib")
#include <d3d8.h>
#include <d3dx8.h>

//Resolution stuff, move to a startup dialog at some point
#define WindowXres 720
#define WindowYres 480
//#define WindowXres 1280
//#define WindowYres 1024
#define WindowXpos (GetSystemMetrics(SM_CXSCREEN)-WindowXres)/2
#define WindowYpos (GetSystemMetrics(SM_CYSCREEN)-WindowYres)/2

//XBS definitions, should just include xbsbase.h instead
struct SCR_INFO 
	{
		int	dummy;
	};
struct ScreenSaver
{
public:
	void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver);
	void (__cdecl* Start) ();
	void (__cdecl* Render) ();
	void (__cdecl* Stop) ();
	void (__cdecl* GetInfo)(SCR_INFO *info);
} ;

//XBS DLL stuff
HINSTANCE hDll;
typedef void (__cdecl *fd_get_module)(struct ScreenSaver* pScr);
fd_get_module get_module;
struct ScreenSaver xbsFuncs;

//DirectX8 variables
LPDIRECT3D8 g_pD3D;
LPDIRECT3DDEVICE8 g_pD3DDevice;
#endif
