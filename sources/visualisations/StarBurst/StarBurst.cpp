//
//
//	StarBurst.cpp   :::   This Is the Starburst XBMC Visualization
//	V0.75			Written by Dinomight
//					dylan@castlegate.net
//				
// 
//
//////////////////////////////////////////////////////////////////////
#include "StarBurst.h"
#include <stdio.h>
#include <math.h>
//#pragma comment (lib, "lib/xbox_dx8.lib" )

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);
extern "C" void d3dGetRenderState(DWORD dwY, DWORD* dwZ);
extern "C" 
{
   struct VIS_INFO {
       bool bWantsFreq;
       int iSyncDelay;
       //      int iAudioDataLength;
       //      int iFreqDataLength;
   };
};







typedef enum _WEIGHT {
	WEIGHT_NONE = 0,
	WEIGHT_A    = 1,
	WEIGHT_B    = 2,
	WEIGHT_C		= 3
} WEIGHT;

#define sqr(x) (x*x)


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

	float	m_pScreen[MAX_BARS*2];			// Current levels on the screen
	float	m_pPeak[MAX_BARS*2];			// Peak levels
	float	m_pWeight[FREQ_DATA_SIZE/2+1];		// A/B/C weighted levels for speed
	float	m_pFreq[MAX_BARS*2];			// Frequency data

	
	int		m_iSampleRate;
	int		m_width;
	int		m_height;
	float		m_centerx;
	float		m_centery;
	float		m_fRotation=0.0f;
	float		angle=0.0f;

	float		startradius;	//radius at which to start each bar
	float		minbar;			//minimum length of a bar
	float		spinrate;		// rate at witch to spin vis
	double		timepassed;
	double		lasttime = 0.0;
	double		currenttime = 0.0;

	float				r1; //floats used for bar colors;
	float				g1;
	float				b1;
	float				a1;
	float				r2;
	float				g2;
	float				b2;
	float				a2;

	static  char m_szVisName[1024];

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
	bool			m_bSeperateBars;



	#define POLE1 20.598997*20.598997	// for A/B/C weighting
	#define POLE2 12194.217*12194.217	// for A/B/C weighting
	#define POLE3 107.65265*107.65265	// for A weighting
	#define POLE4 737.86223*737.86223	// for A weighting
	#define POLE5 158.5*158.5			// for B weighting


VIS_INFO          vInfo;
static LPDIRECT3DDEVICE8 m_pd3dDevice;

	LPDIRECT3DVERTEXBUFFER8			m_pVB;
// A structure for our custom vertex type
struct CUSTOMVERTEX
{
    FLOAT x, y, z, rhw; // The transformed position for the vertex
    DWORD color;        // The vertex color
};



// Our custom FVF, which describes our custom vertex structure
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)



CUSTOMVERTEX g_Vertices[MAX_BARS*4];

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








void ClearArrays()
{
	/* Delete our current arrays, if they exist
	if (m_pScreen) delete[] m_pScreen;
	m_pScreen = NULL;
	if (m_pPeak) delete[] m_pPeak;
	m_pPeak = NULL;
	if (m_pFreq) delete[] m_pFreq;
	m_pFreq = NULL;
	if (m_pWeight) delete[] m_pWeight;
	m_pWeight = NULL;*/
	//g_Vertices = NULL;
}

