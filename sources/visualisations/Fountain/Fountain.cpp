// Fountain.cpp
//
//////////////////////////////////////////////////////////////////////

#include "Util.h"
#include "Fountain.h"
#include "XmlDocument.h"
#include <math.h>
#include <stdio.h>

#define	FREQ_DATA_SIZE 1024			// size of frequency data wanted
#define MAX_BARS 720				// number of bars in the Spectrum
#define MIN_PEAK_DECAY_SPEED 0		// decay speed in dB/frame
#define MAX_PEAK_DECAY_SPEED 4
#define MIN_RISE_SPEED 0.01f		// fraction of actual rise to allow
#define MAX_RISE_SPEED 1
#define MIN_FALL_SPEED 0.01f		// fraction of actual fall to allow
#define MAX_FALL_SPEED 1
#define MIN_FREQUENCY 1				// allowable frequency range
#define MAX_FREQUENCY 24000
#define MIN_LEVEL 0					// allowable level range
#define MAX_LEVEL 96
#define TEXTURE_HEIGHT 256
#define TEXTURE_MID 128
#define TEXTURE_WIDTH 1
#define MAX_CHANNELS 2
#define MAX_SETTINGS 64

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);
extern "C" void d3dGetRenderState(DWORD dwY, DWORD* dwZ);
extern "C" 
{
	struct VIS_INFO {
		bool bWantsFreq;
		int iSyncDelay;
		//		int iAudioDataLength;
		//		int iFreqDataLength;
	};
};

static VIS_INFO		  vInfo;
static char m_szVisName[1024];

static LPDIRECT3DDEVICE8 m_pd3dDevice;
static CParticleSystem m_ParticleSystem;

static HsvColor m_clrColor;
static int m_iHDir = 1;
static int m_iSDir = 1;
static int m_iVDir = 1;

static float m_fElapsedTime;
static DWORD m_dwCurTime;
static DWORD m_dwLastTime;

static float m_fUpdateSpeed			= 1000.0f;
static float m_fRotation			= 0.0f;

static ParticleSystemSettings m_pssSettings[MAX_SETTINGS];
static int m_iCurrSetting = 0;
static int m_iNumSettings = 2;
static bool m_bCycleSettings = true;

static int		m_iSampleRate;

static float	m_pFreq[FREQ_DATA_SIZE];
static float	m_pFreqPrev[FREQ_DATA_SIZE];				

static int		m_iBars		= 12;
static bool		m_bLogScale = false;
static float	m_fMinFreq	= 200;
static float	m_fMaxFreq	= MAX_FREQUENCY;
static float	m_fMinLevel = MIN_LEVEL;
static float	m_fMaxLevel = MAX_LEVEL;

void LoadSettings();
void SetDefaults();
void SetDefaults(ParticleSystemSettings* settings);
void SetDefaults(EffectSettings* settings);
void ShiftColor(ParticleSystemSettings* settings);
void CreateArrays();

void SetEffectSettings(EffectSettings* settings, CXmlDocument* doc, XmlNode node);
D3DXVECTOR3 Shift(EffectSettings* settings);

inline int RandPosNeg() {
	return (float)rand() > RAND_MAX/2.0f ? 1 : -1;
}

extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName)
{
	OutputDebugString("Create()\n");
	m_pd3dDevice = pd3dDevice;
	strcpy(m_szVisName,szVisualisationName);
	//strcpy(m_szVisName,"fountain");
	LoadSettings();
	m_clrColor = HsvColor( 360.0f, 1.0f, .06f );
	vInfo.bWantsFreq = true;

	m_iCurrSetting = -1;
	srand( timeGetTime() );
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
	OutputDebugString("Start()\n");
	m_iSampleRate = iSamplesPerSec;

	//set (or reset) our previous frequency data array
	for (int i=0; i<FREQ_DATA_SIZE; i++)
	{
		m_pFreqPrev[i] = 0.0f;
	}

	if (m_bCycleSettings || m_iNumSettings < 3)
		m_iCurrSetting++;
	else
	{
		int iNextSetting = m_iCurrSetting;
		//TODO: try and fix this to be more random
		while (iNextSetting == m_iCurrSetting)
			iNextSetting = ((float)rand () / RAND_MAX) * m_iNumSettings;
		m_iCurrSetting = iNextSetting;
	}
	//m_iCurrSetting = 3;
	if (m_iCurrSetting >= m_iNumSettings || m_iCurrSetting < 0)
		m_iCurrSetting = 0;

	m_clrColor = m_pssSettings[m_iCurrSetting].m_hsvColor;
	m_iHDir = 1;
	m_iSDir = 1;
	m_iVDir = 1;
	InitParticleSystem(m_pssSettings[m_iCurrSetting]);
}


extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
	ParticleSystemSettings *currSettings = &m_pssSettings[m_iCurrSetting];

	if (iFreqDataLength>FREQ_DATA_SIZE)
		iFreqDataLength = FREQ_DATA_SIZE;

	// Group data into frequency bins by averaging (Ignore the constant term)
	int jmin=2;
	int jmax;
	// FIXME:  Roll conditionals out of loop
	for (int i=0, iBin=0; i < m_iBars; i++, iBin+=2)
	{
		m_pFreq[iBin]=0.000001f;	// almost zero to avoid taking log of zero later
		if (m_bLogScale)
			jmax = (int) (m_fMinFreq*pow(m_fMaxFreq/m_fMinFreq,(float)i/m_iBars)/m_iSampleRate*iFreqDataLength + 0.5f);
		else
			jmax = (int) ((m_fMinFreq + (m_fMaxFreq-m_fMinFreq)*i/m_iBars)/m_iSampleRate*iFreqDataLength + 0.5f);
		// Round up to nearest multiple of 2 and check that jmin is not jmax
		jmax<<=1;
		if (jmax > iFreqDataLength) jmax = iFreqDataLength;
		if (jmax==jmin) jmin-=2;
		for (int j=jmin; j<jmax; j+=2)
		{
			m_pFreq[iBin]+=pFreqData[j]+pFreqData[j+1];
		}
		m_pFreq[iBin] /=(jmax-jmin);
		jmin = jmax;
	}


	// Transform data to dB scale, 0 (Quietest possible) to 96 (Loudest)
	for (int i=0; i < (m_iBars*2); i++)
	{
		m_pFreq[i] = 10*log10(m_pFreq[i]);
		if (m_pFreq[i] > MAX_LEVEL)
			m_pFreq[i] = MAX_LEVEL;
		if (m_pFreq[i] < MIN_LEVEL)
			m_pFreq[i] = MIN_LEVEL;
	}

	// truncate data to the users range
	if (m_pFreq[i] > m_fMaxLevel)
		m_pFreq[i] = m_fMaxLevel;
	if (m_pFreq[i] < m_fMinLevel)
		m_pFreq[i] = m_fMinLevel;

	//if we exceed the rotation sensitivity threshold, reverse our rotation
	int rotationBar = min(m_iBars, currSettings->m_iRotationBar);
	if (abs(m_pFreq[rotationBar] - m_pFreqPrev[rotationBar]) > MAX_LEVEL * currSettings->m_fRotationSensitivity)
		currSettings->m_fRotationSpeed*=-1;

	ShiftColor(currSettings);

	//adjust num to release
	if (currSettings->m_fNumToReleaseMod != 0.0f)
	{
		int numToReleaseBar = min(m_iBars, 10);
		float level = (m_pFreq[numToReleaseBar]/(float)MAX_LEVEL);
		int numToRelease = level * currSettings->m_dwNumToRelease;
		int mod = numToRelease * currSettings->m_fNumToReleaseMod;
		numToRelease+=mod;
		m_ParticleSystem.SetNumToRelease(numToRelease);
	}

	//adjust gravity
	D3DXVECTOR3 vGravity = Shift(&currSettings->m_esGravity);
	m_ParticleSystem.SetGravity(vGravity);

	//adjust wind
	D3DXVECTOR3 vWind = Shift(&currSettings->m_esWind);
	m_ParticleSystem.SetWind(vWind);

	//adjust velocity
	D3DXVECTOR3 vVelocity = Shift(&currSettings->m_esVelocity);
	m_ParticleSystem.SetVelocity(vVelocity);

	//adjust position
	D3DXVECTOR3 vPosition = Shift(&currSettings->m_esPosition);
	m_ParticleSystem.SetPosition(vPosition);

	memcpy(m_pFreqPrev, m_pFreq, FREQ_DATA_SIZE);
}

