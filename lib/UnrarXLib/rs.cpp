#include "rar.hpp"

#define Clean(D,S)  {for (int I=0;I<(S);I++) (D)[I]=0;}

RSCoder::RSCoder(int ParSize)
{
  RSCoder::ParSize=ParSize;
  FirstBlockDone=false;
  gfInit();
  pnInit();
}


void RSCoder::gfInit()
{
  for (int I=0,J=1;I<MAXPAR;I++)
  {
    gfLog[J]=I;
    gfExp[I]=J;
    if ((J<<=1)&256)
      J^=285;
  }
  for (int I=MAXPAR;I<MAXPOL;I++)
    gfExp[I]=gfExp[I-MAXPAR];
}


inline int RSCoder::gfMult(int a,int b)
{
  return(a==0 || b == 0 ? 0:gfExp[gfLog[a]+gfLog[b]]);
}


void RSCoder::pnInit()
{
  int p1[MAXPAR+1],p2[MAXPAR+1];

  Clean(p2,ParSize);
  p2[0]=1;
  for (int I=1;I<=ParSize;I++)
  {
    Clean(p1,ParSize);
    p1[0]=gfExp[I];
    p1[1]=1;
    pnMult(p1,p2,GXPol);
    for (int J=0;J<ParSize;J++)
      p2[J]=GXPol[J];
  }
}


void RSCoder::pnMult(int *p1,int *p2,int *r)
{
  Clean(r,ParSize);
  for (int I=0;I<ParSize;I++)
    if (p1[I]!=0)
      for(int J=0;J<ParSize-I;J++)
        r[I+J]^=gfMult(p1[I],p2[J]);
}


void RSCoder::Encode(byte *Data,int DataSize,byte *DestData)
{
  int ShiftReg[MAXPAR+1];

  Clean(ShiftReg,ParSize+1);
  for (int I=0;I<DataSize;I++)
  {
    int D=Data[I]^ShiftReg[ParSize-1];
    for (int J=ParSize-1;J>0;J--)
      ShiftReg[J]=ShiftReg[J-1]^gfMult(GXPol[J],D);
    ShiftReg[0]=gfMult(GXPol[0],D);
  }
  for (int I=0;I<ParSize;I++)
    DestData[I]=ShiftReg[ParSize-I-1];
}


bool RSCoder::Decode(byte *Data,int DataSize,int *EraLoc,int EraSize)
{
  int SynData[MAXPOL];
  bool AllZeroes=true;
  for (int I=0;I<ParSize;I++)
  {
    int Sum=Data[0],J=1,Exp=gfExp[I+1];
    for (;J+8<=DataSize;J+=8)
    {
      Sum=Data[J]^gfMult(Exp,Sum);
      Sum=Data[J+1]^gfMult(Exp,Sum);
      Sum=Data[J+2]^gfMult(Exp,Sum);
      Sum=Data[J+3]^gfMult(Exp,Sum);
      Sum=Data[J+4]^gfMult(Exp,Sum);
      Sum=Data[J+5]^gfMult(Exp,Sum);
      Sum=Data[J+6]^gfMult(Exp,Sum);
      Sum=Data[J+7]^gfMult(Exp,Sum);
    }
    for (;J<DataSize;J++)
      Sum=Data[J]^gfMult(Exp,Sum);
    if ((SynData[I]=Sum)!=0)
      AllZeroes=false;
  }
  if (AllZeroes)
    return(true);

  if (!FirstBlockDone)
  {
    FirstBlockDone=true;
    Clean(PolB,ParSize+1);
    PolB[0]=1;
    for (int EraPos=0;EraPos<EraSize;EraPos++)
      for (int I=ParSize,M=gfExp[DataSize-EraLoc[EraPos]-1];I>0;I--)
        PolB[I]^=gfMult(M,PolB[I-1]);

    ErrCount=0;
    for (int Root=MAXPAR-DataSize;Root<MAXPAR+1;Root++)
    {
      int Sum=0;
      for (int B=0;B<ParSize+1;B++)
        Sum^=gfMult(gfExp[(B*Root)%MAXPAR],PolB[B]);
      if (Sum==0)
      {
        Dn[ErrCount]=0;
        for (int I=1;I<ParSize+1;I+=2)
          Dn[ErrCount]^= gfMult(PolB[I],gfExp[Root*(I-1)%MAXPAR]);
        ErrorLocs[ErrCount++]=MAXPAR-Root;
      }
    }
  }

  int PolD[MAXPOL];
  pnMult(PolB,SynData,PolD);
  if ((ErrCount<=ParSize) && ErrCount>0)
    for (int I=0;I<ErrCount;I++)
    {
      int Loc=ErrorLocs[I],DLoc=MAXPAR-Loc,N=0;
      for (int J=0;J<ParSize;J++) 
        N^=gfMult(PolD[J],gfExp[DLoc*J%MAXPAR]);
      int DataPos=DataSize-Loc-1;
      if (DataPos>=0 && DataPos<DataSize)
        Data[DataPos]^=gfMult(N,gfExp[MAXPAR-gfLog[Dn[I]]]);
    }
  return(ErrCount<=ParSize);
}
