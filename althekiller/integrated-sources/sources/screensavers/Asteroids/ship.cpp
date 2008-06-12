////////////////////////////////////////////////////////////////////////////
// 
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "ship.h"

////////////////////////////////////////////////////////////////////////////
// 
CShip::CShip()
{
	m_Pos.Zero();
	m_Vel.Zero();
	m_Rot = 0.0f;
	m_Speed = 100.0f;
	m_Size = 10.0f;
	m_BulletDelay = 0.0f;
	m_WarpDelay = 0.0f;

	m_Lines[0][0] = CVector2(0.0f, -m_Size);		m_Lines[0][1] = CVector2( m_Size, m_Size);
	m_Lines[1][0] = CVector2( m_Size, m_Size);		m_Lines[1][1] = CVector2(0.0f, m_Size*0.5f);
	m_Lines[2][0] = CVector2(0.0f, m_Size*0.5f);	m_Lines[2][1] = CVector2(-m_Size,  m_Size);
	m_Lines[3][0] = CVector2(-m_Size, m_Size);		m_Lines[3][1] = CVector2(0.0f,  -m_Size);
}

////////////////////////////////////////////////////////////////////////////
// 
CShip::~CShip()
{
}

////////////////////////////////////////////////////////////////////////////
// 
void		CShip::Update(f32 dt)
{
	m_Pos += m_Vel*dt;

	if (m_BulletDelay > 0.0)
		m_BulletDelay -= dt;
}

////////////////////////////////////////////////////////////////////////////
// 
void		CShip::Draw(CRenderD3D* render)
{
	CRGBA	col = CRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	for (int lnr=0; lnr<SHIPLINES; lnr++)
	{
		render->DrawLine(m_Pos+m_Lines[lnr][0].Rotate(m_Rot), m_Pos+m_Lines[lnr][1].Rotate(m_Rot), col, col);
	}
}

////////////////////////////////////////////////////////////////////////////
// Returns a vector that points in the ships current direction
//
CVector2	CShip::GetDirVec()
{
	return CVector2(0.0f, 1.0f).Rotate(m_Rot);
}

////////////////////////////////////////////////////////////////////////////
//
CVector2	CShip::GetTangDirVec()
{
	return CVector2(1.0f, 0.0f).Rotate(m_Rot);
}

////////////////////////////////////////////////////////////////////////////
// 
bool		CShip::CanFire()
{
	if (m_BulletDelay > 0.0f)
		return false;		
	m_BulletDelay = BULLETDELAY;
	return true;
}

