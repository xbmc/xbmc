#include "rar.hpp"

#ifndef NATIVE_INT64

Int64::Int64()
{
}


Int64::Int64(uint n)
{
  HighPart=0;
  LowPart=n;
}


Int64::Int64(uint HighPart,uint LowPart)
{
  Int64::HighPart=HighPart;
  Int64::LowPart=LowPart;
}


/*
Int64 Int64::operator = (Int64 n)
{
  HighPart=n.HighPart;
  LowPart=n.LowPart;
  return(*this);
}
*/


Int64 Int64::operator << (int n)
{
  Int64 res=*this;
  while (n--)
  {
    res.HighPart<<=1;
    if (res.LowPart & 0x80000000)
      res.HighPart|=1;
    res.LowPart<<=1;
  }
  return(res);
}


Int64 Int64::operator >> (int n)
{
  Int64 res=*this;
  while (n--)
  {
    res.LowPart>>=1;
    if (res.HighPart & 1)
      res.LowPart|=0x80000000;
    res.HighPart>>=1;
  }
  return(res);
}


Int64 operator / (Int64 n1,Int64 n2)
{
  if (n1.HighPart==0 && n2.HighPart==0)
    return(Int64(0,n1.LowPart/n2.LowPart));
  int ShiftCount=0;
  while (n1>n2)
  {
    n2=n2<<1;
    if (++ShiftCount>64)
      return(0);
  }
  Int64 res=0;
  while (ShiftCount-- >= 0)
  {
    res=res<<1;
    if (n1>=n2)
    {
      n1-=n2;
      ++res;
    }
    n2=n2>>1;
  }
  return(res);
}


Int64 operator * (Int64 n1,Int64 n2)
{
  if (n1<0x10000 && n2<0x10000)
    return(Int64(0,n1.LowPart*n2.LowPart));
  Int64 res=0;
  for (int I=0;I<64;I++)
  {
    if (n2.LowPart & 1)
      res+=n1;
    n1=n1<<1;
    n2=n2>>1;
  }
  return(res);
}


Int64 operator % (Int64 n1,Int64 n2)
{
  if (n1.HighPart==0 && n2.HighPart==0)
    return(Int64(0,n1.LowPart%n2.LowPart));
  return(n1-n1/n2*n2);
}


Int64 operator + (Int64 n1,Int64 n2)
{
  n1.LowPart+=n2.LowPart;
  if (n1.LowPart<n2.LowPart)
    n1.HighPart++;
  n1.HighPart+=n2.HighPart;
  return(n1);
}


Int64 operator - (Int64 n1,Int64 n2)
{
  if (n1.LowPart<n2.LowPart)
    n1.HighPart--;
  n1.LowPart-=n2.LowPart;
  n1.HighPart-=n2.HighPart;
  return(n1);
}


Int64 operator += (Int64 &n1,Int64 n2)
{
  n1=n1+n2;
  return(n1);
}


Int64 operator -= (Int64 &n1,Int64 n2)
{
  n1=n1-n2;
  return(n1);
}


Int64 operator *= (Int64 &n1,Int64 n2)
{
  n1=n1*n2;
  return(n1);
}


Int64 operator /= (Int64 &n1,Int64 n2)
{
  n1=n1/n2;
  return(n1);
}


Int64 operator | (Int64 n1,Int64 n2)
{
  n1.LowPart|=n2.LowPart;
  n1.HighPart|=n2.HighPart;
  return(n1);
}


Int64 operator & (Int64 n1,Int64 n2)
{
  n1.LowPart&=n2.LowPart;
  n1.HighPart&=n2.HighPart;
  return(n1);
}


/*
inline void operator -= (Int64 &n1,unsigned int n2)
{
  if (n1.LowPart<n2)
    n1.HighPart--;
  n1.LowPart-=n2;
}


inline void operator ++ (Int64 &n)
{
  if (++n.LowPart == 0)
    ++n.HighPart;
}


inline void operator -- (Int64 &n)
{
  if (n.LowPart-- == 0)
    n.HighPart--;
}
*/

bool operator == (Int64 n1,Int64 n2)
{
  return(n1.LowPart==n2.LowPart && n1.HighPart==n2.HighPart);
}


bool operator > (Int64 n1,Int64 n2)
{
  return((int)n1.HighPart>(int)n2.HighPart || n1.HighPart==n2.HighPart && n1.LowPart>n2.LowPart);
}


bool operator < (Int64 n1,Int64 n2)
{
  return((int)n1.HighPart<(int)n2.HighPart || n1.HighPart==n2.HighPart && n1.LowPart<n2.LowPart);
}


bool operator != (Int64 n1,Int64 n2)
{
  return(n1.LowPart!=n2.LowPart || n1.HighPart!=n2.HighPart);
}


bool operator >= (Int64 n1,Int64 n2)
{
  return(n1>n2 || n1==n2);
}


bool operator <= (Int64 n1,Int64 n2)
{
  return(n1<n2 || n1==n2);
}


void Int64::Set(uint HighPart,uint LowPart)
{
  Int64::HighPart=HighPart;
  Int64::LowPart=LowPart;
}
#endif

void itoa(Int64 n,char *Str)
{
  if (n<=0xffffffff)
  {
    sprintf(Str,"%u",int64to32(n));
    return;
  }

  char NumStr[50];
  int Pos=0;

  do
  {
    NumStr[Pos++]=int64to32(n%10)+'0';
    n=n/10;
  } while (n!=0);

  for (int I=0;I<Pos;I++)
    Str[I]=NumStr[Pos-I-1];
  Str[Pos]=0;
}


Int64 atoil(char *Str)
{
  Int64 n=0;
  while (*Str>='0' && *Str<='9')
  {
    n=n*10+*Str-'0';
    Str++;
  }
  return(n);
}
