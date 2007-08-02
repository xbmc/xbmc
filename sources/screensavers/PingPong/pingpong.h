////////////////////////////////////////////////////////////////////////////
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#pragma once

/***************************** D E F I N E S *******************************/
/****************************** M A C R O S ********************************/
/***************************** C L A S S E S *******************************/

////////////////////////////////////////////////////////////////////////////
//
typedef	struct	TRenderVertex
{
	CVector		pos;
	DWORD		col;
	enum FVF {	FVF_Flags =	D3DFVF_XYZ | D3DFVF_DIFFUSE	};
} TRenderVertex;

////////////////////////////////////////////////////////////////////////////
//
class CPaddel
{
public:
	CVector		m_Pos;
	CVector		m_Size;
	CRGBA		m_Col;

				CPaddel()
				{
					m_Pos.Zero();
					m_Size.Set(1.0f, 5.0f, 0.0f);
					m_Col.Set(1.0f, 1.0f, 1.0f, 1.0f);
				}
};

////////////////////////////////////////////////////////////////////////////
//
class CBall
{
public:
	CVector		m_Pos;
	CVector		m_Vel;
	CVector		m_Size;
	CRGBA		m_Col;
	
				CBall()
				{
					m_Pos.Zero();
					m_Vel.Zero();
					m_Size.Set(1.0f, 1.0f, 0.0f);
					m_Col.Set(1.0f, 1.0f, 1.0f, 1.0f);
				}
};

////////////////////////////////////////////////////////////////////////////
//
class CPingPong
{
public:
					CPingPong();
					~CPingPong();
	bool			RestoreDevice(CRenderD3D* render);
	void			InvalidateDevice(CRenderD3D* render);
	void			Update(f32 dt);
	bool			Draw(CRenderD3D* render);

	CPaddel			m_Paddle[2];
	CBall			m_Ball;
protected:

	// Device objects
	LPDIRECT3DVERTEXBUFFER8		m_VertexBuffer;

	TRenderVertex*	AddQuad(TRenderVertex* vert, const CVector& pos, const CVector& size, DWORD col);
};

/***************************** I N L I N E S *******************************/
