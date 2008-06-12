////////////////////////////////////////////////////////////////////////////
//
// Asteroids Screensaver for XBox Media Center
// Copyright (c) 2005 Joakim Eriksson <je@plane9.com>
//
// The TestXBS framework and program is made by Warren
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
#include "asteroids.h"
#include "XmlDocument.h"
#include "timer.h"
#include <time.h>

static char gScrName[1024];

CAsteroids*	gAsteroids = null;
CRenderD3D		gRender;
CTimer*			gTimer = null;
//CConfig			gConfig;

extern "C" void Stop();

// The states we change that we should restore
DWORD	gStoredState[][2] =
{
	{ D3DRS_ZENABLE, 0},
	{ D3DRS_LIGHTING, 0},
	{ D3DRS_COLORVERTEX, 0},
	{ D3DRS_FILLMODE, 0},
	{ D3DRS_ALPHABLENDENABLE, 0},
	{ D3DRS_MULTISAMPLEANTIALIAS, 0},
	{ D3DRS_EDGEANTIALIAS, 0},
	{ 0, 0}
};

////////////////////////////////////////////////////////////////////////////
// XBMC has loaded us into memory, we should set our core values
// here and load any settings we may have from our config file
//
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int width, int height, const char* szScreenSaverName)
{
	strcpy(gScrName,szScreenSaverName);
//	gConfig.SetDefaults();
//	gConfig.LoadSettings();

	gRender.Init();
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
	gAsteroids = new CAsteroids();
	if (!gAsteroids)
		return;
	gTimer = new CTimer();
	gTimer->Init();
	if (!gRender.RestoreDevice())				Stop();
	if (!gAsteroids->RestoreDevice(&gRender))	Stop();
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us to render a frame of our screensaver. This is called on
// each frame render in XBMC, you should render a single frame only - the DX
// device will already have been cleared.
//
extern "C" void Render()
{
	if (!gAsteroids)
		return;
	gTimer->Update();
	gAsteroids->Update(gTimer->GetDeltaTime());
	gAsteroids->Draw(&gRender);
	gRender.Draw();
}

////////////////////////////////////////////////////////////////////////////
// XBMC tells us to stop the screensaver we should free any memory and release
// any resources we have created.
//
extern "C" void Stop()
{
	if (!gAsteroids)
		return;
	gRender.InvalidateDevice();
	gAsteroids->InvalidateDevice(&gRender);
	SAFE_DELETE(gAsteroids);
	SAFE_DELETE(gTimer);
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

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
void	CConfig::SetDefaults()
{
}

////////////////////////////////////////////////////////////////////////////
// Load settings from the [screensavername].xml configuration file
// the name of the screensaver (filename) is used as the name of
// the xml file - this is sent to us by XBMC when the Init func is called.
//
void	CConfig::LoadSettings()
{
	XmlNode node; //, childNode; //, grandChild;
	CXmlDocument doc;
	
	char szXMLFile[1024];
#ifdef _TEST
	strcpy(szXMLFile, "Asteroids.xml");
#else
	strcpy(szXMLFile, "Q:\\screensavers\\");
	strcat(szXMLFile, gScrName);
	strcat(szXMLFile, ".xml");
#endif

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

			node = doc.GetNextNode(node);
		}
		doc.Close();
	}
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// 
void	CRenderD3D::Init()
{
	m_NumLines = 0;
	m_Verts = null;
}

////////////////////////////////////////////////////////////////////////////
// 
bool		CRenderD3D::RestoreDevice()
{
	LPDIRECT3DDEVICE8	d3dDevice = GetDevice();

	for (int i=0; gStoredState[i][0] != 0; i++)
		d3dGetRenderState((D3DRENDERSTATETYPE)gStoredState[i][0], &gStoredState[i][1]);

	m_D3dDevice->CreateVertexBuffer( 2*NUMLINES*sizeof(TRenderVertex), D3DUSAGE_WRITEONLY|D3DUSAGE_DYNAMIC, TRenderVertex::FVF_Flags, D3DPOOL_DEFAULT, &m_VertexBuffer );
	return true;
}

////////////////////////////////////////////////////////////////////////////
// 
void			CRenderD3D::InvalidateDevice()
{
	LPDIRECT3DDEVICE8	d3dDevice = GetDevice();

	SAFE_RELEASE( m_VertexBuffer ); 

	for (int i=0; gStoredState[i][0] != 0; i++)
		d3dSetRenderState((D3DRENDERSTATETYPE)gStoredState[i][0], gStoredState[i][1]);
}

////////////////////////////////////////////////////////////////////////////
// 
bool			CRenderD3D::Draw()
{
	LPDIRECT3DDEVICE8	d3dDevice = GetDevice();

	if (m_NumLines == 0)
		return true;

	m_VertexBuffer->Unlock();
	m_Verts = null;

	// Setup our texture
	d3dSetTextureStageState(0, D3DTSS_COLOROP,	 D3DTOP_SELECTARG1);
	d3dSetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	d3dSetTextureStageState(0, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE);
	d3dSetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(0, D3DTSS_MIPFILTER, D3DTEXF_NONE);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP);
	d3dSetTextureStageState(1, D3DTSS_COLOROP,	 D3DTOP_DISABLE);
	d3dSetTextureStageState(1, D3DTSS_ALPHAOP,	 D3DTOP_DISABLE);

	d3dSetRenderState(D3DRS_ZENABLE,	FALSE);
	d3dSetRenderState(D3DRS_LIGHTING,	FALSE);
	d3dSetRenderState(D3DRS_COLORVERTEX,TRUE);
	d3dSetRenderState(D3DRS_FILLMODE,    D3DFILL_SOLID );

#ifdef _TEST
	d3dSetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
#else
	// Active line AA (Actually polygon edge AA) This exists only on XBox
	d3dSetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	d3dSetRenderState(D3DRS_MULTISAMPLEANTIALIAS, FALSE);
	d3dSetRenderState(D3DRS_EDGEANTIALIAS, TRUE);
#endif

	d3dSetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	m_D3dDevice->SetTexture( 0, null );
	m_D3dDevice->SetStreamSource(	0, m_VertexBuffer, sizeof(TRenderVertex) );
	m_D3dDevice->SetVertexShader( TRenderVertex::FVF_Flags	);

	CMatrix	m;
	m.Identity();
	m_D3dDevice->SetTransform(D3DTS_PROJECTION, &m);
	m_D3dDevice->SetTransform(D3DTS_VIEW, &m);
	m_D3dDevice->SetTransform(D3DTS_WORLD, &m);

	m_D3dDevice->DrawPrimitive( D3DPT_LINELIST,	0, m_NumLines );

	m_NumLines = 0;
	return true;
}

////////////////////////////////////////////////////////////////////////////
// 
void			CRenderD3D::DrawLine(const CVector2& pos1, const CVector2& pos2, const CRGBA& col1, const CRGBA& col2)
{
	if (m_NumLines >= NUMLINES)
	{
		Draw();
	}

	if (m_Verts == null)
	{
		m_VertexBuffer->Lock( 0, 0, (BYTE**)&m_Verts, 0);
	}

	m_Verts->x = pos1.x; m_Verts->y = pos1.y; m_Verts->z = 0.0f; m_Verts->w = 0.0f; m_Verts->col = col1.RenderColor();	m_Verts++;
	m_Verts->x = pos2.x; m_Verts->y = pos2.y; m_Verts->z = 0.0f; m_Verts->w = 0.0f; m_Verts->col = col2.RenderColor();	m_Verts++;
	
	m_NumLines++;
}
