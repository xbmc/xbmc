// Spectrum.cpp: implementation of the CSpectrum class.
//
//////////////////////////////////////////////////////////////////////

#include "xbmc_vis.h"
#include "XmlDocument.h"
#include <io.h>
#include <math.h>

inline long double sqr( long double arg )
{
  return arg * arg;
}

typedef enum _WEIGHT {
	WEIGHT_NONE = 0,
	WEIGHT_A    = 1,
	WEIGHT_B    = 2,
	WEIGHT_C		= 3
} WEIGHT;


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

    struct VERTEX { D3DXVECTOR4 p; D3DCOLOR col; FLOAT tu, tv; };
	static const DWORD FVF_VERTEX = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1;

	typedef enum _DIRECTION {
		DIRECTION_UPDOWN = 0,
		DIRECTION_LEFTRIGHT = 1,
		DIRECTION_UP = 2,
		DIRECTION_LEFT = 3,
		DIRECTION_DOWN = 4,
		DIRECTION_RIGHT = 5,
	} DIRECTION;



VIS_INFO										vInfo;
	LPDIRECT3DTEXTURE8				m_pTexture[MAX_CHANNELS];				// textures
	LPDIRECT3DTEXTURE8				m_pAlphaTexture;
	LPDIRECT3DVERTEXBUFFER8			m_pVB;

#define SCREEN_HEIGHT 576
#define SCREEN_WIDTH 720
  float m_fPosX;
  float m_fPosY;
  float m_fWidth;
  float m_fHeight;
  LPDIRECT3DTEXTURE8				m_pOutputTexture;

  void ClearArrays();
	void CreateArrays();
	void CreateBarTexture();
	void LoadSettings();
	void SetDefaults();

	// Arrays to store frequency data
	float	m_pScreen[MAX_BARS*2];			// Current levels on the screen
	float	m_pPeak[MAX_BARS*2];			// Peak levels
	float	m_pWeight[FREQ_DATA_SIZE/2+1];		// A/B/C weighted levels for speed
	float	m_pFreq[MAX_BARS*2];			// Frequency data

	int		m_iSampleRate;

	// Member variables that users can play with
	int				m_iBars;				// number of bars to draw
	bool			m_bLogScale;			// true if our frequency is on a log scale
	bool			m_bShowPeaks;			// show peaks?
	bool			m_bAverageLevels;		// show average levels?
	float			m_fPeakDecaySpeed;		// speed of decay (in dB/frame)
	float			m_fRiseSpeed;			// division of rise to actually go up
	float			m_fFallSpeed;			// division of fall to actually go up
	float			m_fMinFreq;				// wanted frequency range
	float			m_fMaxFreq;
	float			m_fMinLevel;			// wanted level range
	float			m_fMaxLevel;
	WEIGHT			m_Weight;				// weighting type to be applied
	bool			m_bMixChannels;			// Mix channels, or stereo?
  bool      m_bSeperateBars;
  static  char m_szVisName[1024];
	// per channel stuff
	D3DCOLOR		m_colTop[MAX_CHANNELS];			// top colour of bars
	D3DCOLOR		m_colMiddle[MAX_CHANNELS];		// middle colour of bars
	D3DCOLOR		m_colBottom[MAX_CHANNELS];		// bottom colour of bars
	D3DCOLOR		m_colPeak[MAX_CHANNELS];		// colour of the peak indicators
	RECT			m_rPosition[MAX_CHANNELS];		// position of the Spectrum
	DIRECTION		m_Direction[MAX_CHANNELS];
	bool			m_bDrawBorder[MAX_CHANNELS];
	LPDIRECT3DDEVICE8 m_pd3dDevice;

  // presets stuff
#define MAX_PRESETS 256
  char      m_szPresetsDir[256];  // as in the xml
  char      m_szPresetsPath[512]; // fully qualified path
  char*     m_szPresets[256];
  int       m_nPresets = 0;
  int       m_nCurrentPreset = 0;

#define POLE1 20.598997*20.598997	// for A/B/C weighting
#define POLE2 12194.217*12194.217	// for A/B/C weighting
#define POLE3 107.65265*107.65265	// for A weighting
#define POLE4 737.86223*737.86223	// for A weighting
#define POLE5 158.5*158.5			// for B weighting

int inline sign(float a) {return (a>0) ? 1 : -1;}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName)
{
  m_fPosX = (float)iPosX;
  m_fPosY = (float)iPosY;
  m_fWidth = (float)iWidth;
  m_fHeight = (float)iHeight;
  strcpy(m_szVisName,szVisualisationName);
	for (int i=0; i<MAX_CHANNELS; i++)
		m_pTexture[i]=NULL;
	m_pAlphaTexture = NULL;
  m_pOutputTexture = NULL;
	m_pVB=NULL;
	m_pd3dDevice = pd3dDevice;
	vInfo.bWantsFreq = true;
  // Clear out our presets
  for (int i=0; i < MAX_PRESETS; i++)
    m_szPresets[i] = NULL;
}

