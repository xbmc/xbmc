// Spectrum.cpp: implementation of the CSpectrum class.
//
//////////////////////////////////////////////////////////////////////

#include "XmlDocument.h"
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <vector>

#include "xbmc_vis.h"

#include "Saville.h"
#pragma comment (lib, "lib/xbox_dx8.lib" )

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);

typedef enum _WEIGHT {
	WEIGHT_NONE = 0,
	WEIGHT_A    = 1,
	WEIGHT_B    = 2,
	WEIGHT_C	= 3
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
#define MAX_PRESETS 256

	VIS_INFO vInfo;

	FREQDATA* m_fdCurrent;
	int m_iHistSize = 5;
	FREQDATA* m_fdFreqData;

	void ClearArrays();
	void CreateArrays();
	void LoadSettings();
	void LoadPreset(int);
  void SetDefaults();

	// Arrays to store frequency data
	float	m_pWeight[FREQ_DATA_SIZE/2+1];	// A/B/C weighted levels for speed
	//float	m_pFreq[MAX_BARS*2];			// Frequency data

	int		m_iSampleRate;

	// Member variables that users can play with
	int				m_iBars;				// number of bars to draw
	bool			m_bLogScale;			// true if our frequency is on a log scale
	float			m_fMinFreq;				// wanted frequency range
	float			m_fMaxFreq;
	float			m_fMinLevel;			// wanted level range
	float			m_fMaxLevel;
	WEIGHT			m_Weight;				// weighting type to be applied
	static			char m_szVisName[1024];

  LPDIRECT3DDEVICE8 m_pd3dDevice;

#define POLE1 20.598997*20.598997	// for A/B/C weighting
#define POLE2 12194.217*12194.217	// for A/B/C weighting
#define POLE3 107.65265*107.65265	// for A weighting
#define POLE4 737.86223*737.86223	// for A weighting
#define POLE5 158.5*158.5			// for B weighting

int m_iHeight;
int m_iWidth;
int m_iPosX;
int m_iPosY;
int m_iBorder = 50;
float m_fBarOffset = 0.3f;
bool m_bDrawBorders = true;
float m_fLineWeight = 1.0f;

float m_fRiseSpeed = 0.5f;
float m_fFallSpeed = 0.5f;

// presets stuff
char      m_szPresetsDir[256];  // as in the xml
char      m_szPresetsPath[512]; // fully qualified path
char*     m_szPresets[256];
int       m_nPresets;
int       m_nCurrentPreset;
// the settings vector
std::vector<VisSetting> m_vecSettings;

DWORD m_dwLastRender = 0;
int m_iDelay = 1000;

D3DXCOLOR m_cBackground = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);
D3DXCOLOR m_cFrontBorder =D3DXCOLOR(0.0f, 0.0f, 0.0f, 1.0f);
D3DXCOLOR m_cFrontFill = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);;
D3DXCOLOR m_cBackBorder = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);;
D3DXCOLOR m_cBackFill = D3DXCOLOR(1.0f, 1.0f, 1.0f, 1.0f);;

GetColor(D3DXCOLOR front, D3DXCOLOR back, int step);

inline float sqr(float f)
{
	return f*f;
}

FRECT::FRECT()
{
	left = top = right = bottom = 0;
}

FRECT::FRECT( float l, float t, float r, float b)
{
	left = l;
	top = t;
	right = r;
	bottom = b;
}

FPOINT::FPOINT()
{
	x = y = 0;
}

FPOINT::FPOINT( float xin, float yin )
{
	x = xin;
	y = yin;
}

int inline sign(float a) {return (a>0) ? 1 : -1;}

