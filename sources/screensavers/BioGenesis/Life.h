// include file for screensaver template

#include <xtl.h>

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);
extern "C" void d3dGetRenderState(DWORD dwY, DWORD* dwZ);
extern "C" void d3dSetTransform(DWORD dwY, D3DMATRIX* dwZ);
extern "C" 
{
	struct SCR_INFO 
	{
		int	dummy;
	};
};

void LoadSettings();
SCR_INFO vInfo;
LPDIRECT3DDEVICE8 g_pd3dDevice;

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)


static  char g_szScrName[1024];
int	g_iWidth;
int g_iHeight;
float g_fRatio;

#define LerpColor(s,e,r) (0xFF000000+((int)(((((e)&0xFF0000)*r)+((s)&0xFF0000)*(1.0f-r)))&0xFF0000)+((int)(((((e)&0xFF00)*r)+((s)&0xFF00)*(1.0f-r)))&0xFF00)+((int)(((((e)&0xFF)*r)+((s)&0xFF)*(1.0f-r)))&0xFF))
D3DCOLOR HSVtoRGB( float h, float s, float v );
void DrawRectangle(int x, int y, int w, int h, D3DCOLOR dwColour);
void CreateGrid();
void reducePalette();

struct CUSTOMVERTEX
{
	FLOAT x, y, z, rhw; // The transformed position for the vertex.
    DWORD color; // The vertex colour.
};