void Initialize()
{
	ClearArrays();
	CreateArrays();
	CreateBarTexture();

	// Now create the vertexbuffer
	if (m_pVB)
	{
		m_pVB->Release();
		m_pVB = NULL;
	}
	m_pd3dDevice->CreateVertexBuffer( m_iBars*4*4*sizeof(VERTEX), D3DUSAGE_WRITEONLY, 0L, D3DPOOL_DEFAULT, &m_pVB );

	VERTEX* v=NULL;
	m_pVB->Lock( 0, 0, (BYTE**)&v, 0L );

	int iVert = 0;
	float fX, fY, fXOffset, fYOffset;
	for (int i=0; i<4; i++) 
	{
		D3DCOLOR col1 = 0xFFFFFFFF; // Change for nice graduations etc...
		D3DCOLOR col2 = 0xFFFFFFFF;
		// Calculate bar base line and width
		fX = (float)m_rPosition[i/2].left;
		fY = (float)m_rPosition[i/2].bottom; 
		if (m_Direction[i/2] == DIRECTION_UPDOWN)
		{
			fXOffset = (float)(m_rPosition[i/2].right-m_rPosition[i/2].left)/m_iBars;
			fYOffset = 0;
		}
		else
		{
			fXOffset = 0;
			fYOffset = (float)(m_rPosition[i/2].top-m_rPosition[i/2].bottom)/m_iBars;
		}
		for (int j=0; j<m_iBars; j++)
		{
      float fPosX1=fX ;
      float fPosX2=fX + fXOffset;
      
			v[iVert].p = D3DXVECTOR4(fPosX1, fY, 0, 0 );
			v[iVert].tu = 0;
			v[iVert].tv = 0;
			v[iVert++].col= col1;	    

			v[iVert].p = D3DXVECTOR4(fPosX2, fY + fYOffset, 0, 0 );
			v[iVert].tu = TEXTURE_WIDTH;
			v[iVert].tv = 0;
			v[iVert++].col= col1;

			v[iVert].p = D3DXVECTOR4(fPosX2, fY + fYOffset, 0, 0 );
			v[iVert].tu = TEXTURE_WIDTH;
			v[iVert].tv = 0;
			v[iVert++].col= col2;

			v[iVert].p = D3DXVECTOR4(fPosX1, fY, 0, 0 );
			v[iVert].tu = 0;
			v[iVert].tv = 0;
			v[iVert++].col= col2;
			fX+=fXOffset;
			fY+=fYOffset;
		}
	}
  m_pVB->Unlock();
}

void LoadPreset(int num);

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
	m_iSampleRate = iSamplesPerSec;

	// Load our settings
	LoadSettings();
  LoadPreset(m_nCurrentPreset);
}

void ClearArrays()
{
	// Delete our current arrays, if they exist
//	if (m_pScreen) delete[] m_pScreen;
//	m_pScreen = NULL;
//	if (m_pPeak) delete[] m_pPeak;
//	m_pPeak = NULL;
//	if (m_pFreq) delete[] m_pFreq;
//	m_pFreq = NULL;
//	if (m_pWeight) delete[] m_pWeight;
//	m_pWeight = NULL;
}

void CreateArrays()
{
	// create fresh arrays again
//	m_pScreen = new float[m_iBars*2];
//	m_pPeak = new float[m_iBars*2];
//	m_pFreq = new float[m_iBars*2];
	// and initialize them
	for (int i=0; i<m_iBars*2; i++)
	{
		m_pScreen[i] = 0.0f;
		m_pPeak[i] = 0.0f;
		m_pFreq[i] = 0.0f;
	}
	// and the weight array
	if (m_Weight == WEIGHT_NONE)
		return;
	// calculate the weights (squared)
	float f2;
//	m_pWeight = new float[FREQ_DATA_SIZE/2+1];
	for (int i=0; i<FREQ_DATA_SIZE/2+1; i++)
	{
		f2 = (float)sqr((float)i*m_iSampleRate/FREQ_DATA_SIZE);
		if (m_Weight == WEIGHT_A)
			m_pWeight[i] = (float)sqr(POLE2*sqr(f2)/(f2+POLE1)/(f2+POLE2)/sqrt(f2+POLE3)/sqrt(f2+POLE4));
		else if (m_Weight == WEIGHT_B)
			m_pWeight[i] = (float)sqr(POLE2*f2*sqrt(f2)/(f2+POLE1)/(f2+POLE2)/sqrt(f2+POLE5));
		else  // m_Weight == WEIGHT_C
			m_pWeight[i] = (float)sqr(POLE2*f2/(f2+POLE1)/(f2+POLE2));
	}
}

