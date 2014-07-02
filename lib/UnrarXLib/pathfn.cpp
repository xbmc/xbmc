// THIS FILE IS MODIFIED TO WORK WITH XBMC

#include "rar.hpp"

char* PointToName(const char *Path)
{
  //const char *Found=NULL;
  for (const char *s=&Path[strlen(Path)-1];s>=Path;s--)
    if (IsPathDiv(*s))
      return (char*)(s+1);
//  if (Found!=NULL)
  //  return((char*)Found);
  return (char*)((*Path && IsDriveDiv(Path[1]) && charnext(Path)==Path+1) ? Path+2:Path);
}

/*
Dumbass broken routine!!
Not searching in reverse to strip off filename portion of path!
{
  const char *Found=NULL;
  for (const char *s=Path;*s!=0;s=charnext(s))
    if (IsPathDiv(*s))
      Found=(char*)(s+1);
  if (Found!=NULL)
    return((char*)Found);
  return (char*)((*Path && IsDriveDiv(Path[1]) && charnext(Path)==Path+1) ? Path+2:Path);
}
*/

wchar* PointToName(const wchar *Path)
{
  for (int I=strlenw(Path)-1;I>=0;I--)
    if (IsPathDiv(Path[I]))
      return (wchar*)&Path[I+1];
  return (wchar*)((*Path && IsDriveDiv(Path[1])) ? Path+2:Path);
}


char* PointToLastChar(const char *Path)
{
  for (const char *s=Path,*p=Path;;p=s,s=charnext(s))
    if (*s==0)
      return((char *)p);
}


char* ConvertPath(const char *SrcPath,char *DestPath)
{
  const char *DestPtr=SrcPath;
  for (const char *s=DestPtr;*s!=0;s++)
    if (IsPathDiv(s[0]) && s[1]=='.' && s[2]=='.' && IsPathDiv(s[3]))
      DestPtr=s+4;
  while (*DestPtr)
  {
    const char *s=DestPtr;
    if (s[0] && IsDriveDiv(s[1]))
      s+=2;
    if (s[0]=='\\' && s[1]=='\\')
    {
      const char *Slash=strchr(s+2,'\\');
      if (Slash!=NULL && (Slash=strchr(Slash+1,'\\'))!=NULL)
        s=Slash+1;
    }
    for (const char *t=s;*t!=0;t++)
      if (IsPathDiv(*t))
        s=t+1;
      else
        if (*t!='.')
          break;
    if (s==DestPtr)
      break;
    DestPtr=s;
  }
  if (DestPath!=NULL)
  {
    char TmpStr[NM];
    strncpy(TmpStr,DestPtr,sizeof(TmpStr)-1);
    strcpy(DestPath,TmpStr);
  }
  return((char *)DestPtr);
}


wchar* ConvertPath(const wchar *SrcPath,wchar *DestPath)
{
  const wchar *DestPtr=SrcPath;
  for (const wchar *s=DestPtr;*s!=0;s++)
    if (IsPathDiv(s[0]) && s[1]=='.' && s[2]=='.' && IsPathDiv(s[3]))
      DestPtr=s+4;
  while (*DestPtr)
  {
    const wchar *s=DestPtr;
    if (s[0] && IsDriveDiv(s[1]))
      s+=2;
    if (s[0]=='\\' && s[1]=='\\')
    {
      const wchar *Slash=strchrw(s+2,'\\');
      if (Slash!=NULL && (Slash=strchrw(Slash+1,'\\'))!=NULL)
        s=Slash+1;
    }
    for (const wchar *t=s;*t!=0;t++)
      if (IsPathDiv(*t))
        s=t+1;
      else
        if (*t!='.')
          break;
    if (s==DestPtr)
      break;
    DestPtr=s;
  }
  if (DestPath!=NULL)
  {
    wchar TmpStr[NM];
    strncpyw(TmpStr,DestPtr,sizeof(TmpStr)/sizeof(TmpStr[0])-1);
    strcpyw(DestPath,TmpStr);
  }
  return((wchar *)DestPtr);
}