extern "C" void Render()
{
	//
	// Set up our view
	//
	SetupCamera();
	SetupRotation(0.0f, 0.0f, m_fRotation+=m_pssSettings[m_iCurrSetting].m_fRotationSpeed);
	SetupPerspective();
  m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, NULL);

	//
	// The particle system will need to know how much time has passed since 
	// the last time it was updated, so we'll need to keep track of how much   
	// time has elapsed since the last frame update...
	//

	m_dwCurTime = timeGetTime();
	m_fElapsedTime = (m_dwCurTime - m_dwLastTime)/m_fUpdateSpeed;
	m_dwLastTime = m_dwCurTime;
	m_ParticleSystem.Update( m_fElapsedTime );


	//
	// Prepare to render particle system
	//

	//
	// Setting D3DRS_ZWRITEENABLE to FALSE makes the Z-Buffer read-only, which 
	// helps remove graphical artifacts generated from  rendering a list of 
	// particles that haven't been sorted by distance to the eye.
	//
	// Setting D3DRS_ALPHABLENDENABLE to TRUE allows particles, which overlap, 
	// to alpha blend with each other correctly.
	//

	d3dSetRenderState(D3DRS_CULLMODE, false); //D3DCULL_CCW);
	d3dSetRenderState(D3DRS_LIGHTING, TRUE);

	d3dSetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	d3dSetRenderState( D3DRS_ZWRITEENABLE, FALSE );
	d3dSetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	d3dSetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );

	//
	// Render particle system
	//

	m_ParticleSystem.Render( m_pd3dDevice );
}


extern "C" void Stop()
{
	OutputDebugString("Stop()\n");
  m_ParticleSystem.dtor();
  //free(m_pssSettings);
}


extern "C" void GetInfo(VIS_INFO* pInfo)
{
	memcpy(pInfo,&vInfo,sizeof(struct VIS_INFO ) );
}



void CreateArrays()
{
	ZeroMemory(m_pFreq, 1024);
}