void CreateBarTexture()
{
  // create render target
  if (m_pOutputTexture)
  {
    m_pOutputTexture->Release();
    m_pOutputTexture = NULL;
  }
	if (D3D_OK != m_pd3dDevice->CreateTexture( SCREEN_WIDTH, SCREEN_HEIGHT, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &m_pOutputTexture ))
		return;

/*	m_pTexture = NULL;
	D3DXIMAGE_INFO pSrcInfo;
	if (D3DXCreateTextureFromFileEx(m_pd3dDevice, "e:\\texture.bmp",
		 D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
		 D3DX_FILTER_NONE , D3DX_FILTER_NONE, 0xFF000000, &pSrcInfo, NULL, &m_pTexture)!=D3D_OK)
	{
		OutputDebugString("Unable to find texture to test ");
//		OutputDebugString(szFilename);
		OutputDebugString("\n");
	}
	else
	{
		DWORD width = pSrcInfo.Width;
		DWORD height = pSrcInfo.Height;
	}*/
	if (m_pAlphaTexture)
	{
		m_pAlphaTexture->Release();
		m_pAlphaTexture = NULL;
	}
	if (D3D_OK != m_pd3dDevice->CreateTexture( TEXTURE_WIDTH, TEXTURE_HEIGHT, 1, 0, D3DFMT_LIN_A8, 0, &m_pAlphaTexture ))
		return;
	
	D3DLOCKED_RECT rectLocked;
	if ( D3D_OK == m_pAlphaTexture->LockRect(0,&rectLocked,NULL,0L  ) )
	{
		BYTE *pBuff   = (BYTE*)rectLocked.pBits;	
		DWORD strideScreen=rectLocked.Pitch;

		for (DWORD y=0; y < TEXTURE_HEIGHT; y++)
		{
			for (DWORD x=0; x < TEXTURE_WIDTH; x++)
			{
				*pBuff++ = (BYTE)(x * 16);
			}
		}
		m_pAlphaTexture->UnlockRect(0);
	}

	for (int i=0; i<MAX_CHANNELS; i++)
	{
		if (m_pTexture[i])
		{
			m_pTexture[i]->Release();
			m_pTexture[i] = NULL;
		}
		if (D3D_OK != m_pd3dDevice->CreateTexture( TEXTURE_WIDTH, TEXTURE_HEIGHT+1, 1, 0, D3DFMT_LIN_A8R8G8B8, 0, &m_pTexture[i] ))
			return;

		D3DLOCKED_RECT rectLocked;
		
		if ( D3D_OK == m_pTexture[i]->LockRect(0,&rectLocked,NULL,0L  ) )
		{
			BYTE *pBuff   = (BYTE*)rectLocked.pBits;	
			DWORD strideScreen=rectLocked.Pitch;
			if (pBuff)
			{	
				int blue1 = m_colTop[i] & 0xFF;
				int green1 = (m_colTop[i]>>8)& 0xFF;
				int red1 = (m_colTop[i]>>16) & 0xFF;
				int blue2 = m_colMiddle[i] & 0xFF;
				int green2 = (m_colMiddle[i]>>8)& 0xFF;
				int red2 = (m_colMiddle[i]>>16) & 0xFF;
				int blue3 = m_colBottom[i] & 0xFF;
				int green3 = (m_colBottom[i]>>8)& 0xFF;
				int red3 = (m_colBottom[i]>>16) & 0xFF;
				for (DWORD y=0; y < TEXTURE_MID; y++)
				{
					BYTE* pDest = (BYTE*)rectLocked.pBits + strideScreen*y;
					// colour transistion
					BYTE blue = (BYTE)((blue2*y+blue3*(TEXTURE_MID-y))/TEXTURE_MID);
					BYTE green = (BYTE)((green2*y+green3*(TEXTURE_MID-y))/TEXTURE_MID);
					BYTE red = (BYTE)((red2*y+red3*(TEXTURE_MID-y))/TEXTURE_MID);
					for (int x=0; x<TEXTURE_WIDTH; x++)
					{
						BYTE alpha = 0xFF;
//						if (y%8==0 || y%8 == 7)	// just added to show off alpha ability
//							alpha = 0xaF;
//						if (x==0)
//							alpha = 0x7F;
						*pDest++ = blue;
						*pDest++ = green;
						*pDest++ = red;
						*pDest++ = alpha;
					}
				}
				for (DWORD y=TEXTURE_MID; y < TEXTURE_HEIGHT; y++)
				{
					BYTE* pDest = (BYTE*)rectLocked.pBits + strideScreen*y;
					// colour transistion
					BYTE blue = (BYTE)((blue1*(y-TEXTURE_MID)+blue2*(TEXTURE_HEIGHT-y))/(TEXTURE_HEIGHT-TEXTURE_MID));
					BYTE green = (BYTE)((green1*(y-TEXTURE_MID)+green2*(TEXTURE_HEIGHT-y))/(TEXTURE_HEIGHT-TEXTURE_MID));
					BYTE red = (BYTE)((red1*(y-TEXTURE_MID)+red2*(TEXTURE_HEIGHT-y))/(TEXTURE_HEIGHT-TEXTURE_MID));
					for (int x=0; x<TEXTURE_WIDTH; x++)
					{
						BYTE alpha = 0xFF;
	//					if (y%8==0 || y%8 == 7)	// just added to show off alpha ability
	//						alpha = 0xaF;
//						if (x==0)
	//						alpha = 0x7F;
						*pDest++ = blue;
						*pDest++ = green;
						*pDest++ = red;
						*pDest++ = alpha;
					}
				}
				// finally the peak colour
				BYTE* pDest = (BYTE*)rectLocked.pBits + strideScreen*TEXTURE_HEIGHT;
				BYTE blue = (BYTE)m_colPeak[i];
				BYTE green = (BYTE)(m_colPeak[i]>>8);
				BYTE red = (BYTE)(m_colPeak[i]>>16);
				for (int x=0; x<TEXTURE_WIDTH; x++)
				{
					BYTE alpha = 0xFF;
//					if (x==0)
//						alpha = 0x7F;
					*pDest++ = blue;
					*pDest++ = green;
					*pDest++ = red;
					*pDest++ = alpha;
				}
			}	
			m_pTexture[i]->UnlockRect(0);
		}
	}
}

extern "C" void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
	if (iFreqDataLength>FREQ_DATA_SIZE)
		iFreqDataLength = FREQ_DATA_SIZE;
	// weight the data using A,B or C-weighting
	if (m_Weight != WEIGHT_NONE)
	{
		for (int i=0; i<iFreqDataLength+2; i+=2)
		{
			pFreqData[i]*=m_pWeight[i>>1];
			pFreqData[i+1]*=m_pWeight[i>>1];
		}
	}
	// Group data into frequency bins by averaging (Ignore the constant term)
	int jmin=2;
	int jmax;
	// FIXME:  Roll conditionals out of loop
	for (int i=0, iBin=0; i < m_iBars; i++, iBin+=2)
	{
		m_pFreq[iBin]=0.000001f;	// almost zero to avoid taking log of zero later
		m_pFreq[iBin+1]=0.000001f;
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
			if (m_bMixChannels)
			{
				if (m_bAverageLevels)
					m_pFreq[iBin]+=pFreqData[j]+pFreqData[j+1];
				else 
				{
					if (pFreqData[j]>m_pFreq[iBin])
						m_pFreq[iBin]=pFreqData[j];
					if (pFreqData[j+1]>m_pFreq[iBin])
						m_pFreq[iBin]=pFreqData[j+1];
				}
			}
			else
			{
				if (m_bAverageLevels)
				{
					m_pFreq[iBin]+=pFreqData[j];
					m_pFreq[iBin+1]+=pFreqData[j+1];
				}
				else
				{
					if (pFreqData[j]>m_pFreq[iBin])
						m_pFreq[iBin]=pFreqData[j];
					if (pFreqData[j+1]>m_pFreq[iBin+1])
						m_pFreq[iBin+1]=pFreqData[j+1];
				}
			}
		}
		if (m_bAverageLevels)
		{
			if (m_bMixChannels)
				m_pFreq[iBin] /=(jmax-jmin);
			else
			{
				m_pFreq[iBin] /= (jmax-jmin)/2;
				m_pFreq[iBin+1] /= (jmax-jmin)/2;
			}
		}
		jmin = jmax;
	}
	// Transform data to dB scale, 0 (Quietest possible) to 96 (Loudest)
	for (int i=0; i < m_iBars*2; i++)
	{
		m_pFreq[i] = 10*log10(m_pFreq[i]);
		if (m_pFreq[i] > MAX_LEVEL)
			m_pFreq[i] = MAX_LEVEL;
		if (m_pFreq[i] < MIN_LEVEL)
			m_pFreq[i] = MIN_LEVEL;
	}
}

