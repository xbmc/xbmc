#pragma once
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

/*
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZRHW|D3DFVF_DIFFUSE)
struct CUSTOMVERTEX
{
	FLOAT x, y, z, rhw; // The transformed position for the vertex.
    DWORD color; // The vertex colour.
};*/
struct Fractal;
struct FractalController;

struct FractalSettings
{
  int frame;
  int nextTransition;
  int animationCountdown;
  
  int transitionTime;
  int animationTime;
  float presetChance;

  int iMaxObjects;
  int iMaxDepth;
  float morphSpeed;
  FractalController * fractalcontroller;
  Fractal * fractal;
};