void CreateArrays()
{
	// create fresh arrays again
//	m_pScreen = new float[m_iBars*2];
//	m_pPeak = new float[m_iBars*2];
//	m_pFreq = new float[m_iBars*2];
	// and initialize them
		CUSTOMVERTEX nullV = { 0.0f, 0.0f, 0.0f, 0.0f, 0x00000000, }; 

	for (int i=0; i<m_iBars*2; i++)
	{
		m_pScreen[i] = 0.0f;
		m_pPeak[i] = 0.0f;
		m_pFreq[i] = 0.0f;
		
		g_Vertices[i*2] = nullV;
		g_Vertices[i*2+1] = nullV;

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

void SetupCamera()
{
    //Here we will setup the camera.
    //The camera has three settings: "Camera Position", "Look at Position" and "Up Direction"
    //We have set the following:
    //Camera Position: (0, 0, -30)
    //Look at Position: (0, 0, 0)
    //Up direction: Y-Axis.
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH(&matView, &D3DXVECTOR3(0.0f, 0.0f,-50.0f), //Camera Position
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

extern "C" void SetDefaults()
{
	m_iBars = 50;
	m_bLogScale=false;
	vInfo.iSyncDelay=16;
	m_fPeakDecaySpeed=0.5f;
	m_fRiseSpeed=0.5f;
	m_fFallSpeed=0.5f;
	m_Weight = WEIGHT_NONE;
	m_bMixChannels = true;
	m_fMinFreq = 80;
	m_fMaxFreq = 16000;
	m_fMinLevel = 1;
	m_fMaxLevel = 80;
	m_bShowPeaks = true;
	m_bAverageLevels = false;
	spinrate = 1.0f/30.0f;
	startradius = 0.0f;
	minbar = 0.0f;
	//inital color
		r2 = 255;
		g2 = 200;
		b2 = 0;
		a2 = 255;
	//finalColor
		r1 = 163;
		g1 = 192;
		b1 = 255;
		a1 = 255;
	// color Diff
		r2 -= r1;
		g2 -= g1;
		b2 -= b1;
		a2 -= a1;

}


extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iScreenWidth, int iScreenHeight, const char* szVisName)
{
	m_width = iScreenWidth;
	m_height = iScreenHeight;
	m_centerx = m_width/2.0f + iPosX;
	m_centery = m_height/2.0f + iPosY;

	strcpy(m_szVisName,szVisName);
	//for (int i=0; i<MAX_CHANNELS; i++)
	//	m_pTexture[i]=NULL;
	m_pVB=NULL;
	m_pd3dDevice = pd3dDevice;
	vInfo.bWantsFreq = true;
	// Load the settings
	LoadSettings();
	ClearArrays();
	CreateArrays();
	
	    // Turn off culling, so we see the front and back of the triangle
   //m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
	//g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);


    // Turn off D3D lighting, since we are providing our own vertex colors
    //m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );
	//m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );


}



extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
	m_iSampleRate = iSamplesPerSec;

	LoadSettings();
	ClearArrays();
	CreateArrays();

	if (m_pVB)
	{
		m_pVB->Release();
		m_pVB = NULL;
	}
	InitGeometry();
	
    m_pd3dDevice->CreateVertexBuffer( sizeof(g_Vertices),
                                                  D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_MANAGED, &m_pVB );
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



void SetupMatrices()
{

    // For our world matrix, we will just rotate the object about the y-axis.
    D3DXMATRIX matWorld;
    //D3DXMatrixRotationY( &matWorld, timeGetTime()/150.0f );
    //m_pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Set up our view matrix. A view matrix can be defined given an eye point,
    // a point to lookat, and a direction for which way is up. Here, we set the
    // eye five units back along the z-axis and up three units, look at the
    // origin, and define "up" to be in the y-direction.
    const D3DXVECTOR3 vEyePos( 0.0f, -10.0f, -50.0f );
    const D3DXVECTOR3 vLookAt( 0.0f, 0.0f,  0.0f );
    const D3DXVECTOR3 vUp    ( 0.0f, 1.0f,  0.0f );
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH( &matView, &vEyePos, &vLookAt, &vUp );
    //m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    // For the projection matrix, we set up a perspective transform (which
    // transforms geometry from 3D view space to 2D viewport space, with
    // a perspective divide making objects smaller in the distance). To build
    // a perspective transform, we need the field of view (1/4 pi is common),
    // the aspect ratio, and the near and far clipping planes (which define at
    // what distances geometry should be no longer be rendered).
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0f, 1.0f, 200.0f );
    //m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );
}

HRESULT InitGeometry()
{
    // Initialize three vertices for rendering a triangle
    CUSTOMVERTEX g_Vertices[] =
    {
        { 200.0f, 200.0f, 0.5f, 1.0f, 0xff00ff00, }, // x, y, z, rhw, color
        { 300.0f, 200.0f, 0.5f, 1.0f, 0xff00ff00, },
		{ 300.0f, 300.0f, 0.5f, 1.0f, 0xff00ff00, },
		{ 200.0f, 300.0f, 0.5f, 1.0f, 0xff00ff00, },
		{ 200.0f, 300.0f, 0.5f, 1.0f, 0xff00ff00, },

		};

    // Create the vertex buffer.
    m_pd3dDevice->CreateVertexBuffer( sizeof(g_Vertices),
                                                  D3DUSAGE_WRITEONLY, D3DFVF_CUSTOMVERTEX,
                                                  D3DPOOL_MANAGED, &m_pVB );
    
    // Fill the vertex buffer.
    CUSTOMVERTEX* pVertices;
    m_pVB->Lock( 0, 0, (BYTE**)&pVertices, 0 );
    memcpy( pVertices, g_Vertices, sizeof(g_Vertices) );
    m_pVB->Unlock();

    return S_OK;
}

extern "C" void Render()
{
	if (!m_pVB)
		return;

	if (lasttime = 0.0)
		lasttime = timeGetTime();
	
	currenttime = timeGetTime();

	timepassed = (currenttime - lasttime);

	float PI = 3.141592653589793f;
	float devisions = (2.0f*PI)/(m_iBars);
	float dwidth = devisions/2.3f;


	angle += (2.0f*PI)/(spinrate*1000)*(timepassed/250000.0);

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
	

	
	
	if (angle >2.0f*PI)
		angle -= 2.0f*PI;
	float x1 = 0;
	float y1 = 0;
	float x2 = 0;
	float y2 = 0;
	float radius=0;
	int iChannels = m_bMixChannels ? 1 : 2;
	 

//	for (int j=0; j<iChannels; j++){
	int j = 0;

	int points = 4;
	float scaler = (m_height/2 - minbar - startradius)/(m_fMaxLevel - m_fMinLevel);
	D3DCOLOR color1 = D3DCOLOR_RGBA((int)r1,(int)g1,(int)b1,(int)a1); 
	for (int i=0; i < m_iBars*2; i+=2)
	{
		radius =  m_pScreen[i+j] * scaler + minbar + startradius;

		x1 = sin(angle - dwidth) * radius;		
		y1 = cos(angle - dwidth) * radius;
		x2 = sin(angle + dwidth) * radius;		
		y2 = cos(angle + dwidth) * radius;		
		float x3 = sin(angle) * startradius;
		float y3 = cos(angle) * startradius;


		float colorscaler = ((m_pScreen[i+j])/(m_fMaxLevel - m_fMinLevel));
		

		
		D3DCOLOR color2 = D3DCOLOR_RGBA((int)((colorscaler*r2)+r1),(int)((colorscaler*g2)+g1),(int)((colorscaler*b2)+b1),(int) ((colorscaler*a2)+a1));
		//color1 = color2;
		CUSTOMVERTEX b = { m_centerx + x3, m_centery + y3, 0.5f, 1.0f, color2, };
		CUSTOMVERTEX a1 = { m_centerx + x1, m_centery + y1, 0.5f, 1.0f, color2, }; 
		CUSTOMVERTEX a2 = { m_centerx + x2, m_centery + y2, 0.5f, 1.0f, color2, }; 
		g_Vertices[(((i+2)/2 -1)*points)] = b;
		g_Vertices[(((i+2)/2 -1)*points)+1] = a1;
		g_Vertices[(((i+2)/2 -1)*points)+2] = a2;
		g_Vertices[(((i+2)/2 -1)*points)+3] = b;


		angle += devisions;




	}
//	}


    
    // Fill the vertex buffer.

	CUSTOMVERTEX* pVertices;
	m_pVB->Lock( 0, 0, (BYTE**)&pVertices, 0 );
	memcpy( pVertices, g_Vertices, sizeof(g_Vertices) );
	m_pVB->Unlock();

	//SetupCamera();
	//SetupRotation(m_fRotation+=5.0f, m_fRotation+=5.0f, m_fRotation+=5.0f);
	//SetupPerspective();

	m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(CUSTOMVERTEX) );
	m_pd3dDevice->SetVertexShader( D3DFVF_CUSTOMVERTEX );
	//for (int j=0; j<iChannels; j++){
	m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, m_iBars*4-2);





    // Create the vertex buffer.
	//}
	return;
}

