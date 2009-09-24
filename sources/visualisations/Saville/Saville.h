// Spectrum.h: interface for the CSpectrum class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Spectrum_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
#define AFX_Spectrum_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

struct VERTEX { D3DXVECTOR4 p; D3DCOLOR col; };
static const DWORD FVF_VERTEX = D3DFVF_XYZRHW|D3DFVF_DIFFUSE;

typedef struct FRECT
{
	public:
		FRECT( );
		FRECT( float left, float top, float right, float bottom);

		float left, top, right, bottom;
} FRECT;

typedef struct FPOINT
{
	public:
		FPOINT( );
		FPOINT( float x, float y );

		float x, y;
} FPOINT;

struct FREQDATA
{
	float freq[1024];
	float screen[1024];
	bool bDrawn;
	FREQDATA* prev;
	FREQDATA* next;
};

void FillRect(const FRECT* rect, D3DXCOLOR color);
void DrawRect(const FRECT* rect, D3DXCOLOR color, float width);
void DrawLine(const FPOINT* a, const FPOINT* b, D3DXCOLOR color, float width);

#endif // !defined(AFX_Spectrum_H__99B9A52D_ED09_4540_A887_162A68217A31__INCLUDED_)
