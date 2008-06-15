#include "rar.hpp"

ScanTree::ScanTree(StringList *FileMasks,int Recurse,bool GetLinks,int GetDirs)
{
  ScanTree::FileMasks=FileMasks;
  ScanTree::Recurse=Recurse;
  ScanTree::GetLinks=GetLinks;
  ScanTree::GetDirs=GetDirs;

  SetAllMaskDepth=0;
  *CurMask=0;
  *CurMaskW=0;
  memset(FindStack,0,sizeof(FindStack));
  Depth=0;
  Errors=0;
  FastFindFile=false;
  *ErrArcName=0;
  Cmd=NULL;
}


ScanTree::~ScanTree()
{
  for (int I=Depth;I>=0;I--)
    if (FindStack[I]!=NULL)
      delete FindStack[I];
}


int ScanTree::GetNext(FindData *FindData)
{
  if (Depth<0)
    return(SCAN_DONE);

  int FindCode;
  while (1)
  {
    if ((*CurMask==0 || (FastFindFile && Depth==0)) && !PrepareMasks())
      return(SCAN_DONE);
    FindCode=FindProc(FindData);
    if (FindCode==SCAN_ERROR)
    {
      Errors++;
      continue;
    }
    if (FindCode==SCAN_NEXT)
      continue;
    if (FindCode==SCAN_SUCCESS && FindData->IsDir && GetDirs==SCAN_SKIPDIRS)
      continue;
    if (FindCode==SCAN_DONE && PrepareMasks())
      continue;
    break;
  }
  return(FindCode);
}


bool ScanTree::PrepareMasks()
{
  if (!FileMasks->GetString(CurMask,CurMaskW,sizeof(CurMask)))
    return(false);
#ifdef _WIN_32
  UnixSlashToDos(CurMask);
#endif
  char *Name=PointToName(CurMask);
  if (*Name==0)
    strcat(CurMask,MASKALL);
  if (Name[0]=='.' && (Name[1]==0 || (Name[1]=='.' && Name[2]==0)))
  {
    AddEndSlash(CurMask);
    strcat(CurMask,MASKALL);
  }
  SpecPathLength=Name-CurMask;
//  if (SpecPathLength>1)
//    SpecPathLength--;

  bool WideName=(*CurMaskW!=0);

  if (WideName)
  {
    wchar *NameW=PointToName(CurMaskW);
    if (*NameW==0)
      strcatw(CurMaskW,MASKALLW);
    if (NameW[0]=='.' && (NameW[1]==0 || (NameW[1]=='.' && NameW[2]==0)))
    {
      AddEndSlash(CurMaskW);
      strcatw(CurMaskW,MASKALLW);
    }
    SpecPathLengthW=NameW-CurMaskW;
  }
  else
  {
    wchar WideMask[NM];
    CharToWide(CurMask,WideMask);
    SpecPathLengthW=PointToName(WideMask)-WideMask;
  }
  Depth=0;

  strcpy(OrigCurMask,CurMask);
  strcpyw(OrigCurMaskW,CurMaskW);

  return(true);
}