extern "C" void Render()
{
	// safety to make sure we have our textures defined
	if (!m_pTexture[0] || !m_pTexture[1] || !m_pAlphaTexture)
		return;
	if (!m_pVB)
		return;

	for (int i=0; i < m_iBars*2; i++)
	{
		// truncate data to the users range
		if (m_pFreq[i] > m_fMaxLevel)
			m_pFreq[i] = m_fMaxLevel;
		m_pFreq[i]-=m_fMinLevel;
		if (m_pFreq[i] < 0)
			m_pFreq[i] = 0;
		// Smooth out the movement
		if (m_pFreq[i] > m_pScreen[i])
			m_pScreen[i] += (m_pFreq[i]-m_pScreen[i])*m_fRiseSpeed;
		else
			m_pScreen[i] -= (m_pScreen[i]-m_pFreq[i])*m_fFallSpeed;
		// Work out the peaks
		if (m_pScreen[i] >= m_pPeak[i])
		{
			m_pPeak[i] = m_pScreen[i];
		}
		else
		{
			m_pPeak[i]-=m_fPeakDecaySpeed;
			if (m_pPeak[i] < 0)
				m_pPeak[i] = 0;
		}
	}

	// Transform our vertex data...
	VERTEX* v=NULL;
	m_pVB->Lock( 0, 0, (BYTE**)&v, 0L );
	// Calculate the scale + baseline
	float fScale, fBaseLine, fTextureScale;
	float height;
	// How many channels do we have?
	int iChannels = m_bMixChannels ? 1 : 2;
	for (int j=0; j<iChannels; j++)
	{
		if (m_Direction[j] == DIRECTION_UPDOWN)
		{
			fScale = (float)(m_rPosition[j].bottom-m_rPosition[j].top)/(m_fMaxLevel-m_fMinLevel);
			fBaseLine = (float)m_rPosition[j].bottom;
			fTextureScale = (float)TEXTURE_HEIGHT / (m_fMaxLevel-m_fMinLevel);
			for (int i=0, iVert=j*m_iBars*8; i<m_iBars*2;i+=2, iVert+=4)
			{
				height = fBaseLine-m_pScreen[i+j]*fScale;
				v[iVert].p.y = height;
				v[iVert].tv = m_pScreen[i+j]*fTextureScale;
				v[iVert+1].p.y = height;
				v[iVert+1].tv = m_pScreen[i+j]*fTextureScale;
			}
			// Peaks
			if (m_bShowPeaks)
			{
				for (int i=0, iVert=m_iBars*(4+j*8); i<m_iBars*2; i+=2, iVert+=4)
				{
					height = fBaseLine-m_pPeak[i+j]*fScale;
					v[iVert].p.y   = height;
					v[iVert+1].p.y = height;
					if (m_pPeak[i+j]>2.0f/abs(fScale))
					{
						v[iVert+2].p.y = height-2.0f*sign(fScale);
						v[iVert+3].p.y = height-2.0f*sign(fScale);
					}
					else
					{
						v[iVert+2].p.y = height;
						v[iVert+3].p.y = height;
					}
					if (m_colPeak[j])
					{
						v[iVert].tv   = TEXTURE_HEIGHT;
						v[iVert+1].tv = TEXTURE_HEIGHT;
						v[iVert+2].tv = TEXTURE_HEIGHT+1;
						v[iVert+3].tv = TEXTURE_HEIGHT+1;
					}
					else
					{
						v[iVert].tv   = m_pPeak[i+j]*fTextureScale;
						v[iVert+1].tv = m_pPeak[i+j]*fTextureScale;
						v[iVert+2].tv = (m_pPeak[i+j]-1.5f)*fTextureScale;
						v[iVert+3].tv = (m_pPeak[i+j]-1.5f)*fTextureScale;
					}
				}
			}
		}
		else // m_Direction==LEFTRIGHT
		{
			fScale = (float)(m_rPosition[j].left-m_rPosition[j].right)/(m_fMaxLevel-m_fMinLevel);
			fBaseLine = (float)m_rPosition[j].left;
			fTextureScale = (float)TEXTURE_HEIGHT / (m_fMaxLevel-m_fMinLevel);
			for (int i=0, iVert=j*m_iBars*8; i<m_iBars*2;i+=2, iVert+=4)
			{
				height = fBaseLine-m_pScreen[i+j]*fScale;
				v[iVert].p.x = height;
				v[iVert].tv = m_pScreen[i+j]*fTextureScale;
				v[iVert+1].p.x = height;
				v[iVert+1].tv = m_pScreen[i+j]*fTextureScale;
			}
			// Peaks
			if (m_bShowPeaks)
			{
				for (int i=0, iVert=m_iBars*(4+j*8); i<m_iBars*2; i+=2, iVert+=4)
				{
					height = fBaseLine-m_pPeak[i+j]*fScale;
					v[iVert].p.x   = height;
					v[iVert+1].p.x = height;
					if (m_pPeak[i+j]>2.0f/abs(fScale))
					{
						v[iVert+2].p.x = height-2.0f*sign(fScale);
						v[iVert+3].p.x = height-2.0f*sign(fScale);
					}
					else
					{
						v[iVert+2].p.x = height;
						v[iVert+3].p.x = height;
					}
					if (m_colPeak[j])
					{
						v[iVert].tv   = TEXTURE_HEIGHT;
						v[iVert+1].tv = TEXTURE_HEIGHT;
						v[iVert+2].tv = TEXTURE_HEIGHT+1;
						v[iVert+3].tv = TEXTURE_HEIGHT+1;
					}
					else
					{
						v[iVert].tv   = m_pPeak[i+j]*fTextureScale;
						v[iVert+1].tv = m_pPeak[i+j]*fTextureScale;
						v[iVert+2].tv = (m_pPeak[i+j]-1.5f)*fTextureScale;
						v[iVert+3].tv = (m_pPeak[i+j]-1.5f)*fTextureScale;
					}
				}
			}
		}
	}
	m_pVB->Unlock();

	// Set state to render the image
	m_pd3dDevice->SetTexture( 0, m_pTexture[0] );
	d3dSetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	d3dSetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	d3dSetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
	d3dSetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
	d3dSetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
	d3dSetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
	d3dSetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	d3dSetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
	d3dSetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
	d3dSetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
	d3dSetRenderState( D3DRS_ZENABLE,      FALSE );
	d3dSetRenderState( D3DRS_FOGENABLE,    FALSE );
	d3dSetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	d3dSetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
	d3dSetRenderState( D3DRS_CULLMODE,     D3DCULL_NONE );
	d3dSetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	d3dSetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
	d3dSetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	m_pd3dDevice->SetVertexShader( FVF_VERTEX );
  LPDIRECT3DSURFACE8 oldTarget, newTarget;
  m_pOutputTexture->GetSurfaceLevel(0, &newTarget);
  m_pd3dDevice->GetRenderTarget(&oldTarget);
  m_pd3dDevice->SetRenderTarget(newTarget, NULL);
  m_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, NULL);
	// Render the image
	m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
	m_pd3dDevice->DrawPrimitive( D3DPT_QUADLIST, 0, m_iBars*2 );
	m_pd3dDevice->SetTexture(0, m_pTexture[1]);
	m_pd3dDevice->DrawPrimitive( D3DPT_QUADLIST, m_iBars*8, m_iBars*2 );

	VERTEX v2[4];

	m_pd3dDevice->SetTexture(0, NULL);	// reset texture
	// draw borders for Pike
	for (int j=0; j<iChannels; j++)
	{
		if (!m_bDrawBorder[j])
			continue;
		v2[0].p = D3DXVECTOR4((float)m_rPosition[j].left, (float)m_rPosition[j].top, 0, 0 );
		v2[0].tu = 0;
		v2[0].tv = 0;
		v2[0].col= 0x4FFFFFFF;	    
		v2[1].p = D3DXVECTOR4((float)m_rPosition[j].right, (float)m_rPosition[j].top, 0, 0 );
		v2[1].tu = 0;
		v2[1].tv = 0;
		v2[1].col= 0x4FFFFFFF;	    
		v2[2].p = D3DXVECTOR4((float)m_rPosition[j].right, (float)m_rPosition[j].bottom, 0, 0 );
		v2[2].tu = 0;
		v2[2].tv = 0;
		v2[2].col= 0x4FFFFFFF;	    
		v2[3].p = D3DXVECTOR4((float)m_rPosition[j].left, (float)m_rPosition[j].bottom, 0, 0 );
		v2[3].tu = 0;
		v2[3].tv = 0;
		v2[3].col= 0x4FFFFFFF;	    
		m_pd3dDevice->DrawPrimitiveUP( D3DPT_QUADLIST, 1, v2, sizeof(VERTEX));
	}
  m_pd3dDevice->SetRenderTarget(oldTarget, NULL);
  newTarget->Release();
  oldTarget->Release();
  // blit onto screen in the appropriate place
	v2[0].p = D3DXVECTOR4(m_fPosX, m_fPosY, 0, 0 );
	v2[0].tu = 0;
	v2[0].tv = 0;
	v2[0].col= 0xFFFFFFFF;	    
	v2[1].p = D3DXVECTOR4(m_fPosX + m_fWidth, m_fPosY, 0, 0 );
	v2[1].tu = SCREEN_WIDTH;
	v2[1].tv = 0;
	v2[1].col= 0xFFFFFFFF;	    
	v2[2].p = D3DXVECTOR4(m_fPosX + m_fWidth, m_fPosY + m_fHeight, 0, 0 );
	v2[2].tu = SCREEN_WIDTH;
	v2[2].tv = SCREEN_HEIGHT;
	v2[2].col= 0xFFFFFFFF;	    
	v2[3].p = D3DXVECTOR4(m_fPosX, m_fPosY + m_fHeight, 0, 0 );
	v2[3].tu = 0;
	v2[3].tv = SCREEN_HEIGHT;
	v2[3].col= 0xFFFFFFFF;
  m_pd3dDevice->SetTexture(0, m_pOutputTexture);
	m_pd3dDevice->DrawPrimitiveUP( D3DPT_QUADLIST, 1, v2, sizeof(VERTEX));
  m_pd3dDevice->SetTexture(0, NULL);
	return;
}

