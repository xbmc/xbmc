#include "rar.hpp"

MKDIR_CODE MakeDir(const char *Name,const wchar *NameW,uint Attr)
{
#ifdef _WIN_32
  int Success;
#if !defined(_XBOX) && !defined(_LINUX)
  if (WinNT() && NameW!=NULL && *NameW!=0)
    Success=CreateDirectoryW(NameW,NULL);
  else
#endif
    Success=CreateDirectory(Name,NULL);
  if (Success)
  {
    SetFileAttr(Name,NameW,Attr);
    return(MKDIR_SUCCESS);
  }
  int ErrCode=GetLastError();
  if (ErrCode==ERROR_FILE_NOT_FOUND || ErrCode==ERROR_PATH_NOT_FOUND)
    return(MKDIR_BADPATH);
  return(MKDIR_ERROR);
#endif
#ifdef _EMX
#ifdef _DJGPP
  if (mkdir(Name,(Attr & FA_RDONLY) ? 0:S_IWUSR)==0)
#else
  if (__mkdir(Name)==0)
#endif
  {
    SetFileAttr(Name,NameW,Attr);
    return(MKDIR_SUCCESS);
  }
  return(errno==ENOENT ? MKDIR_BADPATH:MKDIR_ERROR);
#endif
#ifdef _UNIX
  int prevmask=umask(0);
  int ErrCode=Name==NULL ? -1:mkdir(Name,(mode_t)Attr);
  umask(prevmask);
  if (ErrCode==-1)
    return(errno==ENOENT ? MKDIR_BADPATH:MKDIR_ERROR);
  return(MKDIR_SUCCESS);
#endif
}


void CreatePath(const char *Path,const wchar *PathW,bool SkipLastName)
{
#ifdef _WIN_32
  uint DirAttr=0;
#else
  uint DirAttr=0777;
#endif
#ifdef UNICODE_SUPPORTED
  bool Wide=PathW!=NULL && *PathW!=0 && UnicodeEnabled();
#else
  bool Wide=false;
#endif
  bool IgnoreAscii=false;

  const char *s=Path;
  for (int PosW=0;;PosW++)
  {
    if (s==NULL || s-Path>=NM || *s==0)
      IgnoreAscii=true;
    if (Wide && (PosW>=NM || PathW[PosW]==0) || !Wide && IgnoreAscii)
      break;
    if (Wide && PathW[PosW]==CPATHDIVIDER || !Wide && *s==CPATHDIVIDER)
    {
      wchar *DirPtrW=NULL,DirNameW[NM];
      if (Wide)
      {
        strncpyw(DirNameW,PathW,PosW);
        DirNameW[PosW]=0;
        DirPtrW=DirNameW;
      }
      char DirName[NM];
      if (IgnoreAscii)
        WideToChar(DirPtrW,DirName);
      else
      {
#ifndef DBCS_SUPPORTED
        if (*s!=CPATHDIVIDER)
          for (const char *n=s;*n!=0 && n-Path<NM;n++)
            if (*n==CPATHDIVIDER)
            {
              s=n;
              break;
            }
#endif
        strncpy(DirName,Path,s-Path);
        DirName[s-Path]=0;
      }
      if (MakeDir(DirName,DirPtrW,DirAttr)==MKDIR_SUCCESS)
      {
#ifndef GUI
        mprintf(St(MCreatDir),DirName);
        mprintf(" %s",St(MOk));
#endif
      }
    }
    if (!IgnoreAscii)
      s=charnext(s);
  }
  if (!SkipLastName)
    MakeDir(Path,PathW,DirAttr);
}


