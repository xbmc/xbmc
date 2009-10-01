#include "rar.hpp"

FindFile::FindFile()
{
  *FindMask=0;
  *FindMaskW=0;
  FirstCall=TRUE;
#ifdef _WIN_32
  hFind=INVALID_HANDLE_VALUE;
#else
  dirp=NULL;
#endif
}


FindFile::~FindFile()
{
#ifdef _WIN_32
  if (hFind!=INVALID_HANDLE_VALUE)
    FindClose(hFind);
#else
  if (dirp!=NULL)
    closedir(dirp);
#endif
}


void FindFile::SetMask(const char *FindMask)
{
  strcpy(FindFile::FindMask,FindMask);
  if (*FindMaskW==0)
    CharToWide(FindMask,FindMaskW);
  FirstCall=TRUE;
}


void FindFile::SetMaskW(const wchar *FindMaskW)
{
  if (FindMaskW==NULL)
    return;
  strcpyw(FindFile::FindMaskW,FindMaskW);
  if (*FindMask==0)
    WideToChar(FindMaskW,FindMask);
  FirstCall=TRUE;
}


bool FindFile::Next(struct FindData *fd,bool GetSymLink)
{
  fd->Error=false;
  if (*FindMask==0)
    return(false);
#ifdef _WIN_32
  if (FirstCall)
  {
    if ((hFind=Win32Find(INVALID_HANDLE_VALUE,FindMask,FindMaskW,fd))==INVALID_HANDLE_VALUE)
      return(false);
  }
  else
    if (Win32Find(hFind,FindMask,FindMaskW,fd)==INVALID_HANDLE_VALUE)
      return(false);
#else
  if (FirstCall)
  {
    char DirName[NM];
    strcpy(DirName,FindMask);
    RemoveNameFromPath(DirName);
    if (*DirName==0)
      strcpy(DirName,".");
/*
    else
    {
      int Length=strlen(DirName);
      if (Length>1 && DirName[Length-1]==CPATHDIVIDER && (Length!=3 || !IsDriveDiv(DirName[1])))
        DirName[Length-1]=0;
    }
*/
    if ((dirp=opendir(DirName))==NULL)
    {
      fd->Error=(errno!=ENOENT);
      return(false);
    }
  }
  while (1)
  {
    struct dirent *ent=readdir(dirp);
    if (ent==NULL)
      return(false);
    if (strcmp(ent->d_name,".")==0 || strcmp(ent->d_name,"..")==0)
      continue;
    if (CmpName(FindMask,ent->d_name,MATCH_NAMES))
    {
      char FullName[NM];
      strcpy(FullName,FindMask);
      strcpy(PointToName(FullName),ent->d_name);
      if (!FastFind(FullName,NULL,fd,GetSymLink))
      {
        ErrHandler.OpenErrorMsg(FullName);
        continue;
      }
      strcpy(fd->Name,FullName);
      break;
    }
  }
  *fd->NameW=0;
#ifdef _APPLE
  if (!LowAscii(fd->Name))
    UtfToWide(fd->Name,fd->NameW,sizeof(fd->NameW));
#elif defined(UNICODE_SUPPORTED)
  if (!LowAscii(fd->Name) && UnicodeEnabled())
    CharToWide(fd->Name,fd->NameW);
#endif
#endif
  fd->IsDir=IsDir(fd->FileAttr);
  FirstCall=FALSE;
  char *Name=PointToName(fd->Name);
  if (strcmp(Name,".")==0 || strcmp(Name,"..")==0)
    return(Next(fd));
  return(true);
}


bool FindFile::FastFind(const char *FindMask,const wchar *FindMaskW,struct FindData *fd,bool GetSymLink)
{
  fd->Error=false;
#ifndef _UNIX
  if (IsWildcard(FindMask,FindMaskW))
    return(false);
#endif    
#ifdef _WIN_32
  HANDLE hFind=Win32Find(INVALID_HANDLE_VALUE,FindMask,FindMaskW,fd);
  if (hFind==INVALID_HANDLE_VALUE)
    return(false);
  FindClose(hFind);
#else
  struct stat st;
  if (GetSymLink)
  {
#ifdef SAVE_LINKS
    if (lstat(FindMask,&st)!=0)
#else
    if (stat(FindMask,&st)!=0)
#endif
    {
      fd->Error=(errno!=ENOENT);
      return(false);
    }
  }
  else
    if (stat(FindMask,&st)!=0)
    {
      fd->Error=(errno!=ENOENT);
      return(false);
    }
#ifdef _DJGPP
  fd->FileAttr=chmod(FindMask,0);
#elif defined(_EMX)
  fd->FileAttr=st.st_attr;
#else
  fd->FileAttr=st.st_mode;
#endif
  fd->IsDir=IsDir(st.st_mode);
  fd->Size=st.st_size;
  fd->mtime=st.st_mtime;
  fd->atime=st.st_atime;
  fd->ctime=st.st_ctime;
  fd->FileTime=fd->mtime.GetDos();
  strcpy(fd->Name,FindMask);
  *fd->NameW=0;
#ifdef _APPLE
  if (!LowAscii(fd->Name))
    UtfToWide(fd->Name,fd->NameW,sizeof(fd->NameW));
#elif defined(UNICODE_SUPPORTED)
  if (!LowAscii(fd->Name) && UnicodeEnabled())
    CharToWide(fd->Name,fd->NameW);
#endif
#endif
  fd->IsDir=IsDir(fd->FileAttr);
  return(true);
}