void SaveSettings();

extern "C" void Stop()
{
  m_vecSettings.clear();
  SaveSettings();
	ClearArrays();
	if (m_pVB)
		m_pVB->Release();
	m_pVB = NULL;
	for (int i=0; i<MAX_CHANNELS; i++)
	{
		if (m_pTexture[i])
			m_pTexture[i]->Release();
		m_pTexture[i] = NULL;
	}
	if (m_pAlphaTexture)
		m_pAlphaTexture->Release();
	m_pAlphaTexture = NULL;
  if (m_pOutputTexture)
    m_pOutputTexture->Release();
  m_pOutputTexture = NULL;

  for (int i=0; i < MAX_PRESETS; i++)
  {
    if (m_szPresets[i])
      delete[] m_szPresets[i];
    m_szPresets[i] = NULL;
  }
}

// Load settings from the Spectrum.xml configuration file
void LoadSettings()
{
  char szXMLFile[1024];
  strcpy(szXMLFile,"P:\\Visualisations\\");
  strcat(szXMLFile,m_szVisName);
  strcat(szXMLFile,".xml");
  FILE *f = fopen(szXMLFile,"r");
  if (!f)
  {
    strcpy(szXMLFile,"T:\\Visualisations\\");
    strcat(szXMLFile,m_szVisName);
    strcat(szXMLFile,".xml");
  }
  else
    fclose(f);
  
  CXmlDocument doc;
  if (doc.Load(szXMLFile) < 0)
    return;

  XmlNode childNode;
  XmlNode node = doc.GetNextNode(XML_ROOT_NODE);
	while (node>0)
	{
		if (strcmpi(doc.GetNodeTag(node),"visualisation"))
		{
			node = doc.GetNextNode(node);
			continue;
		}
		if (childNode = doc.GetChildNode(node,"presets"))
		{
			char *presets = doc.GetNodeText(childNode);
		  // Check if its a zip or a folder
		  int len = strlen(presets);
		  if (len < 4 || strcmpi(presets + len - 4, ".zip") != 0)
		  {
			  // Normal folder
			  strcpy(m_szPresetsPath,  "Q:\\visualisations\\");
			  strcat(m_szPresetsPath,  presets);
			  strcat(m_szPresetsPath, "\\");
		  }
		  else
		  {
			  // Zip file
			  strcpy(m_szPresetsPath, "zip://q%3A%5Cvisualisations%5C"); 
			  strcat(m_szPresetsPath,  presets);
			  strcat(m_szPresetsPath, "/");
		  }
      // save directory for later
      strcpy(m_szPresetsDir, presets);
    }
    if (childNode = doc.GetChildNode(node,"currentpreset"))
    {
      m_nCurrentPreset = atoi(doc.GetNodeText(childNode));
    }
		node = doc.GetNextNode(node);
  }

  // ok, load up our presets
  m_nPresets = 0;
  if (strlen(m_szPresetsPath) > 0)
  { // run through and grab all presets in this folder...
	  struct _finddata_t c_file;
	  long hFile;
    char szMask[512];

	  strcpy(szMask, m_szPresetsPath);
	  int len = strlen(szMask);
	  if (szMask[len-1] != '/') 
	  {
		  strcat(szMask, "/");
	  }
	  strcat(szMask, "*.*");
   
		if( (hFile = _findfirst(szMask, &c_file )) != -1L )		// note: returns filename -without- path
    {
			do
      {
        int len = strlen(c_file.name);
        if (len <= 4 || strcmpi(&c_file.name[len - 4], ".xml") != 0)
          continue;
        if (m_szPresets[m_nPresets]) delete[] m_szPresets[m_nPresets];
				m_szPresets[m_nPresets] = new char[len - 4 + 1];
        strncpy(m_szPresets[m_nPresets], c_file.name, len - 4);
        m_szPresets[m_nPresets][len - 4] = 0;
        m_nPresets++;
			}
			while(_findnext(hFile,&c_file) == 0 && m_nPresets < MAX_PRESETS);

			_findclose( hFile );

      // sort the presets (slow, but how many presets can one really have?)
      for (int i = 0; i < m_nPresets; i++)
      {
        for (int j = i+1; j < m_nPresets; j++)
        {
          if (strcmpi(m_szPresets[i], m_szPresets[j]) > 0)
          { // swap i, j
            char temp[256];
            strcpy(temp, m_szPresets[i]);
            delete[] m_szPresets[i];
            m_szPresets[i] = new char[strlen(m_szPresets[j]) + 1];
            strcpy(m_szPresets[i], m_szPresets[j]);
            delete[] m_szPresets[j];
            m_szPresets[j] = new char[strlen(temp) + 1];
            strcpy(m_szPresets[j], temp);
          }
        }
      }
    }
  }
  // setup our settings structure (passable to GUI)
  m_vecSettings.clear();
  // nothing currently
}

