#include "rar.hpp"

// BE WARNED THIS FILE IS HEAVILY MODIFIED TO BE USED WITH XBMC

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "Util.h"
#include "utils/URIUtils.h"

//static File *CreatedFiles[32];
static int RemoveCreatedActive=0;

File::File()
  :  m_File(*(new XFILE::CFile()))

{
//  hFile=BAD_HANDLE;
  *FileName=0;
  *FileNameW=0;
  NewFile=false;
  LastWrite=false;
  HandleType=FILE_HANDLENORMAL;
  SkipClose=false;
  IgnoreReadErrors=false;
  ErrorType=FILE_SUCCESS;
  OpenShared=false;
  AllowDelete=true;
  CloseCount=0;
  AllowExceptions=true;
}


File::~File()
{
  /*if (hFile!=BAD_HANDLE && !SkipClose)
    if (NewFile)
      Delete();
    else
      Close();*/
  m_File.Close();
  delete &m_File;
}


void File::operator = (File &SrcFile)
{
  //hFile=SrcFile.hFile;
  m_File = SrcFile.m_File;
  strcpy(FileName,SrcFile.FileName);
  NewFile=SrcFile.NewFile;
  LastWrite=SrcFile.LastWrite;
  HandleType=SrcFile.HandleType;
  SrcFile.SkipClose=true;
}


bool File::Open(const char *Name,const wchar *NameW,bool OpenShared,bool Update)
{
 // Below commented code was left behind on spiffs request for possible later usage
 
  /*ErrorType=FILE_SUCCESS;
  FileHandle hNewFile;
  if (File::OpenShared)
    OpenShared=true;
#ifdef _WIN_32
  uint Access=GENERIC_READ;
  if (Update)
    Access|=GENERIC_WRITE;
  uint ShareMode=FILE_SHARE_READ;
  if (OpenShared)
    ShareMode|=FILE_SHARE_WRITE;
#ifndef _XBOX
  if (WinNT() && NameW!=NULL && *NameW!=0)
    hNewFile=CreateFileW(NameW,Access,ShareMode,NULL,OPEN_EXISTING,
                         FILE_FLAG_SEQUENTIAL_SCAN,NULL);
  else
#endif
    hNewFile=CreateFile(Name,Access,ShareMode,NULL,OPEN_EXISTING,
                        FILE_FLAG_SEQUENTIAL_SCAN,NULL);

  if (hNewFile==BAD_HANDLE && GetLastError()==ERROR_FILE_NOT_FOUND)
    ErrorType=FILE_NOTFOUND;
#else
  int flags=Update ? O_RDWR:O_RDONLY;
#ifdef O_BINARY
  flags|=O_BINARY;
#if defined(_AIX) && defined(_LARGE_FILE_API)
  flags|=O_LARGEFILE;
#endif
#endif
#if defined(_EMX) && !defined(_DJGPP)
  int sflags=OpenShared ? SH_DENYNO:SH_DENYWR;
  int handle=sopen(Name,flags,sflags);
#else
  int handle=open(Name,flags);
#ifdef LOCK_EX
  if (!OpenShared && Update && handle>=0 && flock(handle,LOCK_EX|LOCK_NB)==-1)
  {
    close(handle);
    return(false);
  }
#endif
#endif
  hNewFile=handle==-1 ? BAD_HANDLE:fdopen(handle,Update ? UPDATEBINARY:READBINARY);
  if (hNewFile==BAD_HANDLE && errno==ENOENT)
    ErrorType=FILE_NOTFOUND;
#endif
  NewFile=false;
  HandleType=FILE_HANDLENORMAL;
  SkipClose=false;
  bool success=hNewFile!=BAD_HANDLE;*/
  char _name[NM];
  if (NameW!=NULL)
    WideToUtf(NameW, _name, sizeof(_name));
  else
    strcpy(_name, Name);
  bool success;
  if (Update)
    success = m_File.OpenForWrite(_name);
  else
    success = m_File.Open(_name);
  if (success)
  {
//    hFile=hNewFile;
    if (NameW!=NULL)
      strcpyw(FileNameW,NameW);
    else
      *FileNameW=0;
    if (Name!=NULL)
      strcpy(FileName,Name);
    else
      WideToChar(NameW,FileName);
    //AddFileToList(hFile);
    AddFileToList();
  }
  return(success);
}


