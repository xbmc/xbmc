#include "D3DCube.h"
#include <string>

bool plusX[] = {0,0,1,0,1,1,0,0,1,0,1,1,1,1,1,1,1,1,1,1,0,1,0,0,0,0,0,0,0,0};
bool plusY[] = {1,1,1,1,1,1,0,1,0,1,1,0,0,1,0,1,1,0,0,1,0,1,1,0,0,1,0,1,1,0};
bool plusZ[] = {0,1,0,1,1,0,0,0,0,0,0,0,0,0,1,0,1,1,1,1,1,1,1,1,1,1,0,1,0,0};

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);

void CD3DCube::CleanUp()
{
	if (m_pVertexBuffer)
		m_pVertexBuffer->Release();
}

bool CD3DCube::Init(LPDIRECT3DDEVICE8 pD3DDevice, float fX1, float fY1, float fZ1, float fHeight, float fWidth, float fDepth)
{
	m_bInited = false;
	m_pd3dDevice = pD3DDevice;
	m_fX = fX1;
	m_fY = fY1;
	m_fZ = fZ1;
	m_fHeight = fHeight;
	m_fWidth = fWidth;
	m_fDepth = fDepth;

	if(FAILED(m_pd3dDevice->CreateVertexBuffer(NoVs * sizeof(CUSTOMVERTEX),
                                               0, D3DFVF_CUSTOMVERTEX,
                                               D3DPOOL_DEFAULT, &m_pVertexBuffer)))
	{
        return false;
	}

	//set up vertices XYZ
	for(int i=0; i<NoVs; i++)
	{
		cvVertices[i].x = m_fX;
		cvVertices[i].y = m_fY;
		cvVertices[i].z = m_fZ;

		if(plusX[i])
			cvVertices[i].x += m_fWidth;
		if(plusY[i])
			cvVertices[i].y += m_fHeight;
		if(plusZ[i])
			cvVertices[i].z += m_fDepth;
	}

	SetColourMulti();

	//texture coordinates
	for(int i=0; i<NoVs; i+=6)
	{
		cvVertices[i].tu = 0.0f; cvVertices[i].tv = 1.0f;
		cvVertices[i+1].tu = 0.0f; cvVertices[i+1].tv = 0.0f;
		cvVertices[i+2].tu = 1.0f; cvVertices[i+2].tv = 1.0f;
		cvVertices[i+3].tu = 0.0f; cvVertices[i+3].tv = 0.0f;
		cvVertices[i+4].tu = 1.0f; cvVertices[i+4].tv = 0.0f;
		cvVertices[i+5].tu = 1.0f; cvVertices[i+5].tv = 1.0f;
	}

	m_bInited = true;
	return true;
}

void CD3DCube::SetColourMulti()
{
	cvVertices[0].colour = D3DCOLOR_XRGB(0, 255, 0);
	cvVertices[1].colour = D3DCOLOR_XRGB(255, 255, 255);
	cvVertices[2].colour = D3DCOLOR_XRGB(255, 0, 0);
	cvVertices[3].colour = D3DCOLOR_XRGB(255, 255, 255);
	cvVertices[4].colour = D3DCOLOR_XRGB(0, 0, 255);
	cvVertices[5].colour = D3DCOLOR_XRGB(255, 0, 0);
	
	cvVertices[6].colour = D3DCOLOR_XRGB(0, 0, 255);
	cvVertices[7].colour = D3DCOLOR_XRGB(0, 255, 0);
	cvVertices[8].colour = D3DCOLOR_XRGB(255, 255, 255);
	cvVertices[9].colour = D3DCOLOR_XRGB(0, 255, 0);
	cvVertices[10].colour = D3DCOLOR_XRGB(255, 0, 0);
	cvVertices[11].colour = D3DCOLOR_XRGB(255, 255, 255);
	
	cvVertices[12].colour = D3DCOLOR_XRGB(255, 255, 255);
	cvVertices[13].colour = D3DCOLOR_XRGB(255, 0, 0);
	cvVertices[14].colour = D3DCOLOR_XRGB(0, 0, 255);
	cvVertices[15].colour = D3DCOLOR_XRGB(255, 0, 0);
	cvVertices[16].colour = D3DCOLOR_XRGB(255, 255, 255);
	cvVertices[17].colour = D3DCOLOR_XRGB(0, 0, 255);

	cvVertices[18].colour = D3DCOLOR_XRGB(0, 255, 0);
	cvVertices[19].colour = D3DCOLOR_XRGB(0, 0, 255);
	cvVertices[20].colour = D3DCOLOR_XRGB(255, 0, 0);
	cvVertices[21].colour = D3DCOLOR_XRGB(0, 0, 255);
	cvVertices[22].colour = D3DCOLOR_XRGB(255, 255, 255);
	cvVertices[23].colour = D3DCOLOR_XRGB(255, 0, 0);

	cvVertices[24].colour = D3DCOLOR_XRGB(255, 0, 0);
	cvVertices[25].colour = D3DCOLOR_XRGB(255, 255, 255);
	cvVertices[26].colour = D3DCOLOR_XRGB(0, 0, 255);
	cvVertices[27].colour = D3DCOLOR_XRGB(255, 255, 255);
	cvVertices[28].colour = D3DCOLOR_XRGB(0, 255, 0);
	cvVertices[29].colour = D3DCOLOR_XRGB(0, 0, 255);
}

bool CD3DCube::Set(float fHeight)
{
	VOID *pVertices;
	
	if(!m_bInited)
		return false;

	m_fHeight = fHeight;
	for(int i=0; i<NoVs; i++)
	{
		cvVertices[i].y = m_fY;
		if(plusY[i])
			cvVertices[i].y += m_fHeight;
	}

	//Get a pointer to the vertex buffer vertices and lock the vertex buffer
    if(FAILED(m_pVertexBuffer->Lock(0, sizeof(cvVertices), (BYTE**)&pVertices, 0)))
        return false;

    //Copy our stored vertices values into the vertex buffer
    memcpy(pVertices, cvVertices, sizeof(cvVertices));

    //Unlock the vertex buffer
    m_pVertexBuffer->Unlock();
	
	return true;
}


void CD3DCube::Render(LPDIRECT3DTEXTURE8 pTexture)
{
	m_pd3dDevice->SetStreamSource(0, m_pVertexBuffer, sizeof(CUSTOMVERTEX));
	m_pd3dDevice->SetVertexShader(D3DFVF_CUSTOMVERTEX);

	if(pTexture)
	{
        m_pd3dDevice->SetTexture(0, pTexture);
		d3dSetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	} else {
		m_pd3dDevice->SetTexture(0, NULL);
	}

	m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 10);
}