void LoadSettings()
{
	OutputDebugString("LoadSettings()\n");
    m_ParticleSystem.ctor();

	SetDefaults();
    m_ParticleSystem.Init( m_pd3dDevice );

	XmlNode node, node2, node3, node4, node5;
	CXmlDocument doc;

	m_iNumSettings = 0;
	char szXMLFile[1024];
	strcpy(szXMLFile,"P:\\Visualisations\\");
  strcat(szXMLFile,"fountain.xml");
  FILE *f = fopen(szXMLFile,"r");
  if (!f)
  {
    strcpy(szXMLFile,"T:\\Visualisations\\");
    strcat(szXMLFile,"fountain.xml");
  }
  else
    fclose(f);

	// Load the config file
	if (doc.Load(szXMLFile)<0)
	{
		OutputDebugString("Failed to load fountain.xml for global settings\n");
		return;
	}
	
	OutputDebugString("Loaded fountain.xml for global settings\n");

	node = doc.GetNextNode(XML_ROOT_NODE);
	OutputDebugString("Loaded root node()\n");
	//first loop is for global settings
	while (node>0)
	{
		if (!strcmpi(doc.GetNodeTag(node),"visualisation"))
		{
			node = doc.GetNextNode(node);
			continue;
		}
		
		//first set all global settings
		if (node2 = doc.GetChildNode(node,"MaxParticles"))
		{
			m_ParticleSystem.SetMaxParticles(atoi(doc.GetNodeText(node2)));
		}

		if (node2 = doc.GetChildNode(node,"bars"))
		{
			m_iBars = (atoi(doc.GetNodeText(node2)));
		}

		if (node2 = doc.GetChildNode(node,"freqscale"))
		{
			m_bLogScale = !strcmpi(doc.GetNodeText(node2),"log");
		}

		if (node2 = doc.GetChildNode(node,"freqmin"))
		{
			m_fMinFreq = atof(doc.GetNodeText(node2));
			if (m_fMinFreq < MIN_FREQUENCY) m_fMinFreq = MIN_FREQUENCY;
			if (m_fMinFreq > MAX_FREQUENCY-1) m_fMinFreq = MAX_FREQUENCY-1;
		}

		if (node2 = doc.GetChildNode(node,"freqmax"))
		{
			m_fMaxFreq = atof(doc.GetNodeText(node2));
			if (m_fMaxFreq <= m_fMinFreq) m_fMaxFreq = m_fMinFreq+1;
			if (m_fMaxFreq > MAX_FREQUENCY) m_fMaxFreq = MAX_FREQUENCY;
		}

		if (node2 = doc.GetChildNode(node,"levelmin"))
		{
			m_fMinLevel = atof(doc.GetNodeText(node2));
			if (m_fMinLevel < MIN_LEVEL) m_fMinLevel = MIN_LEVEL;
			if (m_fMinLevel > MAX_LEVEL-1) m_fMinLevel = MAX_LEVEL-1;
		}

		if (node2 = doc.GetChildNode(node,"levelmax"))
		{
			m_fMaxLevel = atof(doc.GetNodeText(node2));
			if (m_fMaxLevel <= m_fMinLevel) m_fMaxLevel = m_fMinLevel+1;
			if (m_fMaxLevel > MAX_LEVEL) m_fMaxLevel = MAX_LEVEL;
		}

		if (node2 = doc.GetChildNode(node,"CycleParticleSystems"))
		{
			m_bCycleSettings = !strcmpi(doc.GetNodeText(node2), "true");
		}

		//now count number of individual settings
		if (!strcmpi(doc.GetNodeTag(node),"ParticleSystem"))
		{
			m_iNumSettings++;
		}

		node = doc.GetNextNode(node);
	}

	//if we found some settings, reset our settings array
	m_iNumSettings = min(m_iNumSettings, MAX_SETTINGS);

	//set up individual particle systems with default settings
	for(int i = 0; i < m_iNumSettings; i++)
	{
		SetDefaults(&m_pssSettings[i]);
	}

	//now populate individual settings if we found any
	if (m_iNumSettings > 0)
	{
		node = doc.GetNextNode(XML_ROOT_NODE);
		int iPos = -1;
		XmlNode currChild = -1;
		while (node>0 && iPos < m_iNumSettings)
		{
			if (!strcmpi(doc.GetNodeTag(node),"visualisation"))
			{
				node = doc.GetNextNode(node);
				continue;
			}

			if (node2 = doc.GetChildNode(node, "ParticleSystem"))
			{
				if (node2 > currChild)
				{
					currChild = node2;
					iPos++;
					continue;
				}

				if (node3 = doc.GetChildNode(node,"ParticleTexture"))
				{
					//TODO: figure out how to get this working (not sure why it doesn't right now)
					//m_pssSettings[iPos].m_chTexFile = doc.GetNodeText(node3);
				}

				if (node3 = doc.GetChildNode(node,"RotationSpeed"))
				{
					m_pssSettings[iPos].m_fRotationSpeed = atof(doc.GetNodeText(node3));
				}

				if (node3 = doc.GetChildNode(node,"RotationSensitivity"))
				{
					m_pssSettings[iPos].m_fRotationSensitivity = atof(doc.GetNodeText(node3));
				}

				if (node3 = doc.GetChildNode(node,"RotationBar"))
				{
					m_pssSettings[iPos].m_iRotationBar = atoi(doc.GetNodeText(node3));
				}

				if (node3 = doc.GetChildNode(node2,"Color"))
				{
					if (node4 = doc.GetChildNode(node3, "Hue")) {
						if (node5 = doc.GetChildNode(node4, "Min")) {
							m_pssSettings[iPos].m_csHue.min = atof(doc.GetNodeText(node5));
						}
						
						if (node5 = doc.GetChildNode(node4, "Max")) {
							m_pssSettings[iPos].m_csHue.max = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "ShiftRate")) {
							m_pssSettings[iPos].m_csHue.shiftRate = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "Modifier")) {
							m_pssSettings[iPos].m_csHue.modifier = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "Variation")) {
							m_pssSettings[iPos].m_csHue.variation = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "Bar")) {
							m_pssSettings[iPos].m_csHue.bar = atoi(doc.GetNodeText(node5));
						}
					}

					if (node4 = doc.GetChildNode(node3, "Saturation")) {
						if (node5 = doc.GetChildNode(node4, "Min")) {
							m_pssSettings[iPos].m_csSaturation.min = atof(doc.GetNodeText(node5));
						}
						
						if (node5 = doc.GetChildNode(node4, "Max")) {
							m_pssSettings[iPos].m_csSaturation.max = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "ShiftRate")) {
							m_pssSettings[iPos].m_csSaturation.shiftRate = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "Modifier")) {
							m_pssSettings[iPos].m_csSaturation.modifier = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "Variation")) {
							m_pssSettings[iPos].m_csSaturation.variation = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "Bar")) {
							m_pssSettings[iPos].m_csSaturation.bar = atoi(doc.GetNodeText(node5));
						}
					}

					if (node4 = doc.GetChildNode(node3, "Value")) {
						if (node5 = doc.GetChildNode(node4, "Min")) {
							m_pssSettings[iPos].m_csValue.min = atof(doc.GetNodeText(node5));
						}
						
						if (node5 = doc.GetChildNode(node4, "Max")) {
							m_pssSettings[iPos].m_csValue.max = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "ShiftRate")) {
							m_pssSettings[iPos].m_csValue.shiftRate = atof(doc.GetNodeText(node5));
						}
						
						if (node5 = doc.GetChildNode(node4, "Modifier")) {
							m_pssSettings[iPos].m_csValue.modifier = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "Variation")) {
							m_pssSettings[iPos].m_csValue.variation = atof(doc.GetNodeText(node5));
						}

						if (node5 = doc.GetChildNode(node4, "Bar")) {
							m_pssSettings[iPos].m_csValue.bar = atoi(doc.GetNodeText(node5));
						}
					}
				}

				if (node3 = doc.GetChildNode(node2,"AirResistance"))
				{
					m_pssSettings[iPos].m_bAirResistence = !strcmpi(doc.GetNodeText(node3),"true");
				}

				if (node3 = doc.GetChildNode(node2,"NumToRelease"))
				{
					m_pssSettings[iPos].m_dwNumToRelease = atoi(doc.GetNodeText(node3));
				}

				if (node3 = doc.GetChildNode(node2,"NumToReleaseModifier"))
				{
					m_pssSettings[iPos].m_fNumToReleaseMod = atof(doc.GetNodeText(node3));
				}

				if (node3 = doc.GetChildNode(node2,"LifeCycle"))
				{
					m_pssSettings[iPos].m_fLifeCycle = atof(doc.GetNodeText(node3));
				}

				if (node3 = doc.GetChildNode(node2,"ReleaseInterval"))
				{
					m_pssSettings[iPos].m_fReleaseInterval = atof(doc.GetNodeText(node3));
				}

				if (node3 = doc.GetChildNode(node2,"Size"))
				{
					m_pssSettings[iPos].m_fSize = atof(doc.GetNodeText(node3));
				}

				if (node3 = doc.GetChildNode(node2,"VelocityVar"))
				{
					m_pssSettings[iPos].m_fVelocityVar = atof(doc.GetNodeText(node3));
				}

				if (node3 = doc.GetChildNode(node2,"ParticleImage"))
				{
					m_pssSettings[iPos].m_chTexFile = doc.GetNodeText(node3);
				}

				if (node3 = doc.GetChildNode(node2,"Gravity"))
				{
					SetEffectSettings(&m_pssSettings[iPos].m_esGravity, &doc, node3);
				}

				if (node3 = doc.GetChildNode(node2,"Position"))
				{
					SetEffectSettings(&m_pssSettings[iPos].m_esPosition, &doc, node3);
				}

				if (node3 = doc.GetChildNode(node2,"Velocity"))
				{
					SetEffectSettings(&m_pssSettings[iPos].m_esVelocity, &doc, node3);

				}

				if (node3 = doc.GetChildNode(node2,"Wind"))
				{
					SetEffectSettings(&m_pssSettings[iPos].m_esWind, &doc, node3);
				}

			}
			node = doc.GetNextNode(node);
		}
	}
	doc.Close();
}