void SaveSettings()
{
  char szXMLFile[1024];
  strcpy(szXMLFile,"P:\\Visualisations\\");
  strcat(szXMLFile,m_szVisName);
  strcat(szXMLFile,".xml");

  WriteXML doc;
  if (!doc.Open(szXMLFile, "visualisation"))
    return;

  doc.WriteTag("presets", m_szPresetsDir);
  doc.WriteTag("currentpreset", m_nCurrentPreset);
  doc.Close();
}

void LoadPreset(int nPreset)
{
	bool bReverseFreq[2] = {false, false};

	// Set up the defaults
	SetDefaults();

  if (m_nPresets > 0)
  {
    if (nPreset < 0) nPreset = m_nPresets - 1;
    if (nPreset >= m_nPresets) nPreset = 0;
    m_nCurrentPreset = nPreset;

    char szPresetFile[1024];
    strcpy(szPresetFile, m_szPresetsPath);
    int len = strlen(szPresetFile);
    if (len > 0 && szPresetFile[len-1] != '/')
      strcat(szPresetFile, "/");
    strcat(szPresetFile, m_szPresets[m_nCurrentPreset]);
    strcat(szPresetFile, ".xml");
    CXmlDocument doc;
	  if (doc.Load(szPresetFile)>=0)
	  {
      XmlNode childNode, grandChild;
		  XmlNode node = doc.GetNextNode(XML_ROOT_NODE);
		  while (node>0)
		  {
			  if (strcmpi(doc.GetNodeTag(node),"preset"))
			  {
				  node = doc.GetNextNode(node);
				  continue;
			  }
			  if (childNode = doc.GetChildNode(node,"bars"))
			  {
				  m_iBars = atoi(doc.GetNodeText(childNode));
				  if (m_iBars < 1)
					  m_iBars = 1;
				  if (m_iBars > MAX_BARS)
					  m_iBars = MAX_BARS;
			  }

			  if (childNode = doc.GetChildNode(node,"freqscale"))
			  {
				  m_bLogScale = !strcmpi(doc.GetNodeText(childNode),"log");
			  }
			  if (childNode = doc.GetChildNode(node,"syncdelay"))
			  {
				  vInfo.iSyncDelay = atoi(doc.GetNodeText(childNode));
				  if (vInfo.iSyncDelay < 0)
					  vInfo.iSyncDelay = 0;
			  }
			  if (childNode = doc.GetChildNode(node,"peakdecayspeed"))
			  {
				  m_fPeakDecaySpeed = (float)atof(doc.GetNodeText(childNode));
				  if (m_fPeakDecaySpeed < MIN_PEAK_DECAY_SPEED)
					  m_fPeakDecaySpeed = MIN_PEAK_DECAY_SPEED;
				  if (m_fPeakDecaySpeed > MAX_PEAK_DECAY_SPEED)
					  m_fPeakDecaySpeed = MAX_PEAK_DECAY_SPEED;
			  }
			  if (childNode = doc.GetChildNode(node,"risespeed"))
			  {
				  m_fRiseSpeed = (float)atof(doc.GetNodeText(childNode));
				  if (m_fRiseSpeed < MIN_RISE_SPEED)
					  m_fRiseSpeed = MIN_RISE_SPEED;
				  if (m_fRiseSpeed > MAX_RISE_SPEED)
					  m_fRiseSpeed = MAX_RISE_SPEED;
			  }
			  if (childNode = doc.GetChildNode(node,"fallspeed"))
			  {
				  m_fFallSpeed = (float)atof(doc.GetNodeText(childNode));
				  if (m_fFallSpeed < MIN_FALL_SPEED)
					  m_fFallSpeed = MIN_FALL_SPEED;
				  if (m_fFallSpeed > MAX_FALL_SPEED)
					  m_fFallSpeed = MAX_FALL_SPEED;
			  }
			  if (childNode = doc.GetChildNode(node,"weight"))
			  {
				  if (!strcmpi(doc.GetNodeText(childNode),"none"))
					  m_Weight = WEIGHT_NONE;
				  if (!strcmpi(doc.GetNodeText(childNode),"A"))
					  m_Weight = WEIGHT_A;
				  if (!strcmpi(doc.GetNodeText(childNode),"B"))
					  m_Weight = WEIGHT_B;
				  if (!strcmpi(doc.GetNodeText(childNode),"C"))
					  m_Weight = WEIGHT_C;
			  }
			  if (childNode = doc.GetChildNode(node,"mixchannels"))
			  {
				  m_bMixChannels = !strcmpi(doc.GetNodeText(childNode),"true");
			  }
        
			  if (childNode = doc.GetChildNode(node,"seperatebars"))
			  {
				  m_bSeperateBars = !strcmpi(doc.GetNodeText(childNode),"true");
			  }
			  if (childNode = doc.GetChildNode(node,"freqmin"))
			  {
				  m_fMinFreq = (float)atof(doc.GetNodeText(childNode));
				  if (m_fMinFreq < MIN_FREQUENCY) m_fMinFreq = MIN_FREQUENCY;
				  if (m_fMinFreq > MAX_FREQUENCY-1) m_fMinFreq = MAX_FREQUENCY-1;
			  }
			  if (childNode = doc.GetChildNode(node,"freqmax"))
			  {
				  m_fMaxFreq = (float)atof(doc.GetNodeText(childNode));
				  if (m_fMaxFreq <= m_fMinFreq) m_fMaxFreq = m_fMinFreq+1;
				  if (m_fMaxFreq > MAX_FREQUENCY) m_fMaxFreq = MAX_FREQUENCY;
			  }
			  if (childNode = doc.GetChildNode(node,"levelmin"))
			  {
				  m_fMinLevel = (float)atof(doc.GetNodeText(childNode));
				  if (m_fMinLevel < MIN_LEVEL) m_fMinLevel = MIN_LEVEL;
				  if (m_fMinLevel > MAX_LEVEL-1) m_fMinLevel = MAX_LEVEL-1;
			  }
			  if (childNode = doc.GetChildNode(node,"levelmax"))
			  {
				  m_fMaxLevel = (float)atof(doc.GetNodeText(childNode));
				  if (m_fMaxLevel <= m_fMinLevel) m_fMaxLevel = m_fMinLevel+1;
				  if (m_fMaxLevel > MAX_LEVEL) m_fMaxLevel = MAX_LEVEL;
			  }
			  if (childNode = doc.GetChildNode(node, "showpeaks"))
			  {
				  m_bShowPeaks = !strcmpi(doc.GetNodeText(childNode),"true");
			  }
			  if (childNode = doc.GetChildNode(node, "leveltype"))
			  {
				  m_bAverageLevels = !strcmpi(doc.GetNodeText(childNode),"average");
			  }
			  if (childNode = doc.GetChildNode(node,"leftchannel"))
			  {
				  if (grandChild = doc.GetChildNode(childNode,"left"))
					  m_rPosition[0].left = atoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"top"))
					  m_rPosition[0].top = atoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"width"))
					  m_rPosition[0].right = atoi(doc.GetNodeText(grandChild)) + m_rPosition[0].left;
				  if (grandChild = doc.GetChildNode(childNode,"height"))
					  m_rPosition[0].bottom = atoi(doc.GetNodeText(grandChild)) + m_rPosition[0].top;
				  if (grandChild = doc.GetChildNode(childNode,"peakcolour"))
					  m_colPeak[0] = htoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"topcolour"))
					  m_colTop[0] = htoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"middlecolour"))
					  m_colMiddle[0] = htoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"bottomcolour"))
					  m_colBottom[0] = htoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"direction"))
				  {
					  if (!strcmpi(doc.GetNodeText(grandChild),"up")) m_Direction[0] = DIRECTION_UP;
					  if (!strcmpi(doc.GetNodeText(grandChild),"down")) m_Direction[0] = DIRECTION_DOWN;
					  if (!strcmpi(doc.GetNodeText(grandChild),"left")) m_Direction[0] = DIRECTION_LEFT;
					  if (!strcmpi(doc.GetNodeText(grandChild),"right")) m_Direction[0] = DIRECTION_RIGHT;
				  }
				  if (grandChild = doc.GetChildNode(childNode,"reversefreq"))
					  bReverseFreq[0] = !strcmpi(doc.GetNodeText(grandChild),"true");
				  if (grandChild = doc.GetChildNode(childNode,"border"))
					  m_bDrawBorder[0] = !strcmpi(doc.GetNodeText(grandChild),"true");
			  }
			  if (childNode = doc.GetChildNode(node,"rightchannel"))
			  {
				  if (grandChild = doc.GetChildNode(childNode,"left"))
					  m_rPosition[1].left = atoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"top"))
					  m_rPosition[1].top = atoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"width"))
					  m_rPosition[1].right = atoi(doc.GetNodeText(grandChild)) + m_rPosition[1].left;
				  if (grandChild = doc.GetChildNode(childNode,"height"))
					  m_rPosition[1].bottom = atoi(doc.GetNodeText(grandChild)) + m_rPosition[1].top;
				  if (grandChild = doc.GetChildNode(childNode,"peakcolour"))
					  m_colPeak[1] = htoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"topcolour"))
					  m_colTop[1] = htoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"middlecolour"))
					  m_colMiddle[1] = htoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"bottomcolour"))
					  m_colBottom[1] = htoi(doc.GetNodeText(grandChild));
				  if (grandChild = doc.GetChildNode(childNode,"direction"))
				  {
					  if (!strcmpi(doc.GetNodeText(grandChild),"up")) m_Direction[1] = DIRECTION_UP;
					  if (!strcmpi(doc.GetNodeText(grandChild),"down")) m_Direction[1] = DIRECTION_DOWN;
					  if (!strcmpi(doc.GetNodeText(grandChild),"left")) m_Direction[1] = DIRECTION_LEFT;
					  if (!strcmpi(doc.GetNodeText(grandChild),"right")) m_Direction[1] = DIRECTION_RIGHT;
				  }
				  if (grandChild = doc.GetChildNode(childNode,"reversefreq"))
					  bReverseFreq[1] = !strcmpi(doc.GetNodeText(grandChild),"true");
				  if (grandChild = doc.GetChildNode(childNode,"border"))
					  m_bDrawBorder[1] = !strcmpi(doc.GetNodeText(grandChild),"true");
			  }
			  node = doc.GetNextNode(node);
		  }
		  doc.Close();
	  }
  }
	int temp;
	// Recalculate our rectangles
	for (int i=0; i<MAX_CHANNELS; i++)
	{
		if (m_Direction[i] == DIRECTION_DOWN)
		{	// down - flip top and bottom
			temp = m_rPosition[i].top;
			m_rPosition[i].top = m_rPosition[i].bottom;
			m_rPosition[i].bottom = temp;
		}
		if (m_Direction[i] == DIRECTION_LEFT)
		{   // left - flip right and left
			temp = m_rPosition[i].left;
			m_rPosition[i].left = m_rPosition[i].right;
			m_rPosition[i].right = temp;
		}
		// convert direction to down to the basics.
		m_Direction[i] = (DIRECTION)(m_Direction[i] & 1);
		if (bReverseFreq[i])
		{
			if (m_Direction[i] == DIRECTION_UPDOWN)
			{	// flip the left and right over
				temp = m_rPosition[i].left;
				m_rPosition[i].left = m_rPosition[i].right;
				m_rPosition[i].right = temp;
			}
			else
			{	// flip the top and bottom over
				temp = m_rPosition[i].top;
				m_rPosition[i].top = m_rPosition[i].bottom;
				m_rPosition[i].bottom = temp;
			}
		}
	}
  Initialize();
}

