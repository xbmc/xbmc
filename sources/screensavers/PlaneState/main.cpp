////////////////////////////////////////////////////////////////////////////
//
// Planestate Screensaver for XBox Media Center
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
#include "planestate.h"
#include "XmlDocument.h"
#include "timer.h"
#include <time.h>

static char gScrName[1024];

CPlanestate*	gPlanestate = null;
CRenderD3D		gRender;
CTimer*			gTimer = null;
f32				gCfgProbability[NUMCFGS] = { 0.35f, 0.35f,0.15f, 0.15f };	// The probability that we pick a specific configuration. Should sum up to 1.0

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

	gPlanestate = null;
	gTimer = null;
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us we should get ready to start rendering. This function
// is called once when the screensaver is activated by XBMC.
//
extern "C" void Start()
{
	srand((u32)time(null));
	gPlanestate = new CPlanestate(gCfgProbability);
	if (!gPlanestate)
		return;
	gTimer = new CTimer();
	gTimer->Init();
	if (!gPlanestate->RestoreDevice(&gRender))
		Stop();
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us to render a frame of our screensaver. This is called on
// each frame render in XBMC, you should render a single frame only - the DX
// device will already have been cleared.
//
extern "C" void Render()
{
	if (!gPlanestate)
		return;
	gTimer->Update();
	gPlanestate->Update(gTimer->GetDeltaTime());
	gPlanestate->Draw(&gRender);
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us to stop the screensaver we should free any memory and release
// any resources we have created.
//
extern "C" void Stop()
{
	if (gPlanestate)
		gPlanestate->InvalidateDevice(&gRender);
	SAFE_DELETE( gPlanestate );
	SAFE_DELETE( gTimer );

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

			if (childNode = doc.GetChildNode(node,"CfgProbability1"))	gCfgProbability[0] = (f32)atof(doc.GetNodeText(childNode));
			if (childNode = doc.GetChildNode(node,"CfgProbability2"))	gCfgProbability[1] = (f32)atof(doc.GetNodeText(childNode));
			if (childNode = doc.GetChildNode(node,"CfgProbability3"))	gCfgProbability[2] = (f32)atof(doc.GetNodeText(childNode));
			if (childNode = doc.GetChildNode(node,"CfgProbability4"))	gCfgProbability[3] = (f32)atof(doc.GetNodeText(childNode));

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


