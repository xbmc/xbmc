/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "stdafx.h"
#include "emu_dx8.h"
#include "GraphicContext.h"

extern "C" void OutputDebug(char* strDebug)
{
  OutputDebugString(strDebug);
}
extern "C" void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ)
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->SetTextureStageState(x, (D3DTEXTURESTAGESTATETYPE)dwY, dwZ);
#endif
}

extern "C" void d3dSetRenderState(DWORD dwY, DWORD dwZ)
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->SetRenderState((D3DRENDERSTATETYPE)dwY, dwZ);
#endif
}
extern "C" void d3dGetRenderState(DWORD dwY, DWORD* dwZ)
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->GetRenderState((D3DRENDERSTATETYPE)dwY, dwZ);
#endif
}
extern "C" void d3dSetTransform( DWORD dwY, D3DMATRIX* dwZ )
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->SetTransform( (D3DTRANSFORMSTATETYPE)dwY, dwZ );
#endif
}

#ifdef HAS_SDL
extern "C" bool d3dCreateTexture(unsigned int width, unsigned int height, SDL_Surface **pTexture)
{
   if (pTexture == NULL)
      return false;
   *pTexture = SDL_CreateRGBSurface(SDL_HWSURFACE, width, height, 32, RMASK, GMASK, BMASK, AMASK);
   return (*pTexture != NULL);
}
#else
extern "C" bool d3dCreateTexture(unsigned int width, unsigned int height, LPDIRECT3DTEXTURE8 *pTexture)
{
  return (D3D_OK == g_graphicsContext.Get3DDevice()->CreateTexture(width, height, 1, 0, D3DFMT_LIN_A8R8G8B8 , D3DPOOL_MANAGED, pTexture));
}
#endif

extern "C" void d3dDrawIndexedPrimitive(D3DPRIMITIVETYPE primType, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primCount)
{
#ifndef HAS_SDL
  g_graphicsContext.Get3DDevice()->DrawIndexedPrimitive(primType, minIndex, numVertices, startIndex, primCount);
#endif
}
