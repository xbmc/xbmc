////////////////////////////////////////////////////////////////////////////
//
// Asteroids Screensaver for XBox Media Center
// Copyright (c) 2005 Joakim Eriksson <je@plane9.com>
//
////////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "asteroids.h"

////////////////////////////////////////////////////////////////////////////
//
CAsteroids::CAsteroids()
{
}

////////////////////////////////////////////////////////////////////////////
//
CAsteroids::~CAsteroids()
{
}

////////////////////////////////////////////////////////////////////////////
//
bool	CAsteroids::RestoreDevice(CRenderD3D* render)
{
	Init();
	return true;
}

////////////////////////////////////////////////////////////////////////////
//
void	CAsteroids::InvalidateDevice(CRenderD3D* render)
{
}

////////////////////////////////////////////////////////////////////////////
//
void	CAsteroids::Init()
{
	m_LevelTime = 0;

	for (int i=0; i<NUMBULLETS; i++)		m_Bullets[i].m_State = BS_NONE;
	for (int i=0; i<NUMASTEROIDS; i++)		m_Asteroids[i].m_State = AS_NONE;

	for (int i=0; i<NUMASTEROIDS/NUMASTEROIDFRAGMENTS; i++)
	{
		m_Asteroids[i].Init(AT_BIG);
		m_Asteroids[i].m_Pos = CVector2((f32)gRender.m_Width*RandFloat(), (f32)gRender.m_Height*RandFloat());
		m_Asteroids[i].SetVel(CVector2(RandSFloat()*100.0f, RandSFloat()*100.0f));
		m_Asteroids[i].m_State = AS_ACTIVE;
	}
	Warp();
}

////////////////////////////////////////////////////////////////////////////
//
void		CAsteroids::Update(f32 dt)
{
	m_LevelTime += dt;

	// Check if we need to restart everything
	int	numActiveAsteroids = 0;
	for (int anr=0; anr<NUMASTEROIDS; anr++)
	{
		CAsteroid* asteroid = &m_Asteroids[anr];
		if (asteroid->m_State == AS_NONE)	
			continue;
		numActiveAsteroids++;
	}
	if ((numActiveAsteroids == 0) || (m_LevelTime > MAXLEVELTIME))
	{
		Init();
	}

	// Warp every now and then so we can get the last stray asteroids quicker
	m_Ship.m_WarpDelay += dt;
	if (m_Ship.m_WarpDelay > WARPDELAY)
	{
		m_Ship.m_WarpDelay = 0.0f;
		Warp();
	}

	ShipAI(dt);

	// Update
	m_Ship.Update(dt);
	for (int bnr=0; bnr<NUMBULLETS; bnr++)		m_Bullets[bnr].Update(dt);
	for (int anr=0; anr<NUMASTEROIDS; anr++)	m_Asteroids[anr].Update(dt);

	PerformCollisions();
}

////////////////////////////////////////////////////////////////////////////
//
void	CAsteroids::ShipAI(f32 dt)
{
	// Find the closest asteroid and aim at it
	int closestIndex = -1;
	f32	distMin = 0;
	for (int anr=0; anr<NUMASTEROIDS; anr++)
	{
		CAsteroid* asteroid = &m_Asteroids[anr];
		if (asteroid->m_State != AS_ACTIVE)	
			continue;
		f32 dist = SquareMagnitude(m_Ship.m_Pos-asteroid->m_Pos);
		if ((closestIndex == -1) || (dist < distMin))
		{
			closestIndex = anr;
			distMin = dist;
		}
	}	
	if (closestIndex == -1)
		return;

	CAsteroid* asteroid = &m_Asteroids[closestIndex];
	CVector2 dir = Normalized(m_Ship.m_Pos-asteroid->m_Pos);
	f32 backOrFront = DotProduct(dir, m_Ship.GetDirVec());
	f32 leftOrRight = DotProduct(dir, m_Ship.GetTangDirVec());
	if (ISEQUAL(backOrFront, 1.0f, 0.01f) && m_Ship.CanFire() && distMin < SQR(200.0f))
	{
		// We are aimed correctly so shoot
		CBullet* bullet = NewBullet();
		if (bullet)
		{
			bullet->Fire(m_Ship.m_Pos-(m_Ship.GetDirVec()*m_Ship.m_Size), m_Ship.GetDirVec()*-200.0f);
		}
	}
	else
	{
		f32 rotSpeed = 200.0f;
		if (backOrFront > 0.0f)
		{
			m_Ship.m_Rot += leftOrRight*rotSpeed*dt;
		}
		else
		{
			if (leftOrRight > 0.0f)
				m_Ship.m_Rot += rotSpeed*dt;
			else
				m_Ship.m_Rot -= rotSpeed*dt;
		}
	}
}

