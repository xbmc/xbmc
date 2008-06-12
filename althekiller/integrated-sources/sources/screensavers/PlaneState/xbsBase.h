#ifndef _XBS_BASE_H_
#define _XBS_BASE_H_

// modify the following structure to include any information
// you may want to pass back to xbmc
extern "C" 
{
	struct SCR_INFO 
	{
		int	dummy;
	};
};

// Size of the textures we are using
#define TEXTURESIZE		256

//Include correct headers/includes for XBOX or PC test mode
#ifdef _TEST //PC TEST MODE
	// DX8 PC libraries
	#pragma comment(lib, "D3d8.lib")
	#pragma comment(lib, "D3dx8.lib")
	// DX8 PC Include
	#include <d3dx8.h>
	// Put additional PC-only testing includes here
	#define TEXTUREFORMAT	D3DFMT_A8R8G8B8

	#define TEXTUREUV		1.0f

	#define d3dSetRenderState			d3dDevice->SetRenderState
	#define d3dGetRenderState			d3dDevice->GetRenderState
	#define d3dSetTextureStageState		d3dDevice->SetTextureStageState
#else // XBOX mode
	// use the 'dummy' dx8 lib - this allow you to make
	// DX8 calls which XBMC will emulate for you.
	#pragma comment (lib, "lib/xbox_dx8.lib" )
	#include <xtl.h>
	//Put additional xbox-only includes here
	#define TEXTUREFORMAT	D3DFMT_LIN_A8R8G8B8 		// The D3DFMT_A8R8G8B8 format is swizzeld on xbox

	// Also in the linear formats the UV's dont go from 0.0 to 1.0. They go from 0.0 to <TextureSize>
	#define TEXTUREUV		TEXTURESIZE

	extern "C" void d3dGetRenderState(DWORD dwY, DWORD* dwZ);
	extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ);
	extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
#endif // _TEST


//screensaver definition for export
extern "C" 
{
	struct ScreenSaver
	{
	public:
		void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver);
		void (__cdecl* Start) ();
		void (__cdecl* Render) ();
		void (__cdecl* Stop) ();
		void (__cdecl* GetInfo)(SCR_INFO *info);
	} ;
};


#endif
