
#ifndef _EMU_REGISTRY_H
#define _EMU_REGISTRY_H

#include "emu_registry.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define HCRYPTPROV DWORD
  /********************************************************
   *
   *       Declaration of registry access functions
   *       Copyright 2000 Eugene Kuznetsov  (divx@euro.ru)
   *
   ********************************************************/
#ifdef _XBOX
#define HKEY_CLASSES_ROOT       ((HKEY) 0x80000000)
#define HKEY_CURRENT_USER       ((HKEY) 0x80000001)
#define HKEY_LOCAL_MACHINE      ((HKEY) 0x80000002)
#define HKEY_USERS              ((HKEY) 0x80000003)
#define HKEY_PERFORMANCE_DATA   ((HKEY) 0x80000004)
#define HKEY_CURRENT_CONFIG     ((HKEY) 0x80000005)
#define HKEY_DYN_DATA           ((HKEY) 0x80000006)

#define REG_DWORD                   ( 4 )   // 32-bit number
#define REG_SZ                      ( 1 )   // null terminated string
#define REG_CREATED_NEW_KEY 0x00000001
#endif
  /*
   * registry provider structs
   */ 
  /*typedef struct value_entA
  {   LPSTR ve_valuename;
      DWORD ve_valuelen;
      DWORD_PTR ve_valueptr;
      DWORD ve_type;
  } VALENTA, *PVALENTA;
   
  typedef struct value_entW {
      LPWSTR ve_valuename;
      DWORD ve_valuelen;
      DWORD_PTR ve_valueptr;
      DWORD ve_type;
  } VALENTW, *PVALENTW;*/

  typedef ACCESS_MASK REGSAM;
  ///////////////////////////////////////////////////////////////////////////////

  LONG WINAPI dllRegCloseKey (HKEY hKey);

  LONG WINAPI dllRegOpenKeyA (HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult);

  LONG WINAPI dllRegSetValueA (HKEY hKey, LPCTSTR lpSubKey, DWORD dwType,
                               LPCTSTR lpData, DWORD cbData);

  LONG WINAPI dllRegOpenKeyExA (HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions,
                                REGSAM samDesired, PHKEY phkResult);

  LONG WINAPI dllRegEnumKeyExA (HKEY hKey, DWORD dwIndex, LPTSTR lpName,
                                LPDWORD lpcName, LPDWORD lpReserved, LPTSTR lpClass,
                                LPDWORD lpcClass, PFILETIME lpftLastWriteTime);

  LONG WINAPI dllRegDeleteKeyA (HKEY hKey, LPCTSTR lpSubKey);

  LONG WINAPI dllRegQueryValueExA (HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved,
                                   LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);

  LONG WINAPI dllRegQueryValueExW (HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved,
                                   LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);

  LONG WINAPI dllRegCreateKeyA (HKEY hKey, LPCTSTR lpSubKey, PHKEY phkResult);

  LONG WINAPI dllRegSetValueExA (HKEY hKey, LPCTSTR lpValueName, DWORD Reserved,
                                 DWORD dwType, const BYTE* lpData, DWORD cbData);

  LONG WINAPI dllRegCreateKeyExA (HKEY hKey, LPCTSTR lpSubKey, DWORD Reserved,
                                  LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired,
                                  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                  PHKEY phkResult, LPDWORD lpdwDisposition);

  LONG WINAPI dllRegEnumValueA (HKEY hKey, DWORD dwIndex, LPTSTR lpValueName,
                                LPDWORD lpcValueName, LPDWORD lpReserved,
                                LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);

  LONG WINAPI dllRegQueryInfoKeyA( HKEY hkey, LPSTR class_, LPDWORD class_len, LPDWORD reserved,
                                   LPDWORD subkeys, LPDWORD max_subkey, LPDWORD max_class,
                                   LPDWORD values, LPDWORD max_value, LPDWORD max_data,
                                   LPDWORD security, FILETIME *modif );
                                   
  LONG WINAPI dllRegQueryValueA (HKEY hKey, LPCTSTR lpSubKey, LPTSTR lpValue, PLONG lpcbValue);


  void free_registry(void);
  int save_registry(char* filename);

  BOOL WINAPI dllCryptAcquireContextA(HCRYPTPROV* phProv, LPCTSTR pszContainer, LPCTSTR pszProvider, DWORD dwProvType, DWORD dwFlags);
  BOOL WINAPI dllCryptGenRandom(HCRYPTPROV hProv, DWORD dwLen, BYTE* pbBuffer);
  BOOL WINAPI dllCryptReleaseContext(HCRYPTPROV hProv, DWORD dwFlags);

#ifdef __cplusplus
}
#endif

#endif // _EMU_REGISTRY_H
