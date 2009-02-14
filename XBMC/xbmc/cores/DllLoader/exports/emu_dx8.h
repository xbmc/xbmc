#ifndef _EMU_DX8_H
#define _EMU_DX8_H

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

extern "C"
{
  void OutputDebug(char* strDebug);
  void d3dSetTextureStageState( int x, DWORD dwY, DWORD dwZ);
  void d3dSetRenderState(DWORD dwY, DWORD dwZ);
  void d3dGetRenderState(DWORD dwY, DWORD* dwZ);
  void d3dSetTransform(DWORD dwY, D3DMATRIX* dwZ);
  bool d3dCreateTexture(unsigned int width, unsigned int height, LPDIRECT3DTEXTURE8 *pTexture);
  void d3dDrawIndexedPrimitive(D3DPRIMITIVETYPE primType, unsigned int minIndex, unsigned int numVertices, unsigned int startIndex, unsigned int primCount);
#ifdef _XBOX
  HRESULT WINAPI d3dXCreateTextureFromFileA(LPDIRECT3DDEVICE8 pDevice, LPCSTR pSrcFile, LPDIRECT3DTEXTURE8* ppTexture);
  HRESULT WINAPI d3dXCreateCubeTextureFromFileA(LPDIRECT3DDEVICE8 pDevice, LPCSTR pSrcFile, LPDIRECT3DCUBETEXTURE8* ppCubeTexture);
  HRESULT WINAPI d3dXCreateTextureFromFileExA(
    LPDIRECT3DDEVICE8 pDevice,
    LPCSTR pSrcFile,
    UINT Width,
    UINT Height,
    UINT MipLevels,
    DWORD Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    DWORD Filter,
    DWORD MipFilter,
    D3DCOLOR ColorKey,
    D3DXIMAGE_INFO* pSrcInfo,
    PALETTEENTRY* pPalette,
    LPDIRECT3DTEXTURE8* ppTexture
  );
#endif
}

#endif // _EMU_DX8_H

