/*
* XBoxMediaCenter
* Copyright (c) 2003 Frodo/jcmarshall
* Portions Copyright (c) by the authors of ffmpeg / xvid /mplayer
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#include "../../stdafx.h"
#include "PixelShaderRenderer.h"
#include "GUIFontManager.h"


CPixelShaderRenderer::CPixelShaderRenderer(LPDIRECT3DDEVICE8 pDevice)
    : CXBoxRenderer(pDevice)
{
}

unsigned int CPixelShaderRenderer::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps)
{
  CXBoxRenderer::Configure(width, height, d_width, d_height, fps);
  m_bConfigured = true;
  return 0;
}

void CPixelShaderRenderer::PrepareDisplay()
{
  ++m_iYV12DecodeBuffer %= m_NumYV12Buffers;
  CXBoxRenderer::PrepareDisplay();
}

void CPixelShaderRenderer::Render()
{

  ResetEvent(m_eventTexturesDone);

  // this is the low memory renderer
  RenderLowMem();

  //Okey, when the gpu is done with the textures here, they are free to be modified again
  m_pD3DDevice->InsertCallback(D3DCALLBACK_WRITE,&TextureCallback, (DWORD)m_eventTexturesDone);

  RenderOSD();

#ifndef _DEBUG
  if (g_stSettings.m_bShowFreeMem)
#endif
    if (g_graphicsContext.IsFullScreenVideo())
    {
      // in debug mode, show freememory
      CStdStringW wszText;
      MEMORYSTATUS stat;
      GlobalMemoryStatus(&stat);
      wszText.Format(L"FreeMem %d/%d", stat.dwAvailPhys, stat.dwTotalPhys);

      CGUIFont* pFont = g_fontManager.GetFont("font13");
      if (pFont)
      {
        pFont->DrawText( 60, 40, 0xffffffff, wszText);
      }
    }
}
