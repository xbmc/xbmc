////////////////////////////////////////////////////////////////////////////
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xbsBase.h"
#include <stdio.h>
#include <stdlib.h>
#include "types.h"

/***************************** D E F I N E S *******************************/
/****************************** M A C R O S ********************************/
/***************************** C L A S S E S *******************************/

////////////////////////////////////////////////////////////////////////////
// 
class CRenderD3D
{
public:
	LPDIRECT3DDEVICE8	GetDevice()		{ return m_D3dDevice; }

	LPDIRECT3DDEVICE8	m_D3dDevice;
	
	int			m_Width;
	int			m_Height;

};

/***************************** G L O B A L S *******************************/

extern "C" void Stop();
void LoadSettings();
void SetDefaults();

/***************************** I N L I N E S *******************************/
