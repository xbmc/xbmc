// metaball classed derived from IsoSurface
// Simon Windmill (siw@coolpowers.com)

#ifndef BLOBBY_H
#define BLOBBY_H

#include "xbsBase.h"

#include "IsoSurface.h"

const int MAXBLOBPOINTS = 5;

struct BlobPoint
{
	D3DXVECTOR3 m_Position;
	float m_fInfluence;
	D3DXVECTOR3 m_Speeds;
};

class Blobby : public IsoSurface
{

public:
	virtual float Sample( float mx, float my, float mz );
	void AnimatePoints( float ticks);

	BlobPoint m_BlobPoints[MAXBLOBPOINTS];
	int	m_iNumPoints;
	
	float m_fMoveScale;	// we scale the movements so we don't see the ugly boundary cases
};

#endif