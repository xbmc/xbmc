////////////////////////////////////////////////////////////////////////////
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "animatorfloat.h"

/***************************** D E F I N E S *******************************/
/****************************** M A C R O S ********************************/
/***************************** C L A S S E S *******************************/

////////////////////////////////////////////////////////////////////////////
//
class CBackground
{
public:
					CBackground();
					~CBackground();
	bool			RestoreDevice(CRenderD3D* render);
	void			InvalidateDevice(CRenderD3D* render);
	void			Update(f32 dt);
	bool			Draw(CRenderD3D* render);

	CVectorAnimator	m_RotAnim;			// Used to rotate the background

	// Device objects
	LPDIRECT3DVERTEXBUFFER8		m_VertexBuffer;
	LPDIRECT3DTEXTURE8			m_Texture;

	bool			CreateTexture(CRenderD3D* render);
};

/***************************** I N L I N E S *******************************/
