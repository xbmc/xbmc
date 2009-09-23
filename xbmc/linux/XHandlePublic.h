#ifndef XHANDLEPUBLIC_H
#define XHANDLEPUBLIC_H

struct  CXHandle;
typedef CXHandle* HANDLE;
typedef HANDLE*   LPHANDLE;

bool CloseHandle(HANDLE hObject);

#define DUPLICATE_CLOSE_SOURCE 0x00000001
#define DUPLICATE_SAME_ACCESS  0x00000002

BOOL WINAPI DuplicateHandle(
  HANDLE hSourceProcessHandle,
  HANDLE hSourceHandle,
  HANDLE hTargetProcessHandle,
  LPHANDLE lpTargetHandle,
  DWORD dwDesiredAccess,
  BOOL bInheritHandle,
  DWORD dwOptions
);

#endif
