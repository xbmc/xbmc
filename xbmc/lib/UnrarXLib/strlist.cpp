#include "rar.hpp"

StringList::StringList()
{
  Reset();
}


StringList::~StringList()
{
}


void StringList::Reset()
{
  Rewind();
  StringData.Reset();
  StringDataW.Reset();
  PosDataW.Reset();
  StringsCount=0;
  SavePosNumber=0;
}


unsigned int StringList::AddString(const char *Str)
{
  return(AddString(Str,NULL));
}


unsigned int StringList::AddString(const char *Str,const wchar *StrW)
{
  int PrevSize=StringData.Size();
  StringData.Add(strlen(Str)+1);
  strcpy(&StringData[PrevSize],Str);
  if (StrW!=NULL && *StrW!=0)
  {
    int PrevPos=PosDataW.Size();
    PosDataW.Add(1);
    PosDataW[PrevPos]=PrevSize;

    int PrevSizeW=StringDataW.Size();
    StringDataW.Add(strlenw(StrW)+1);
    strcpyw(&StringDataW[PrevSizeW],StrW);
  }
  StringsCount++;
  return(PrevSize);
}


bool StringList::GetString(char *Str,int MaxLength)
{
  return(GetString(Str,NULL,MaxLength));
}


bool StringList::GetString(char *Str,wchar *StrW,int MaxLength)
{
  char *StrPtr;
  wchar *StrPtrW;
  if (Str==NULL || !GetString(&StrPtr,&StrPtrW))
    return(false);
  strncpy(Str,StrPtr,MaxLength);
  if (StrW!=NULL)
    strncpyw(StrW,NullToEmpty(StrPtrW),MaxLength);
  return(true);
}


#ifndef SFX_MODULE
bool StringList::GetString(char *Str,wchar *StrW,int MaxLength,int StringNum)
{
  SavePosition();
  Rewind();
  bool RetCode=true;
  while (StringNum-- >=0)
    if (!GetString(Str,StrW,MaxLength))
    {
      RetCode=false;
      break;
    }
  RestorePosition();
  return(RetCode);
}
#endif


char* StringList::GetString()
{
  char *Str;
  GetString(&Str,NULL);
  return(Str);
}



bool StringList::GetString(char **Str,wchar **StrW)
{
  if (CurPos>=StringData.Size())
  {
    *Str=NULL;
    return(false);
  }
  *Str=&StringData[CurPos];
  if (PosDataItem<PosDataW.Size() && PosDataW[PosDataItem]==CurPos)
  {
    PosDataItem++;
    if (StrW!=NULL)
      *StrW=&StringDataW[CurPosW];
    CurPosW+=strlenw(&StringDataW[CurPosW])+1;
  }
  else
    if (StrW!=NULL)
      *StrW=NULL;
  CurPos+=strlen(*Str)+1;
  return(true);
}


char* StringList::GetString(unsigned int StringPos)
{
  if (StringPos>=StringData.Size())
    return(NULL);
  return(&StringData[StringPos]);
}


void StringList::Rewind()
{
  CurPos=0;
  CurPosW=0;
  PosDataItem=0;
}


int StringList::GetBufferSize()
{
  return(StringData.Size()+StringDataW.Size());
}


#ifndef SFX_MODULE
bool StringList::Search(char *Str,wchar *StrW,bool CaseSensitive)
{
  SavePosition();
  Rewind();
  bool Found=false;
  char *CurStr;
  wchar *CurStrW;
  while (GetString(&CurStr,&CurStrW))
  {
    if ((CaseSensitive ? strcmp(Str,CurStr):stricomp(Str,CurStr))!=0)
      continue;
    if (StrW!=NULL && CurStrW!=NULL)
      if ((CaseSensitive ? strcmpw(StrW,CurStrW):stricmpw(StrW,CurStrW))!=0)
        continue;
    Found=true;
    break;
  }
  RestorePosition();
  return(Found);
}
#endif


#ifndef SFX_MODULE
void StringList::SavePosition()
{
  if (SavePosNumber<sizeof(SaveCurPos)/sizeof(SaveCurPos[0]))
  {
    SaveCurPos[SavePosNumber]=CurPos;
    SaveCurPosW[SavePosNumber]=CurPosW;
    SavePosDataItem[SavePosNumber]=PosDataItem;
    SavePosNumber++;
  }
}
#endif


#ifndef SFX_MODULE
void StringList::RestorePosition()
{
  if (SavePosNumber>0)
  {
    SavePosNumber--;
    CurPos=SaveCurPos[SavePosNumber];
    CurPosW=SaveCurPosW[SavePosNumber];
    PosDataItem=SavePosDataItem[SavePosNumber];
  }
}
#endif