#ifdef _WIN_32
HANDLE FindFile::Win32Find(HANDLE hFind,const char *Mask,const wchar *MaskW,struct FindData *fd)
{
#if !defined(_XBOX) && !defined(_LINUX)
#ifndef _WIN_CE
  if (WinNT())
#endif
  {
    wchar WideMask[NM];
    if (MaskW!=NULL && *MaskW!=0)
      strcpyw(WideMask,MaskW);
    else
      CharToWide(Mask,WideMask);

    WIN32_FIND_DATAW FindData;
    if (hFind==INVALID_HANDLE_VALUE)
    {
      hFind=FindFirstFileW(WideMask,&FindData);
      if (hFind==INVALID_HANDLE_VALUE)
      {
        int SysErr=GetLastError();
        fd->Error=(SysErr!=ERROR_FILE_NOT_FOUND &&
                   SysErr!=ERROR_PATH_NOT_FOUND &&
                   SysErr!=ERROR_NO_MORE_FILES);
      }
    }
    else
      if (!FindNextFileW(hFind,&FindData))
      {
        hFind=INVALID_HANDLE_VALUE;
        fd->Error=GetLastError()!=ERROR_NO_MORE_FILES;
      }

    if (hFind!=INVALID_HANDLE_VALUE)
    {
      strcpyw(fd->NameW,WideMask);
      strcpyw(PointToName(fd->NameW),FindData.cFileName);
      WideToChar(fd->NameW,fd->Name);
      fd->Size=int32to64(FindData.nFileSizeHigh,FindData.nFileSizeLow);
      fd->FileAttr=FindData.dwFileAttributes;
      WideToChar(FindData.cAlternateFileName,fd->ShortName);
      fd->ftCreationTime=FindData.ftCreationTime;
      fd->ftLastAccessTime=FindData.ftLastAccessTime;
      fd->ftLastWriteTime=FindData.ftLastWriteTime;
      fd->mtime=FindData.ftLastWriteTime;
      fd->ctime=FindData.ftCreationTime;
      fd->atime=FindData.ftLastAccessTime;
      fd->FileTime=fd->mtime.GetDos();

#ifndef _WIN_CE
      if (LowAscii(fd->NameW))
        *fd->NameW=0;
#endif
    }
  }
#ifndef _WIN_CE
  else
#endif //_XBOX
  {
    char CharMask[NM];
    if (Mask!=NULL && *Mask!=0)
      strcpy(CharMask,Mask);
    else
      WideToChar(MaskW,CharMask);

    WIN32_FIND_DATA FindData;
    if (hFind==INVALID_HANDLE_VALUE)
    {
      hFind=FindFirstFile(CharMask,&FindData);
      if (hFind==INVALID_HANDLE_VALUE)
      {
        int SysErr=GetLastError();
        fd->Error=SysErr!=ERROR_FILE_NOT_FOUND && SysErr!=ERROR_PATH_NOT_FOUND;
      }
    }
    else
      if (!FindNextFile(hFind,&FindData))
      {
        hFind=INVALID_HANDLE_VALUE;
        fd->Error=GetLastError()!=ERROR_NO_MORE_FILES;
      }

    if (hFind!=INVALID_HANDLE_VALUE)
    {
      strcpy(fd->Name,CharMask);
      strcpy(PointToName(fd->Name),FindData.cFileName);
      CharToWide(fd->Name,fd->NameW);
      fd->Size=int32to64(FindData.nFileSizeHigh,FindData.nFileSizeLow);
      fd->FileAttr=FindData.dwFileAttributes;
      strcpy(fd->ShortName,FindData.cAlternateFileName);
      fd->ftCreationTime=FindData.ftCreationTime;
      fd->ftLastAccessTime=FindData.ftLastAccessTime;
      fd->ftLastWriteTime=FindData.ftLastWriteTime;
      fd->mtime=FindData.ftLastWriteTime;
      fd->ctime=FindData.ftCreationTime;
      fd->atime=FindData.ftLastAccessTime;
      fd->FileTime=fd->mtime.GetDos();
      if (LowAscii(fd->Name))
        *fd->NameW=0;
    }
  }
#endif
  return(hFind);
}
#endif