void SetEffectSettings(EffectSettings* settings, CXmlDocument* doc, XmlNode node)
{
	XmlNode child;
	if (child = doc->GetChildNode(node, "X")) {
		settings->vector.x = atof(doc->GetNodeText(child));
	}

	if (child = doc->GetChildNode(node, "Y")) {
		settings->vector.y = atof(doc->GetNodeText(child));
	}
	
	if (child = doc->GetChildNode(node, "Z")) {
		settings->vector.z = atof(doc->GetNodeText(child));
	}

	if (child = doc->GetChildNode(node, "BarX")) {
		settings->bars.x = atoi(doc->GetNodeText(child));
	}

	if (child = doc->GetChildNode(node, "BarY")) {
		settings->bars.y = atoi(doc->GetNodeText(child));
	}

	if (child = doc->GetChildNode(node, "BarZ")) {
		settings->bars.z = atoi(doc->GetNodeText(child));
	}

	if (child = doc->GetChildNode(node, "Modifier")) {
		settings->modifier = atof(doc->GetNodeText(child));
	}

	if (child = doc->GetChildNode(node,"ModificationMode"))
	{
		if (!strcmpi(doc->GetNodeText(child),"EXPONENTIAL"))
		{
			settings->modificationMode = MODIFICATION_MODE_EXPONENTIAL;
		}
		else
		{
			settings->modificationMode = MODIFICATION_MODE_LINEAR;
		}
	}

	if (child = doc->GetChildNode(node,"Mode"))
	{
		if (!strcmpi(doc->GetNodeText(child),"DIFFERENCE"))
		{
			settings->mode = MODE_DIFFERENCE;
		}
		else if (!strcmpi(doc->GetNodeText(child),"LEVEL"))
		{
			settings->mode = MODE_LEVEL;
		}
		else
		{
			settings->mode = MODE_BOTH;
		}
	}

	if (child = doc->GetChildNode(node,"Invert"))
	{
		settings->bInvert = !strcmpi(doc->GetNodeText(child),"true");
	}
}


