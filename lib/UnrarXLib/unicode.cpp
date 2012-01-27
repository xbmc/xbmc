#include "rar.hpp"

// yuvalt: wcstombs is not the way to go since it does not convert
// from utf8 to utf16. Luickly, there's a UTF8/UTF16 conversion
// functions in here which are used if _APPLE is defined.
// Therefore, define _APPLE in this case to do proper conversion
#undef MBFUNCTIONS
#undef _WIN_32
#define _APPLE

bool WideToChar(const wchar *Src,char *Dest,int DestSize)
{
  bool RetCode=true;
#ifdef _WIN_32
  if (WideCharToMultiByte(CP_ACP,0,Src,-1,Dest,DestSize,NULL,NULL)==0)
    RetCode=false;
#else
#ifdef _APPLE
  WideToUtf(Src,Dest,DestSize);
#else
#ifdef MBFUNCTIONS
  if (wcstombs(Dest,Src,DestSize)==(uint)-1)
    RetCode=false;
#else
  if (UnicodeEnabled())
  {
    for (int I=0;I<DestSize;I++)
    {
      Dest[I]=(char)Src[I];
      if (Src[I]==0)
        break;
  }
#endif
#endif
#endif
  return(RetCode);
}


bool CharToWide(const char *Src,wchar *Dest,int DestSize)
{
  bool RetCode=true;
#ifdef _WIN_32
  if (MultiByteToWideChar(CP_ACP,0,Src,-1,Dest,DestSize)==0)
    RetCode=false;
#else
#ifdef _APPLE
  UtfToWide(Src,Dest,DestSize);
#else
#ifdef MBFUNCTIONS
  mbstowcs(Dest,Src,DestSize);
#else
  if (UnicodeEnabled())
  {
    for (int I=0;I<DestSize;I++)
    {
      Dest[I]=(wchar_t)Src[I];
      if (Src[I]==0)
        break;
  }
#endif
#endif
#endif
  return(RetCode);
}


byte* WideToRaw(const wchar *Src,byte *Dest,int DestSize)
{
  for (int I=0;I<DestSize;I++,Src++)
  {
    Dest[I*2]=(byte)*Src;
    Dest[I*2+1]=(byte)(*Src>>8);
    if (*Src==0)
      break;
  }
  return(Dest);
}


wchar* RawToWide(const byte *Src,wchar *Dest,int DestSize)
{
  for (int I=0;I<DestSize;I++)
    if ((Dest[I]=Src[I*2]+(Src[I*2+1]<<8))==0)
      break;
  return(Dest);
}


#ifdef _APPLE
void WideToUtf(const wchar *Src,char *Dest,int DestSize)
{
  DestSize--;
  while (*Src!=0 && --DestSize>=0)
  {
    uint c=*(Src++);
    if (c<0x80)
      *(Dest++)=c;
    else
      if (c<0x800 && --DestSize>=0)
      {
        *(Dest++)=(0xc0|(c>>6));
        *(Dest++)=(0x80|(c&0x3f));
      }
      else
        if (c<0x10000 && (DestSize-=2)>=0)
        {
          *(Dest++)=(0xe0|(c>>12));
          *(Dest++)=(0x80|((c>>6)&0x3f));
          *(Dest++)=(0x80|(c&0x3f));
        }
  }
  *Dest=0;
}
#endif


#ifdef _APPLE
void UtfToWide(const char *Src,wchar *Dest,int DestSize)
{
  DestSize--;
  while (*Src!=0)
  {
    uint c=(byte)*(Src++),d;
    if (c<0x80)
      d=c;
    else
      if ((c>>5)==6)
      {
        if ((*Src&0xc0)!=0x80)
          break;
        d=((c&0x1f)<<6)|(*Src&0x3f);
        Src++;
      }
      else
        if ((c>>4)==14)
        {
          if ((Src[0]&0xc0)!=0x80 || (Src[1]&0xc0)!=0x80)
            break;
          d=((c&0xf)<<12)|((Src[0]&0x3f)<<6)|(Src[1]&0x3f);
          Src+=2;
        }
        else
          break;
    if (--DestSize<0)
      break;
    *(Dest++)=d;
  }
  *Dest=0;
}
#endif


bool UnicodeEnabled()
{
#ifdef UNICODE_SUPPORTED
  #ifdef _EMX
    return(uni_ready);
  #else
    return(true);
  #endif
#else
  return(false);
#endif
}


int strlenw(const wchar *str)
{
  int length=0;
  while (*(str++)!=0)
    length++;
  return(length);
}


wchar* strcpyw(wchar *dest,const wchar *src)
{
  do {
    *(dest++)=*src;
  } while (*(src++)!=0);
  return(dest);
}


wchar* strncpyw(wchar *dest,const wchar *src,int n)
{
  do {
    *(dest++)=*src;
  } while (*(src++)!=0 && --n > 0);
  return(dest);
}


wchar* strcatw(wchar *dest,const wchar *src)
{
  return(strcpyw(dest+strlenw(dest),src));
}


#ifndef SFX_MODULE
wchar* strncatw(wchar *dest,const wchar *src,int n)
{
  dest+=strlenw(dest);
  while (true)
    if (--n<0)
    {
      *dest=0;
      break;
    }
    else
      if ((*(dest++)=*(src++))==0)
        break;
  return(dest);
}
#endif


int strcmpw(const wchar *s1,const wchar *s2)
{
  while (*s1==*s2)
  {
    if (*s1==0)
      return(0);
    s1++;
    s2++;
  }
  return(*s1<*s2 ? -1:1);
}


int strncmpw(const wchar *s1,const wchar *s2,int n)
{
  while (n-->0)
  {
    if (*s1<*s2)
      return(-1);
    if (*s1>*s2)
      return(-1);
    if (*s1==0)
      break;
    s1++;
    s2++;
  }
  return(0);
}


#ifndef SFX_MODULE
int stricmpw(const wchar *s1,const wchar *s2)
{
  char Ansi1[NM*sizeof(wchar)],Ansi2[NM*sizeof(wchar)];
  WideToChar(s1,Ansi1,sizeof(Ansi1));
  WideToChar(s2,Ansi2,sizeof(Ansi2));
  return(stricomp(Ansi1,Ansi2));
}
#endif


#if !defined(SFX_MODULE) && !defined(_WIN_CE)
inline int strnicmpw_w2c(const wchar *s1,const wchar *s2,int n)
{
  wchar Wide1[NM*2],Wide2[NM*2];
  strncpyw(Wide1,s1,sizeof(Wide1)/sizeof(Wide1[0])-1);
  strncpyw(Wide2,s2,sizeof(Wide2)/sizeof(Wide2[0])-1);
  Wide1[Min((int)(sizeof(Wide1)/sizeof(Wide1[0])-1),n)]=0;
  Wide2[Min((int)(sizeof(Wide2)/sizeof(Wide2[0])-1),n)]=0;
  char Ansi1[NM*2],Ansi2[NM*2];
  WideToChar(Wide1,Ansi1,sizeof(Ansi1));
  WideToChar(Wide2,Ansi2,sizeof(Ansi2));
  return(stricomp(Ansi1,Ansi2));
}
#endif


#ifndef SFX_MODULE
int strnicmpw(const wchar *s1,const wchar *s2,int n)
{
  return(strnicmpw_w2c(s1,s2,n));
}
#endif


wchar* strchrw(const wchar *s,int c)
{
  while (*s)
  {
    if (*s==c)
      return((wchar *)s);
    s++;
  }
  return(NULL);
}


wchar* strrchrw(const wchar *s,int c)
{
  for (int I=strlenw(s)-1;I>=0;I--)
    if (s[I]==c)
      return((wchar *)(s+I));
  return(NULL);
}


wchar* strpbrkw(const wchar *s1,const wchar *s2)
{
  while (*s1)
  {
    if (strchrw(s2,*s1)!=NULL)
      return((wchar *)s1);
    s1++;
  }
  return(NULL);
}


#ifndef SFX_MODULE
wchar* strlowerw(wchar *Str)
{
  for (wchar *ChPtr=Str;*ChPtr;ChPtr++)
    if (*ChPtr<128)
      *ChPtr=loctolower((char)*ChPtr);
  return(Str);
}
#endif


#ifndef SFX_MODULE
wchar* strupperw(wchar *Str)
{
  for (wchar *ChPtr=Str;*ChPtr;ChPtr++)
    if (*ChPtr<128)
      *ChPtr=loctoupper((char)*ChPtr);
  return(Str);
}
#endif


#ifndef SFX_MODULE
int toupperw(int ch)
{
  return((ch<128) ? loctoupper(ch):ch);
}
#endif


int atoiw(const wchar *s)
{
  int n=0;
  while (*s>='0' && *s<='9')
  {
    n=n*10+(*s-'0');
    s++;
  }
  return(n);
}


#ifdef DBCS_SUPPORTED
SupportDBCS gdbcs;

SupportDBCS::SupportDBCS()
{
  Init();
}


void SupportDBCS::Init()
{
#if defined(_LINUX)
  DBCSMode = true;
  for (int I=0;I<sizeof(IsLeadByte)/sizeof(IsLeadByte[0]);I++)
    IsLeadByte[I]=true;
#else
  CPINFO CPInfo;
  GetCPInfo(CP_ACP,&CPInfo);
  DBCSMode=CPInfo.MaxCharSize > 1;
  for (int I=0;I<sizeof(IsLeadByte)/sizeof(IsLeadByte[0]);I++)
    IsLeadByte[I]=IsDBCSLeadByte(I);
#endif
}


char* SupportDBCS::charnext(const char *s)
{
  return (char *)(IsLeadByte[*s] ? s+2:s+1);
}


uint SupportDBCS::strlend(const char *s)
{
  uint Length=0;
  while (*s!=0)
  {
    if (IsLeadByte[*s])
      s+=2;
    else
      s++;
    Length++;
  }
  return(Length);
}


char* SupportDBCS::strchrd(const char *s, int c)
{
  while (*s!=0)
    if (IsLeadByte[*s])
      s+=2;
    else
      if (*s==c)
        return((char *)s);
      else
        s++;
  return(NULL);
}


void SupportDBCS::copychrd(char *dest,const char *src)
{
  dest[0]=src[0];
  if (IsLeadByte[src[0]])
    dest[1]=src[1];
}


char* SupportDBCS::strrchrd(const char *s, int c)
{
  const char *found=NULL;
  while (*s!=0)
    if (IsLeadByte[*s])
      s+=2;
    else
    {
      if (*s==c)
        found=s;
      s++;
    }
  return((char *)found);
}
#endif