////////////////////////////////////////////////////////////////////////////
//
void	CAsteroids::PerformCollisions()
{
	for (int anr=0; anr<NUMASTEROIDS; anr++)
	{
		CAsteroid* asteroid = &m_Asteroids[anr];
		if (asteroid->m_State != AS_ACTIVE)	
			continue;

		// If we are close to an asteroid we warp away
		if (SquareMagnitude(asteroid->m_Pos-m_Ship.m_Pos) < SQR(asteroid->m_Size+m_Ship.m_Size))
		{
			Warp();
		}

		// Has a bullet hit an asteroid?
		for (int bnr=0; bnr<NUMBULLETS; bnr++)
		{
			CBullet* bullet = &m_Bullets[bnr];
			if (bullet->m_State == BS_NONE)	
				continue;

			if (asteroid->Intersects(bullet->m_Pos))
			{
				m_Ship.m_WarpDelay = 0.0f;			// Reset warp time if we hit something
				asteroid->Explode(bullet->m_Vel);
				if (asteroid->m_Type == AT_BIG)
				{
					for (int newA=0; newA<3; newA++)
					{
						CAsteroid* smallAsteroid = NewAsteroid();
						if (smallAsteroid)
						{
							smallAsteroid->Init(AT_SMALL);
							smallAsteroid->m_Pos = asteroid->m_Pos+CVector2(RandSFloat()*asteroid->m_Size*0.9f, RandSFloat()*asteroid->m_Size*0.9f);
							smallAsteroid->SetVel(CVector2(RandSFloat()*120.0f, RandSFloat()*120.0f));
							smallAsteroid->m_State = AS_ACTIVE;
						}
					}
				}
				bullet->m_State = BS_NONE;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////
//
bool		CAsteroids::Draw(CRenderD3D* render)
{
	m_Ship.Draw(render);
	for (int i=0; i<NUMBULLETS; i++)	m_Bullets[i].Draw(render);
	for (int i=0; i<NUMASTEROIDS; i++)	m_Asteroids[i].Draw(render);
	return true;
}

////////////////////////////////////////////////////////////////////////////
//
CBullet*	CAsteroids::NewBullet()
{
	for (int i=0; i<NUMBULLETS; i++)
	{
		if (m_Bullets[i].m_State == BS_NONE)
		{
			return &m_Bullets[i];
		}
	}
	return null;
}

////////////////////////////////////////////////////////////////////////////
//
CAsteroid*	CAsteroids::NewAsteroid()
{
	for (int i=0; i<NUMASTEROIDS; i++)
	{
		if (m_Asteroids[i].m_State == AS_NONE)
		{
			return &m_Asteroids[i];
		}
	}
	return null;
}

////////////////////////////////////////////////////////////////////////////
//
void	CAsteroids::Warp()
{
	while (1)
	{
		bool	valid = true;
		// Get new pos. Keep us away from the corners
		m_Ship.m_Pos = CVector2((f32)gRender.m_Width*RandFloat(0.2f, 0.8f), (f32)gRender.m_Height*RandFloat(0.2f, 0.8f));
		for (int anr=0; anr<NUMASTEROIDS; anr++)
		{
			CAsteroid* asteroid = &m_Asteroids[anr];
			if (asteroid->m_State != AS_ACTIVE)	
				continue;
		
			if (asteroid->Intersects(m_Ship.m_Pos))
			{
				valid = false;
			}
		}

		if (valid)
			return;
	}
}


