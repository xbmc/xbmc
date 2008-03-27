#include "Visualization.h"
#include "ParticleSystem.h"

LPDIRECT3DDEVICE8 m_pd3dDevice;

CParticleSystem *m_pParticleSystem;

float r, g, b;

//float  g_fElpasedTime;
//double g_dCurTime;
//double g_dLastTime;


void Create(LPDIRECT3DDEVICE8 pd3dDevice)
{
	m_pd3dDevice = pd3dDevice;
	
	D3DXMATRIX matView;
    D3DXMatrixLookAtLH( &matView, &D3DXVECTOR3(0.0f, 0.0f,-10.0f), 
		                          &D3DXVECTOR3(0.0f, 0.0f, 0.0f), 
		                          &D3DXVECTOR3(0.0f, 1.0f, 0.0f ) );
    m_pd3dDevice->SetTransform( D3DTS_VIEW, &matView );

    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH( &matProj, D3DX_PI/4, 1.0f, 1.0f, 200.0f );
    m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matProj );

	m_pd3dDevice->SetRenderState( D3DRS_LIGHTING,  FALSE );

	InitParticles();

}

void InitParticles()
{
	m_pParticleSystem = new CParticleSystem();

    m_pParticleSystem->SetTexture( szTexFile );
    //m_pParticleSystem->SetMaxParticles( 500 );
    //m_pParticleSystem->SetNumToRelease( 5 );
    //m_pParticleSystem->SetReleaseInterval( 0.05f );
    //m_pParticleSystem->SetLifeCycle( 4.0f );
    //m_pParticleSystem->SetSize( 0.3f );
    //m_pParticleSystem->SetColor( D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ));
    //m_pParticleSystem->SetPosition( D3DXVECTOR3( 0.0f, 0.0f, 0.0f ) );
    //m_pParticleSystem->SetVelocity( D3DXVECTOR3( 0.0f, 5.0f, 0.0f ) );
    //m_pParticleSystem->SetGravity( D3DXVECTOR3( 0.0f, 0.0f, 0.0f ) );
    //m_pParticleSystem->SetWind( D3DXVECTOR3( 2.0f, 0.0f, 0.0f ) );
    //m_pParticleSystem->SetVelocityVar( 1.5f );

    m_pParticleSystem->SetMaxParticles( 1000 );
    m_pParticleSystem->SetNumToRelease( 2 );
    m_pParticleSystem->SetReleaseInterval( 0.01f );
    m_pParticleSystem->SetLifeCycle( 3.0f );
    m_pParticleSystem->SetSize( 0.4f );
    m_pParticleSystem->SetColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ));
    m_pParticleSystem->SetPosition( D3DXVECTOR3( 0.0f, 0.0f, 0.0f ) );
    m_pParticleSystem->SetVelocity( D3DXVECTOR3( 0.0f, 5.0f, 0.0f ) );
    m_pParticleSystem->SetGravity( D3DXVECTOR3( 0.0f, 0.0f, 0.0f ) );
    m_pParticleSystem->SetWind( D3DXVECTOR3( 0.0f, 0.0f, -20.0f ) );
    m_pParticleSystem->SetVelocityVar( 2.5f );

    m_pParticleSystem->Init( m_pd3dDevice );
}


void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample)
{
}

void AudioData(short* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
}

void Render()
{
	//
	// The particle system will need to know how much time has passed since 
	// the last time it was updated, so we'll need to keep track of how much   
	// time has elapsed since the last frame update...
	//
	
	r+=0.001f;
	if (r >= 1.0f)
		r = 0.0f;

	g+=0.01f;
	if (g >= 1.0f)
		g = 0.0f;

	b+=0.0001f;
	if (b >= 1.0f)
		b= 0.0f;
	
	m_pParticleSystem->SetColor(D3DXCOLOR(r, g, b, 1.0f));
	m_pParticleSystem->Update( 0.005f );


	//
	// Render particle system
	//

    //m_pd3dDevice->SetTexture( 0, m_pParticleSystem->GetTextureObject() );
	m_pParticleSystem->Render( m_pd3dDevice );
}

void Stop()
{
}

void GetInfo(VIS_INFO* pInfo)
{
}