void SetDefaults()
{
	m_ParticleSystem.SetMaxParticles(1000);
	vInfo.iSyncDelay=15;
	SetDefaults(&m_pssSettings[0]);
	SetDefaults(&m_pssSettings[1]);
}

void SetDefaults(ParticleSystemSettings* settings)
{
	settings->m_bAirResistence		= true;
	settings->m_chTexFile			= szDefaultTexFile;
	settings->m_hsvColor			= HsvColor(0.0f, 1.0f, 0.6f);
	settings->m_dwNumToRelease		= 2;
	settings->m_fNumToReleaseMod	= 0.0f;
	settings->m_fLifeCycle			= 3.0f;
	settings->m_fReleaseInterval	= 0.0f;
	settings->m_fSize				= 0.4f;
	settings->m_fVelocityVar		= 1.5f;
	
	SetDefaults(&settings->m_esGravity);
	settings->m_esGravity.vector = D3DXVECTOR3(0, 0, -15);
	
	SetDefaults(&settings->m_esPosition);
	
	SetDefaults(&settings->m_esVelocity);
	settings->m_esVelocity.vector = D3DXVECTOR3(-4, 4, 0);
	
	SetDefaults(&settings->m_esWind);
	settings->m_esWind.vector = D3DXVECTOR3(2, -2, 0);

	settings->m_csHue.min		= 0.0f;
	settings->m_csHue.max		= 360.0f;
	settings->m_csHue.shiftRate	= 0.1f;
	settings->m_csHue.modifier	= 360.0f;
	settings->m_csHue.variation	= 45.0f;
	settings->m_csHue.bar		= 1;

	settings->m_csSaturation.min		= 1.0f;
	settings->m_csSaturation.min		= 1.0f;
	settings->m_csSaturation.shiftRate	= 0.0f;
	settings->m_csSaturation.modifier	= 0.0f;
	settings->m_csSaturation.variation	= 0.0f;
	settings->m_csSaturation.bar		= 1;

	settings->m_csValue.min			= 0.2f;
	settings->m_csValue.min			= 0.6f;
	settings->m_csValue.shiftRate	= 0.0005f;
	settings->m_csValue.modifier	= 0.0f;
	settings->m_csValue.variation	= 0.3f;
	settings->m_csValue.bar			= 1;

	settings->m_fRotationSpeed			= 0.1f;
	settings->m_fRotationSensitivity	= 0.02;
	settings->m_iRotationBar			= 1;
}

void SetDefaults(EffectSettings* settings)
{
	settings->bars				= D3DXVECTOR3( 1, 1, 1 );
	settings->bInvert			= false;
	settings->modifier			= 0.0f;
	settings->mode				= MODE_BOTH;
	settings->modificationMode	= MODIFICATION_MODE_LINEAR;
	settings->vector			= D3DXVECTOR3( 0, 0, 0 );
}



void SetupCamera()
{
    //Here we will setup the camera.
    //The camera has three settings: "Camera Position", "Look at Position" and "Up Direction"
    //We have set the following:
    //Camera Position: (0, 0, -30)
    //Look at Position: (0, 0, 0)
    //Up direction: Y-Axis.
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH(&matView, &D3DXVECTOR3(0.0f, 0.0f,-30.0f), //Camera Position
                                 &D3DXVECTOR3(0.0f, 0.0f, 0.0f), //Look At Position
                                 &D3DXVECTOR3(0.0f, 1.0f, 0.0f)); //Up Direction

    m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
}

