// cpBlobs
// XBMC screensaver displaying metaballs moving around in an environment
// Simon Windmill (siw@coolpowers.com)

#include "cpBlobsMain.h"

#include "Blobby.h"

Blobby *m_pBlobby;

////////////////////////////////////////////////////////////////////////////////

static float g_fTicks = 0.0f;

////////////////////////////////////////////////////////////////////////////////

// these global parameters can all be user-controlled via the XML file

float g_fTickSpeed = 0.01f;

D3DXVECTOR3 g_WorldRotSpeeds;
char g_strCubemap[1024];
char g_strDiffuseCubemap[1024];
char g_strSpecularCubemap[1024];

bool g_bShowCube = true;
	
DWORD g_BlendStyle;

DWORD g_BGTopColor, g_BGBottomColor;

float g_fFOV, g_fAspectRatio;

////////////////////////////////////////////////////////////////////////////////

// stuff for the environment cube
struct CubeVertex
{
	D3DXVECTOR3 position;
	D3DXVECTOR3 normal;
};

#define FVF_CUBEVERTEX D3DFVF_XYZ | D3DFVF_NORMAL

// man, how many times have you typed (or pasted) this data for a cube's
// vertices and normals, eh?
CubeVertex g_cubeVertices[] =
{
    {D3DXVECTOR3(-1.0f, 1.0f,-1.0f), D3DXVECTOR3(0.0f, 0.0f,1.0f), },
    {D3DXVECTOR3( 1.0f, 1.0f,-1.0f), D3DXVECTOR3(0.0f, 0.0f,1.0f), },
    {D3DXVECTOR3(-1.0f,-1.0f,-1.0f), D3DXVECTOR3(0.0f, 0.0f,1.0f), },
    {D3DXVECTOR3( 1.0f,-1.0f,-1.0f), D3DXVECTOR3(0.0f, 0.0f,1.0f), },

    {D3DXVECTOR3(-1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), },
    {D3DXVECTOR3(-1.0f,-1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), },
    {D3DXVECTOR3( 1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), },
    {D3DXVECTOR3( 1.0f,-1.0f, 1.0f), D3DXVECTOR3(0.0f, 0.0f, -1.0f), },

    {D3DXVECTOR3(-1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, -1.0f, 0.0f), },
    {D3DXVECTOR3( 1.0f, 1.0f, 1.0f), D3DXVECTOR3(0.0f, -1.0f, 0.0f), },
    {D3DXVECTOR3(-1.0f, 1.0f,-1.0f), D3DXVECTOR3(0.0f, -1.0f, 0.0f), },
    {D3DXVECTOR3( 1.0f, 1.0f,-1.0f), D3DXVECTOR3(0.0f, -1.0f, 0.0f), },

    {D3DXVECTOR3(-1.0f,-1.0f, 1.0f), D3DXVECTOR3(0.0f,1.0f, 0.0f), },
    {D3DXVECTOR3(-1.0f,-1.0f,-1.0f), D3DXVECTOR3(0.0f,1.0f, 0.0f), },
    {D3DXVECTOR3( 1.0f,-1.0f, 1.0f), D3DXVECTOR3(0.0f,1.0f, 0.0f), },
    {D3DXVECTOR3( 1.0f,-1.0f,-1.0f), D3DXVECTOR3(0.0f,1.0f, 0.0f), },

    {D3DXVECTOR3( 1.0f, 1.0f,-1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f), },
    {D3DXVECTOR3( 1.0f, 1.0f, 1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f), },
    {D3DXVECTOR3( 1.0f,-1.0f,-1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f), },
    {D3DXVECTOR3( 1.0f,-1.0f, 1.0f), D3DXVECTOR3(-1.0f, 0.0f, 0.0f), },

    {D3DXVECTOR3(-1.0f, 1.0f,-1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f), },
    {D3DXVECTOR3(-1.0f,-1.0f,-1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f), },
    {D3DXVECTOR3(-1.0f, 1.0f, 1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f), },
    {D3DXVECTOR3(-1.0f,-1.0f, 1.0f), D3DXVECTOR3(1.0f, 0.0f, 0.0f), }
};

LPDIRECT3DVERTEXBUFFER8 g_pCubeVertexBuffer = NULL;

////////////////////////////////////////////////////////////////////////////////

// stuff for the background plane

struct BG_VERTEX 
{
    D3DXVECTOR4 position;
    DWORD       color;
};

BG_VERTEX g_BGVertices[4];

////////////////////////////////////////////////////////////////////////////////

