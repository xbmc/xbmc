#include "Util.h"

//-----------------------------------------------------------------------------
// Name: getRandomMinMax()
// Desc: Gets a random number between min/max boundaries
//-----------------------------------------------------------------------------
float getRandomMinMax( float fMin, float fMax )
{
    float fRandNum = (float)rand () / RAND_MAX;
    return fMin + (fMax - fMin) * fRandNum;
}

float randomizeSign(float f)
{
    float fRandNum = (float)rand () / RAND_MAX;
	return fRandNum > 0.5 ? f : -f;
}
