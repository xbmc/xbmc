#include <xtl.h>
#include "XGraphics.h"
#include <stdio.h>
#include "XmlDocument.h"

#include "ShadeWorm.h"


// Pixel shader for colour remapping
const char* ShadeWorm_c::m_shaderColourMapSrc = 
			"xps.1.1\n"
			"tex t0\n"
			"texreg2ar t1, t0\n"
			"mul r0, v0.a, t1\n"
			"xfc zero, zero, zero, r0.rgb, zero, zero, 1-zero\n";


ShadeWorm_c::ShadeWorm_c()
{
	m_spriteTexture = NULL;
	m_bufferTexture = NULL;
	m_colMapTexture = NULL;
	m_pShader		= 0;
	m_pUcode		= NULL;
	m_numColMaps	= 0;
}

ShadeWorm_c::~ShadeWorm_c()
{

}

//-- Create -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void ShadeWorm_c::Create(LPDIRECT3DDEVICE8 device, int width, int height, const char* saverName)
{
	m_device = device;
	m_width = width;
	m_height = height;
	strcpy(m_saverName, saverName);

} // Create

//-- Start --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
bool ShadeWorm_c::Start()
{
	if (D3D_OK != D3DXCreateTextureFromFile(m_device, "q:\\ScreenSavers\\MrC\\Particle.bmp", &m_spriteTexture))
		return false;

	if (D3D_OK  != m_device->CreateTexture(512, 512, 1, 0, D3DFMT_X8R8G8B8, 0, &m_bufferTexture))
		return false;

	if (D3D_OK  != m_device->CreateTexture(1, 256, 1, 0, D3DFMT_X8R8G8B8, 0, &m_colMapTexture))
		return false;

	if (S_OK != XGAssembleShader(NULL, m_shaderColourMapSrc, strlen(m_shaderColourMapSrc), 0, NULL, &m_pUcode, NULL, NULL, NULL, NULL, NULL))
		return false;

	if (D3D_OK  != m_device->CreatePixelShader((D3DPIXELSHADERDEF*)(DWORD*)m_pUcode->pData, &m_pShader))
	{
		return false;
	}

	LoadSettings(m_saverName);

	if (m_numColMaps == 0)
		return false;

	return true;

} // Start

//-- Stop ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void ShadeWorm_c::Stop()
{
	if (m_pShader)
		m_device->DeletePixelShader(m_pShader);

	if (m_pUcode)
		m_pUcode->Release();

	if (m_colMapTexture)
		m_colMapTexture->Release();

	if (m_bufferTexture)
		m_bufferTexture->Release();

	if (m_spriteTexture)
		m_spriteTexture->Release();

	for (int i = 0; i < m_numColMaps; i++)
	{
		delete[] m_colMaps[i].m_colMap;
	}

} // Stop

