/* 
 *  Copyright (C) 2003-2006 Gabest
 *  http://www.gabest.org
 *
 *  Copyright (C) 2005-2010 Team XBMC
 *  http://www.xbmc.org
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
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include <d3dx9shader.h>
#include "DSUtil/DSUtil.h"
#include "DSUtil/SmartPtr.h"
#include "threads/CriticalSection.h"
#include "threads/SingleLock.h"

class CPixelShaderCompiler
{
  typedef HRESULT (WINAPI * D3DXCompileShaderPtr) (
        LPCSTR    pSrcData,
        UINT    SrcDataLen,
        CONST D3DXMACRO* pDefines,
        LPD3DXINCLUDE  pInclude,
        LPCSTR    pFunctionName,
        LPCSTR    pProfile,
        DWORD    Flags,
        LPD3DXBUFFER*  ppShader,
        LPD3DXBUFFER*  ppErrorMsgs,
        LPD3DXCONSTANTTABLE* ppConstantTable);

  typedef HRESULT (WINAPI * D3DXDisassembleShaderPtr) (
        CONST DWORD*  pShader, 
        bool    EnableColorCode, 
        LPCSTR    pComments, 
        LPD3DXBUFFER*  ppDisassembly);
  
  D3DXCompileShaderPtr m_pD3DXCompileShader;
  D3DXDisassembleShaderPtr m_pD3DXDisassembleShader;

public:
  CPixelShaderCompiler(bool fStaySilent = false);
  virtual ~CPixelShaderCompiler();

  HRESULT CompileShader(
        LPCSTR pSrcData,
        LPCSTR pFunctionName,
        LPCSTR pProfile,
        DWORD Flags,
        IDirect3DPixelShader9** ppPixelShader,
        CStdString* disasm = NULL,
        CStdString* errmsg = NULL);
};