void SetDirTime(const char *Name,RarTime *ftm,RarTime *ftc,RarTime *fta)
{
#ifdef _WIN_32
  bool sm=ftm!=NULL && ftm->IsSet();
  bool sc=ftc!=NULL && ftc->IsSet();
  bool sa=ftc!=NULL && fta->IsSet();
  if (!WinNT())
    return;
  HANDLE hFile=CreateFile(Name,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
                          NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
  if (hFile==INVALID_HANDLE_VALUE)
    return;
  FILETIME fm,fc,fa;
  if (sm)
    ftm->GetWin32(&fm);
  if (sc)
    ftc->GetWin32(&fc);
  if (sa)
    fta->GetWin32(&fa);
  SetFileTime(hFile,sc ? &fc:NULL,sa ? &fa:NULL,sm ? &fm:NULL);
  CloseHandle(hFile);
#endif
#if defined(_UNIX) || defined(_EMX)
  File::SetCloseFileTimeByName(Name,ftm,fta);
#endif
}


bool IsRemovable(const char *Name)
{
#if defined(_XBOX) || defined(_LINUX)
	return false;
//#ifdef _WIN_32
#elif defined(_WIN_32)
  char Root[NM];
  GetPathRoot(Name,Root);
  int Type=GetDriveType(*Root ? Root:NULL);
  return(Type==DRIVE_REMOVABLE || Type==DRIVE_CDROM);
#elif defined(_EMX)
  char Drive=toupper(Name[0]);
  return((Drive=='A' || Drive=='B') && Name[1]==':');
#else
  return(false);
#endif
}


#ifndef SFX_MODULE
Int64 GetFreeDisk(const char *Name)
{
#if  defined(_XBOX) || defined(_LINUX)
	char Root[NM];
	GetPathRoot(Name,Root);

	ULARGE_INTEGER uiTotalSize,uiTotalFree,uiUserFree;
    uiUserFree.u.LowPart=uiUserFree.u.HighPart=0;
	if ( GetDiskFreeSpaceEx( Root, &uiUserFree, &uiTotalSize, &uiTotalFree ) ) {
		return(int32to64(uiUserFree.u.HighPart,uiUserFree.u.LowPart));
	}
	return 0;

//#ifdef _WIN_32
#elif defined(_WIN_32)
  char Root[NM];
  GetPathRoot(Name,Root);

  typedef BOOL (WINAPI *GETDISKFREESPACEEX)(
    LPCTSTR,PULARGE_INTEGER,PULARGE_INTEGER,PULARGE_INTEGER
   );
  static GETDISKFREESPACEEX pGetDiskFreeSpaceEx=NULL;

  if (pGetDiskFreeSpaceEx==NULL)
  {
	HMODULE hKernel=GetModuleHandle("kernel32.dll");
    if (hKernel!=NULL)
      pGetDiskFreeSpaceEx=(GETDISKFREESPACEEX)GetProcAddress(hKernel,"GetDiskFreeSpaceExA");
  }
  if (pGetDiskFreeSpaceEx!=NULL)
  {
    GetFilePath(Name,Root);
    ULARGE_INTEGER uiTotalSize,uiTotalFree,uiUserFree;
    uiUserFree.u.LowPart=uiUserFree.u.HighPart=0;
    if (pGetDiskFreeSpaceEx(*Root ? Root:NULL,&uiUserFree,&uiTotalSize,&uiTotalFree) &&
        uiUserFree.u.HighPart<=uiTotalFree.u.HighPart)
      return(int32to64(uiUserFree.u.HighPart,uiUserFree.u.LowPart));
  }

  DWORD SectorsPerCluster,BytesPerSector,FreeClusters,TotalClusters;
  if (!GetDiskFreeSpace(*Root ? Root:NULL,&SectorsPerCluster,&BytesPerSector,&FreeClusters,&TotalClusters))
    return(1457664);
  Int64 FreeSize=SectorsPerCluster*BytesPerSector;
  FreeSize=FreeSize*FreeClusters;
  return(FreeSize);
#elif defined(_BEOS)
  char Root[NM];
  GetFilePath(Name,Root);
  dev_t Dev=dev_for_path(*Root ? Root:".");
  if (Dev<0)
    return(1457664);
  fs_info Info;
  if (fs_stat_dev(Dev,&Info)!=0)
    return(1457664);
  Int64 FreeSize=Info.block_size;
  FreeSize=FreeSize*Info.free_blocks;
  return(FreeSize);
#elif defined(_UNIX)
  return(1457664);
#elif defined(_EMX)
  int Drive=(!isalpha(Name[0]) || Name[1]!=':') ? 0:toupper(Name[0])-'A'+1;
  if (_osmode == OS2_MODE)
  {
    FSALLOCATE fsa;
    if (DosQueryFSInfo(Drive,1,&fsa,sizeof(fsa))!=0)
      return(1457664);
    Int64 FreeSize=fsa.cSectorUnit*fsa.cbSector;
    FreeSize=FreeSize*fsa.cUnitAvail;
    return(FreeSize);
  }
  else
  {
    union REGS regs,outregs;
    memset(&regs,0,sizeof(regs));
    regs.h.ah=0x36;
    regs.h.dl=Drive;
    _int86 (0x21,&regs,&outregs);
    if (outregs.x.ax==0xffff)
      return(1457664);
    Int64 FreeSize=outregs.x.ax*outregs.x.cx;
    FreeSize=FreeSize*outregs.x.bx;
    return(FreeSize);
  }
#else
  #define DISABLEAUTODETECT
  return(1457664);
#endif
}
#endif


bool FileExist(const char *Name,const wchar *NameW)
{
#ifdef _WIN_32
#if !defined(_XBOX) && !defined(_LINUX)
    if (WinNT() && NameW!=NULL && *NameW!=0)
      return(GetFileAttributesW(NameW)!=0xffffffff);
    else
#endif
      return(GetFileAttributes(Name)!=0xffffffff);
#elif defined(ENABLE_ACCESS)
  return(access(Name,0)==0);
#else
  struct FindData FD;
  return(FindFile::FastFind(Name,NameW,&FD));
#endif
}


bool WildFileExist(const char *Name,const wchar *NameW)
{
  if (IsWildcard(Name,NameW))
  {
    FindFile Find;
    Find.SetMask(Name);
    Find.SetMaskW(NameW);
    struct FindData fd;
    return(Find.Next(&fd));
  }
  return(FileExist(Name,NameW));
}


bool IsDir(uint Attr)
{
#if defined (_WIN_32) || defined(_EMX)
  return(Attr!=0xffffffff && (Attr & 0x10)!=0);
#endif
#if defined(_UNIX)
  return((Attr & 0xF000)==0x4000);
#endif
}


bool IsUnreadable(uint Attr)
{
#if defined(_UNIX) && defined(S_ISFIFO) && defined(S_ISSOCK) && defined(S_ISCHR)
  return(S_ISFIFO(Attr) || S_ISSOCK(Attr) || S_ISCHR(Attr));
#endif
  return(false);
}


bool IsLabel(uint Attr)
{
#if defined (_WIN_32) || defined(_EMX)
  return((Attr & 8)!=0);
#else
  return(false);
#endif
}


bool IsLink(uint Attr)
{
#ifdef _UNIX
  return((Attr & 0xF000)==0xA000);
#endif
  return(false);
}






bool IsDeleteAllowed(uint FileAttr)
{
#if defined(_WIN_32) || defined(_EMX)
  return((FileAttr & (FA_RDONLY|FA_SYSTEM|FA_HIDDEN))==0);
#else
  return((FileAttr & (S_IRUSR|S_IWUSR))==(S_IRUSR|S_IWUSR));
#endif
}


void PrepareToDelete(const char *Name,const wchar *NameW)
{
#if defined(_WIN_32) || defined(_EMX)
  SetFileAttr(Name,NameW,0);
#endif
#ifdef _UNIX
  chmod(Name,S_IRUSR|S_IWUSR|S_IXUSR);
#endif
}


uint GetFileAttr(const char *Name,const wchar *NameW)
{
#ifdef _WIN_32
#if !defined(_XBOX) && !defined(_LINUX)
    if (WinNT() && NameW!=NULL && *NameW!=0)
      return(GetFileAttributesW(NameW));
    else
#endif
      return(GetFileAttributes(Name));
#elif defined(_DJGPP)
  return(_chmod(Name,0));
#else
  struct stat st;
  if (stat(Name,&st)!=0)
    return(0);
#ifdef _EMX
  return(st.st_attr);
#else
  return(st.st_mode);
#endif
#endif
}


bool SetFileAttr(const char *Name,const wchar *NameW,uint Attr)
{
  bool success;
#ifdef _WIN_32
#if !defined(_XBOX) && !defined(_LINUX)
    if (WinNT() && NameW!=NULL && *NameW!=0)
      success=SetFileAttributesW(NameW,Attr)!=0;
    else
#endif
      success=SetFileAttributes(Name,Attr)!=0;
#elif defined(_DJGPP)
  success=_chmod(Name,1,Attr)!=-1;
#elif defined(_EMX)
  success=__chmod(Name,1,Attr)!=-1;
#elif defined(_UNIX)
  success=chmod(Name,(mode_t)Attr)==0;
#else
  success=false;
#endif
  return(success);
}


void ConvertNameToFull(const char *Src,char *Dest)
{
#ifdef _WIN_32
//#ifndef _WIN_CE
#if !defined(_WIN_CE) && !defined(_XBOX) && !defined(_LINUX)
  char FullName[NM],*NamePtr;
  if (GetFullPathName(Src,sizeof(FullName),FullName,&NamePtr))
    strcpy(Dest,FullName);
  else
#endif
    if (Src!=Dest)
      strcpy(Dest,Src);
#else
  char FullName[NM];
  if (IsPathDiv(*Src) || IsDiskLetter(Src))
    strcpy(FullName,Src);
  else
  {
    getcwd(FullName,sizeof(FullName));
    AddEndSlash(FullName);
    strcat(FullName,Src);
  }
  strcpy(Dest,FullName);
#endif
}


#ifndef SFX_MODULE
void ConvertNameToFull(const wchar *Src,wchar *Dest)
{
  if (Src==NULL || *Src==0)
  {
    *Dest=0;
    return;
  }
#ifdef _WIN_32
#ifndef _WIN_CE
  if (WinNT())
#endif
  {
//#ifndef _WIN_CE
#if !defined(_WIN_CE) && !defined(_XBOX) && !defined(_LINUX)
    wchar FullName[NM],*NamePtr;
    if (GetFullPathNameW(Src,sizeof(FullName)/sizeof(FullName[0]),FullName,&NamePtr))
      strcpyw(Dest,FullName);
    else
#endif
      if (Src!=Dest)
        strcpyw(Dest,Src);
  }
#ifndef _WIN_CE
  else
  {
    char AnsiName[NM];
    WideToChar(Src,AnsiName);
    ConvertNameToFull(AnsiName,AnsiName);
    CharToWide(AnsiName,Dest);
  }
#endif
#else
  char AnsiName[NM];
  WideToChar(Src,AnsiName);
  ConvertNameToFull(AnsiName,AnsiName);
  CharToWide(AnsiName,Dest);
#endif
}
#endif


#ifndef SFX_MODULE
char *MkTemp(char *Name)
{
  int Length=strlen(Name);
  if (Length<=6)
    return(NULL);
  int Random=clock();
  for (int Attempt=0;;Attempt++)
  {
    sprintf(Name+Length-6,"%06u",Random+Attempt);
    Name[Length-4]='.';
    if (!FileExist(Name))
      break;
    if (Attempt==1000)
      return(NULL);
  }
  return(Name);
}
#endif




#ifndef SFX_MODULE
uint CalcFileCRC(File *SrcFile,Int64 Size)
{
  SaveFilePos SavePos(*SrcFile);
  const int BufSize=0x10000;
  Array<byte> Data(BufSize);
  Int64 BlockCount=0;
  uint DataCRC=0xffffffff;
  int ReadSize;


  SrcFile->Seek(0,SEEK_SET);
  while ((ReadSize=SrcFile->Read(&Data[0],int64to32(Size==INT64ERR ? Int64(BufSize):Min(Int64(BufSize),Size))))!=0)
  {
    ++BlockCount;
    if ((BlockCount & 15)==0)
    {
      Wait();
    }
    DataCRC=CRC(DataCRC,&Data[0],ReadSize);
    if (Size!=INT64ERR)
      Size-=ReadSize;
  }
  return(DataCRC^0xffffffff);
}
#endif


bool RenameFile(const char *SrcName,const wchar *SrcNameW,const char *DestName,const wchar *DestNameW)
{
  return(rename(SrcName,DestName)==0);
}


bool DelFile(const char *Name)
{
  return(DelFile(Name,NULL));
}


bool DelFile(const char *Name,const wchar *NameW)
{
  return(remove(Name)==0);
}


bool DelDir(const char *Name)
{
  return(DelDir(Name,NULL));
}


bool DelDir(const char *Name,const wchar *NameW)
{
  return(rmdir(Name)==0);
}
