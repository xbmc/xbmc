// include file for screensaver template

#include <xtl.h>

extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);

#define D3DFVF_TLVERTEX D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1

extern "C" 
{
	struct SCR_INFO 
	{
		int	dummy;
	};
};

SCR_INFO vInfo;

void LoadSettings();



