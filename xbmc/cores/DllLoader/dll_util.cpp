/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/log.h"
#include "dll_util.h"

#ifdef TARGET_WINDOWS
#include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>

#ifdef _cplusplus
extern "C"
{
#endif

static int iDllDummyOutputCall = 0;
void dll_dummy_output(char* dllname, char* funcname)
{
  CLog::Log(LOGERROR, "%s: Unresolved function called (%s), Count number %d", dllname, funcname, ++iDllDummyOutputCall);
}

// this piece of asm code only calls dll_dummy_output(s, s) and will return NULL
unsigned char dummy_func[] = {
    0x55,                            // push        ebp
    0x8b, 0xec,                      // mov         ebp,esp
    0xa1, 0, 0, 0, 0,                // mov         eax,dword ptr [0 0 0 0]
    0x50,                            // push        eax
    0xa1, 0, 0, 0, 0,                // mov         eax,dword ptr [0 0 0 0]
    0x50,                            // push        eax
    0xff, 0x15, 0, 0, 0, 0,          // call        dword ptr[dll_dummy_output]
    0x83, 0xc4, 0x08,                // add         esp,8
    0x33, 0xc0,                      // xor         eax,eax       // return NULL
    0x5d,                            // pop         ebp
    0xc3                            // ret
  };

/* Create a new callable function
 * This allocates a few bytes with the next content
 *
 * 1 function in assembly code (dummy_func)
 * 2 datapointer               (pointer to dll string)
 * 3 datapointer               (pointer to function string)
 * 4 datapointer               (pointer to function string)
 * 5 string                    (string of chars representing dll name)
 * 6 string                    (string of chars representing function name)
 */
uintptr_t create_dummy_function(const char* strDllName, const char* strFunctionName)
{
  size_t iFunctionSize = sizeof(dummy_func);
  size_t iDllNameSize = strlen(strDllName) + 1;
  size_t iFunctionNameSize = strlen(strFunctionName) + 1;

  // allocate memory for function + strings + 3 x 4 bytes for three datapointers
  char* pData = (char*)malloc(iFunctionSize + 12 + iDllNameSize + iFunctionNameSize);
  if (!pData)
    return 0;

  char* offDataPointer1 = pData + iFunctionSize;
  char* offDataPointer2 = pData + iFunctionSize + 4;
  char* offDataPointer3 = pData + iFunctionSize + 8;
  char* offStringDll = pData + iFunctionSize + 12;
  char* offStringFunc = pData + iFunctionSize + 12 + iDllNameSize;

  // 1 copy assembly code
  memcpy(pData, dummy_func, iFunctionSize);

  // insert pointers to datapointers into assembly code (fills 0x00000000 in dummy_func)
  *(int*)(pData + 4) = (intptr_t)offDataPointer1;
  *(int*)(pData + 10) = (intptr_t)offDataPointer2;
  *(int*)(pData + 17) = (intptr_t)offDataPointer3;

  // 2 fill datapointer with pointer to 5 (string)
  *(int*)offDataPointer1 = (intptr_t)offStringFunc;
  // 3 fill datapointer with pointer to 6 (string)
  *(int*)offDataPointer2 = (intptr_t)offStringDll;
  // 4 fill datapointer with pointer to dll_dummy_output
  *(int*)offDataPointer3 = (intptr_t)dll_dummy_output;

  // copy arguments to 5 (string) and 6 (string)
  memcpy(offStringDll, strDllName, iDllNameSize);
  memcpy(offStringFunc, strFunctionName, iFunctionNameSize);

  return (uintptr_t)pData;
}

uintptr_t get_win_function_address(const char* strDllName, const char* strFunctionName)
{
#ifdef TARGET_WINDOWS
  HMODULE handle = GetModuleHandle(strDllName);
  if(handle == NULL)
  {
    handle = LoadLibrary(strDllName);
  }
  if(handle != NULL)
  {
    uintptr_t pGNSI = (uintptr_t)GetProcAddress(handle, strFunctionName);
    if(pGNSI != NULL)
      return pGNSI;
  }
#endif
  return 0;
}

#ifdef _cplusplus
}
#endif
