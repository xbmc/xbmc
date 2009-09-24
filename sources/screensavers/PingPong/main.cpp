////////////////////////////////////////////////////////////////////////////
//
// PingPong Screensaver for XBox Media Center
// Copyright (c) 2005 Joakim Eriksson <je@plane9.com>
//
// Thanks goes to Warren for his 'TestXBS' program!
//
////////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "pingpong.h"
#include "XmlDocument.h"
#include "timer.h"
#include <time.h>

static char gScrName[1024];

CPingPong*		gPingPong = null;
CRenderD3D		gRender;
CTimer*			gTimer = null;
CRGBA			gCol[3];

extern "C" void Stop();

////////////////////////////////////////////////////////////////////////////
// XBMC has loaded us into memory, we should set our core values
// here and load any settings we may have from our config file
//
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int width, int height, const char* szScreenSaverName)
{
	strcpy(gScrName,szScreenSaverName);
	LoadSettings();

	gRender.m_D3dDevice = pd3dDevice;
	gRender.m_Width	= width;
	gRender.m_Height= height;
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us we should get ready to start rendering. This function
// is called once when the screensaver is activated by XBMC.
//
extern "C" void Start()
{
	srand((u32)time(null));
	gPingPong = new CPingPong();
	if (!gPingPong)
		return;

	gPingPong->m_Paddle[0].m_Col = gCol[0];
	gPingPong->m_Paddle[1].m_Col = gCol[1];
	gPingPong->m_Ball.m_Col = gCol[2];

	gTimer = new CTimer();
	gTimer->Init();
	if (!gPingPong->RestoreDevice(&gRender))
		Stop();
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us to render a frame of our screensaver. This is called on
// each frame render in XBMC, you should render a single frame only - the DX
// device will already have been cleared.
//
extern "C" void Render()
{
	if (!gPingPong)
		return;
	gTimer->Update();
	gPingPong->Update(gTimer->GetDeltaTime());
	gPingPong->Draw(&gRender);
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us to stop the screensaver we should free any memory and release
// any resources we have created.
//
extern "C" void Stop()
{
	if (!gPingPong)
		return;
	gPingPong->InvalidateDevice(&gRender);
	SAFE_DELETE(gPingPong);
	SAFE_DELETE(gTimer);
}

////////////////////////////////////////////////////////////////////////////
// Load settings from the [screensavername].xml configuration file
// the name of the screensaver (filename) is used as the name of
// the xml file - this is sent to us by XBMC when the Init func is called.
//
void LoadSettings()
{
	XmlNode node, childNode; //, grandChild;
	CXmlDocument doc;
	
	// Set up the defaults
	SetDefaults();

	char szXMLFile[1024];
	strcpy(szXMLFile, "Q:\\screensavers\\");
	strcat(szXMLFile, gScrName);
	strcat(szXMLFile, ".xml");

	// Load the config file
	if (doc.Load(szXMLFile) >= 0)
	{
		node = doc.GetNextNode(XML_ROOT_NODE);
		while(node > 0)
		{
			if (strcmpi(doc.GetNodeTag(node),"screensaver"))
			{
				node = doc.GetNextNode(node);
				continue;
			}

			if (childNode = doc.GetChildNode(node,"ColorPaddle1"))	sscanf(doc.GetNodeText(childNode), "%f %f %f", &gCol[0].r, &gCol[0].g, &gCol[0].b);
			if (childNode = doc.GetChildNode(node,"ColorPaddle2"))	sscanf(doc.GetNodeText(childNode), "%f %f %f", &gCol[1].r, &gCol[1].g, &gCol[1].b);
			if (childNode = doc.GetChildNode(node,"ColorBall"))		sscanf(doc.GetNodeText(childNode), "%f %f %f", &gCol[2].r, &gCol[2].g, &gCol[2].b);

			node = doc.GetNextNode(node);
		}
		doc.Close();
	}
}

////////////////////////////////////////////////////////////////////////////
// set any default values for your screensaver's parameters
//
void SetDefaults()
{
	for (int i=0; i<3; i++)
		gCol[i].Set(1.0f, 1.0f, 1.0f, 1.0f);
	return;
}

////////////////////////////////////////////////////////////////////////////
// not used, but can be used to pass info back to XBMC if required in the future
//
extern "C" void GetInfo(SCR_INFO* pInfo)
{
	return;
}

////////////////////////////////////////////////////////////////////////////
//
extern "C" void __declspec(dllexport) get_module(struct ScreenSaver* pScr)
{
	pScr->Create = Create;
	pScr->Start = Start;
	pScr->Render = Render;
	pScr->Stop = Stop;
	pScr->GetInfo = GetInfo;
}


