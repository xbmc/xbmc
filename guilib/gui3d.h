/*!
	\file gui3d.h
	\brief 
	*/

#ifndef GUILIB_GUI3D_H
#define GUILIB_GUI3D_H
#pragma once

//#define ALLOW_TEXTURE_COMPRESSION 

#ifdef _XBOX
  #include <xtl.h>  
  #define GUI_D3D_FMT D3DFMT_LIN_A8R8G8B8
#else
  #include <windows.h>
  #include <d3dx8.h>
  #include <d3dx8tex.h>
  #define GUI_D3D_FMT D3DFMT_X8R8G8B8

#endif

#endif