void SetupPerspective()
{
    //Here we specify the field of view, aspect ration and near and far clipping planes.
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI/4, 1.0f, 1.0f, 100.0f);
    m_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

void SetupRotation(float x, float y, float z)
{
	////Here we will rotate our view around the x, y and z axis.
	D3DXMATRIX matView, matRot;
	m_pd3dDevice->GetTransform(D3DTS_VIEW, &matView);

	D3DXMatrixRotationYawPitchRoll(&matRot, x, y, z); 

	D3DXMatrixMultiply(&matView, &matView, &matRot);

	m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);
}

void ShiftColor(ParticleSystemSettings* settings)
{
	float hadjust	= m_pssSettings[m_iCurrSetting].m_csHue.shiftRate;
	float hmin		= m_pssSettings[m_iCurrSetting].m_csHue.min;
	float hmax		= m_pssSettings[m_iCurrSetting].m_csHue.max;
	
	float sadjust	= m_pssSettings[m_iCurrSetting].m_csSaturation.shiftRate;
	float smin		= m_pssSettings[m_iCurrSetting].m_csSaturation.min;
	float smax		= m_pssSettings[m_iCurrSetting].m_csSaturation.max;

	float vadjust	= m_pssSettings[m_iCurrSetting].m_csValue.shiftRate;
	float vmin		= m_pssSettings[m_iCurrSetting].m_csValue.min;
	float vmax		= m_pssSettings[m_iCurrSetting].m_csValue.max;

	m_clrColor.h += hadjust * m_iHDir;
	if ( m_clrColor.h >= hmax  || m_clrColor.h <= hmin )
	{
		m_clrColor.h -= hadjust * m_iHDir;
		m_iHDir *= -1;
	}

	m_clrColor.h = min(m_clrColor.h, hmax);
	m_clrColor.h = max(m_clrColor.h, hmin);

	m_clrColor.s += sadjust * m_iSDir;
	if ( m_clrColor.s >= smax  || m_clrColor.s <= smin )
	{
		m_clrColor.s -= sadjust * m_iSDir;
		m_iSDir *= -1;
	}

	m_clrColor.s = min(m_clrColor.s, smax);
	m_clrColor.s = max(m_clrColor.s, smin);

	m_clrColor.v += vadjust * m_iVDir;
	if ( m_clrColor.v >= vmax  || m_clrColor.v <= vmin )
	{
		m_clrColor.v -= vadjust * m_iVDir;
		m_iVDir *= -1;
	}

	m_clrColor.v = min(m_clrColor.v, vmax);
	m_clrColor.v = max(m_clrColor.v, vmin);

	float audioh = (m_pFreq[settings->m_csHue.bar] / MAX_LEVEL)			* settings->m_csHue.modifier;
	float audios = (m_pFreq[settings->m_csSaturation.bar] / MAX_LEVEL)	* settings->m_csSaturation.modifier;
	float audiov = (m_pFreq[settings->m_csValue.bar] / MAX_LEVEL)		* settings->m_csValue.modifier;

	float h = m_clrColor.h + audioh;
	while(h > 360)
		h -= 360;

	float s = m_clrColor.s + audios;
	while(s > 1)
		s -= 1;

	float v = m_clrColor.v + audiov;
	while(v > 1)
		v -= 1;

	h = min(h, hmax);
	h = max(h, hmin);

	s = min(s, smax);
	s = max(s, smin);

	v = min(v, vmax);
	v = max(v, vmin);

	m_ParticleSystem.SetColor( HsvColor(h, s, v) );
}