int htoi(char *str) /* Convert hex string to integer */
{
	unsigned int digit, number = 0;
	while (*str)
	{
		if (isdigit(*str))
			digit = *str - '0';
		else
			digit = tolower(*str)-'a'+10;
		number<<=4;
		number+=digit;
		str++;
	}
	return number;  
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iWidth, int iHeight, const char* szVisualisationName)
{
	m_iHeight	= iHeight;
	m_iWidth	= iWidth;
  m_iPosX = iPosX;
  m_iPosY = iPosY;
	
	strcpy(m_szVisName,szVisualisationName);
	m_pd3dDevice = pd3dDevice;
	vInfo.bWantsFreq = true;
	// Load the settings
  printf("load settings!");
	LoadSettings();
  printf("load preset %i!",m_nCurrentPreset);
  LoadPreset(m_nCurrentPreset);
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
	m_iSampleRate = iSamplesPerSec;

	// Initialize our arrays
	LoadSettings();
  LoadPreset(m_nCurrentPreset);
	ClearArrays();
	CreateArrays();
}

void ClearArrays()
{
}

void CreateArrays()
{
	// create fresh arrays again
	// and initialize them
	// and the weight array
	if (m_Weight == WEIGHT_NONE)
		return;
	// calculate the weights (squared)
	float f2;
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

	if (m_fdFreqData)
		free(m_fdFreqData);

	m_fdFreqData = ( FREQDATA* ) malloc( m_iHistSize * sizeof(FREQDATA) );

	for(int i = 0; i < m_iHistSize; i++)
	{
		ZeroMemory(m_fdFreqData[i].freq, 1024);
		ZeroMemory(m_fdFreqData[i].screen, 1024);
		m_fdFreqData[i].bDrawn = false;

		if (i > 0)
			m_fdFreqData[i].prev = &m_fdFreqData[i-1];
		if (i < (m_iHistSize - 1))
			m_fdFreqData[i].next = &m_fdFreqData[i+1];
	}

	if (m_iHistSize > 0)
	{
		m_fdFreqData[m_iHistSize - 1].next = &m_fdFreqData[0];
		m_fdFreqData[0].prev = &m_fdFreqData[m_iHistSize - 1];
	}
	else
	{
		m_fdFreqData[0].next = &m_fdFreqData[0];
		m_fdFreqData[0].prev = &m_fdFreqData[0];
	}

	m_fdCurrent = &m_fdFreqData[0];
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
	for (int i=0, iBin=0; i < m_iBars*2+2; i++, iBin+=2)
	{
		m_fdCurrent->freq[iBin]=0.000001f;	// almost zero to avoid taking log of zero later
		m_fdCurrent->freq[iBin+1]=0.000001f;
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
			m_fdCurrent->freq[iBin]+=pFreqData[j]+pFreqData[j+1];
		}
		m_fdCurrent->freq[iBin] /=(jmax-jmin);
		jmin = jmax;
	}
	// Transform data to dB scale, 0 (Quietest possible) to 96 (Loudest)
	for (int i=0; i < (m_iBars*4)+2; i++)
	{
		m_fdCurrent->freq[i] = 10*log10(m_fdCurrent->freq[i]);
		if (m_fdCurrent->freq[i] > MAX_LEVEL)
			m_fdCurrent->freq[i] = MAX_LEVEL;
		if (m_fdCurrent->freq[i] < MIN_LEVEL)
			m_fdCurrent->freq[i] = MIN_LEVEL;
	}
}

