// based on "stars5" from hugo elias (http://freespace.virgin.net/hugo.elias)
//------------------------------------------------------------------------------

#include "StarField.h"

#include <stdio.h>

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)
const int POINTSPERSTAR = 2;

CStarField::CStarField(void)
		  : m_pStars(0)
		  , m_nStarCnt(1000)
		  , m_fGammaValue(1.0f)
		  , m_fBrightness(0.3f)
		  , m_fMaxVelocity(10.f)
		  , m_fVelocity(0.f)
		  , m_fZoom(3.f/2.f)
		  , m_fFieldExpanse(1.5f)
		  , m_pd3dDevice(0) 
		  , m_pVertices(0)
		  , m_pCurVertice(0)
{
	memset(&m_Screen, 0, sizeof(m_Screen));
	memset(&m_Field, 0, sizeof(m_Field));
	memset(&m_Cam, 0, sizeof(m_Cam));
	memset(&m_dwPalette, 0, sizeof(m_dwPalette));

	for (int i = 0; i < 256; i++)
	{
		m_fBrightTable[i] = 0.f;
	}
}

CStarField::CStarField(unsigned int nNumStars, float fGamma, float fBrightness, 
					   float fSpeed, float fZoom, float fExpanse)
		  : m_pStars(0)
		  , m_nStarCnt(nNumStars)
		  , m_fGammaValue(fGamma)
		  , m_fBrightness(fBrightness)
		  , m_fMaxVelocity(fSpeed)
		  , m_fVelocity(0.f)
		  , m_fZoom(fZoom)
		  , m_fFieldExpanse(fExpanse)
		  , m_pd3dDevice(0) 
		  , m_pVertices(0)
		  , m_pCurVertice(0)
{
	memset(&m_Screen, 0, sizeof(m_Screen));
	memset(&m_Field, 0, sizeof(m_Field));
	memset(&m_Cam, 0, sizeof(m_Cam));
	memset(&m_dwPalette, 0, sizeof(m_dwPalette));

	for (int i = 0; i < 256; i++)
	{
		m_fBrightTable[i] = 0.f;
	}
}

CStarField::~CStarField(void)
{
	Destroy();
}

int CStarField::Create(int iWidth, int iHeight)
{
	m_Screen.iWidth = iWidth;
	m_Screen.iHeight = iHeight;
	m_Screen.iMidX = iWidth / 2;
	m_Screen.iMidY = iHeight / 2;

	m_Field.fMinX	 = -320.f;
	m_Field.fMaxX	 =  320.f;
	m_Field.fWidth	 = m_Field.fMaxX - m_Field.fMinX;
	m_Field.fMinY	 = -320.f;
	m_Field.fMaxY	 =  320.f;
	m_Field.fHeight	 = m_Field.fMaxY - m_Field.fMinY;
	m_Field.fMinZ	 = -128.f;
	m_Field.fMaxZ	 = 640.f;
	m_Field.fLength	 = m_Field.fMaxZ - m_Field.fMinZ;

	if (m_pStars)
	{
		delete[] m_pStars;
	}
	
	m_pStars = new ST_STAR[m_nStarCnt+1];

	if (m_pStars == NULL)
	{
		return -1;
	}

	unsigned int n;

	for (n = 0; n < 256; n++)
	{
		double br = pow(256 / (double)max(n, (unsigned)1), 2) - pow(256 / 255., 2);

		if (br < 0)
		{	
			br = 0.;
		}

		br = pow(1. - pow((1. - m_fBrightness), br), 0.66);

		m_fBrightTable[n] = (float)(br * 255);
	}


	for (n = 0; n < m_nStarCnt; n++)
	{
		ResetStar(&m_pStars[n]);
		m_pStars[n].z = RangeRand(m_Field.fMinZ, m_Field.fLength);
	}

	for (n = 1; n < 256; n++)
	{
		char c = GammaCorrect(n, m_fGammaValue) >> 2;
	//	SetPalette(n, c, c, c);
		SetPalette(n, n, n, n);
	}
	
	if (m_pVertices)
	{
		delete[] m_pVertices;
	}

	m_pVertices = new ST_CUSTOMVERTEX[(m_nStarCnt+1) * POINTSPERSTAR];

	if (m_pVertices == NULL)
	{
		return -1;
	}

	m_pCurVertice = m_pVertices;

	return 0;
}

char CStarField::GammaCorrect(unsigned char c, float g)
{
	return (char)(pow(((float)c) / 255.f, 1.f / g) * 255.f);
}	

void CStarField::Destroy(void)
{
	if (m_pStars)
	{
		delete[] m_pStars;
		m_pStars = NULL;
	}

	if (m_pVertices)
	{
		delete[] m_pVertices;
		m_pVertices = NULL;
		m_pCurVertice = NULL;
	}
}