#if !defined(SHELL_EXT) && !defined(SFX_MODULE)
void File::TOpen(const char *Name,const wchar *NameW)
{
  if (!WOpen(Name,NameW))
    ErrHandler.Exit(OPEN_ERROR);
}
#endif


bool File::WOpen(const char *Name,const wchar *NameW)
{
  if (Open(Name,NameW))
    return(true);
  ErrHandler.OpenErrorMsg(Name);
  return(false);
}


bool File::Create(const char *Name,const wchar *NameW)
{
// Below commented code was left behind on spiffs request for possible later usage 
/*#ifdef _WIN_32
#ifndef _XBOX
  if (WinNT() && NameW!=NULL && *NameW!=0)
    hFile=CreateFileW(NameW,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,
                      CREATE_ALWAYS,0,NULL);
  else
#endif
    hFile=CreateFile(Name,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,
                     CREATE_ALWAYS,0,NULL);
#else
  hFile=fopen(Name,CREATEBINARY);
#endif*/
  char _name[NM];
  if (NameW!=NULL)
    WideToUtf(NameW, _name, sizeof(_name));
  else
    strcpy(_name, Name);
  std::string strPath = URIUtils::GetDirectory(_name);
  CUtil::CreateDirectoryEx(strPath);
  m_File.OpenForWrite(_name,true);
  NewFile=true;
  HandleType=FILE_HANDLENORMAL;
  SkipClose=false;
  if (NameW!=NULL)
    strcpyw(FileNameW,NameW);
  else
    *FileNameW=0;
  if (Name!=NULL)
    strcpy(FileName,Name);
  else
    WideToChar(NameW,FileName);
  //AddFileToList(hFile);
  AddFileToList();
  //return(hFile!=BAD_HANDLE);
  return true;
}


//void File::AddFileToList(FileHandle hFile)
void File::AddFileToList()
{
  //if (hFile!=BAD_HANDLE)
    //for (int I=0;I<sizeof(CreatedFiles)/sizeof(CreatedFiles[0]);I++)
    /*for (int I=0;I<32;I++)
      if (CreatedFiles[I]==NULL)
      {
        CreatedFiles[I]=this;
        break;
      }*/
}


#if !defined(SHELL_EXT) && !defined(SFX_MODULE)
void File::TCreate(const char *Name,const wchar *NameW)
{
  if (!WCreate(Name,NameW))
    ErrHandler.Exit(FATAL_ERROR);
}
#endif


bool File::WCreate(const char *Name,const wchar *NameW)
{
  if (Create(Name,NameW))
    return(true);
  ErrHandler.SetErrorCode(CREATE_ERROR);
  ErrHandler.CreateErrorMsg(Name);
  return(false);
}


bool File::Close()
{
  bool success=true;
  /*if (HandleType!=FILE_HANDLENORMAL)
    HandleType=FILE_HANDLENORMAL;
  else
    if (hFile!=BAD_HANDLE)
    {*/
      if (!SkipClose)
      {
#if defined(_WIN_32) || defined(TARGET_POSIX)
        //success=CloseHandle(hFile) != FALSE;
        m_File.Close();
#else
        success=fclose(hFile)!=EOF;
#endif
/*        if (success || !RemoveCreatedActive)
          //for (int I=0;I<sizeof(CreatedFiles)/sizeof(CreatedFiles[0]);I++)
          for (int I=0;I<32;I++)
            if (CreatedFiles[I]==this)
            {
              CreatedFiles[I]=NULL;
              break;
            }*/
      }
      //hFile=BAD_HANDLE;
      if (!success && AllowExceptions)
        ErrHandler.CloseError(FileName);
    //}
  CloseCount++;
  return(success);
  //return(true);
}
  

void File::Flush()
{
  m_File.Flush();
/*#ifdef _WIN_32
  FlushFileBuffers(hFile);
#else
  fflush(hFile);
#endif*/
}


bool File::Delete()
{
  /*if (HandleType!=FILE_HANDLENORMAL || !AllowDelete)
    return(false);
  if (hFile!=BAD_HANDLE)
    Close();
  return(DelFile(FileName,FileNameW));*/
  return m_File.Delete(FileName);
}


bool File::Rename(const char *NewName)
{
  bool success=strcmp(FileName,NewName)==0;
  if (!success)
    success=rename(FileName,NewName)==0;
  if (success)
  {
    strcpy(FileName,NewName);
    *FileNameW=0;
  }
  return(success);
}


