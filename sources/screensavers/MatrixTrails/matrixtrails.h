////////////////////////////////////////////////////////////////////////////
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "column.h"

/***************************** D E F I N E S *******************************/
/****************************** M A C R O S ********************************/
/***************************** C L A S S E S *******************************/

////////////////////////////////////////////////////////////////////////////
//
typedef	struct	TRenderVertex
{
	CVector		pos;
	f32			w;
	DWORD		col;
	f32			u, v;
	enum FVF {	FVF_Flags =	D3DFVF_XYZRHW | D3DFVF_DIFFUSE	| D3DFVF_TEX1};
} TRenderVertex;

////////////////////////////////////////////////////////////////////////////
//
class CMatrixTrails
{
public:
					CMatrixTrails();
					~CMatrixTrails();
	bool			RestoreDevice(CRenderD3D* render);
	void			InvalidateDevice(CRenderD3D* render);
	void			Update(f32 dt);
	bool			Draw(CRenderD3D* render);

protected:
	int				m_NumColumns;
	int				m_NumRows;
	CColumn*		m_Columns;
	CVector			m_CharSize, m_CharSizeTex;

	// Device objects
	LPDIRECT3DVERTEXBUFFER8		m_VertexBuffer;
	LPDIRECT3DTEXTURE8			m_Texture;
};

/***************************** I N L I N E S *******************************/
