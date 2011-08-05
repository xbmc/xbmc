#include "StdAfx.h"
#include ".\fontinstaller.h"
#include <fstream>

CFontInstaller::CFontInstaller()
{
  if(HMODULE hGdi = GetModuleHandle(_T("gdi32.dll")))
  {
    pAddFontMemResourceEx = (HANDLE (WINAPI *)(PVOID,DWORD,PVOID,DWORD*))GetProcAddress(hGdi, "AddFontMemResourceEx");
    pAddFontResourceEx = (int (WINAPI *)(LPCTSTR,DWORD,PVOID))GetProcAddress(hGdi, "AddFontResourceExA");
    pRemoveFontMemResourceEx = (BOOL (WINAPI *)(HANDLE))GetProcAddress(hGdi, "RemoveFontMemResourceEx");
    pRemoveFontResourceEx = (BOOL (WINAPI *)(LPCTSTR,DWORD,PVOID))GetProcAddress(hGdi, "RemoveFontResourceExA");
  }

  if(HMODULE hGdi = GetModuleHandle(_T("kernel32.dll")))
  {
    pMoveFileEx = (BOOL (WINAPI *)(LPCTSTR, LPCTSTR, DWORD))GetProcAddress(hGdi, "MoveFileExA");
  }
}

CFontInstaller::~CFontInstaller()
{
  UninstallFonts();
}

bool CFontInstaller::InstallFont(const std::vector<BYTE>& data)
{
  return InstallFont(&data[0], data.size());
}

bool CFontInstaller::InstallFont(const void* pData, UINT len)
{
  return InstallFontFile(pData, len) || InstallFontMemory(pData, len);
}

void CFontInstaller::UninstallFonts()
{
  if(pRemoveFontMemResourceEx)
  {
    std::list<HANDLE>::iterator pos = m_fonts.begin();
    while(pos != m_fonts.end()) { pRemoveFontMemResourceEx(*pos); ++pos; }
    m_fonts.clear();
  }

  if(pRemoveFontResourceEx)
  {
    std::list<CStdString>::iterator pos = m_files.begin();
    while(pos != m_files.end())
    {
      CStdString fn = (*pos); pos++;
      pRemoveFontResourceEx(fn, FR_PRIVATE, 0);
      if(!DeleteFile(fn) && pMoveFileEx)
        pMoveFileEx(fn, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    }
    
    m_files.clear();
  }
}

bool CFontInstaller::InstallFontMemory(const void* pData, UINT len)
{
  if(!pAddFontMemResourceEx)
    return false;

  DWORD nFonts = 0;
  HANDLE hFont = pAddFontMemResourceEx((PVOID)pData, len, NULL, &nFonts);
  if(hFont && nFonts > 0) m_fonts.push_back(hFont);
  return hFont && nFonts > 0;
}

bool CFontInstaller::InstallFontFile(const void* pData, UINT len)
{
  if(!pAddFontResourceEx) 
    return false;

  std::ofstream f;
  TCHAR path[MAX_PATH], fn[MAX_PATH];
  if(!GetTempPath(MAX_PATH, path) || !GetTempFileName(path, _T("g_font"), 0, fn))
    return false;

  f.open(fn, std::ios_base::binary | std::ios_base::out);
  if (!f.fail())
  {
    f.write((const char *)pData, len);
    f.close();

    if(pAddFontResourceEx(fn, FR_PRIVATE, 0) > 0)
    {
      m_files.push_back(fn);
      return true;
    }
  }

  DeleteFile(fn);
  return false;
}
