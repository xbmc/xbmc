/*
* Shade Worm Screen Saver for XBox Media Center
* Copyright (c) 2005 MrC
*
* Ver 1.0 27 Feb 2005	MrC
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


#include "ShadeWorm.h"

#pragma comment (lib, "lib/xbox_dx8.lib")

ShadeWorm_c*		g_shadeWorm = NULL;

struct SCR_INFO 
{
	int	dummy;
};

//-- Create -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreenSaverName)
{
	g_shadeWorm = new ShadeWorm_c;
	g_shadeWorm->Create(pd3dDevice, iWidth, iHeight, szScreenSaverName);

} // Create

//-- Start --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Start()
{
	srand(::GetTickCount());

	if (g_shadeWorm)
	{
		if (!g_shadeWorm->Start())
		{
			// Creation failure
			g_shadeWorm->Stop();
			delete g_shadeWorm;
			g_shadeWorm = NULL;
		}
	}

} // Start

//-- Render -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Render()
{
	if (g_shadeWorm)
		g_shadeWorm->Render();

} // Render

//-- Stop ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Stop()
{
	if (g_shadeWorm)
	{
		g_shadeWorm->Stop();
		delete g_shadeWorm;
		g_shadeWorm = NULL;
	}

} // Stop

//-- GetInfo ------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void GetInfo(SCR_INFO* pInfo)
{
	// not used, but can be used to pass info
	// back to XBMC if required in the future
	return;
}

extern "C" 
{

	struct ScreenSaver
	{
	public:
		void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver);
		void (__cdecl* Start) ();
		void (__cdecl* Render) ();
		void (__cdecl* Stop) ();
		void (__cdecl* GetInfo)(SCR_INFO *info);
	} ;


	void __declspec(dllexport) get_module(struct ScreenSaver* pScr)
	{
		pScr->Create = Create;
		pScr->Start = Start;
		pScr->Render = Render;
		pScr->Stop = Stop;
		pScr->GetInfo = GetInfo;
	}
};