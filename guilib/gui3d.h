/*!
	\file gui3d.h
	\brief 
	*/

#ifndef GUILIB_GUI3D_H
#define GUILIB_GUI3D_H
#pragma once

#define ALLOW_TEXTURE_COMPRESSION 

#ifdef _XBOX
#ifdef ALLOW_TEXTURE_COMPRESSION
	#define GUI_D3D_FMT D3DFMT_A8R8G8B8
#else
  #define GUI_D3D_FMT D3DFMT_LIN_A8R8G8B8
#endif
#else
  #include <windows.h>
  #include <d3dx8.h>
  #include <d3dx8tex.h>
  #define GUI_D3D_FMT D3DFMT_X8R8G8B8

#endif

#endif