void File::Write(const void *Data,int Size)
{
// Below commented code was left behind on spiffs request for possible later usage
  /*if (Size==0)
    return;
//#ifndef _WIN_CE
#if !defined(_WIN_CE) && !defined(_XBOX)
  if (HandleType!=FILE_HANDLENORMAL)
    switch(HandleType)
    {
      case FILE_HANDLESTD:
#ifdef _WIN_32
        hFile=GetStdHandle(STD_OUTPUT_HANDLE);
#else
        hFile=stdout;
#endif
        break;
      case FILE_HANDLEERR:
#ifdef _WIN_32
        hFile=GetStdHandle(STD_ERROR_HANDLE);
#else
        hFile=stderr;
#endif
        break;
    }
#endif*/
  while (1)
  {
    bool success = true;
#if defined(_WIN_32) || defined(TARGET_POSIX)
    DWORD Written=0;
    if (HandleType!=FILE_HANDLENORMAL)
    {
      const int MaxSize=0x4000;
      for (int I = 0; I < Size && success; I += MaxSize)
        success = m_File.Write((byte*)Data + I, Min(Size - I, MaxSize)) == Min(Size - I, MaxSize);
    }
    else
    {
      success = m_File.Write(Data, Size) == Size;
    }
#else
    success=fwrite(Data,1,Size,hFile)==Size && !ferror(hFile);
#endif
    if (!success && AllowExceptions && HandleType==FILE_HANDLENORMAL)
    {
#if defined(_WIN_32) && !defined(SFX_MODULE) && !defined(RARDLL)
      int ErrCode=GetLastError();
      Int64 FilePos=Tell();
      Int64 FreeSize=GetFreeDisk(FileName);
      SetLastError(ErrCode);
      if (FreeSize>Size && FilePos-Size<=0xffffffff && FilePos+Size>0xffffffff)
        ErrHandler.WriteErrorFAT(FileName);
#endif
      if (ErrHandler.AskRepeatWrite(FileName))
      {
#if !defined(_WIN_32) && !defined(TARGET_POSIX)
        clearerr(hFile);
#endif
      if (Written<(unsigned int)Size && Written>0)
          Seek(Tell()-Written,SEEK_SET);
        continue;
      }
      ErrHandler.WriteError(NULL,FileName);
    }
    break;
  }
  LastWrite=true;
}


int File::Read(void *Data,int Size)
{
  Int64 FilePos = 0;
  if (IgnoreReadErrors)
    FilePos=Tell();
  int ReadSize;
  while (true)
  {
    ReadSize=DirectRead(Data,Size);
    if (ReadSize==-1)
    {
      ErrorType=FILE_READERROR;
      if (AllowExceptions)
      {
        if (IgnoreReadErrors)
        {
          ReadSize=0;
          for (int I=0;I<Size;I+=512)
          {
            Seek(FilePos+I,SEEK_SET);
            int SizeToRead=Min(Size-I,512);
            int ReadCode=DirectRead(Data,SizeToRead);
            ReadSize+=(ReadCode==-1) ? 512:ReadCode;
          }
        }
        else
        {
          if (HandleType==FILE_HANDLENORMAL && ErrHandler.AskRepeatRead(FileName))
            continue;
          ErrHandler.ReadError(FileName);
        }
      }
    }
    break;
  }
  
  return(ReadSize);
}


int File::DirectRead(void *Data,int Size)
{
  int Read = 0;
  while (Size)
  {
    int nRead = m_File.Read(Data,Size);
    if (nRead <= 0)
      break;
    Read += nRead;
    Data = (void*)(((char*)Data)+nRead);
    Size -= nRead;
  }
  //if (Read == 0)
   // return -1;

  return Read;
#if 0
  #ifdef _WIN_32
  const int MaxDeviceRead=20000;
#endif
// Below commented code was left behind on spiffs request for possible later usage
 
//#ifndef _WIN_CE
/*#if !defined(_WIN_CE) && !defined(_XBOX)
  if (HandleType==FILE_HANDLESTD)
  {
#ifdef _WIN_32
    if (Size>MaxDeviceRead)
      Size=MaxDeviceRead;
    hFile=GetStdHandle(STD_INPUT_HANDLE);
#else
    hFile=stdin;
#endif
  }
#endif
#ifdef _WIN_32
  DWORD Read;
  //if (!ReadFile(hFile,Data,Size,&Read,NULL))
  Read = m_File.Read(Data,Size);
  if ((Read != Size) && (m_File.GetPosition() != m_File.GetLength()))
  {
    if (IsDevice() && Size>MaxDeviceRead)
      return(DirectRead(Data,MaxDeviceRead));
    if (HandleType==FILE_HANDLESTD && GetLastError()==ERROR_BROKEN_PIPE)
      return(0);
    return(-1);
  }
  return(Read);
#else
  if (LastWrite)
  {
    fflush(hFile);
    LastWrite=false;
  }
  clearerr(hFile);
  int ReadSize=fread(Data,1,Size,hFile);
  if (ferror(hFile))
    return(-1);
  return(ReadSize);
#endif*/
#endif
}