extern "C" void Render()
{
	if ((m_dwLastRender + m_iDelay) < timeGetTime())
	{	
		m_dwLastRender = timeGetTime();
		m_fdCurrent = m_fdCurrent->next;
	}

	for (int i=0; i < m_iBars*2; i++)
	{
		if (!m_fdCurrent->bDrawn)
		{
			m_fdCurrent->screen[i] = m_fdCurrent->freq[i];
			m_fdCurrent->bDrawn = true;
		}
		else
		{
		// truncate data to the users range
		if (m_fdCurrent->freq[i] > m_fMaxLevel)
			m_fdCurrent->freq[i] = m_fMaxLevel;
		m_fdCurrent->freq[i]-=m_fMinLevel;
		if (m_fdCurrent->freq[i] < 0)
			m_fdCurrent->freq[i] = 0;
		
		//smooth out the movements
		if (m_fdCurrent->freq[i] > m_fdCurrent->screen[i])
				m_fdCurrent->screen[i] += (m_fdCurrent->freq[i]-m_fdCurrent->screen[i])*m_fRiseSpeed;
			else
				m_fdCurrent->screen[i] -= (m_fdCurrent->screen[i]-m_fdCurrent->freq[i])*m_fFallSpeed;
		}
	}

	// Set state to render the image
	d3dSetRenderState( D3DRS_ZENABLE,      FALSE );
	d3dSetRenderState( D3DRS_FOGENABLE,    FALSE );
	d3dSetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
	d3dSetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
	d3dSetRenderState( D3DRS_CULLMODE,     D3DCULL_NONE );
	d3dSetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
	d3dSetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
	d3dSetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	m_pd3dDevice->SetVertexShader( FVF_VERTEX );

	m_pd3dDevice->SetTexture(0, NULL);	// reset texture

	//clear background
	FillRect(&FRECT(m_iPosX, m_iPosY, m_iPosX + m_iWidth, m_iPosY + m_iHeight), m_cBackground);

	float fScale = ((float)m_iHeight - (m_iBorder * 2))/(float)m_fMaxLevel;
	int iLeft = m_iPosX + m_iBorder;
	int iTop = m_iPosY + m_iBorder;
	int iRight = m_iPosX + m_iWidth - m_iBorder;
	int iBottom = m_iPosY + m_iHeight - m_iBorder;
	//float fBarWidth = ((float)iRight - iLeft)/(float)m_iBars;
	float fBarWidth = ((float)iRight - iLeft)/((float)m_iBars + (m_iHistSize * m_fBarOffset));

	FREQDATA* fd = m_fdCurrent->next;
	for(int i = m_iHistSize-1; i >=0; i--)
	{
		D3DXCOLOR cFill		= GetColor(m_cFrontFill, m_cBackFill, i);
		D3DXCOLOR cBorder	= GetColor(m_cFrontBorder, m_cBackBorder, i);

		for(int j =0; j < (m_iBars * 2); j+=2)
		{
			int left	= iLeft + ((j)/2*fBarWidth);
			int top		= iBottom - (fd->screen[j]*fScale);
			int right	= (((j)/2+1)*fBarWidth) + iLeft;
			int bottom	= iBottom;
			left	+= m_fBarOffset * fBarWidth * i;
			right	+= m_fBarOffset * fBarWidth * i;
			top		-= m_fBarOffset * fBarWidth * i;
			bottom	-= m_fBarOffset * fBarWidth * i;

			FillRect(&FRECT(left, top, right, bottom), cFill);
			if (m_bDrawBorders)
			{
				DrawRect(&FRECT(left, top, right, bottom), cBorder, m_fLineWeight);
			}
			else
			{
				//draw bottom
				DrawLine(&FPOINT(left, bottom), &FPOINT(right, bottom), cBorder, m_fLineWeight);
				
				//draw top
				DrawLine(&FPOINT(left, top), &FPOINT(right, top), cBorder, m_fLineWeight);
				
				//draw left side
				if (j == 0) //its the far left bar
				{
					DrawLine(&FPOINT(left, bottom), &FPOINT(left, top), cBorder, m_fLineWeight);
				}
				else
				{
					int prevTop = iBottom - (fd->screen[j-2]*fScale);
					prevTop -= m_fBarOffset * fBarWidth * i;
					DrawLine(&FPOINT(left, prevTop), &FPOINT(left, top), cBorder, m_fLineWeight);
				}

				//draw final right side
				if (j == (m_iBars * 2) - 2)
				{
					DrawLine(&FPOINT(right, bottom), &FPOINT(right, top), cBorder, m_fLineWeight);
				}
			}
		}
		fd = fd->next;
	}
}


extern "C" void Stop()
{
	ClearArrays();
	if (m_fdFreqData)
		free(m_fdFreqData);

}

