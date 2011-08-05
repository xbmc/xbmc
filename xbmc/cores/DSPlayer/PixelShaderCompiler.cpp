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

#ifdef HAS_DS_PLAYER

#include "Filters\RendererSettings.h"
#include "PixelShaderCompiler.h"
#include "windowing/WindowingFactory.h"
#include "utils/Log.h"
#include "DSUtil/SmartPtr.h"

CPixelShaderCompiler::CPixelShaderCompiler(bool fStaySilent)
  : m_pD3DXCompileShader(NULL)
  , m_pD3DXDisassembleShader(NULL)
{
  HINSTANCE    hDll;
  hDll = g_dsSettings.GetD3X9Dll();

  if(hDll)
  {
    m_pD3DXCompileShader = (D3DXCompileShaderPtr)GetProcAddress(hDll, "D3DXCompileShader");
    m_pD3DXDisassembleShader = (D3DXDisassembleShaderPtr)GetProcAddress(hDll, "D3DXDisassembleShader");
  }

  if(!fStaySilent)
  {
    if(!hDll)
    {
      CLog::Log(LOGERROR,"%s Cannot load D3DX9_xx.DLL, pixel shaders will not work.",__FUNCTION__);
    }
    else if(!m_pD3DXCompileShader || !m_pD3DXDisassembleShader) 
    {
      CLog::Log(LOGERROR,"%s Cannot find necessary function entry points in D3DX9_xx.DLL, pixel shaders will not work.",__FUNCTION__);
    }
  }
}

CPixelShaderCompiler::~CPixelShaderCompiler()
{
}

HRESULT CPixelShaderCompiler::CompileShader(
    LPCSTR pSrcData,
    LPCSTR pFunctionName,
    LPCSTR pProfile,
    DWORD Flags,
    IDirect3DPixelShader9** ppPixelShader,
    CStdString* disasm,
    CStdString* errmsg)
{
  if(!m_pD3DXCompileShader || !m_pD3DXDisassembleShader)
    return E_FAIL;

  HRESULT hr;

  Com::SmartPtr<ID3DXBuffer> pShader, pDisAsm, pErrorMsgs;
  hr = m_pD3DXCompileShader(pSrcData, strlen(pSrcData), NULL, NULL, pFunctionName, pProfile, Flags, &pShader, &pErrorMsgs, NULL);

  if(FAILED(hr))
  {
    if(errmsg)
    {
      CStdStringA msg = "Unexpected compiler error";

      if(pErrorMsgs)
      {
        int len = pErrorMsgs->GetBufferSize();
        memcpy(msg.GetBufferSetLength(len), pErrorMsgs->GetBufferPointer(), len);
      }

      *errmsg = msg;
    }

    return hr;
  }

  if(ppPixelShader)
  {
    if(!g_Windowing.Get3DDevice()) return E_FAIL;
    hr = g_Windowing.Get3DDevice()->CreatePixelShader((DWORD*)pShader->GetBufferPointer(), ppPixelShader);
    if(FAILED(hr)) return hr;
  }

  if(disasm)
  {
    hr = m_pD3DXDisassembleShader((DWORD*)pShader->GetBufferPointer(), FALSE, NULL, &pDisAsm);
    if(SUCCEEDED(hr) && pDisAsm) *disasm = CStdStringA((const char*)pDisAsm->GetBufferPointer());
  }

  return S_OK;
}

#endif