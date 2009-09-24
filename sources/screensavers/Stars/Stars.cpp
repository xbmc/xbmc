/*
 * Pyro Screensaver for XBox Media Center
 * Copyright (c) 2004 Team XBMC
 *
 * Ver 1.0 15 Nov 2004	Chris Barnett (Forza)
 *
 * Adapted from the Pyro screen saver by
 *
 *  Jamie Zawinski <jwz@jwz.org>
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

#include "Stars.h"
#include "StarField.h"
#include "XmlDocument.h"

#include <stdio.h>
#include <math.h>

#pragma comment (lib, "lib/xbox_dx8.lib" )

CStarField* g_pStarField = NULL;

struct ST_SETTINGS
{
	char szScrName[1024];
	int iWidth;
	int iHeight;
	
	int   iNumStars;
	float fGamma;
	float fBrightness;
	float fSpeed;
	float fZoom;
	float fExpanse;
};

struct ST_SETTINGS g_Settings = 
{
	"", 0, 0, 1000, 1.f, 0.2f, 10.0f, 1.5f, 1.5f
};

LPDIRECT3DDEVICE8 g_pd3dDevice;


//////////////////////////////////////////////////////////////////////
// This is a quick and dirty hack to show a simple screensaver ...
//////////////////////////////////////////////////////////////////////

extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreenSaverName)
{
	g_Settings.iWidth = iWidth;
	g_Settings.iHeight = iHeight;
	strcpy(g_Settings.szScrName, szScreenSaverName);
	
	g_pd3dDevice = pd3dDevice;

	LoadSettings();
}

extern "C" void Start()
{
	srand(::GetTickCount());

	g_pStarField = new CStarField(g_Settings.iNumStars,
								  g_Settings.fGamma,
								  g_Settings.fBrightness,
								  g_Settings.fSpeed,
								  g_Settings.fZoom,
								  g_Settings.fExpanse);
	if (g_pStarField)
	{
		g_pStarField->SetD3DDevice(g_pd3dDevice);
		g_pStarField->Create(g_Settings.iWidth, g_Settings.iHeight);
	}

	return;
}

extern "C" void Render()
{	
	if (g_pStarField)
	{
		g_pStarField->RenderFrame();
	}
}

extern "C" void Stop()
{
	delete g_pStarField;
	g_pStarField = NULL;
}

void LoadSettings(void)
{
	XmlNode node, childNode;
	CXmlDocument doc;

	char szXMLFile[1024];
	strcpy(szXMLFile, "Q:\\screensavers\\");
	strcat(szXMLFile, g_Settings.szScrName);
	strcat(szXMLFile, ".xml");

	// Load the config file
	if (doc.Load(szXMLFile) >= 0)
	{
		node = doc.GetNextNode(XML_ROOT_NODE);

		while(node > 0)
		{
			if (strcmpi(doc.GetNodeTag(node), "screensaver"))
			{
				node = doc.GetNextNode(node);
				continue;
			}

			if (childNode = doc.GetChildNode(node, "NumStars"))
			{
				g_Settings.iNumStars = atoi(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node, "Gamma"))
			{
				g_Settings.fGamma = (float)atof(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node, "Brightness"))
			{
				g_Settings.fBrightness = (float)atof(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node, "Speed"))
			{
				g_Settings.fSpeed = (float)atof(doc.GetNodeText(childNode));
			}
			
			if (childNode = doc.GetChildNode(node, "Zoom"))
			{
				g_Settings.fZoom = (float)atof(doc.GetNodeText(childNode));
			}
	
			if (childNode = doc.GetChildNode(node, "Expanse"))
			{
				g_Settings.fExpanse = (float)atof(doc.GetNodeText(childNode));
			}
				
			node = doc.GetNextNode(node);
		}
		doc.Close();
	}
}

extern "C" void GetInfo(SCR_INFO* pInfo)
{
	return;
}

extern "C" 
{

struct ScreenSaver
{
public:
	void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver);
	void (__cdecl* Start)();
	void (__cdecl* Render)();
	void (__cdecl* Stop)();
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