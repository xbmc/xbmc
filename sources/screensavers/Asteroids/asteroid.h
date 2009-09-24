////////////////////////////////////////////////////////////////////////////
// 
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#pragma once

/***************************** D E F I N E S *******************************/

#define ASTEROIDNUMLINES		20
#define ASTEROIDEXPTIME			2.0f

enum EAsteroidState
{
	AS_NONE,
	AS_ACTIVE,
	AS_EXPLODING
};

enum EAsteroidType
{
	AT_BIG,
	AT_SMALL
};

/****************************** M A C R O S ********************************/
/***************************** C L A S S E S *******************************/

////////////////////////////////////////////////////////////////////////////
// 
class CAsteroid
{
public:
					CAsteroid();
					~CAsteroid();
	void			Init(EAsteroidType type);
	void			Update(f32 dt);
	void			Draw(CRenderD3D* render);

	void			Explode(const CVector2& vel);
	bool			Intersects(const CVector2& pos);

	void			SetVel(const CVector2& vel);

	EAsteroidType	m_Type;
	EAsteroidState	m_State;
	CVector2		m_Pos;
	f32				m_Rot, m_RotVel;
	f32				m_Size;

	f32				m_Time;

	CVector2		m_Lines[ASTEROIDNUMLINES][2];

	// Used when exploding
	CVector2		m_LineVel[ASTEROIDNUMLINES];
	f32				m_LineRot[ASTEROIDNUMLINES];

protected:
	CVector2		m_Vel;

};

/***************************** I N L I N E S *******************************/