extern "C" void Stop()
{
	ClearArrays();
	if (m_pVB)
		m_pVB->Release();
	m_pVB = NULL;
	//if(m_pd3dDevice)
	//	m_pd3dDevice->Release();
	m_pd3dDevice = NULL;
}


void LoadSettings()
{
	XmlNode node, childNode, grandChild;
	CXmlDocument doc;
	bool bReverseFreq[2] = {false, false};

	// Set up the defaults
	SetDefaults();


  char szXMLFile[1024];
  strcpy(szXMLFile,"P:\\Visualisations\\");
  strcat(szXMLFile,"starburst.xml");
  FILE *f = fopen(szXMLFile,"r");
  if (!f)
  {
    strcpy(szXMLFile,"T:\\Visualisations\\");
    strcat(szXMLFile,"starburst.xml");
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
      
			if (childNode = doc.GetChildNode(node,"initialcolorR"))
			{
				r1 = (float)atof(doc.GetNodeText(childNode));	
			}
			if (childNode = doc.GetChildNode(node,"initialcolorG"))
			{
				g1 = (float)atof(doc.GetNodeText(childNode));	
			}

			if (childNode = doc.GetChildNode(node,"initialcolorB"))
			{
				b1 = (float)atof(doc.GetNodeText(childNode));	
			}

			if (childNode = doc.GetChildNode(node,"initialcolorA"))
			{
				a1 = (float)atof(doc.GetNodeText(childNode));	
			}

			if (childNode = doc.GetChildNode(node,"finalcolorR"))
			{
				r2 = (float)atof(doc.GetNodeText(childNode));
				r2 -= r1;
			}
			if (childNode = doc.GetChildNode(node,"finalcolorG"))
			{
				g2 = (float)atof(doc.GetNodeText(childNode));
				g2 -=g1;
			}

			if (childNode = doc.GetChildNode(node,"finalcolorB"))
			{
				b2 = (float)atof(doc.GetNodeText(childNode));
				b2 -= b1;
			}

			if (childNode = doc.GetChildNode(node,"finalcolorA"))
			{
				a2 = (float)atof(doc.GetNodeText(childNode));
				a2 -= a1;
			}
			if (childNode = doc.GetChildNode(node,"spinrate"))
			{
				spinrate = (float)atof(doc.GetNodeText(childNode));
			}
			if (childNode = doc.GetChildNode(node,"startradius"))
			{
				startradius = (float)atof(doc.GetNodeText(childNode));
			}
			if (childNode = doc.GetChildNode(node,"minbar"))
			{
				minbar = (float)atof(doc.GetNodeText(childNode));
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
			node = doc.GetNextNode(node);
		}
		doc.Close();
	}
}

