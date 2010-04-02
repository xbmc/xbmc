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
#include "DShowUtil/DShowUtil.h"
#include "AllocatorCommon.h"
#include "VMR9AllocatorPresenter.h"
#include "EVRAllocatorPresenter.h"

bool IsVMR9InGraph(IFilterGraph* pFG)
{
  BeginEnumFilters(pFG, pEF, pBF)
    if(Com::SmartQIPtr<IVMRWindowlessControl9>(pBF)) return(true);
  EndEnumFilters
    return(false);
}

//

HRESULT CreateAP9(const CLSID& clsid, HWND hWnd, ISubPicAllocatorPresenter** ppAP)
{
  CheckPointer(ppAP, E_POINTER);

  *ppAP = NULL;

  HRESULT hr = E_FAIL;
  CStdString Error; 
  if(clsid == CLSID_VMR9AllocatorPresenter && !(*ppAP = DNew CVMR9AllocatorPresenter(hWnd, hr, Error)))
    return E_OUTOFMEMORY;

  if(*ppAP == NULL)
    return E_FAIL;

  (*ppAP)->AddRef();

  if(FAILED(hr))
  {
    Error += L"\n";
    Error += GetWindowsErrorMessage(hr, NULL);

    CLog::Log(LOGERROR, "%s %s", __FUNCTION__, Error.c_str());
    (*ppAP)->Release();
    *ppAP = NULL;
  }
  else if (!Error.IsEmpty())
  {
    CLog::Log(LOGWARNING, "%s %s", __FUNCTION__, Error.c_str());
  }

  return hr;
}

HRESULT CreateEVR(const CLSID& clsid, HWND hWnd, ISubPicAllocatorPresenter** ppAP)
{
  HRESULT    hr = E_FAIL;
  if (clsid == CLSID_EVRAllocatorPresenter)
  {
    CStdString Error;
    *ppAP  = DNew CEVRAllocatorPresenter(hWnd, hr, Error);
    (*ppAP)->AddRef();

    if(FAILED(hr))
    {
      Error += "\n";
      Error += GetWindowsErrorMessage(hr, NULL);
      CLog::Log(LOGERROR, "%s %s", __FUNCTION__, Error.c_str());
      (*ppAP)->Release();
      *ppAP = NULL;
    }
    else if (!Error.IsEmpty())
    {
      CLog::Log(LOGWARNING, "%s %s", __FUNCTION__, Error.c_str());
    }
  }

  return hr;
}

CStdString GetWindowsErrorMessage(HRESULT _Error, HMODULE _Module)
{

  switch (_Error)
  {
  case D3DERR_WRONGTEXTUREFORMAT               : return _T("D3DERR_WRONGTEXTUREFORMAT");
  case D3DERR_UNSUPPORTEDCOLOROPERATION        : return _T("D3DERR_UNSUPPORTEDCOLOROPERATION");
  case D3DERR_UNSUPPORTEDCOLORARG              : return _T("D3DERR_UNSUPPORTEDCOLORARG");
  case D3DERR_UNSUPPORTEDALPHAOPERATION        : return _T("D3DERR_UNSUPPORTEDALPHAOPERATION");
  case D3DERR_UNSUPPORTEDALPHAARG              : return _T("D3DERR_UNSUPPORTEDALPHAARG");
  case D3DERR_TOOMANYOPERATIONS                : return _T("D3DERR_TOOMANYOPERATIONS");
  case D3DERR_CONFLICTINGTEXTUREFILTER         : return _T("D3DERR_CONFLICTINGTEXTUREFILTER");
  case D3DERR_UNSUPPORTEDFACTORVALUE           : return _T("D3DERR_UNSUPPORTEDFACTORVALUE");
  case D3DERR_CONFLICTINGRENDERSTATE           : return _T("D3DERR_CONFLICTINGRENDERSTATE");
  case D3DERR_UNSUPPORTEDTEXTUREFILTER         : return _T("D3DERR_UNSUPPORTEDTEXTUREFILTER");
  case D3DERR_CONFLICTINGTEXTUREPALETTE        : return _T("D3DERR_CONFLICTINGTEXTUREPALETTE");
  case D3DERR_DRIVERINTERNALERROR              : return _T("D3DERR_DRIVERINTERNALERROR");
  case D3DERR_NOTFOUND                         : return _T("D3DERR_NOTFOUND");
  case D3DERR_MOREDATA                         : return _T("D3DERR_MOREDATA");
  case D3DERR_DEVICELOST                       : return _T("D3DERR_DEVICELOST");
  case D3DERR_DEVICENOTRESET                   : return _T("D3DERR_DEVICENOTRESET");
  case D3DERR_NOTAVAILABLE                     : return _T("D3DERR_NOTAVAILABLE");
  case D3DERR_OUTOFVIDEOMEMORY                 : return _T("D3DERR_OUTOFVIDEOMEMORY");
  case D3DERR_INVALIDDEVICE                    : return _T("D3DERR_INVALIDDEVICE");
  case D3DERR_INVALIDCALL                      : return _T("D3DERR_INVALIDCALL");
  case D3DERR_DRIVERINVALIDCALL                : return _T("D3DERR_DRIVERINVALIDCALL");
  case D3DERR_WASSTILLDRAWING                  : return _T("D3DERR_WASSTILLDRAWING");
  case D3DOK_NOAUTOGEN                         : return _T("D3DOK_NOAUTOGEN");
  case D3DERR_DEVICEREMOVED                    : return _T("D3DERR_DEVICEREMOVED");
  case S_NOT_RESIDENT                          : return _T("S_NOT_RESIDENT");
  case S_RESIDENT_IN_SHARED_MEMORY             : return _T("S_RESIDENT_IN_SHARED_MEMORY");
  case S_PRESENT_MODE_CHANGED                  : return _T("S_PRESENT_MODE_CHANGED");
  case S_PRESENT_OCCLUDED                      : return _T("S_PRESENT_OCCLUDED");
  case D3DERR_DEVICEHUNG                       : return _T("D3DERR_DEVICEHUNG");
  }

  CStdString errmsg;
  LPVOID lpMsgBuf;
  if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS|FORMAT_MESSAGE_FROM_HMODULE,
    _Module, _Error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL))
  {
    errmsg = (LPCTSTR)lpMsgBuf;
    LocalFree(lpMsgBuf);
  }
  CStdString Temp;
  Temp.Format("0x%08x ", _Error);
  return Temp + errmsg;
}