int CStarField::RenderFrame(void)
{
	if (m_pStars == NULL)
	{
		return -1;
	}

	m_Screen.fZoom = (float)m_Screen.iMidX * m_fZoom;
	m_fVelocity += (m_fMaxVelocity - m_fVelocity) * .01f;
		
	m_Cam.aacc	= (m_Cam.aacc + RangeRand(-.00001f, .00002f)) * .99f;
	m_Cam.bacc	= (m_Cam.bacc + RangeRand(-.00001f, .00002f)) * .99f;
	m_Cam.cacc	= (m_Cam.cacc + RangeRand(-.00001f, .00002f)) * .992f;
	m_Cam.avel	= (m_Cam.avel + m_Cam.aacc) * .99f;
	m_Cam.bvel	= (m_Cam.bvel + m_Cam.bacc) * .99f;
	m_Cam.cvel	= (m_Cam.cvel + m_Cam.cacc) * .992f;
	m_Cam.a		= (m_Cam.a + m_Cam.avel) * .99f;
	m_Cam.b		= (m_Cam.b + m_Cam.bvel) * .99f;
	m_Cam.c		= (m_Cam.c + m_Cam.cvel) * .992f;
	
	float sina = (float)sin(-m_Cam.a);
	float cosa = (float)cos(-m_Cam.a);
	float sinb = (float)sin(-m_Cam.b);
	float cosb = (float)cos(-m_Cam.b);
	float sinc = (float)sin(-m_Cam.c);
	float cosc = (float)cos(-m_Cam.c);

	m_pCurVertice = m_pVertices;
	m_nDrawnStars = 0;

	for (unsigned int n = 0; n < m_nStarCnt; n++)
	{
		ST_STAR* pStar = &m_pStars[n];

		// move star
		pStar->z -= pStar->v * m_fVelocity;

		if (pStar->z < m_Field.fMinZ)
		{
			ResetStar(pStar);
		}

		// rotate star
		// rotate wrt a
		pStar->rx = pStar->x * cosa + pStar->z * sina;
		pStar->rz = pStar->z * cosa - pStar->x * sina;
		// rotate wrt b
		pStar->ry = pStar->y * cosb + pStar->rz * sinb;
		pStar->rz = pStar->rz * cosb - pStar->y * sinb;
		// rotate wrt c
		float fTempRX = pStar->rx * cosc + pStar->ry * sinc;
		pStar->ry = pStar->ry * cosc - pStar->rx * sinc;
		pStar->rx = fTempRX;

		// prepare to plot
		float nsx = (float)m_Screen.iMidX + pStar->rx / pStar->rz * m_Screen.fZoom;
		float nsy = (float)m_Screen.iMidY - pStar->ry / pStar->rz * m_Screen.fZoom;

		int dist = (int)pStar->rz;
		
		float xd = nsx - pStar->sx;
		float yd = nsy - pStar->sy;
		
		float len = sqrt(xd*xd + yd*yd);
				
		float c = m_fBrightTable[min(dist >> 1, 255)];
				
		if (len > 2)
		{
			//c /= (len / 2);
		}
		
		// plot star
		if (pStar->rz > 1.0f)
		{
			if (pStar->plot)
			{
				if (pStar->sx > 2 && pStar->sx < m_Screen.iWidth && 
					pStar->sy > 2 && pStar->sy < m_Screen.iHeight - 2 &&
					nsx > 2 && nsx < m_Screen.iWidth && 
					nsy > 2 && nsy < m_Screen.iHeight - 2)
				{
					DrawStar(pStar->sx, pStar->sy, nsx, nsy, (int)c);
				}
			}

			pStar->sx = nsx;
			pStar->sy = nsy;
			pStar->plot = true;
		}
	}

	DoDraw();

	return 0;
}

void CStarField::ResetStar(ST_STAR* pStar)
{
	pStar->v = 1.f / (float)pow(.1f + 9.9f * (float)rand() / (float)RAND_MAX, m_fFieldExpanse);
	pStar->x = RangeRand(m_Field.fMinX, m_Field.fWidth);
	pStar->y = RangeRand(m_Field.fMinY, m_Field.fHeight);
	pStar->z = m_Field.fMaxZ;
	pStar->plot = false;
}

void CStarField::DrawStar(float x1, float y1, float x2, float y2, int iBrightness)
{
	if (m_pVertices == NULL || m_pCurVertice == NULL)
	{
		return;
	}

	m_pCurVertice->x = x1;
	m_pCurVertice->y = y1;
	m_pCurVertice->z = 0.0f;
	m_pCurVertice->rhw = 1.0f;
	m_pCurVertice->color = m_dwPalette[iBrightness];
	m_pCurVertice++;

//	m_pCurVertice->x = x2;
//	m_pCurVertice->y = y2;	
	m_pCurVertice->x = x2 > x1 ? x2 + 1 : x2 - 1;
	m_pCurVertice->y = y2 > y1 ? y2 + 1 : y2 - 1;
	m_pCurVertice->z = 0.0f;
	m_pCurVertice->rhw = 1.0f;
	m_pCurVertice->color = m_dwPalette[iBrightness];
	m_pCurVertice++;
	
	m_nDrawnStars++;
}

void CStarField::DoDraw(void)
{
	if (m_pd3dDevice == NULL)
	{
		return;
	}

	if (m_nStarCnt == 0 || m_nDrawnStars == 0)
	{
		return;
	}
	
	UINT nVSize = m_nStarCnt * POINTSPERSTAR * sizeof(ST_CUSTOMVERTEX);

	LPDIRECT3DVERTEXBUFFER8 pVB = NULL;

	HRESULT hr = m_pd3dDevice->CreateVertexBuffer(nVSize, 0, D3DFVF_CUSTOMVERTEX, 
													D3DPOOL_DEFAULT, &pVB);
	if (FAILED(hr))
	{
		return;
	}

	VOID* pVertices;

	pVB->Lock(0, nVSize, (BYTE**)&pVertices, 0);
	memcpy(pVertices, m_pVertices, nVSize);
	pVB->Unlock();

	m_pd3dDevice->SetVertexShader(D3DFVF_CUSTOMVERTEX);
    m_pd3dDevice->SetStreamSource(0, pVB, sizeof(ST_CUSTOMVERTEX));
	m_pd3dDevice->DrawPrimitive(D3DPT_LINELIST, 0, m_nDrawnStars);

	pVB->Release();
}

void CStarField::SetPalette(unsigned int nIndex, int iRed, int iGreen, int iBlue)
{
	if (nIndex < 256)
	{
		m_dwPalette[nIndex] = D3DCOLOR_XRGB((BYTE)iRed, (BYTE)iGreen, (BYTE)iBlue);
	}
}



