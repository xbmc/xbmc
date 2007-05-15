#ifndef __CONV_UTILS__H__
#define __CONV_UTILS__H__


int WideCharToMultiByte(
  UINT CodePage, 
  DWORD dwFlags, 
  LPCWSTR lpWideCharStr,
  int cchWideChar, 
  LPSTR lpMultiByteStr, 
  int cbMultiByte,
  LPCSTR lpDefaultChar,    
  LPBOOL lpUsedDefaultChar
);

int MultiByteToWideChar(
  UINT CodePage, 
  DWORD dwFlags,         
  LPCSTR lpMultiByteStr, 
  int cbMultiByte,       
  LPWSTR lpWideCharStr,  
  int cchWideChar        
);


DWORD GetLastError();
VOID  SetLastError(DWORD dwErrCode);

#endif
