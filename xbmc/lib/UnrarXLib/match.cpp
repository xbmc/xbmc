#include "rar.hpp"

static bool match(char *pattern,char *string);
static bool match(wchar *pattern,wchar *string);

inline uint toupperc(byte ch)
{
/*
*/
#if defined(_WIN_32)
  return((uint)CharUpper((LPTSTR)(ch)));
#elif defined(_UNIX)
  return(ch);
#else
  return(toupper(ch));
#endif
}


inline uint touppercw(uint ch)
{
/*
*/
#if defined(_UNIX)
  return(ch);
#else
  return(toupperw(ch));
#endif
}


bool CmpName(char *Wildcard,char *Name,int CmpPath)
{
  CmpPath&=MATCH_MODEMASK;
  
  if (CmpPath!=MATCH_NAMES)
  {
    int WildLength=strlen(Wildcard);
    if (CmpPath!=MATCH_EXACTPATH && strnicompc(Wildcard,Name,WildLength)==0)
    {
      char NextCh=Name[WildLength];
      if (NextCh=='\\' || NextCh=='/' || NextCh==0)
        return(true);
    }
    char Path1[NM],Path2[NM];
    GetFilePath(Wildcard,Path1);
    GetFilePath(Name,Path2);
    if (stricompc(Wildcard,Path2)==0)
      return(true);
    if ((CmpPath==MATCH_PATH || CmpPath==MATCH_EXACTPATH) && stricompc(Path1,Path2)!=0)
      return(false);
    if (CmpPath==MATCH_SUBPATH || CmpPath==MATCH_WILDSUBPATH)
    {
      if (IsWildcard(Path1))
        return(match(Wildcard,Name));
      else if (CmpPath==MATCH_SUBPATH || IsWildcard(Wildcard))
        {
          if (*Path1 && strnicompc(Path1,Path2,strlen(Path1))!=0)
            return(false);
        }
        else if (stricompc(Path1,Path2)!=0)
          return(false);
    }
  }
  char *Name1=PointToName(Wildcard);
  char *Name2=PointToName(Name);
  if (strnicompc("__rar_",Name2,6)==0)
    return(false);
  return(match(Name1,Name2));
}


#ifndef SFX_MODULE
bool CmpName(wchar *Wildcard,wchar *Name,int CmpPath)
{
  CmpPath&=MATCH_MODEMASK;

  if (CmpPath!=MATCH_NAMES)
  {
    int WildLength=strlenw(Wildcard);
    if (CmpPath!=MATCH_EXACTPATH && strnicompcw(Wildcard,Name,WildLength)==0)
    {
      wchar NextCh=Name[WildLength];
      if (NextCh==L'\\' || NextCh==L'/' || NextCh==0)
        return(true);
    }
    wchar Path1[NM],Path2[NM];
    GetFilePath(Wildcard,Path1);
    GetFilePath(Name,Path2);
    if ((CmpPath==MATCH_PATH || CmpPath==MATCH_EXACTPATH) && stricompcw(Path1,Path2)!=0)
      return(false);
    if (CmpPath==MATCH_SUBPATH || CmpPath==MATCH_WILDSUBPATH)
    {
      if (IsWildcard(NULL,Path1))
        return(match(Wildcard,Name));
      else if (CmpPath==MATCH_SUBPATH || IsWildcard(NULL,Wildcard))
      {
        if (*Path1 && strnicompcw(Path1,Path2,strlenw(Path1))!=0)
          return(false);
      }
      else if (stricompcw(Path1,Path2)!=0)
        return(false);
    }
  }
  wchar *Name1=PointToName(Wildcard);
  wchar *Name2=PointToName(Name);
  if (strnicompcw(L"__rar_",Name2,6)==0)
    return(false);
  return(match(Name1,Name2));
}
#endif


bool match(char *pattern,char *string)
{
  for (;; ++string)
  {
    char stringc=toupperc(*string);
    char patternc=toupperc(*pattern++);
    switch (patternc)
    {
      case 0:
        return(stringc==0);
      case '?':
        if (stringc == 0)
          return(false);
        break;
      case '*':
        if (*pattern==0)
          return(true);
        if (*pattern=='.')
        {
          if (pattern[1]=='*' && pattern[2]==0)
            return(true);
          char *dot=strchr(string,'.');
          if (pattern[1]==0)
            return (dot==NULL || dot[1]==0);
          if (dot!=NULL)
          {
            string=dot;
            if (strpbrk(pattern,"*?")==NULL && strchr(string+1,'.')==NULL)
              return(stricompc(pattern+1,string+1)==0);
          }
        }

        while (*string)
          if (match(pattern,string++))
            return(true);
        return(false);
      default:
        if (patternc != stringc)
        {
          if (patternc=='.' && stringc==0)
            return(match(pattern,string));
          else
            return(false);
        }
        break;
    }
  }
}


#ifndef SFX_MODULE
bool match(wchar *pattern,wchar *string)
{
  for (;; ++string)
  {
    wchar stringc=touppercw(*string);
    wchar patternc=touppercw(*pattern++);
    switch (patternc)
    {
      case 0:
        return(stringc==0);
      case '?':
        if (stringc == 0)
          return(false);
        break;
      case '*':
        if (*pattern==0)
          return(true);
        if (*pattern=='.')
        {
          if (pattern[1]=='*' && pattern[2]==0)
            return(true);
          wchar *dot=strchrw(string,'.');
          if (pattern[1]==0)
            return (dot==NULL || dot[1]==0);
          if (dot!=NULL)
          {
            string=dot;
            if (strpbrkw(pattern,L"*?")==NULL && strchrw(string+1,'.')==NULL)
              return(stricompcw(pattern+1,string+1)==0);
          }
        }

        while (*string)
          if (match(pattern,string++))
            return(true);
        return(false);
      default:
        if (patternc != stringc)
        {
          if (patternc=='.' && stringc==0)
            return(match(pattern,string));
          else
            return(false);
        }
        break;
    }
  }
}
#endif


int stricompc(const char *Str1,const char *Str2)
{
#if defined(_UNIX)
  return(strcmp(Str1,Str2));
#else
  return(stricomp(Str1,Str2));
#endif
}


#ifndef SFX_MODULE
int stricompcw(const wchar *Str1,const wchar *Str2)
{
#if defined(_UNIX)
  return(strcmpw(Str1,Str2));
#else
  return(stricmpw(Str1,Str2));
#endif
}
#endif


int strnicompc(const char *Str1,const char *Str2,int N)
{
#if defined(_UNIX)
  return(strncmp(Str1,Str2,N));
#else
  return(strnicomp(Str1,Str2,N));
#endif
}


#ifndef SFX_MODULE
int strnicompcw(const wchar *Str1,const wchar *Str2,int N)
{
#if defined(_UNIX)
  return(strncmpw(Str1,Str2,N));
#else
  return(strnicmpw(Str1,Str2,N));
#endif
}
#endif