extern "C" void GetInfo(VIS_INFO* pInfo)
{
	pInfo->bWantsFreq =vInfo.bWantsFreq;
	pInfo->iSyncDelay=vInfo.iSyncDelay;
}





extern "C" 
{
   struct Visualisation
   {
   public:
       // Gets called once during initialisation. Allows vis. plugin to initialize itself
       // pd3dDevice is a pointer to the DX8 device which you can use for DX8 
       // iScreenWidth/iScreenheight are the GUI's screenwidth/screenheight in pixels
       // szVisualisationName contains the name of the visualisation like goom or spectrum,...
       void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iPosX, int iPosY, int iScreenWidth, int iScreenHeight, const char* szVisualisationName);
       
       // gets called at the start of a new song
       // iChannels = number of audio channels
       // iBitsPerSample = bits of each sample ( 8 or 16)
       // iSamplesPerSec = number of samples / sec.
       // szSongName = name of the current song
       void (__cdecl* Start)(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName);
       
       // gets called if xbmc has new audio samples for the vis. plugin
       // pAudioData is a short [channels][iAudioDataLength] array of raw audio samples
       // pFreqData is a array of float[channels][iFreqDataLength] of fft-ed audio samples
       void (__cdecl* AudioData)(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength);

       // gets called if vis. plugin should render itself
       void (__cdecl* Render) ();
       
       // gets called if vis. should stop & cleanup
       void (__cdecl* Stop)();
       
       // gets called if xbmc wants to know the visz. parameters specified by the vis_info struct
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