// Load settings from the Spectrum.xml configuration file
/*void LoadSettings()
{
	XmlNode node, childNode, grandChild;
	CXmlDocument doc;
	// Set up the defaults
	SetDefaults();


	char szXMLFile[1024];
  strcpy(szXMLFile,"P:\\Visualisations\\");
  strcat(szXMLFile,"saville.xml");
  FILE *f = fopen(szXMLFile,"r");
  if (!f)
  {
    strcpy(szXMLFile,"T:\\Visualisations\\");
    strcat(szXMLFile,"saville.xml");
  }
  else
    fclose(f);

	// Load the config file
	if (doc.Load(szXMLFile)>=0)
	{
//		OutputDebugString("Done\n");
		node = doc.GetNextNode(XML_ROOT_NODE);
		while (node>0)
		{
			if (strcmpi(doc.GetNodeTag(node),"visualisation"))
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

			if (childNode = doc.GetChildNode(node,"DrawBarBorders"))
			{
				m_bDrawBorders = !strcmpi(doc.GetNodeText(childNode), "true");
			}

			if (childNode = doc.GetChildNode(node,"Border"))
			{
				m_iBorder = atoi(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node,"HistoryCount"))
			{
				m_iHistSize = atoi(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node,"HistoryDelay"))
			{
				m_iDelay = atoi(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node,"HistoryOffset"))
			{
				m_fBarOffset = atof(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node,"LineWeight"))
			{
				m_fLineWeight = atof(doc.GetNodeText(childNode));
			}

			if (childNode = doc.GetChildNode(node, "Background"))
			{
				if (grandChild = doc.GetChildNode(childNode, "Red"))
					m_cBackground.r = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Green"))
					m_cBackground.g = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Blue"))
					m_cBackground.b = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Alpha"))
					m_cBackground.a = atof(doc.GetNodeText(grandChild));
			}

			if (childNode = doc.GetChildNode(node, "FrontFill"))
			{
				if (grandChild = doc.GetChildNode(childNode, "Red"))
					m_cFrontFill.r = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Green"))
					m_cFrontFill.g = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Blue"))
					m_cFrontFill.b = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Alpha"))
					m_cFrontFill.a = atof(doc.GetNodeText(grandChild));
			}

			if (childNode = doc.GetChildNode(node, "FrontBorder"))
			{
				if (grandChild = doc.GetChildNode(childNode, "Red"))
					m_cFrontBorder.r = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Green"))
					m_cFrontBorder.g = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Blue"))
					m_cFrontBorder.b = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Alpha"))
					m_cFrontBorder.a = atof(doc.GetNodeText(grandChild));
			}

			if (childNode = doc.GetChildNode(node, "BackFill"))
			{
				if (grandChild = doc.GetChildNode(childNode, "Red"))
					m_cBackFill.r = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Green"))
					m_cBackFill.g = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Blue"))
					m_cBackFill.b = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Alpha"))
					m_cBackFill.a = atof(doc.GetNodeText(grandChild));
			}

			if (childNode = doc.GetChildNode(node, "BackBorder"))
			{
				if (grandChild = doc.GetChildNode(childNode, "Red"))
					m_cBackBorder.r = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Green"))
					m_cBackBorder.g = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Blue"))
					m_cBackBorder.b = atof(doc.GetNodeText(grandChild));
				if (grandChild = doc.GetChildNode(childNode, "Alpha"))
					m_cBackBorder.a = atof(doc.GetNodeText(grandChild));
			}


			node = doc.GetNextNode(node);

		}
		doc.Close();
	}
}*/

