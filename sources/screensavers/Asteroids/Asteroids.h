////////////////////////////////////////////////////////////////////////////
//
// Author:
//   Joakim Eriksson
//
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ship.h"
#include "asteroid.h"
#include "bullet.h"

/***************************** D E F I N E S *******************************/

#define NUMASTEROIDFRAGMENTS	3
#define NUMASTEROIDS	(10*NUMASTEROIDFRAGMENTS)
#define NUMBULLETS		10
#define MAXLEVELTIME	(5*60)			// Max time before we reset the whole level

/****************************** M A C R O S ********************************/
/***************************** C L A S S E S *******************************/

////////////////////////////////////////////////////////////////////////////
//
class CAsteroids
{
public:
					CAsteroids();
					~CAsteroids();
	bool			RestoreDevice(CRenderD3D* render);
	void			InvalidateDevice(CRenderD3D* render);
	void			Update(f32 dt);
	bool			Draw(CRenderD3D* render);

protected:
	CShip			m_Ship;
	CBullet			m_Bullets[NUMBULLETS];
	CAsteroid		m_Asteroids[NUMASTEROIDS];
	f32				m_LevelTime;

	CBullet*		NewBullet();
	CAsteroid*		NewAsteroid();
	void			Init();
	void			Warp();
	void			ShipAI(f32 dt);
	void			PerformCollisions();
};

/***************************** I N L I N E S *******************************/
