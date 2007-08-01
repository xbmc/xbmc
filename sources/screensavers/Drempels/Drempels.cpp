
#include <xtl.h>

#pragma comment (lib, "lib/xbox_dx8.lib")


struct SCR_INFO 
{
	int	dummy;
};

extern long FXW;
extern long FXH;
int g_width;
int g_height;
LPDIRECT3DDEVICE8 g_pd3dDevice;

void DrempelsInit();
void DrempelsRender();
void DrempelsExit();

//-- Create -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreenSaverName)
{
  FXW = 512;
  FXH = 512;
  g_width = iWidth;
  g_height = iHeight;
  g_pd3dDevice = pd3dDevice;

} // Create

//-- Start --------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Start()
{
	srand(::GetTickCount());
  DrempelsInit();

} // Start

//-- Render -------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  DrempelsRender();

} // Render

//-- Stop ---------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void Stop()
{
  DrempelsExit();

} // Stop

//-- GetInfo ------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
extern "C" void GetInfo(SCR_INFO* pInfo)
{
	// not used, but can be used to pass info
	// back to XBMC if required in the future
	return;
}

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


	void __declspec(dllexport) get_module(struct ScreenSaver* pScr)
	{
		pScr->Create = Create;
		pScr->Start = Start;
		pScr->Render = Render;
		pScr->Stop = Stop;
		pScr->GetInfo = GetInfo;
	}
};