int ScanTree::FindProc(FindData *FindData)
{
  if (*CurMask==0)
    return(SCAN_NEXT);
  FastFindFile=false;
  if (FindStack[Depth]==NULL)
  {
    bool Wildcards=IsWildcard(CurMask,CurMaskW);
    bool FindCode=!Wildcards && FindFile::FastFind(CurMask,CurMaskW,FindData,GetLinks);
    bool IsDir=FindCode && FindData->IsDir;
    bool SearchAll=!IsDir && (Depth>0 || Recurse==RECURSE_ALWAYS || (Wildcards && Recurse==RECURSE_WILDCARDS));
    if (Depth==0)
      SearchAllInRoot=SearchAll;
    if (SearchAll || Wildcards)
    {
      FindStack[Depth]=new FindFile;
      char SearchMask[NM];
      strcpy(SearchMask,CurMask);
      if (SearchAll)
        strcpy(PointToName(SearchMask),MASKALL);
      FindStack[Depth]->SetMask(SearchMask);
      if (*CurMaskW)
      {
        wchar SearchMaskW[NM];
        strcpyw(SearchMaskW,CurMaskW);
        if (SearchAll)
          strcpyw(PointToName(SearchMaskW),MASKALLW);
        FindStack[Depth]->SetMaskW(SearchMaskW);
      }
    }
    else
    {
      FastFindFile=true;
      if (!FindCode)
      {
        if (Cmd!=NULL && Cmd->ExclCheck(CurMask,true))
          return(SCAN_NEXT);
        ErrHandler.OpenErrorMsg(ErrArcName,CurMask);
        return(FindData->Error ? SCAN_ERROR:SCAN_NEXT);
      }
    }
  }

  if (!FastFindFile && !FindStack[Depth]->Next(FindData,GetLinks))
  {
    bool Error=FindData->Error;

#ifdef _WIN_32
    if (Error && strstr(CurMask,"System Volume Information\\")!=NULL)
      Error=false;
#endif

    if (Cmd!=NULL && Cmd->ExclCheck(CurMask,true))
      Error=false;

#ifndef SILENT
    if (Error)
    {
      Log(NULL,St(MScanError),CurMask);
    }
#endif

    char DirName[NM];
    wchar DirNameW[NM];
    *DirName=0;
    *DirNameW=0;

    delete FindStack[Depth];
    FindStack[Depth--]=NULL;
    while (Depth>=0 && FindStack[Depth]==NULL)
      Depth--;
    if (Depth < 0)
    {
      if (Error)
        Errors++;
      return(SCAN_DONE);
    }
    char *Slash=strrchrd(CurMask,CPATHDIVIDER);
    if (Slash!=NULL)
    {
      char Mask[NM];
      strcpy(Mask,Slash);
      if (Depth<SetAllMaskDepth)
        strcpy(Mask+1,PointToName(OrigCurMask));
      *Slash=0;
      strcpy(DirName,CurMask);
      char *PrevSlash=strrchrd(CurMask,CPATHDIVIDER);
      if (PrevSlash==NULL)
        strcpy(CurMask,Mask+1);
      else
        strcpy(PrevSlash,Mask);
    }

    if (*CurMaskW!=0)
    {
      wchar *Slash=strrchrw(CurMaskW,CPATHDIVIDER);
      if (Slash!=NULL)
      {
        wchar Mask[NM];
        strcpyw(Mask,Slash);
        *Slash=0;
        strcpyw(DirNameW,CurMaskW);
        wchar *PrevSlash=strrchrw(CurMaskW,CPATHDIVIDER);
        if (PrevSlash==NULL)
          strcpyw(CurMaskW,Mask+1);
        else
          strcpyw(PrevSlash,Mask);
      }
#ifndef _WIN_CE
      if (LowAscii(CurMaskW))
        *CurMaskW=0;
#endif
    }
    if (GetDirs==SCAN_GETDIRSTWICE &&
        FindFile::FastFind(DirName,DirNameW,FindData,GetLinks) && FindData->IsDir)
      return(Error ? SCAN_ERROR:SCAN_SUCCESS);
    return(Error ? SCAN_ERROR:SCAN_NEXT);
  }

  if (FindData->IsDir)
  {
    if (!FastFindFile && Depth==0 && !SearchAllInRoot)
      return(GetDirs==SCAN_GETCURDIRS ? SCAN_SUCCESS:SCAN_NEXT);

//    if (GetDirs==SCAN_GETCURDIRS && Depth==0 && !SearchAllInRoot)
//      return(SCAN_SUCCESS);

    char Mask[NM];
    bool MaskAll=FastFindFile;

    strcpy(Mask,MaskAll ? MASKALL:PointToName(CurMask));
    strcpy(CurMask,FindData->Name);

    if (strlen(CurMask)+strlen(Mask)+1>=NM || Depth>=MAXSCANDEPTH-1)
    {
#ifndef SILENT
      Log(NULL,"\n%s%c%s",CurMask,CPATHDIVIDER,Mask);
      Log(NULL,St(MPathTooLong));
#endif
      return(SCAN_ERROR);
    }

    AddEndSlash(CurMask);
    strcat(CurMask,Mask);

    if (*CurMaskW && *FindData->NameW==0)
      CharToWide(FindData->Name,FindData->NameW);
    if (*FindData->NameW!=0)
    {
      wchar Mask[NM];
      if (FastFindFile)
        strcpyw(Mask,MASKALLW);
      else
        if (*CurMaskW)
          strcpyw(Mask,PointToName(CurMaskW));
        else
          CharToWide(PointToName(CurMask),Mask);
      strcpyw(CurMaskW,FindData->NameW);
      AddEndSlash(CurMaskW);
      strcatw(CurMaskW,Mask);
    }
    Depth++;
    if (MaskAll)
      SetAllMaskDepth=Depth;
  }
  if (!FastFindFile && !CmpName(CurMask,FindData->Name,MATCH_NAMES))
    return(SCAN_NEXT);
  return(SCAN_SUCCESS);
}