// fill in background vertex array with values that will
// completely cover screen
void SetupGradientBackground( DWORD dwTopColor, DWORD dwBottomColor )
{
	float x1 = -0.5f;
	float y1 = -0.5f;
	float x2 = (float)m_iWidth - 0.5f;
    float y2 = (float)m_iHeight - 0.5f;
	
	g_BGVertices[0].position = D3DXVECTOR4( x2, y1, 0.0f, 1.0f );
    g_BGVertices[0].color = dwTopColor;

    g_BGVertices[1].position = D3DXVECTOR4( x2, y2, 0.0f, 1.0f );
    g_BGVertices[1].color = dwBottomColor;

    g_BGVertices[2].position = D3DXVECTOR4( x1, y1, 0.0f, 1.0f );
    g_BGVertices[2].color = dwTopColor;

    g_BGVertices[3].position = D3DXVECTOR4( x1, y2, 0.0f, 1.0f );
    g_BGVertices[3].color = dwBottomColor;
	
	return;
}

///////////////////////////////////////////////////////////////////////////////


void RenderGradientBackground()
{
    // clear textures
    m_pd3dDevice->SetTexture( 0, NULL );
	m_pd3dDevice->SetTexture( 1, NULL );
    d3dSetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_DISABLE );
	d3dSetTextureStageState( 1, D3DTSS_COLOROP, D3DTOP_DISABLE );

	// don't write to z-buffer
	d3dSetRenderState( D3DRS_ZENABLE, FALSE ); 
    
	m_pd3dDevice->SetVertexShader( D3DFVF_XYZRHW | D3DFVF_DIFFUSE );
	m_pd3dDevice->DrawPrimitiveUP( D3DPT_TRIANGLESTRIP, 2, g_BGVertices, sizeof(BG_VERTEX) );

	// restore state
	d3dSetRenderState( D3DRS_ZENABLE, TRUE ); 

	return;
}

////////////////////////////////////////////////////////////////////////////////

LPDIRECT3DCUBETEXTURE8	g_pCubeTexture	= NULL;
LPDIRECT3DCUBETEXTURE8	g_pDiffuseCubeTexture	= NULL;
LPDIRECT3DCUBETEXTURE8	g_pSpecularCubeTexture	= NULL;

// XBMC has loaded us into memory,
// we should set our core values
// here and load any settings we
// may have from our config file
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreenSaverName)
{
#ifdef _TEST
	strcpy( m_szScrName, "cpBlobs" );
#else
	strcpy(m_szScrName,szScreenSaverName);
#endif
	m_pd3dDevice = pd3dDevice;
	m_iWidth = iWidth;
	m_iHeight = iHeight;

	m_pBlobby = new Blobby();
	m_pBlobby->m_iNumPoints = 5;
	
	// Load the settings
	LoadSettings();
}

