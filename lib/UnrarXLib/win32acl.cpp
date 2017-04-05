static void SetPrivileges();

static bool ReadSacl=false;



#ifndef SFX_MODULE
void ExtractACL(Archive &Arc,char *FileName,wchar *FileNameW)
{
  return;
}
#endif


void ExtractACLNew(Archive &Arc,char *FileName,wchar *FileNameW)
{
#if defined(_XBOX) || defined(_LINUX) || defined(TARGET_WINDOWS) || defined(TARGET_WIN10)
  return;
#else
  if (!WinNT())
    return;

  Array<byte> SubData;
  if (!Arc.ReadSubData(&SubData,NULL))
    return;

  SetPrivileges();

  SECURITY_INFORMATION  si=OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|
                           DACL_SECURITY_INFORMATION;
  if (ReadSacl)
    si|=SACL_SECURITY_INFORMATION;
  SECURITY_DESCRIPTOR *sd=(SECURITY_DESCRIPTOR *)&SubData[0];

  int SetCode;
  if (FileNameW!=NULL)
    SetCode=SetFileSecurityW(FileNameW,si,sd);
  else
    SetCode=SetFileSecurity(FileName,si,sd);

  if (!SetCode)
  {
    Log(Arc.FileName,St(MACLSetError),FileName);
    ErrHandler.SysErrMsg();
    ErrHandler.SetErrorCode(WARNING);
  }
#endif
}


void SetPrivileges()
{
#if defined(_XBOX) || defined(_LINUX)
  return;
#else
  static bool InitDone=false;
  if (InitDone)
    return;
  InitDone=true;

  HANDLE hToken;

  if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
    return;

  TOKEN_PRIVILEGES tp;
  tp.PrivilegeCount = 1;
  tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  if (LookupPrivilegeValue(NULL,SE_SECURITY_NAME,&tp.Privileges[0].Luid))
    if (AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL) &&
        GetLastError() == ERROR_SUCCESS)
      ReadSacl=true;

  if (LookupPrivilegeValue(NULL,SE_RESTORE_NAME,&tp.Privileges[0].Luid))
    AdjustTokenPrivileges(hToken, FALSE, &tp, 0, NULL, NULL);

  CloseHandle(hToken);
#endif
}