void SetExt(char *Name,const char *NewExt)
{
  char *Dot=GetExt(Name);
  if (NewExt==NULL)
  {
    if (Dot!=NULL)
      *Dot=0;
  }
  else
    if (Dot==NULL)
    {
      strcat(Name,".");
      strcat(Name,NewExt);
    }
    else
      strcpy(Dot+1,NewExt);
}


#ifndef SFX_MODULE
void SetExt(wchar *Name,const wchar *NewExt)
{
  if (Name==NULL || *Name==0)
    return;
  wchar *Dot=GetExt(Name);
  if (NewExt==NULL)
  {
    if (Dot!=NULL)
      *Dot=0;
  }
  else
    if (Dot==NULL)
    {
      strcatw(Name,L".");
      strcatw(Name,NewExt);
    }
    else
      strcpyw(Dot+1,NewExt);
}
#endif


#ifndef SFX_MODULE
void SetSFXExt(char *SFXName)
{
#ifdef _UNIX
  SetExt(SFXName,"sfx");
#endif

#if defined(_WIN_32) || defined(_EMX)
  SetExt(SFXName,"exe");
#endif
}
#endif


#ifndef SFX_MODULE
void SetSFXExt(wchar *SFXName)
{
  if (SFXName==NULL || *SFXName==0)
    return;

#ifdef _UNIX
  SetExt(SFXName,L"sfx");
#endif

#if defined(_WIN_32) || defined(_EMX)
  SetExt(SFXName,L"exe");
#endif
}
#endif


char *GetExt(const char *Name)
{
  if (Name)
    return (char*)strrchr(Name, '.');
  return NULL;
}

wchar *GetExt(const wchar *Name)
{
  if (Name)
    return strrchrw(Name, L'.');
  return NULL;
}


bool CmpExt(const char *Name,const char *Ext)
{
  char *NameExt=GetExt(Name);
  return(NameExt!=NULL && stricomp(NameExt+1,Ext)==0);
}


bool IsWildcard(const char *Str,const wchar *StrW)
{
  if (StrW!=NULL && *StrW!=0)
    return(strpbrkw(StrW,L"*?")!=NULL);
  return(Str==NULL ? false:strpbrk(Str,"*?")!=NULL);
}


bool IsPathDiv(int Ch)
{
#if defined(_WIN_32) || defined(_EMX)
  return(Ch=='\\' || Ch=='/');
#else
  return(Ch==CPATHDIVIDER);
#endif
}


bool IsDriveDiv(int Ch)
{
#ifdef _UNIX
  return(false);
#else
  return(Ch==':');
#endif
}


int GetPathDisk(const char *Path)
{
  if (IsDiskLetter(Path))
    return(toupper(*Path)-'A');
  else
    return(-1);
}


void AddEndSlash(char *Path)
{
  char *LastChar=PointToLastChar(Path);
  if (*LastChar!=0 && *LastChar!=CPATHDIVIDER)
    strcat(LastChar,PATHDIVIDER);
}


void AddEndSlash(wchar *Path)
{
  int Length=strlenw(Path);
  if (Length>0 && Path[Length-1]!=CPATHDIVIDER)
    strcatw(Path,PATHDIVIDERW);
}


void GetFilePath(const char *FullName,char *Path)
{
  int PathLength=PointToName(FullName)-FullName;
  strncpy(Path,FullName,PathLength);
  Path[PathLength]=0;
}


void GetFilePath(const wchar *FullName,wchar *Path)
{
  const wchar *PathPtr=/*(*FullName && IsDriveDiv(FullName[1])) ? FullName+2:*/FullName;
  int PathLength=PointToName(FullName)-FullName;
  strncpyw(Path,PathPtr,PathLength);
  Path[PathLength]=0;
}


void RemoveNameFromPath(char *Path)
{
  char *Name=PointToName(Path);
  if (Name>=Path+2 && (!IsDriveDiv(Path[1]) || Name>=Path+4))
    Name--;
  *Name=0;
}