void InitParticleSystem(ParticleSystemSettings settings)
{
	//m_chTexFile		= settings.m_chTexFile;
	m_ParticleSystem.SetNumToRelease	( settings.m_dwNumToRelease );
	m_ParticleSystem.SetReleaseInterval	( settings.m_fReleaseInterval );
	m_ParticleSystem.SetLifeCycle		( settings.m_fLifeCycle );
	m_ParticleSystem.SetSize			( settings.m_fSize );
	m_ParticleSystem.SetColor			( settings.m_hsvColor );
	m_ParticleSystem.SetPosition		( settings.m_esPosition.vector );
	m_ParticleSystem.SetVelocity		( settings.m_esVelocity.vector );
	m_ParticleSystem.SetGravity			( settings.m_esGravity.vector );
	m_ParticleSystem.SetWind			( settings.m_esWind.vector );
	m_ParticleSystem.SetAirResistence	( settings.m_bAirResistence );
	m_ParticleSystem.SetVelocityVar		( settings.m_fVelocityVar );
	
	m_ParticleSystem.SetMaxH			( settings.m_csHue.max );
	m_ParticleSystem.SetMinH			( settings.m_csHue.min );
	m_ParticleSystem.SetHVar			( settings.m_csHue.variation );

	m_ParticleSystem.SetMaxS			( settings.m_csSaturation.max );
	m_ParticleSystem.SetMinS			( settings.m_csSaturation.min );
	m_ParticleSystem.SetSVar			( settings.m_csSaturation.variation );

	m_ParticleSystem.SetMaxV			( settings.m_csValue.max );
	m_ParticleSystem.SetMinV			( settings.m_csValue.min );
	m_ParticleSystem.SetVVar			( settings.m_csValue.variation );

	char szTexFile[1024];
	strcpy(szTexFile, "Q:\\Visualisations\\");
	strcat(szTexFile, settings.m_chTexFile);
	m_ParticleSystem.SetTexture( szTexFile, m_pd3dDevice );
}

D3DXVECTOR3 Shift(EffectSettings* settings)
{
	int xBand = min(m_iBars, settings->bars.x);
	int yBand = min(m_iBars, settings->bars.y);
	int zBand = min(m_iBars, settings->bars.z);

	xBand-=1;
	xBand*=2;

	yBand-=1;
	yBand*=2;

	zBand-=1;
	zBand*=2;

	if (settings->modifier == 0.0f)
		return settings->vector;
	
	float x, y, z;
	
	if (settings->mode == MODE_DIFFERENCE)
	{
		x = abs((m_pFreq[xBand] - m_pFreqPrev[xBand])/MAX_LEVEL);
		y = abs((m_pFreq[yBand] - m_pFreqPrev[yBand])/MAX_LEVEL);
		z = abs((m_pFreq[zBand] - m_pFreqPrev[zBand])/MAX_LEVEL);
	}
	else if (settings->mode == MODE_LEVEL)
	{
		x = m_pFreq[xBand]/MAX_LEVEL;
		y = m_pFreq[yBand]/MAX_LEVEL;
		z = m_pFreq[zBand]/MAX_LEVEL;
	}
	else
	{
		x = max(1, (m_pFreq[xBand] + abs(m_pFreq[xBand] - m_pFreqPrev[xBand]))/MAX_LEVEL);
		y = max(1, (m_pFreq[yBand] + abs(m_pFreq[yBand] - m_pFreqPrev[yBand]))/MAX_LEVEL);
		z = max(1, (m_pFreq[zBand] + abs(m_pFreq[zBand] - m_pFreqPrev[zBand]))/MAX_LEVEL);
	}

	if (settings->bInvert)
	{
		x = 1 - x;
		y = 1 - y;
		z = 1 - z;
	}

	x = getRandomMinMax(-x, x);
	y = getRandomMinMax(-y, y);
	z = getRandomMinMax(-z, z);

	if (settings->modificationMode == MODIFICATION_MODE_LINEAR)
	{
		x = (x * settings->modifier * settings->vector.x);
		y = (y * settings->modifier * settings->vector.y);
		z = (z * settings->modifier * settings->vector.z);
	}
	else
	{
		//hack--for some reason, floats seem to cause powf to break ???
		int mod = settings->modifier;
		x = (powf((x + 1) * settings->vector.x, mod));
		y = (powf((y + 1) * settings->vector.y, mod));
		z = (powf((z + 1) * settings->vector.z, mod));

		x = randomizeSign(x);
		y = randomizeSign(y);
		z = randomizeSign(z);
	}
	
	x += settings->vector.x;
	y += settings->vector.y;
	z += settings->vector.z;

	return D3DXVECTOR3(x, y, z);
}

extern "C" 
{
	struct Visualisation
	{
	public:
		void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName);
		void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
		void (__cdecl* AudioData)(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);
		void (__cdecl* Render) ();
		void (__cdecl* Stop)();
		void (__cdecl* GetInfo)(VIS_INFO* pInfo);
	};

	void __declspec(dllexport) get_module(struct Visualisation* pVisz)
	{
		pVisz->Create = Create;
		pVisz->Start = Start;
		pVisz->AudioData = AudioData;
		pVisz->Render = Render;
		pVisz->Stop = Stop;
		pVisz->GetInfo = GetInfo;
	}
};