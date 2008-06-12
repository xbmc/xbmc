////////////////////////////////////////////////////////////////////////////
// 
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#pragma once

/***************************** D E F I N E S *******************************/
/****************************** M A C R O S ********************************/
/***************************** C L A S S E S *******************************/

struct TRenderVertex;

////////////////////////////////////////////////////////////////////////////
//
class CChar
{
public:
	f32			m_Intensity;
	int			m_CharNr;
				CChar()
				{
					m_Intensity = 1.0f;
					m_CharNr = 0;
				}
};

////////////////////////////////////////////////////////////////////////////
//
class CColumn
{
public:
	CChar*			m_Chars;
	int				m_NumChars;
	f32				m_Delay,m_CharDelay;
	int				m_CurChar;
	f32				m_FadeSpeed;

					CColumn();
					~CColumn();
	void			Init(int numChars);
	void			Update(f32 dt);
	TRenderVertex*	UpdateVertexBuffer(TRenderVertex* verts, f32 posX, f32 posY, const CVector& charSize, const CVector2& charSizeTex);
};

/***************************** I N L I N E S *******************************/