// Load settings from the saville.xml configuration file
void LoadSettings()
{
  char szXMLFile[1024];
  strcpy(szXMLFile,"P:\\Visualisations\\");
  strcat(szXMLFile,"saville.xml");
  FILE *f = fopen(szXMLFile,"r");
  SetDefaults();

  if (!f)
  {
    strcpy(szXMLFile,"T:\\Visualisations\\");
    strcat(szXMLFile,"saville.xml");
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
        if (strcmpi(doc.GetNodeTag(node),"visualisation"))
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

        if (childNode = doc.GetChildNode(node,"DrawBarBorders"))
        {
          m_bDrawBorders = !strcmpi(doc.GetNodeText(childNode), "true");
        }

        if (childNode = doc.GetChildNode(node,"Border"))
        {
          m_iBorder = atoi(doc.GetNodeText(childNode));
        }

        if (childNode = doc.GetChildNode(node,"HistoryCount"))
        {
          m_iHistSize = atoi(doc.GetNodeText(childNode));
        }

        if (childNode = doc.GetChildNode(node,"HistoryDelay"))
        {
          m_iDelay = atoi(doc.GetNodeText(childNode));
        }

        if (childNode = doc.GetChildNode(node,"HistoryOffset"))
        {
          m_fBarOffset = atof(doc.GetNodeText(childNode));
        }

        if (childNode = doc.GetChildNode(node,"LineWeight"))
        {
          m_fLineWeight = atof(doc.GetNodeText(childNode));
        }

        if (childNode = doc.GetChildNode(node, "Background"))
        {
          if (grandChild = doc.GetChildNode(childNode, "Red"))
            m_cBackground.r = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Green"))
            m_cBackground.g = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Blue"))
            m_cBackground.b = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Alpha"))
            m_cBackground.a = atof(doc.GetNodeText(grandChild));
        }

        if (childNode = doc.GetChildNode(node, "FrontFill"))
        {
          if (grandChild = doc.GetChildNode(childNode, "Red"))
            m_cFrontFill.r = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Green"))
            m_cFrontFill.g = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Blue"))
            m_cFrontFill.b = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Alpha"))
            m_cFrontFill.a = atof(doc.GetNodeText(grandChild));
        }

        if (childNode = doc.GetChildNode(node, "FrontBorder"))
        {
          if (grandChild = doc.GetChildNode(childNode, "Red"))
            m_cFrontBorder.r = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Green"))
            m_cFrontBorder.g = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Blue"))
            m_cFrontBorder.b = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Alpha"))
            m_cFrontBorder.a = atof(doc.GetNodeText(grandChild));
        }

        if (childNode = doc.GetChildNode(node, "BackFill"))
        {
          if (grandChild = doc.GetChildNode(childNode, "Red"))
            m_cBackFill.r = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Green"))
            m_cBackFill.g = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Blue"))
            m_cBackFill.b = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Alpha"))
            m_cBackFill.a = atof(doc.GetNodeText(grandChild));
        }

        if (childNode = doc.GetChildNode(node, "BackBorder"))
        {
          if (grandChild = doc.GetChildNode(childNode, "Red"))
            m_cBackBorder.r = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Green"))
            m_cBackBorder.g = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Blue"))
            m_cBackBorder.b = atof(doc.GetNodeText(grandChild));
          if (grandChild = doc.GetChildNode(childNode, "Alpha"))
            m_cBackBorder.a = atof(doc.GetNodeText(grandChild));
        }
        node = doc.GetNextNode(node);
      }
      doc.Close();
    }
    printf("loaded preset %s",szPresetFile);
    ClearArrays();
    CreateArrays();
  }
}

void SetDefaults()
{
	m_iBars = 128;
	m_bLogScale=true;
	vInfo.iSyncDelay=15;
	m_Weight = WEIGHT_NONE;
	m_fMinFreq = 80;
	m_fMaxFreq = 16000;
	m_fMinLevel = 0;
	m_fMaxLevel = 80;
}




extern "C" void GetInfo(VIS_INFO* pInfo)
{
  pInfo->bWantsFreq =vInfo.bWantsFreq;
	pInfo->iSyncDelay=vInfo.iSyncDelay;
}