#ifndef SFX_MODULE
void RemoveNameFromPath(wchar *Path)
{
  wchar *Name=PointToName(Path);
  if (Name>=Path+2 && (!IsDriveDiv(Path[1]) || Name>=Path+4))
    Name--;
  *Name=0;
}
#endif


#ifndef SFX_MODULE
bool EnumConfigPaths(char *Path,int Number)
{
#ifdef _EMX
  static char RARFileName[NM];
  if (Number==-1)
    strcpy(RARFileName,Path);
  if (Number!=0)
    return(false);
  if (_osmode==OS2_MODE)
  {
    PTIB ptib;
    PPIB ppib;
    DosGetInfoBlocks(&ptib, &ppib);
    DosQueryModuleName(ppib->pib_hmte,NM,Path);
  }
  else
    strcpy(Path,RARFileName);
  RemoveNameFromPath(Path);
  return(true);
#elif defined(_UNIX)
  if (Number==0)
  {
    char *EnvStr=getenv("HOME");
    if (EnvStr==NULL)
      return(false);
    strncpy(Path,EnvStr,NM);
    Path[NM-1]=0;
    return(true);
  }
  static const char *AltPath[]={
    "/etc","/usr/lib","/usr/local/lib","/usr/local/etc"
  };
  Number--;
  if (Number<0 || Number>=(int)(sizeof(AltPath)/sizeof(AltPath[0])))
    return(false);
  strcpy(Path,AltPath[Number]);
  return(true);
#elif defined(_WIN_32)
  if (Number!=0)
    return(false);
#if !defined(TARGET_POSIX)
  GetModuleFileName(NULL,Path,NM);
  RemoveNameFromPath(Path);
#endif
  return(true);
#else
  return(false);
#endif
}
#endif


#ifndef SFX_MODULE
void GetConfigName(const char *Name,char *FullName, bool CheckExist)
{
  for (int I=0;EnumConfigPaths(FullName,I);I++)
  {
    AddEndSlash(FullName);
    strcat(FullName,Name);
    if (!CheckExist || WildFileExist(FullName))
      break;
  }
}
#endif


char* GetVolNumPart(char *ArcName)
{
  char *ChPtr=ArcName+strlen(ArcName)-1;
  while (!isdigit(*ChPtr) && ChPtr>ArcName)
    ChPtr--;
  char *NumPtr=ChPtr;
  while (isdigit(*NumPtr) && NumPtr>ArcName)
    NumPtr--;
  while (NumPtr>ArcName && *NumPtr!='.')
  {
    if (isdigit(*NumPtr))
    {
      char *Dot=strchrd(PointToName(ArcName),'.');
      if (Dot!=NULL && Dot<NumPtr)
        ChPtr=NumPtr;
      break;
    }
    NumPtr--;
  }
  return(ChPtr);
}


void NextVolumeName(char *ArcName,bool OldNumbering)
{
  char *ChPtr;
  if ((ChPtr=GetExt(ArcName))==NULL)
  {
    strcat(ArcName,".rar");
    ChPtr=GetExt(ArcName);
  }
  else
    if (ChPtr[1]==0 || stricomp(ChPtr+1,"exe")==0 || stricomp(ChPtr+1,"sfx")==0)
      strcpy(ChPtr+1,"rar");
  if (!OldNumbering)
  {
    ChPtr=GetVolNumPart(ArcName);

    while ((++(*ChPtr))=='9'+1)
    {
      *ChPtr='0';
      ChPtr--;
      if (ChPtr<ArcName || !isdigit(*ChPtr))
      {
        for (char *EndPtr=ArcName+strlen(ArcName);EndPtr!=ChPtr;EndPtr--)
          *(EndPtr+1)=*EndPtr;
        *(ChPtr+1)='1';
        break;
      }
    }
  }
  else
    if (!isdigit(*(ChPtr+2)) || !isdigit(*(ChPtr+3)))
      strcpy(ChPtr+2,"00");
    else
    {
      ChPtr+=3;
      while ((++(*ChPtr))=='9'+1)
        if (*(ChPtr-1)=='.')
        {
          *ChPtr='A';
          break;
        }
        else
        {
          *ChPtr='0';
          ChPtr--;
        }
    }
}


