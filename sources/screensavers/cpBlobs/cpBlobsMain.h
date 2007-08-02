// include file for screensaver template
#include "xbsBase.h"

//variables
LPDIRECT3DDEVICE8 m_pd3dDevice;
char m_szScrName[1024];
int	m_iWidth;
int m_iHeight;
SCR_INFO vInfo;

//XML Settings function prototypes
void LoadSettings();
void SetDefaults();

#ifdef _TEST
inline void d3dSetRenderState(DWORD dwY, DWORD dwZ)
	{ m_pd3dDevice->SetRenderState((D3DRENDERSTATETYPE)dwY,dwZ); }
inline void d3dGetRenderState(DWORD dwY, DWORD* dwZ)
	{ m_pd3dDevice->GetRenderState((D3DRENDERSTATETYPE)dwY,dwZ); }
inline void d3dSetTextureStageState(int x, DWORD dwY, DWORD dwZ)
	{ m_pd3dDevice->SetTextureStageState(x, (D3DTEXTURESTAGESTATETYPE)dwY,dwZ); }
inline void d3dSetTransform( DWORD dwY, D3DMATRIX *dwZ )
	{ m_pd3dDevice->SetTransform( (D3DTRANSFORMSTATETYPE)dwY, dwZ ); }

#endif