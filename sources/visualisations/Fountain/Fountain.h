#include <xtl.h>
#pragma comment (lib, "lib/xbox_dx8.lib" )
#include "ParticleSystem.h"

#define szDefaultTexFile "particle.bmp"

typedef enum WEIGHT
{
	WEIGHT_NONE = 0,
	WEIGHT_A    = 1,
	WEIGHT_B    = 2,
	WEIGHT_C	= 3
} WEIGHT;

typedef enum MODE
{
	MODE_DIFFERENCE	= 0,
	MODE_LEVEL		= 1,
	MODE_BOTH		= 2
} MODE;

typedef enum MODIFICATION_MODE
{
	MODIFICATION_MODE_LINEAR		= 0,
	MODIFICATION_MODE_EXPONENTIAL	= 1
} MODIFICATION_MODE;

struct EffectSettings
{
    D3DXVECTOR3 vector;
    D3DXVECTOR3 bars;
	float modifier;
	MODIFICATION_MODE modificationMode;
	MODE mode;
	bool bInvert;
};

struct ColorSetting
{
	float min;
	float max;
	float shiftRate;
	float modifier;
	float variation;
	int bar;
};

struct ParticleSystemSettings
{
    DWORD       m_dwNumToRelease;
    float       m_fReleaseInterval;
    float       m_fLifeCycle;
    float       m_fSize;
    HsvColor	m_hsvColor;
    bool        m_bAirResistence;
    float       m_fVelocityVar;
    char       *m_chTexFile;

	ColorSetting m_csHue;
	ColorSetting m_csSaturation;
	ColorSetting m_csValue;

	float		m_fRotationSpeed;
	float		m_fRotationSensitivity;
	int			m_iRotationBar;

    EffectSettings m_esGravity;
    EffectSettings m_esPosition;
    EffectSettings m_esVelocity;
    EffectSettings m_esWind;

	float		m_fNumToReleaseMod;
	MODE		m_mMode;
	bool		m_bInvert;
};

void InitParticleSystem(ParticleSystemSettings settings);
void InitParticles();
void SetupCamera();
void SetupPerspective();
void SetupRotation(float x, float y, float z);

inline long double sqr( long double arg )
{
  return arg * arg;
}