bool IsNameUsable(const char *Name)
{
  // only for xbox
  if ( Name == NULL) return false;
  char cIllegalChars[] = "<>=?;\"*+,/|";
  unsigned int iIllegalCharSize = strlen(cIllegalChars);
  bool isIllegalChar;
  unsigned int iSize = strlen(Name);
  if(iSize > 42) return false;
  
  for (unsigned int i = 0; i < iSize; i++)
  {
    isIllegalChar = false;
    // check for illegal chars
    for (unsigned j = 0; j < iIllegalCharSize; j++)
      if (Name[i] == cIllegalChars[j]) isIllegalChar = true;
    // FATX only allows chars from 32 till 127
    if (isIllegalChar == true || Name[i] < 32 || Name[i] > 126) return false;
  }
  return true;
  /*
#ifndef _UNIX
  if (Name[0] && Name[1] && strchr(Name+2,':')!=NULL)
    return(false);
#endif
  if (strlen(Name) < 43) return 1;
  else return 0;
  return(*Name!=0 && strpbrk(Name,"?*<>|")==NULL);
  */
}


void MakeNameUsable(char *Name, bool bKeepExtension, bool IsFATX)
{
  // Changed to be compatible with xbmc's MakeLegalFileName function
  // (xbox only)

  if ( Name == NULL) return;
  char cIllegalChars[] = "<>=?;\"*+,/|";
  unsigned int iIllegalCharSize = strlen(cIllegalChars);
  bool isIllegalChar;
  unsigned int iSize = strlen(Name);
  unsigned int iNewStringSize = 0;
  char* strNewString = new char[iSize + 1];

  // only copy the legal characters to the new filename
  for (unsigned int i = 0; i < iSize; i++)
  {
    isIllegalChar = false;
    // check for illigal chars
    for (unsigned j = 0; j < iIllegalCharSize; j++)
      if (Name[i] == cIllegalChars[j]) isIllegalChar = true;
    // FATX only allows chars from 32 till 127
    if (isIllegalChar == false &&
        Name[i] > 31 && Name[i] < 127) strNewString[iNewStringSize++] = Name[i];
  }
  strNewString[iNewStringSize] = '\0';

  if (IsFATX)
  {
    // since we can only write to samba shares and hd, we assume this has to be a fatx filename
    // thus we have to strip it down to 42 chars (samba doesn't have this limitation)
  
    char* FileName = PointToName(strNewString);
    int iFileNameSize = strlen(FileName);
    // no need to keep the extension, just strip it down to 42 characters
    if (iFileNameSize > 42 && bKeepExtension == false) FileName[42] = '\0';

    // we want to keep the extension
    else if (iFileNameSize > 42 && bKeepExtension == true)
    {
      char strExtension[42];
      unsigned int iExtensionLength = iFileNameSize - (strrchr(FileName, '.') - FileName);
      strcpy(strExtension, (FileName + iFileNameSize - iExtensionLength));

      strcpy(FileName + (42 - iExtensionLength), strExtension);
    }
  }

  strcpy(Name, strNewString);
  delete[] strNewString;
}

char* UnixSlashToDos(char *SrcName,char *DestName,uint MaxLength)
{
  if (DestName!=NULL && DestName!=SrcName)
    strcpy(DestName,SrcName);
  for (char *s=SrcName;*s!=0;s=charnext(s))
  {
    if (*s=='/')
    {
      if (DestName==NULL)
        *s='\\';
      else
        DestName[s-SrcName]='\\';
    }
  }
  return(DestName==NULL ? SrcName:DestName);
}


