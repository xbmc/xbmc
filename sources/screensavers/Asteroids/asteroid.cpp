////////////////////////////////////////////////////////////////////////////
// 
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "asteroid.h"

////////////////////////////////////////////////////////////////////////////
// 
CAsteroid::CAsteroid()
{
	Init(AT_BIG);
}

////////////////////////////////////////////////////////////////////////////
// 
CAsteroid::~CAsteroid()
{
}

////////////////////////////////////////////////////////////////////////////
// 
void		CAsteroid::Update(f32 dt)
{
	switch (m_State)
	{
		case AS_ACTIVE:
			m_Pos += m_Vel*dt;
			m_Rot += m_RotVel*dt;			

			// Wrap around the screen
			if (m_Pos.x-m_Size/2.0f > gRender.m_Width)	m_Pos.x -= gRender.m_Width+m_Size;
			if (m_Pos.x+m_Size/2.0f < 0.0f)				m_Pos.x += gRender.m_Width+m_Size;
			if (m_Pos.y-m_Size/2.0f > gRender.m_Height)	m_Pos.y -= gRender.m_Height+m_Size;
			if (m_Pos.y+m_Size/2.0f < 0.0f)				m_Pos.y += gRender.m_Height+m_Size;
			break;
		case AS_EXPLODING:
			m_Time -= dt;
			if (m_Time < 0.0f)
			{
				m_State = AS_NONE;
			}
			for (int lNr=0; lNr<ASTEROIDNUMLINES; lNr++)
			{
				m_Lines[lNr][0] += m_LineVel[lNr]*dt;
				m_Lines[lNr][1] += m_LineVel[lNr]*dt;

				// Rotate the line
				CVector2 center = (m_Lines[lNr][1]+m_Lines[lNr][0])*0.5f;
				m_Lines[lNr][0] = center+(m_Lines[lNr][0]-center).Rotate(m_LineRot[lNr]*dt);
				m_Lines[lNr][1] = center+(m_Lines[lNr][1]-center).Rotate(m_LineRot[lNr]*dt);
			}
			break;
	}
}

////////////////////////////////////////////////////////////////////////////
// 
void		CAsteroid::Draw(CRenderD3D* render)
{
	if (m_State == AS_NONE)
		return;

	CRGBA	col = CRGBA(1.0f, 1.0f, 1.0f, 1.0f);
	if (m_State == AS_EXPLODING)
	{
		col = CRGBA(m_Time/ASTEROIDEXPTIME, m_Time/ASTEROIDEXPTIME, m_Time/ASTEROIDEXPTIME, 1.0f);
	}

	for (int lNr=0; lNr<ASTEROIDNUMLINES; lNr++)
	{
		render->DrawLine(m_Pos+m_Lines[lNr][0].Rotate(m_Rot), m_Pos+m_Lines[lNr][1].Rotate(m_Rot), col, col);
	}
}

////////////////////////////////////////////////////////////////////////////
// 
void		CAsteroid::Explode(const CVector2& vel)
{
	m_Time = ASTEROIDEXPTIME;
	m_State = AS_EXPLODING;

	for (int lNr=0; lNr<ASTEROIDNUMLINES; lNr++)
	{
		m_LineVel[lNr] = CVector2(RandSFloat()*100.0f, RandSFloat()*100.0f);
		m_LineRot[lNr] = RandSFloat()*300.0f;
	}
}

////////////////////////////////////////////////////////////////////////////
// 
bool		CAsteroid::Intersects(const CVector2& pos)
{
	if (SquareMagnitude(m_Pos-pos) < SQR(m_Size))
		return true;
	return false;
}

////////////////////////////////////////////////////////////////////////////
// 
void	CAsteroid::Init(EAsteroidType type)
{
	m_State = AS_NONE;
	m_Type = type;
	m_Pos.Zero();
	m_Vel.Zero();
	m_Rot	= RandFloat(0.0f, 360.0f);
	m_RotVel= RandFloat(-100.0f, 100.0f);
	m_Time	= 0.0f;

	if (type == AT_BIG)	m_Size	= RandFloat(25.0f, 35.0f);
	else				m_Size	= RandFloat(5.0f, 15.0f);

	// Create the asteroid by forming a circle that we randomize a bit
	f32			rot = 0.0f;
	CVector2	prev(0.0f, 0.0f);
	for (int lNr=0; lNr<ASTEROIDNUMLINES; lNr++)
	{
		CVector2 v = CVector2(0.0f, m_Size*RandFloat(0.7f, 1.0f)).Rotate((f32)(lNr+1)*(360.0f/(f32)ASTEROIDNUMLINES));
		m_Lines[lNr][0] = prev;
		m_Lines[lNr][1] = v;
		prev = v;
	}
	m_Lines[0][0] = m_Lines[ASTEROIDNUMLINES-1][1];
}

////////////////////////////////////////////////////////////////////////////
//
void	CAsteroid::SetVel(const CVector2& vel)
{
	m_Vel = vel;

	// This is a screensave so we need to make sure we never get a velocity of zero
	// If we do we might burn in an asteroid on the screen
	if (SquareMagnitude(m_Vel) < 0.1f)
	{
		m_Vel.Set(0.1f, RandSFloat()*0.1f);
	}
}
