// include file for screensaver template
#ifndef __WATER_H_
#define __WATER_H_

#include <xtl.h>
#include "waterfield.h" 
//#include "sphere.h"

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
void SetAnimation();

struct WaterSettings
{
  //C_Sphere * sphere;
  WaterField * waterField;
  int effectType;
  int frame;
  int nextEffectTime;
  int nextTextureTime;
  int effectCount;
  float scaleX;
  bool isWireframe;
  bool isTextureMode;
  char szTextureSearchPath[1024];
};

#endif 