//-- Render -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void ShadeWorm_c::Render()
{
	static State_e state = STATE_NEWCOL;
	static int fadeCol = 0;

	IDirect3DSurface8* oldSurface;
	IDirect3DSurface8* newSurface;
	m_device->GetRenderTarget(&oldSurface);
	oldSurface->Release();

	m_bufferTexture->GetSurfaceLevel(0, &newSurface);
	m_device->SetRenderTarget(newSurface, NULL);
	newSurface->Release();


	if (state == STATE_NEWCOL)
	{
		if (m_randomColMap)
		{
			int newCol = rand() % m_numColMaps;
			if (newCol == m_colMap)
				m_colMap = (m_colMap+1) % m_numColMaps;
			else
				m_colMap = newCol;
		}
		else
		{
			m_colMap = (m_colMap+1) % m_numColMaps;
		}
		CreateColMap(m_colMap);

		m_device->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L );

		float x = 256.0f + (100 - (rand() % 200));
		float y = 256.0f + (100 - (rand() % 200));

		for (int i = 0; i < m_numWorms; i++)
		{
			m_worms[i].vx = 0;
			m_worms[i].vy = 0;
			m_worms[i].x = x;
			m_worms[i].y = y;
		}

		state = STATE_FADEUP;
		m_timer = 60 * m_drawTime;
	}

	d3dSetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	d3dSetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	d3dSetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	d3dSetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
	d3dSetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	d3dSetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );

	d3dSetRenderState(D3DRS_ALPHABLENDENABLE, true);
	d3dSetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	d3dSetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	d3dSetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

	d3dSetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
	d3dSetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
	d3dSetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);
	d3dSetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);

	if (state == STATE_FADEDOWN || state == STATE_DRAWING)
	{
		for (int i = 0; i < m_numWorms; i++)
		{
			m_worms[i].vx += GetRand();
			Clamp(-1, 1, m_worms[i].vx);
			m_worms[i].vy += GetRand();
			Clamp(-1, 1, m_worms[i].vy);

			m_worms[i].x += m_worms[i].vx;
			Clamp(0, 512, m_worms[i].x);
			m_worms[i].y += m_worms[i].vy;
			Clamp(0, 512, m_worms[i].y);
		}

		m_device->SetTexture(0, m_spriteTexture);

		for (int i = 0; i < m_numWorms; i++)
			RenderSprite(m_worms[i].x , m_worms[i].y, 5, 5, 0x05ffffff);
	}

	if (state == STATE_FADEUP)
	{
		fadeCol += 0x08;
		if (fadeCol >= 0xff)
		{
			fadeCol = 0xff;
			state = STATE_DRAWING;
		}
	}
	else if (state == STATE_FADEDOWN)
	{
		fadeCol -= 0x08;
		if (fadeCol <= 0)
		{
			fadeCol = 0;
			state = STATE_NEWCOL;
		}
	}
	else if (state == STATE_DRAWING)
	{
		m_timer--;
		if (m_timer <= 0)
			state = STATE_FADEDOWN;
	}

	d3dSetRenderState(D3DRS_ALPHABLENDENABLE, false);

	m_device->SetRenderTarget(oldSurface, NULL);

	m_device->SetTexture(0, m_bufferTexture);
	m_device->SetTexture(1, m_colMapTexture);

	d3dSetTextureStageState(1, D3DTSS_MAGFILTER, D3DTEXF_POINT);
	d3dSetTextureStageState(1, D3DTSS_MINFILTER, D3DTEXF_POINT);
	d3dSetTextureStageState(1, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP);
	d3dSetTextureStageState(1, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP);
	m_device->SetPixelShader(m_pShader);

	float size = fadeCol / 255.0f;
	RenderSprite(m_width/2.0f, m_height/2.0f, m_width/(2.0f * size), m_height/(2.0f * size), fadeCol << 24);

	m_device->SetPixelShader(0);
	m_device->SetTexture(0, NULL);
	m_device->SetTexture(1, NULL);

} // Render

//-- LoadSettings -------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void ShadeWorm_c::LoadSettings(const char* name)
{
	XmlNode node;
	CXmlDocument doc;
	int colId;
	int index;
	int col;

	// Set up the defaults
	m_numWorms = 16;
	m_drawTime = 8;
	m_numColMaps = 0;
	m_randomColMap = false;

	char szXMLFile[1024];
	strcpy(szXMLFile, "Q:\\screensavers\\");
	strcat(szXMLFile, name);
	strcat(szXMLFile, ".xml");

	// Load the config file
	if (doc.Load(szXMLFile) >= 0)
	{
		node = doc.GetNextNode(XML_ROOT_NODE);
		while(node > 0)
		{
			if (!strcmpi(doc.GetNodeTag(node),"NumWorms"))
			{
				m_numWorms = atoi(doc.GetNodeText(node));
				Clamp(1, 32, m_numWorms);
			}
			else if (!strcmpi(doc.GetNodeTag(node),"DrawTime"))
			{
				m_drawTime = atoi(doc.GetNodeText(node));
				Clamp(1, 10, m_drawTime);
			}
			else if (!strcmpi(doc.GetNodeTag(node),"RandomColMap"))
			{
				m_randomColMap = !strcmpi(doc.GetNodeText(node),"true");
			}
			else if (!strcmpi(doc.GetNodeTag(node),"Map"))
			{
				node = doc.GetNextNode(node);

				if (m_numColMaps < MAX_COLMAPS)
				{

					colId = 0;
					m_colMaps[m_numColMaps].m_numCols = 0;
					m_colMaps[m_numColMaps].m_colMap = new ColMapEntry_t[256];

					while (node>0)
					{
						if (!strcmpi(doc.GetNodeTag(node),"Col"))
						{
							if (colId < 256)
							{
								char* txt = doc.GetNodeText(node);

								sscanf(txt, "%d %x\n", &index, &col);
								if (index > 255) index = 255;
								if (index < 0) index = 0;

								m_colMaps[m_numColMaps].m_colMap[colId].m_index = index;
								m_colMaps[m_numColMaps].m_colMap[colId].m_col = col;
								colId++;
							}
						}
						else if (!strcmpi(doc.GetNodeTag(node),"/Map"))
						{
							m_colMaps[m_numColMaps].m_numCols = colId;
							m_numColMaps++;
							break;
						}

						node = doc.GetNextNode(node);
					}
				}
			}

			node = doc.GetNextNode(node);
		}
		doc.Close();
	}

} // LoadSettings