void SetDefaults()
{
	m_iBars = 128;
	m_bLogScale=true;
	vInfo.iSyncDelay=0;
	m_fPeakDecaySpeed=0.5f;
	m_fRiseSpeed=0.5f;
	m_fFallSpeed=0.5f;
	m_Weight = WEIGHT_NONE;
	m_bMixChannels = false;
	m_fMinFreq = 80;
	m_fMaxFreq = 16000;
	m_fMinLevel = 0;
	m_fMaxLevel = 80;
	m_bShowPeaks = true;
	m_bAverageLevels = false;
  m_bSeperateBars=false;
	// per channel stuff
	m_rPosition[0].left = 104;
	m_rPosition[0].right = 616;
	m_rPosition[0].top = 158;
	m_rPosition[0].bottom = 286;
	m_colBottom[0] = 0x1A0642;
	m_colMiddle[0] = 0x852CAF;
	m_colTop[0] = 0xFF1589;
	m_colPeak[0] = 0xFFFFFF;
	m_Direction[0] = DIRECTION_UPDOWN;
	m_bDrawBorder[0] = false;
	m_rPosition[1].left = 104;
	m_rPosition[1].right = 616;
	m_rPosition[1].top = 418;
	m_rPosition[1].bottom = 290;
	m_colBottom[1] = 0x25036A;	// Dark Blue
	m_colMiddle[1] = 0x9A3AEC;	// Mid Green/Blue
	m_colTop[1] = 0xF91576;		// Bright Red
	m_colPeak[1] = 0xFFFFFF;			// use above range
	m_Direction[1] = DIRECTION_UPDOWN;
	m_bDrawBorder[1] = false;
}

extern "C" bool OnAction(long action, void *param)
{
  printf("OnAction %i called with param=%p", action, param);
  if (!m_nPresets)
    return false;
  if (action == VIS_ACTION_PREV_PRESET)
    LoadPreset(m_nCurrentPreset-1);
  if (action == VIS_ACTION_NEXT_PRESET)
    LoadPreset(m_nCurrentPreset+1);
  if (action == VIS_ACTION_LOAD_PRESET && param)
    LoadPreset(*(int *)param);
  SaveSettings();
  return true;
}

extern "C" void GetInfo(VIS_INFO* pInfo)
{
  pInfo->bWantsFreq =vInfo.bWantsFreq;
	pInfo->iSyncDelay=0;
}

extern "C" void GetSettings(vector<VisSetting> **settings)
{
  if (!settings) return;
  // load in our settings
  LoadSettings();
  *settings = &m_vecSettings;
  return;
}

extern "C" void UpdateSetting(int num)
{
  VisSetting &setting = m_vecSettings[num];
//  SaveSettings();
}

extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
  if (!pPresets || !numPresets || !currentPreset || !locked) return;
  *pPresets = m_szPresets;
  *numPresets = m_nPresets;
  *currentPreset = m_nCurrentPreset;
  *locked = false;
}