// XBMC tells us we should get ready
// to start rendering. This function
// is called once when the screensaver
// is activated by XBMC.
extern "C" void Start()
{	
	d3dSetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	d3dSetRenderState( D3DRS_LIGHTING, FALSE );
	d3dSetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
	d3dSetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	d3dSetRenderState( D3DRS_NORMALIZENORMALS, FALSE );
	
	
	m_pBlobby->Init( m_pd3dDevice );
	
	d3dSetTextureStageState( 0, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
	d3dSetTextureStageState( 0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );
	d3dSetTextureStageState( 1, D3DTSS_MINFILTER, D3DTEXF_LINEAR );
	d3dSetTextureStageState( 1, D3DTSS_MAGFILTER, D3DTEXF_LINEAR );

	D3DXCreateCubeTextureFromFile( m_pd3dDevice, g_strCubemap, &g_pCubeTexture );
	D3DXCreateCubeTextureFromFile( m_pd3dDevice, g_strDiffuseCubemap, &g_pDiffuseCubeTexture );
	D3DXCreateCubeTextureFromFile( m_pd3dDevice, g_strSpecularCubemap, &g_pSpecularCubeTexture );

	m_pd3dDevice->CreateVertexBuffer( 24*sizeof(CubeVertex), 0, 
		                              FVF_CUBEVERTEX, D3DPOOL_DEFAULT, 
                                      &g_pCubeVertexBuffer );
 
    void *pVertices = NULL;

    g_pCubeVertexBuffer->Lock( 0, sizeof(g_cubeVertices), (BYTE**)&pVertices, 0 );
    memcpy( pVertices, g_cubeVertices, sizeof(g_cubeVertices) );
    g_pCubeVertexBuffer->Unlock();

	SetupGradientBackground( g_BGTopColor, g_BGBottomColor );

	return;
}

// XBMC tells us to render a frame of
// our screensaver. This is called on
// each frame render in XBMC, you should
// render a single frame only - the DX
// device will already have been cleared.
extern "C" void Render()
{
	// I know I'm not scaling by time here to get a constant framerate,
	// but I believe this to be acceptable for this application
	m_pBlobby->AnimatePoints( g_fTicks );	
	m_pBlobby->March();

	// setup rotation
	D3DXMATRIX matWorld;
   	D3DXMatrixIdentity( &matWorld );
	D3DXMatrixRotationYawPitchRoll( &matWorld, g_WorldRotSpeeds.x * g_fTicks, g_WorldRotSpeeds.y * g_fTicks, g_WorldRotSpeeds.z * g_fTicks );
   	d3dSetTransform( D3DTS_WORLD, &matWorld );

	// setup viewpoint
	D3DXMATRIX matView;
	D3DXVECTOR3 campos( 0.0f, 0.0f, -0.8f );
	D3DXVECTOR3 camto( 0.0f, 0.0f, 0.0f );
	D3DXVECTOR3 upvec( 0.0f, 1.0f, 0.0f );
	D3DXMatrixLookAtLH( &matView, &campos, &camto, &upvec );
	d3dSetTransform( D3DTS_VIEW, &matView );

  // setup projection
 	D3DXMATRIX matProj;
	D3DXMatrixPerspectiveFovLH( &matProj, D3DXToRadian( g_fFOV ), g_fAspectRatio, 0.05f, 100.0f );
	d3dSetTransform( D3DTS_PROJECTION, &matProj );

	d3dSetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );

	// setup cubemap
	m_pd3dDevice->SetTexture( 0, g_pCubeTexture );
	m_pd3dDevice->SetTexture( 1, NULL );
	d3dSetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
	// rotate the cubemap to match the world
	d3dSetTransform( D3DTS_TEXTURE0, &matWorld );
	d3dSetTransform( D3DTS_TEXTURE1, &matWorld );
    
	// draw the box (inside-out)
	if ( g_bShowCube )
	{
		d3dSetRenderState( D3DRS_CULLMODE, D3DCULL_CW );
		d3dSetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP );
		d3dSetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP );
		d3dSetTextureStageState( 0, D3DTSS_COLOROP, D3DTOP_MODULATE );
		d3dSetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3 );
		d3dSetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION );
		m_pd3dDevice->SetVertexShader( FVF_CUBEVERTEX );
		m_pd3dDevice->SetStreamSource( 0, g_pCubeVertexBuffer, sizeof(CubeVertex) );
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  0, 2 );
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  4, 2 );
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP,  8, 2 );
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 12, 2 );
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 16, 2 );
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 20, 2 );
	}
	else
		RenderGradientBackground();

	m_pd3dDevice->SetTexture( 0, g_pDiffuseCubeTexture );
	m_pd3dDevice->SetTexture( 1, g_pSpecularCubeTexture );
	d3dSetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	d3dSetTextureStageState( 0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3 );
    d3dSetTextureStageState( 0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR );
	d3dSetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
	d3dSetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	d3dSetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_CURRENT );
	d3dSetTextureStageState( 0, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
	d3dSetTextureStageState( 0, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );

	d3dSetTextureStageState( 1, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT3 );
    d3dSetTextureStageState( 1, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR );	
	d3dSetTextureStageState( 1, D3DTSS_COLOROP,   g_BlendStyle );
	d3dSetTextureStageState( 1, D3DTSS_COLORARG1, D3DTA_TEXTURE );
	d3dSetTextureStageState( 1, D3DTSS_COLORARG2, D3DTA_CURRENT );
	d3dSetTextureStageState( 1, D3DTSS_ADDRESSU, D3DTADDRESS_WRAP );
	d3dSetTextureStageState( 1, D3DTSS_ADDRESSV, D3DTADDRESS_WRAP );
	
	m_pBlobby->Render( m_pd3dDevice );

	// increase tick count
	g_fTicks += g_fTickSpeed;

	return;
}

// XBMC tells us to stop the screensaver
// we should free any memory and release
// any resources we have created.
extern "C" void Stop()
{
	if( g_pCubeTexture != NULL ) 
        g_pCubeTexture->Release();

	if( g_pDiffuseCubeTexture != NULL ) 
        g_pDiffuseCubeTexture->Release();

	if( g_pSpecularCubeTexture != NULL ) 
        g_pSpecularCubeTexture->Release();
	
	delete m_pBlobby;

	if ( g_pCubeVertexBuffer != NULL )
    {
        g_pCubeVertexBuffer->Release();
        g_pCubeVertexBuffer = NULL;
    }

	return;
}

///////////////////////////////////////////////////////////////////////////////

extern "C" void GetInfo(SCR_INFO* pInfo)
{
	// not used, but can be used to pass info
	// back to XBMC if required in the future
	return;
}

extern "C" void __declspec(dllexport) get_module(struct ScreenSaver* pScr)
{
	pScr->Create = Create;
	pScr->Start = Start;
	pScr->Render = Render;
	pScr->Stop = Stop;
	pScr->GetInfo = GetInfo;
}