void FillRect(const FRECT* rect, D3DXCOLOR color)
{
	VERTEX v3[4];
	v3[0].p = D3DXVECTOR4(rect->left, rect->bottom, 0, 0 );
	v3[0].col= color;	    
	v3[1].p = D3DXVECTOR4(rect->right, rect->bottom, 0, 0 );
	v3[1].col= color;	    
	v3[2].p = D3DXVECTOR4(rect->right, rect->top, 0, 0 );
	v3[2].col= color;	    
	v3[3].p = D3DXVECTOR4(rect->left, rect->top, 0, 0 );
	v3[3].col= color;	    

	m_pd3dDevice->DrawPrimitiveUP( D3DPT_QUADLIST, 1, v3, sizeof(VERTEX));}

void DrawRect(const FRECT* rect, D3DXCOLOR color, float width)
{
	DrawLine(&FPOINT(rect->left, rect->top),	&FPOINT(rect->right, rect->top),	color, width);
	DrawLine(&FPOINT(rect->right, rect->top),	&FPOINT(rect->right, rect->bottom),	color, width);
	DrawLine(&FPOINT(rect->right, rect->bottom),&FPOINT(rect->left, rect->bottom),	color, width);
	DrawLine(&FPOINT(rect->left, rect->bottom),	&FPOINT(rect->left, rect->top),		color, width);
}

void DrawLine(const FPOINT* p1, const FPOINT* p2, D3DXCOLOR color, float width)
{
	
	//FPOINT a = ConvertPoint(p1);
	//FPOINT b = ConvertPoint(p2);
	
	if (width == 1.0f)
	{
		VERTEX v[2];
		v[0].p = D3DXVECTOR4(p1->x, p1->y, 0.0f, 0.0f);
		v[0].col = color;
		v[1].p = D3DXVECTOR4(p2->x, p2->y, 0.0f, 0.0f);
		v[1].col = color;
		m_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, &v, sizeof(VERTEX));
	}
	else
	{
		//draw left line
		FRECT left = FRECT(		p1->x - (width/2), 
								p1->y + (width/2), 
								p1->x + (width/2), 
								p2->y - (width/2));
		FillRect(&left, color);

		//draw top line
		FRECT top = FRECT(		p1->x - (width/2),
								p1->y + (width/2), 
								p2->x + (width/2),
								p1->y - (width/2));
		FillRect(&top, color);

		////draw right line
		FRECT right = FRECT(	p2->x - (width/2), 
								p1->y + (width/2), 
								p2->x + (width/2), 
								p2->y - (width/2));
		FillRect(&right, color);

		////draw bottom line
		FRECT bottom = FRECT(	p1->x - (width/2),
								p2->y + (width/2), 
								p2->x + (width/2),
								p2->y - (width/2));
		FillRect(&bottom, color);	
	}
}


GetColor(D3DXCOLOR front, D3DXCOLOR back, int step)
{
	float r, g, b, a;
	r = front.r * (m_iHistSize - step);
	r += back.r * step;
	r = r/m_iHistSize;

	g = front.g * (m_iHistSize - step);
	g += back.g * step;
	g = g/m_iHistSize;

	b = front.b * (m_iHistSize - step);
	b += back.b * step;
	b = b/m_iHistSize;

	a = front.a * (m_iHistSize - step);
	a += back.a * step;
	a = a/m_iHistSize;
	return D3DXCOLOR(r, g, b, a); 
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
}

extern "C" void GetPresets(char ***pPresets, int *currentPreset, int *numPresets, bool *locked)
{
  if (!pPresets || !numPresets || !currentPreset || !locked) return;
  *pPresets = m_szPresets;
  *numPresets = m_nPresets;
  *currentPreset = m_nCurrentPreset;
  *locked = false;
}

extern "C" void __declspec(dllexport) get_module(struct Visualisation* pVisz)
{
  pVisz->Create = Create;
  pVisz->Start = Start;
  pVisz->AudioData = AudioData;
  pVisz->Render = Render;
  pVisz->Stop = Stop;
  pVisz->GetInfo = GetInfo;
  pVisz->OnAction = OnAction;
  pVisz->GetSettings = GetSettings;
  pVisz->UpdateSetting = UpdateSetting;
  pVisz->GetPresets = GetPresets;
};
