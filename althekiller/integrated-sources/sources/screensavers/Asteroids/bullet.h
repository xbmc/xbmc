////////////////////////////////////////////////////////////////////////////
// 
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#pragma once

/***************************** D E F I N E S *******************************/

enum EBulletState
{
	BS_NONE,
	BS_ACTIVE
};

/****************************** M A C R O S ********************************/
/***************************** C L A S S E S *******************************/

////////////////////////////////////////////////////////////////////////////
// 
class CBullet
{
public:
					CBullet();
					~CBullet();

	void			Draw(CRenderD3D* render);
	void			Update(f32 dt);

	void			Fire(const CVector2& pos, const CVector2& vel);

	EBulletState	m_State;
	CVector2		m_Pos;
	CVector2		m_Vel;
	f32				m_Size;
};

/***************************** I N L I N E S *******************************/