char* DosSlashToUnix(char *SrcName,char *DestName,uint MaxLength)
{
  if (DestName!=NULL && DestName!=SrcName)
  {
    if (strlen(SrcName)>=MaxLength)
    {
      *DestName=0;
      return(DestName);
    }
    else
      strcpy(DestName,SrcName);
  }
  for (char *s=SrcName;*s!=0;s=charnext(s))
  {
    if (*s=='\\')
    {
      if (DestName==NULL)
        *s='/';
      else
        DestName[s-SrcName]='/';
    }
  }
  return(DestName==NULL ? SrcName:DestName);
}


bool IsFullPath(const char *Path)
{
  char PathOnly[NM];
  GetFilePath(Path,PathOnly);
  if (IsWildcard(PathOnly))
    return(true);
#if defined(_WIN_32) || defined(_EMX)
  return(Path[0]=='\\' && Path[1]=='\\' ||
         IsDiskLetter(Path) && IsPathDiv(Path[2]));
#else
  return(IsPathDiv(Path[0]));
#endif
}


bool IsDiskLetter(const char *Path)
{
  char Letter=toupper(Path[0]);
  return(Letter>='A' && Letter<='Z' && IsDriveDiv(Path[1]));
}


void GetPathRoot(const char *Path,char *Root)
{
  *Root=0;
  if (IsDiskLetter(Path))
    sprintf(Root,"%c:\\",*Path);
  else
    if (Path[0]=='\\' && Path[1]=='\\')
    {
      const char *Slash=strchr(Path+2,'\\');
      if (Slash!=NULL)
      {
        int Length;
        if ((Slash=strchr(Slash+1,'\\'))!=NULL)
          Length=Slash-Path+1;
        else
          Length=strlen(Path);
        strncpy(Root,Path,Length);
        Root[Length]=0;
      }
    }
}


int ParseVersionFileName(char *Name,wchar *NameW,bool Truncate)
{
  int Version=0;
  char *VerText=strrchrd(Name,';');
  if (VerText!=NULL)
  {
    Version=atoi(VerText+1);
    if (Truncate)
      *VerText=0;
  }
  if (NameW!=NULL)
  {
    wchar *VerTextW=strrchrw(NameW,';');
    if (VerTextW!=NULL)
    {
      if (Version==0)
        Version=atoiw(VerTextW+1);
      if (Truncate)
        *VerTextW=0;
    }
  }
  return(Version);
}


#ifndef SFX_MODULE
char* VolNameToFirstName(const char *VolName,char *FirstName,bool NewNumbering)
{
  if (FirstName!=VolName)
    strcpy(FirstName,VolName);
  char *VolNumStart=FirstName;
  if (NewNumbering)
  {
    int N='1';
    for (char *ChPtr=GetVolNumPart(FirstName);ChPtr>FirstName;ChPtr--)
      if (isdigit(*ChPtr))
      {
        *ChPtr=N;
        N='0';
      }
      else
        if (N=='0')
        {
          VolNumStart=ChPtr+1;
          break;
        }
  }
  else
  {
    SetExt(FirstName,"rar");
    VolNumStart=GetExt(FirstName);
  }
  if (!FileExist(FirstName))
  {
    char Mask[NM];
    strcpy(Mask,FirstName);
    SetExt(Mask,"*");
    FindFile Find;
    Find.SetMask(Mask);
    struct FindData FD;
    while (Find.Next(&FD))
    {
      Archive Arc;
      if (Arc.Open(FD.Name,FD.NameW) && Arc.IsArchive(true) && !Arc.NotFirstVolume)
      {
        strcpy(FirstName,FD.Name);
        break;
      }
    }
  }
  return(VolNumStart);
}
#endif




wchar* GetWideName(const char *Name,const wchar *NameW,wchar *DestW)
{
  if (NameW!=NULL && *NameW!=0)
  {
    if (DestW!=NameW)
      strcpyw(DestW,NameW);
  }
  else
    CharToWide(Name,DestW);
  return(DestW);
}





