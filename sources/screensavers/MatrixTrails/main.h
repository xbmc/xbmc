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
class CConfig
{
public:
	int				m_NumColumns;		// Number of character columns
	int				m_NumRows;			// Number of character rows

	CRGBA			m_CharCol;
	f32				m_FadeSpeedMin;		// How quickly the characters fade out
	f32				m_FadeSpeedMax;
	f32				m_CharDelayMin;		// How long we wait before we add a new char
	f32				m_CharDelayMax;

	CVector2		m_CharSizeTex;		// Size of the characters in the texture
	int				m_NumChars;			// Number of characters in the texture

	void			SetDefaults();
	void			LoadSettings();
};


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

extern	CConfig		gConfig;

/***************************** I N L I N E S *******************************/