const wchar_t *GetD3DFormatStr(D3DFORMAT Format)
{
  switch (Format)
  {
  case D3DFMT_R8G8B8 : return L"R8G8B8";
  case D3DFMT_A8R8G8B8 : return L"A8R8G8B8";
  case D3DFMT_X8R8G8B8 : return L"X8R8G8B8";
  case D3DFMT_R5G6B5 : return L"R5G6B5";
  case D3DFMT_X1R5G5B5 : return L"X1R5G5B5";
  case D3DFMT_A1R5G5B5 : return L"A1R5G5B5";
  case D3DFMT_A4R4G4B4 : return L"A4R4G4B4";
  case D3DFMT_R3G3B2 : return L"R3G3B2";
  case D3DFMT_A8 : return L"A8";
  case D3DFMT_A8R3G3B2 : return L"A8R3G3B2";
  case D3DFMT_X4R4G4B4 : return L"X4R4G4B4";
  case D3DFMT_A2B10G10R10: return L"A2B10G10R10";
  case D3DFMT_A8B8G8R8 : return L"A8B8G8R8";
  case D3DFMT_X8B8G8R8 : return L"X8B8G8R8";
  case D3DFMT_G16R16 : return L"G16R16";
  case D3DFMT_A2R10G10B10: return L"A2R10G10B10";
  case D3DFMT_A16B16G16R16 : return L"A16B16G16R16";
  case D3DFMT_A8P8 : return L"A8P8";
  case D3DFMT_P8 : return L"P8";
  case D3DFMT_L8 : return L"L8";
  case D3DFMT_A8L8 : return L"A8L8";
  case D3DFMT_A4L4 : return L"A4L4";
  case D3DFMT_V8U8 : return L"V8U8";
  case D3DFMT_L6V5U5 : return L"L6V5U5";
  case D3DFMT_X8L8V8U8 : return L"X8L8V8U8";
  case D3DFMT_Q8W8V8U8 : return L"Q8W8V8U8";
  case D3DFMT_V16U16 : return L"V16U16";
  case D3DFMT_A2W10V10U10: return L"A2W10V10U10";
  case D3DFMT_UYVY : return L"UYVY";
  case D3DFMT_R8G8_B8G8: return L"R8G8_B8G8";
  case D3DFMT_YUY2 : return L"YUY2";
  case D3DFMT_G8R8_G8B8: return L"G8R8_G8B8";
  case D3DFMT_DXT1 : return L"DXT1";
  case D3DFMT_DXT2 : return L"DXT2";
  case D3DFMT_DXT3 : return L"DXT3";
  case D3DFMT_DXT4 : return L"DXT4";
  case D3DFMT_DXT5 : return L"DXT5";
  case D3DFMT_D16_LOCKABLE : return L"D16_LOCKABLE";
  case D3DFMT_D32: return L"D32";
  case D3DFMT_D15S1: return L"D15S1";
  case D3DFMT_D24S8: return L"D24S8";
  case D3DFMT_D24X8: return L"D24X8";
  case D3DFMT_D24X4S4: return L"D24X4S4";
  case D3DFMT_D16: return L"D16";
  case D3DFMT_D32F_LOCKABLE: return L"D32F_LOCKABLE";
  case D3DFMT_D24FS8 : return L"D24FS8";
  case D3DFMT_D32_LOCKABLE : return L"D32_LOCKABLE";
  case D3DFMT_S8_LOCKABLE: return L"S8_LOCKABLE";
  case D3DFMT_L16: return L"L16";
  case D3DFMT_VERTEXDATA : return L"VERTEXDATA";
  case D3DFMT_INDEX16: return L"INDEX16";
  case D3DFMT_INDEX32: return L"INDEX32";
  case D3DFMT_Q16W16V16U16 : return L"Q16W16V16U16";
  case D3DFMT_MULTI2_ARGB8 : return L"MULTI2_ARGB8";
  case D3DFMT_R16F : return L"R16F";
  case D3DFMT_G16R16F: return L"G16R16F";
  case D3DFMT_A16B16G16R16F: return L"A16B16G16R16F";
  case D3DFMT_R32F : return L"R32F";
  case D3DFMT_G32R32F: return L"G32R32F";
  case D3DFMT_A32B32G32R32F: return L"A32B32G32R32F";
  case D3DFMT_CxV8U8 : return L"CxV8U8";
  case D3DFMT_A1 : return L"A1";
  case D3DFMT_BINARYBUFFER : return L"BINARYBUFFER";
  }
  return L"Unknown";
}
