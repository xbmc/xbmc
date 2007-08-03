#pragma once

#include <xtl.h>

class CStarField
{
protected:
	struct ST_STAR
	{
		float v;
		float x;
		float y;
		float z;
		float rx;
		float ry;
		float rz;
		float sx;
		float sy;
		bool  plot;
	};

	struct ST_ROTATION
	{
		float a, b, c;
		float avel, bvel, cvel;
		float aacc, bacc, cacc;

	};

	struct ST_FIELD
	{
		float fMinX;
		float fMaxX;
		float fWidth;
		float fMinY;
		float fMaxY;
		float fHeight;
		float fMinZ;
		float fMaxZ;
		float fLength;
	};

	struct ST_SCREEN
	{
		int iWidth;
		int iHeight;
		int iMidX;
		int iMidY;
		float fZoom;
	};

	struct ST_CUSTOMVERTEX
	{
		float x, y, z, rhw;		// The transformed position for the vertex.
		unsigned long color;	// The vertex color.
	};

public:
	CStarField(void);
	CStarField(unsigned int nNumStars, float fGamma, float fBrightness, 
			   float fSpeed, float fZoom, float fExpanse);
	virtual ~CStarField(void);

	int Create(int iWidth, int iHeight);
	void Destroy(void);
	int  RenderFrame(void);

	void SetD3DDevice(LPDIRECT3DDEVICE8 pd3dDevice)
	{
		m_pd3dDevice = pd3dDevice;
	}

protected:
	void DrawStar(float x1, float y1, float x2, float y2, int iBrightness);
	void DoDraw(void);
	void SetPalette(unsigned int nIndex, int iRed, int iGreen, int iBlue);

	void ResetStar(ST_STAR* pStar);
	
	char GammaCorrect(unsigned char c, float g);
	
	float RangeRand(float min, float range)
	{
		return min + (float)rand() / (float)RAND_MAX * range;
	}	

protected:
	ST_SCREEN m_Screen;
	ST_FIELD m_Field;
	ST_ROTATION m_Cam;	
	ST_STAR* m_pStars;	
	unsigned int m_nStarCnt;	
	unsigned int m_nDrawnStars;	

	float m_fGammaValue;
	float m_fBrightness;
	float m_fBrightTable[256];

	float m_fMaxVelocity;
	float m_fVelocity;
	float m_fZoom;
	float m_fFieldExpanse;

	unsigned long m_dwPalette[256];

	LPDIRECT3DDEVICE8 m_pd3dDevice;

	ST_CUSTOMVERTEX* m_pVertices;
	ST_CUSTOMVERTEX* m_pCurVertice;
};


