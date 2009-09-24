// include file for screensaver template

#include <xtl.h>

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);
extern "C" void d3dGetRenderState(DWORD dwY, DWORD* dwZ);
extern "C" 
{
	struct SCR_INFO 
	{
		int	dummy;
	};
};

void LoadSettings();
void SetDefaults();
void CreateArrays();

SCR_INFO vInfo;
LPDIRECT3DDEVICE8 m_pd3dDevice;

#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)


LPDIRECT3DVERTEXBUFFER8 g_pVertexBuffer = NULL; // Vertices Buffer

static  char m_szScrName[1024];
int	m_iWidth;
int m_iHeight;

void hsv_to_rgb(double hue, double saturation, double value, double *red, double *green, double *blue);
void DrawRectangle(int x, int y, int w, int h, D3DCOLOR dwColour);

struct MYCUSTOMVERTEX
{
	FLOAT x, y, z, rhw; // The transformed position for the vertex.
    DWORD colour; // The vertex colour.
};