void File::Seek(Int64 Offset,int Method)
{
  if (!RawSeek(Offset,Method) && AllowExceptions)
    ErrHandler.SeekError(FileName);
}


bool File::RawSeek(Int64 Offset,int Method)
{
  /*if (hFile==BAD_HANDLE)
    return(true);*/
  /*if (!is64plus(Offset) && Method!=SEEK_SET)
  {
    Offset=(Method==SEEK_CUR ? Tell():FileLength())+Offset;
    Method=SEEK_SET;
  }*/
#if defined(_WIN_32) || defined(TARGET_POSIX)
  //LONG HighDist=int64to32(Offset>>32);
  //if (SetFilePointer(hFile,int64to32(Offset),&HighDist,Method)==0xffffffff &&
  if (Offset > FileLength())
    return false;

  if (m_File.Seek(Offset,Method) < 0)
  {
    return(false);
  }
#else
  LastWrite=false;
#ifdef _LARGEFILE_SOURCE
  if (fseeko(hFile,Offset,Method)!=0)
#else
  if (fseek(hFile,int64to32(Offset),Method)!=0)
#endif
    return(false);
#endif
  return(true);
}


Int64 File::Tell()
{
#if defined(_WIN_32) || defined(TARGET_POSIX)
  //LONG HighDist=0;
  //uint LowDist=SetFilePointer(hFile,0,&HighDist,FILE_CURRENT);
  //Int64 pos = m_File.GetPosition();
  return m_File.GetPosition();
  /*if (LowDist==0xffffffff && GetLastError()!=NO_ERROR)
    if (AllowExceptions)
      ErrHandler.SeekError(FileName);
    else
      return(-1);
  return(int32to64(HighDist,LowDist));*/
#else
#ifdef _LARGEFILE_SOURCE
  return(ftello(hFile));
#else
  return(ftell(hFile));
#endif
#endif
}


void File::Prealloc(Int64 Size)
{
#ifdef _WIN_32
  if (RawSeek(Size,SEEK_SET))
  {
    Truncate();
    Seek(0,SEEK_SET);
  }
#endif
}


byte File::GetByte()
{
  byte Byte=0;
  Read(&Byte,1);
  return(Byte);
}


void File::PutByte(byte Byte)
{
  Write(&Byte,1);
}


bool File::Truncate()
{
#ifdef _WIN_32
  //return(SetEndOfFile(hFile) != FALSE);
  return true;
#else
  return(false);
#endif
}


void File::SetOpenFileTime(RarTime *ftm,RarTime *ftc,RarTime *fta)
{
#ifdef _WIN_32
// Below commented code was left behind on spiffs request for possible later usage
 
  /*bool sm=ftm!=NULL && ftm->IsSet();
  bool sc=ftc!=NULL && ftc->IsSet();
  bool sa=fta!=NULL && fta->IsSet();
  FILETIME fm,fc,fa;
  if (sm)
    ftm->GetWin32(&fm);
  if (sc)
    ftc->GetWin32(&fc);
  if (sa)
    fta->GetWin32(&fa);
  //SetFileTime(hFile,sc ? &fc:NULL,sa ? &fa:NULL,sm ? &fm:NULL);*/
#endif
}


void File::SetCloseFileTime(RarTime *ftm,RarTime *fta)
{
#if defined(_UNIX) || defined(_EMX)
  SetCloseFileTimeByName(FileName,ftm,fta);
#endif
}