//-- CreateColMap -------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void ShadeWorm_c::CreateColMap(int colIndex)
{
	D3DLOCKED_RECT	lockedRect;
	m_colMapTexture->LockRect(0, &lockedRect, NULL, 0);

	unsigned char* data = (unsigned char*)lockedRect.pBits; 
	unsigned char* data2; 

	ColMap_t&	colMap = m_colMaps[colIndex];
	int colMapSize = colMap.m_numCols;

	for (int j=0; j<colMapSize-1; j++)
	{
		D3DCOLOR	srcCol = colMap.m_colMap[j].m_col;
		D3DCOLOR	dstCol = colMap.m_colMap[j+1].m_col;

		int srcR = (srcCol & 0x00ff0000) >> 16;
		int srcG = (srcCol & 0x0000ff00) >> 8;
		int srcB = (srcCol & 0x000000ff) >> 0;
		int dstR = (dstCol & 0x00ff0000) >> 16;
		int dstG = (dstCol & 0x0000ff00) >> 8;
		int dstB = (dstCol & 0x000000ff) >> 0;
		int range = (colMap.m_colMap[j+1].m_index - colMap.m_colMap[j].m_index) + 1;

		for (int i=0; i<range; i++)
		{
			float d = i / (float)range;
			int resR = srcR + int((dstR - srcR) * d);
			int resG = srcG + int((dstG - srcG) * d);
			int resB = srcB + int((dstB - srcB) * d);

			data2 = data + 	lockedRect.Pitch*(i+colMap.m_colMap[j].m_index);
			data2[0] = resB; // b
			data2[1] = resG; // g
			data2[2] = resR; // r
			data2[3] = 0; // a
		}
	}

} // CreateColMap

//--  RenderSprite  ------------------------------------------------------------
//
//------------------------------------------------------------------------------
void ShadeWorm_c::RenderSprite(float x, float y, float sizeX, float sizeY, int col)
{
	TEXVERTEX	v[4];

	v[0].x = x - sizeX;
	v[0].y = y - sizeY;
	v[0].z = 0;
	v[0].w = 1.0f;
	v[0].colour = col;
	v[0].u = 0;
	v[0].v = 0;

	v[1].x = x + sizeX;
	v[1].y = y - sizeY;
	v[1].z = 0;
	v[1].w = 1.0f;
	v[1].colour = col;
	v[1].u = 1;
	v[1].v = 0;

	v[2].x = x + sizeX;
	v[2].y = y + sizeY;
	v[2].z = 0;
	v[2].w = 1.0f;
	v[2].colour = col;
	v[2].u = 1;
	v[2].v = 1;

	v[3].x = x - sizeX;
	v[3].y = y + sizeY;
	v[3].z = 0;
	v[3].w = 1.0f;
	v[3].colour = col;
	v[3].u = 0;
	v[3].v = 1;

	m_device->SetVertexShader(D3DFVF_TEXVERTEX);
	m_device->DrawPrimitiveUP(D3DPT_QUADLIST, 1, v, sizeof(TEXVERTEX));

} // RenderSprite

//-- Clamp --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void ShadeWorm_c::Clamp(float min, float max, float& val)
{
	if (val < min)
		val = min;
	if (val > max)
		val = max;

} // Clamp

//-- Clamp --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void ShadeWorm_c::Clamp(int min, int max, int& val)
{
	if (val < min)
		val = min;
	if (val > max)
		val = max;

} // Clamp

//-- GetRand ------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
float ShadeWorm_c::GetRand()
{
	return 0.5f - ((rand() % 100) / 100.0f);

} // GetRand