void File::SetCloseFileTimeByName(const char *Name,RarTime *ftm,RarTime *fta)
{
#if defined(_UNIX) || defined(_EMX)
  bool setm=ftm!=NULL && ftm->IsSet();
  bool seta=fta!=NULL && fta->IsSet();
  if (setm || seta)
  {
    struct utimbuf ut;
    if (setm)
      ut.modtime=ftm->GetUnix();
    else
      ut.modtime=fta->GetUnix();
    if (seta)
      ut.actime=fta->GetUnix();
    else
      ut.actime=ut.modtime;
    utime(Name,&ut);
  }
#endif
}


void File::GetOpenFileTime(RarTime *ft)
{
#if defined(_WIN_32) || defined(TARGET_POSIX)
/*  FILETIME FileTime;
  GetFileTime(hFile,NULL,NULL,&FileTime);
  *ft=FileTime;*/
#endif
/*
#if defined(_UNIX) || defined(_EMX)
  struct stat st;
  fstat(fileno(hFile),&st);
  *ft=st.st_mtime;
#endif
*/
}


void File::SetOpenFileStat(RarTime *ftm,RarTime *ftc,RarTime *fta)
{
#ifdef _WIN_32
  //SetOpenFileTime(ftm,ftc,fta);
#endif
}


void File::SetCloseFileStat(RarTime *ftm,RarTime *fta,uint FileAttr)
{
#ifdef _WIN_32
  //SetFileAttr(FileName,FileNameW,FileAttr);
#endif
#ifdef _EMX
  SetCloseFileTime(ftm,fta);
  SetFileAttr(FileName,FileNameW,FileAttr);
#endif
#ifdef _UNIX
  SetCloseFileTime(ftm,fta);
  chmod(FileName,(mode_t)FileAttr);
#endif
}


Int64 File::FileLength()
{
  return (m_File.GetLength());
}


void File::SetHandleType(FILE_HANDLETYPE Type)
{
  HandleType=Type;
}


bool File::IsDevice()
{
  /*if (hFile==BAD_HANDLE)
    return(false);*/
#if defined(_XBOX) || defined(TARGET_POSIX) || defined(_XBMC)
  return false;
//#ifdef _WIN_32
#elif defined(_WIN_32)
  uint Type=GetFileType(hFile);
  return(Type==FILE_TYPE_CHAR || Type==FILE_TYPE_PIPE);
#else
  return(isatty(fileno(hFile)));
#endif
}


#ifndef SFX_MODULE
void File::fprintf(const char *fmt,...)
{
  va_list argptr;
  va_start(argptr,fmt);
  safebuf char Msg[2*NM+1024],OutMsg[2*NM+1024];
  vsprintf(Msg,fmt,argptr);
#ifdef _WIN_32
  for (int Src=0,Dest=0;;Src++)
  {
    char CurChar=Msg[Src];
    if (CurChar=='\n')
      OutMsg[Dest++]='\r';
    OutMsg[Dest++]=CurChar;
    if (CurChar==0)
      break;
  }
#else
  strcpy(OutMsg,Msg);
#endif
  Write(OutMsg,strlen(OutMsg));
  va_end(argptr);
}
#endif


bool File::RemoveCreated()
{
  RemoveCreatedActive++;
  bool RetCode=true;
  //for (int I=0;I<sizeof(CreatedFiles)/sizeof(CreatedFiles[0]);I++)
  /*for (int I=0;I<32;I++)
    if (CreatedFiles[I]!=NULL)
    {
      CreatedFiles[I]->SetExceptions(false);
      bool success;
      if (CreatedFiles[I]->NewFile)
        success=CreatedFiles[I]->Delete();
      else
        success=CreatedFiles[I]->Close();
      if (success)
        CreatedFiles[I]=NULL;
      else
        RetCode=false;
    }
  RemoveCreatedActive--;*/
  return(RetCode);
}


#ifndef SFX_MODULE
long File::Copy(File &Dest,Int64 Length)
{
  Array<char> Buffer(0x10000);
  long CopySize=0;
  bool CopyAll=(Length==INT64ERR);

  while (CopyAll || Length>0)
  {
    Wait();
    int SizeToRead=(!CopyAll && Length<Buffer.Size()) ? int64to32(Length):Buffer.Size();
    int ReadSize=Read(&Buffer[0],SizeToRead);
    if (ReadSize==0)
      break;
    Dest.Write(&Buffer[0],ReadSize);
    CopySize+=ReadSize;
    if (!CopyAll)
      Length-=ReadSize;
  }
  return(CopySize);
}
#endif
