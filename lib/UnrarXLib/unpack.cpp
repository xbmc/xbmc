#include "rar.hpp"

#include "coder.cpp"
#include "suballoc.cpp"
#include "model.cpp"
#ifndef SFX_MODULE
#include "unpack15.cpp"
#include "unpack20.cpp"
#endif

#ifdef _LINUX
#include "XSyncUtils.h"
#include "XEventUtils.h"
#endif

Unpack::Unpack(ComprDataIO *DataIO)
{
  UnpIO=DataIO;
  Window=NULL;
  ExternalWindow=false;
  Suspended=false;
  UnpAllBuf=false;
  UnpSomeRead=false;
}


Unpack::~Unpack()
{
  if (Window!=NULL && !ExternalWindow)
    delete[] Window;
  InitFilters();
}


void Unpack::Init(byte *Window)
{
  if (Window==NULL)
  {
    if (UnpIO->UnpackToMemorySize > -1)
      Unpack::Window = new byte[MAXWINMEMSIZE];
    else
      Unpack::Window=new byte[MAXWINSIZE];
#ifndef ALLOW_EXCEPTIONS
    if (Unpack::Window==NULL)
      ErrHandler.MemoryError();
#endif
  }
  else
  {
    Unpack::Window=Window;
    ExternalWindow=true;
  }
  UnpInitData(false);
}


void Unpack::DoUnpack(int Method,bool Solid)
{
  switch(Method)
  {
#ifndef SFX_MODULE
    case 15:
      Unpack15(Solid);
      break;
    case 20:
    case 26:
      Unpack20(Solid);
      break;
#endif
    case 29:
      Unpack29(Solid);
      break;
  }
}


inline void Unpack::InsertOldDist(unsigned int Distance)
{
  OldDist[3]=OldDist[2];
  OldDist[2]=OldDist[1];
  OldDist[1]=OldDist[0];
  OldDist[0]=Distance;
}


inline void Unpack::InsertLastMatch(unsigned int Length,unsigned int Distance)
{
  LastDist=Distance;
  LastLength=Length;
}


void Unpack::CopyString(unsigned int Length,unsigned int Distance)
{
  unsigned int DestPtr=UnpPtr-Distance;
  if (DestPtr<MAXWINSIZE-260 && UnpPtr<MAXWINSIZE-260)
  {
    Window[UnpPtr++]=Window[DestPtr++];
    while (--Length>0)
      Window[UnpPtr++]=Window[DestPtr++];
  }
  else
    while (Length--)
    {
      Window[UnpPtr]=Window[DestPtr++ & MAXWINMASK];
      UnpPtr=(UnpPtr+1) & MAXWINMASK;
    }
}


int Unpack::DecodeNumber(struct Decode *Dec)
{
  unsigned int Bits;
  unsigned int BitField=getbits() & 0xfffe;
  if (BitField<Dec->DecodeLen[8])
    if (BitField<Dec->DecodeLen[4])
      if (BitField<Dec->DecodeLen[2])
        if (BitField<Dec->DecodeLen[1])
          Bits=1;
        else
          Bits=2;
      else
        if (BitField<Dec->DecodeLen[3])
          Bits=3;
        else
          Bits=4;
    else
      if (BitField<Dec->DecodeLen[6])
        if (BitField<Dec->DecodeLen[5])
          Bits=5;
        else
          Bits=6;
      else
        if (BitField<Dec->DecodeLen[7])
          Bits=7;
        else
          Bits=8;
  else
    if (BitField<Dec->DecodeLen[12])
      if (BitField<Dec->DecodeLen[10])
        if (BitField<Dec->DecodeLen[9])
          Bits=9;
        else
          Bits=10;
      else
        if (BitField<Dec->DecodeLen[11])
          Bits=11;
        else
          Bits=12;
    else
      if (BitField<Dec->DecodeLen[14])
        if (BitField<Dec->DecodeLen[13])
          Bits=13;
        else
          Bits=14;
      else
        Bits=15;

  addbits(Bits);
  unsigned int N=Dec->DecodePos[Bits]+((BitField-Dec->DecodeLen[Bits-1])>>(16-Bits));
  if (N>=Dec->MaxNum)
    N=0;
  return(Dec->DecodeNum[N]);
}


void Unpack::Unpack29(bool Solid)
{
  static unsigned char LDecode[]={0,1,2,3,4,5,6,7,8,10,12,14,16,20,24,28,32,40,48,56,64,80,96,112,128,160,192,224};
  static unsigned char LBits[]=  {0,0,0,0,0,0,0,0,1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4,  4,  5,  5,  5,  5};
  static int DDecode[DC];
  static byte DBits[DC];
  static unsigned int DBitLengthCounts[]= {4,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,14,0,12};
  static unsigned char SDDecode[]={0,4,8,16,32,64,128,192};
  static unsigned char SDBits[]=  {2,2,3, 4, 5, 6,  6,  6};
  unsigned int Bits;

  if (DDecode[1]==0)
  {
    int Dist=0,BitLength=0,Slot=0;
    for (unsigned int I=0;I<sizeof(DBitLengthCounts)/sizeof(DBitLengthCounts[0]);I++,BitLength++)
      for (unsigned int J=0;J<DBitLengthCounts[I];J++,Slot++,Dist+=(1<<BitLength))
      {
        DDecode[Slot]=Dist;
        DBits[Slot]=BitLength;
      }
  }

  FileExtracted=true;

  if (!Suspended)
  {
    UnpInitData(Solid);
    if (!UnpReadBuf())
      return;
    if (!TablesRead)
      if (!ReadTables())
        return;
//    if (!TablesRead && Solid)
  //    if (!ReadTables())
  //      return;
    //if ((!Solid || !TablesRead) && !ReadTables())
    //  return;
  }

  if (PPMError)
    return;

  while (true)
  {
    if (UnpIO->bQuit) 
    {
      FileExtracted=false;
      return;
    }

    UnpPtr&=MAXWINMASK;

    if (InAddr>ReadBorder)
    {
      if (!UnpReadBuf())
        break;
    }
    if (((WrPtr-UnpPtr) & MAXWINMASK)<260 && WrPtr!=UnpPtr)
    {
      UnpWriteBuf();
      if (WrittenFileSize>DestUnpSize)
        return;
      if (Suspended)
      {
        FileExtracted=false;
        return;
      }
    }
    if (UnpBlockType==BLOCK_PPM)
    {
      int Ch=PPM.DecodeChar();
      if (Ch==-1)
      {
        PPMError=true;
        break;
      }
      if (Ch==PPMEscChar)
      {
        int NextCh=PPM.DecodeChar();
        if (NextCh==0)
        {
          if (!ReadTables())
            break;
          continue;
        }
        if (NextCh==2 || NextCh==-1)
          break;
        if (NextCh==3)
        {
          if (!ReadVMCodePPM())
            break;
          continue;
        }
        if (NextCh==4)
        {
          unsigned int Distance=0,Length=0;
          bool Failed=false;
          for (int I=0;I<4 && !Failed;I++)
          {
            int Ch=PPM.DecodeChar();
            if (Ch==-1)
              Failed=true;
            else
              if (I==3)
                Length=(byte)Ch;
              else
                Distance=(Distance<<8)+(byte)Ch;
          }
          if (Failed)
            break;
          CopyString(Length+32,Distance+2);
          continue;
        }
        if (NextCh==5)
        {
          int Length=PPM.DecodeChar();
          if (Length==-1)
            break;
          CopyString(Length+4,1);
          continue;
        }
      }
      Window[UnpPtr++]=Ch;
      continue;
    }

    int Number=DecodeNumber((struct Decode *)&LD);
    if (Number<256)
    {
      Window[UnpPtr++]=(byte)Number;
      continue;
    }
    if (Number>=271)
    {
      int Length=LDecode[Number-=271]+3;
      if ((Bits=LBits[Number])>0)
      {
        Length+=getbits()>>(16-Bits);
        addbits(Bits);
      }

      int DistNumber=DecodeNumber((struct Decode *)&DD);
      unsigned int Distance=DDecode[DistNumber]+1;
      if ((Bits=DBits[DistNumber])>0)
      {
        if (DistNumber>9)
        {
          if (Bits>4)
          {
            Distance+=((getbits()>>(20-Bits))<<4);
            addbits(Bits-4);
          }
          if (LowDistRepCount>0)
          {
            LowDistRepCount--;
            Distance+=PrevLowDist;
          }
          else
          {
            int LowDist=DecodeNumber((struct Decode *)&LDD);
            if (LowDist==16)
            {
              LowDistRepCount=LOW_DIST_REP_COUNT-1;
              Distance+=PrevLowDist;
            }
            else
            {
              Distance+=LowDist;
              PrevLowDist=LowDist;
            }
          }
        }
        else
        {
          Distance+=getbits()>>(16-Bits);
          addbits(Bits);
        }
      }

      if (Distance>=0x2000)
      {
        Length++;
        if (Distance>=0x40000L)
          Length++;
      }

      InsertOldDist(Distance);
      InsertLastMatch(Length,Distance);
      CopyString(Length,Distance);
      continue;
    }
    if (Number==256)
    {
      if (!ReadEndOfBlock())
        break;
      continue;
    }
    if (Number==257)
    {
      if (!ReadVMCode())
        break;
      continue;
    }
    if (Number==258)
    {
      if (LastLength!=0)
        CopyString(LastLength,LastDist);
      continue;
    }
    if (Number<263)
    {
      int DistNum=Number-259;
      unsigned int Distance=OldDist[DistNum];
      for (int I=DistNum;I>0;I--)
        OldDist[I]=OldDist[I-1];
      OldDist[0]=Distance;

      int LengthNumber=DecodeNumber((struct Decode *)&RD);
      int Length=LDecode[LengthNumber]+2;
      if ((Bits=LBits[LengthNumber])>0)
      {
        Length+=getbits()>>(16-Bits);
        addbits(Bits);
      }
      InsertLastMatch(Length,Distance);
      CopyString(Length,Distance);
      continue;
    }
    if (Number<272)
    {
      unsigned int Distance=SDDecode[Number-=263]+1;
      if ((Bits=SDBits[Number])>0)
      {
        Distance+=getbits()>>(16-Bits);
        addbits(Bits);
      }
      InsertOldDist(Distance);
      InsertLastMatch(2,Distance);
      CopyString(2,Distance);
      continue;
    }
  }
  UnpWriteBuf();

  if (UnpIO->UnpackToMemorySize > -1)
  {
    UnpIO->hBufferEmpty->Set();
    while (! UnpIO->hBufferFilled->WaitMSec(1))
      if (UnpIO->hQuit->WaitMSec(1))
        return;
  }
}

bool Unpack::ReadEndOfBlock()
{
  unsigned int BitField=getbits();
  bool NewTable,NewFile=false;
  if (BitField & 0x8000)
  {
    NewTable=true;
    addbits(1);
  }
  else
  {
    NewFile=true;
    NewTable=(BitField & 0x4000);
    addbits(2);
  }
  TablesRead=!NewTable;
  return !(NewFile || (NewTable && !ReadTables()));
}


bool Unpack::ReadVMCode()
{
  unsigned int FirstByte=getbits()>>8;
  addbits(8);
  int Length=(FirstByte & 7)+1;
  if (Length==7)
  {
    Length=(getbits()>>8)+7;
    addbits(8);
  }
  else
    if (Length==8)
    {
      Length=getbits();
      addbits(16);
    }
  Array<byte> VMCode(Length);
  for (int I=0;I<Length;I++)
  {
    if (InAddr>=ReadTop-1 && !UnpReadBuf() && I<Length-1)
      return(false);
    VMCode[I]=getbits()>>8;
    addbits(8);
  }
  return(AddVMCode(FirstByte,&VMCode[0],Length));
}


bool Unpack::ReadVMCodePPM()
{
  unsigned int FirstByte=PPM.DecodeChar();
  if ((int)FirstByte==-1)
    return(false);
  int Length=(FirstByte & 7)+1;
  if (Length==7)
      {
    int B1=PPM.DecodeChar();
    if (B1==-1)
      return(false);
    Length=B1+7;
  }
  else
    if (Length==8)
    {
      int B1=PPM.DecodeChar();
      if (B1==-1)
        return(false);
      int B2=PPM.DecodeChar();
      if (B2==-1)
        return(false);
      Length=B1*256+B2;
    }  
  Array<byte> VMCode(Length);
  for (int I=0;I<Length;I++)
  {
    int Ch=PPM.DecodeChar();
    if (Ch==-1)
      return(false);
    VMCode[I]=Ch;
  }
  return(AddVMCode(FirstByte,&VMCode[0],Length));
}


bool Unpack::AddVMCode(unsigned int FirstByte,byte *Code,int CodeSize)
{
  BitInput Inp;
  Inp.InitBitInput();
  memcpy(Inp.InBuf,Code,Min(BitInput::MAX_SIZE,CodeSize));
  VM.Init();

  int FiltPos;
  if (FirstByte & 0x80)
  {
    FiltPos=RarVM::ReadData(Inp);
    if (FiltPos==0)
      InitFilters();
    else
      FiltPos--;
  }
  else
    FiltPos=LastFilter;
  if (FiltPos>Filters.Size() || FiltPos>OldFilterLengths.Size())
    return(false);
  LastFilter=FiltPos;
  bool NewFilter=(FiltPos==Filters.Size());

  UnpackFilter *Filter;
  if (NewFilter)
  {
    Filters.Add(1);
    Filters[Filters.Size()-1]=Filter=new UnpackFilter;
    OldFilterLengths.Add(1);
    Filter->ExecCount=0;
  }
  else
  {
    Filter=Filters[FiltPos];
    Filter->ExecCount++;
  }

  UnpackFilter *StackFilter=new UnpackFilter;

  int EmptyCount=0;
  for (int I=0;I<PrgStack.Size();I++)
  {
    PrgStack[I-EmptyCount]=PrgStack[I];
    if (PrgStack[I]==NULL)
      EmptyCount++;
    if (EmptyCount>0)
      PrgStack[I]=NULL;
  }
  if (EmptyCount==0)
  {
    PrgStack.Add(1);
    EmptyCount=1;
  }
  int StackPos=PrgStack.Size()-EmptyCount;
  PrgStack[StackPos]=StackFilter;
  StackFilter->ExecCount=Filter->ExecCount;
 
  uint BlockStart=RarVM::ReadData(Inp);
  if (FirstByte & 0x40)
    BlockStart+=258;
  StackFilter->BlockStart=(BlockStart+UnpPtr)&MAXWINMASK;
  if (FirstByte & 0x20)
    StackFilter->BlockLength=RarVM::ReadData(Inp);
  else
    StackFilter->BlockLength=FiltPos<OldFilterLengths.Size() ? OldFilterLengths[FiltPos]:0;
  StackFilter->NextWindow=WrPtr!=UnpPtr && ((WrPtr-UnpPtr)&MAXWINMASK)<=BlockStart;

//  DebugLog("\nNextWindow: UnpPtr=%08x WrPtr=%08x BlockStart=%08x",UnpPtr,WrPtr,BlockStart);

  OldFilterLengths[FiltPos]=StackFilter->BlockLength;

  memset(StackFilter->Prg.InitR,0,sizeof(StackFilter->Prg.InitR));
  StackFilter->Prg.InitR[3]=VM_GLOBALMEMADDR;
  StackFilter->Prg.InitR[4]=StackFilter->BlockLength;
  StackFilter->Prg.InitR[5]=StackFilter->ExecCount;
  if (FirstByte & 0x10)
  {
    unsigned int InitMask=Inp.fgetbits()>>9;
    Inp.faddbits(7);
    for (int I=0;I<7;I++)
      if (InitMask & (1<<I))
        StackFilter->Prg.InitR[I]=RarVM::ReadData(Inp);
  }
  if (NewFilter)
  {
    uint VMCodeSize=RarVM::ReadData(Inp);
    if (VMCodeSize>=0x10000 || VMCodeSize==0)
      return(false);
    Array<byte> VMCode(VMCodeSize);
    for (uint I=0;I<VMCodeSize;I++)
    {
      VMCode[I]=Inp.fgetbits()>>8;
      Inp.faddbits(8);
    }
    VM.Prepare(&VMCode[0],VMCodeSize,&Filter->Prg);
  }
  StackFilter->Prg.AltCmd=&Filter->Prg.Cmd[0];
  StackFilter->Prg.CmdCount=Filter->Prg.CmdCount;

  int StaticDataSize=Filter->Prg.StaticData.Size();
  if (StaticDataSize>0 && StaticDataSize<VM_GLOBALMEMSIZE)
  {
    StackFilter->Prg.StaticData.Add(StaticDataSize);
    memcpy(&StackFilter->Prg.StaticData[0],&Filter->Prg.StaticData[0],StaticDataSize);
  }

  if (StackFilter->Prg.GlobalData.Size()<VM_FIXEDGLOBALSIZE)
  {
    StackFilter->Prg.GlobalData.Reset();
    StackFilter->Prg.GlobalData.Add(VM_FIXEDGLOBALSIZE);
  }
  byte *GlobalData=&StackFilter->Prg.GlobalData[0];
  for (int I=0;I<7;I++)
    VM.SetValue((uint *)&GlobalData[I*4],StackFilter->Prg.InitR[I]);
  VM.SetValue((uint *)&GlobalData[0x1c],StackFilter->BlockLength);
  VM.SetValue((uint *)&GlobalData[0x20],0);
  VM.SetValue((uint *)&GlobalData[0x2c],StackFilter->ExecCount);
  memset(&GlobalData[0x30],0,16);

  if (FirstByte & 8)
  {
    uint DataSize=RarVM::ReadData(Inp);
    if (DataSize>=0x10000)
      return(false);
    unsigned int CurSize=StackFilter->Prg.GlobalData.Size();
    if (CurSize<DataSize+VM_FIXEDGLOBALSIZE)
      StackFilter->Prg.GlobalData.Add(DataSize+VM_FIXEDGLOBALSIZE-CurSize);
    byte *GlobalData=&StackFilter->Prg.GlobalData[VM_FIXEDGLOBALSIZE];
    for (uint I=0;I<DataSize;I++)
    {
      GlobalData[I]=Inp.fgetbits()>>8;
      Inp.faddbits(8);
    }
  }
  return(true);
}


bool Unpack::UnpReadBuf()
{
  int DataSize=ReadTop-InAddr;
  if (DataSize<0)
    return(false);
  if (InAddr>BitInput::MAX_SIZE/2)
  {
    if (DataSize>0)
      memmove(InBuf,InBuf+InAddr,DataSize);
    InAddr=0;
    ReadTop=DataSize;
  }
  else
    DataSize=ReadTop;
  int ReadCode=UnpIO->UnpRead(InBuf+DataSize,(BitInput::MAX_SIZE-DataSize)&~0xf);
  if (ReadCode>0)
    ReadTop+=ReadCode;
  ReadBorder=ReadTop-30;
  return(ReadCode!=-1);
}


void Unpack::UnpWriteBuf()
{
  unsigned int WrittenBorder=WrPtr;
  unsigned int WriteSize=(UnpPtr-WrittenBorder)&MAXWINMASK;
  for (int I=0;I<PrgStack.Size();I++)
  {
    UnpackFilter *flt=PrgStack[I];
    if (flt==NULL)
      continue;
    if (flt->NextWindow)
    {
      flt->NextWindow=false;
      continue;
    }
    unsigned int BlockStart=flt->BlockStart;
    unsigned int BlockLength=flt->BlockLength;
    if (((BlockStart-WrittenBorder)&MAXWINMASK)<WriteSize)
    {
      if (WrittenBorder!=BlockStart)
      {
        UnpWriteArea(WrittenBorder,BlockStart);
        WrittenBorder=BlockStart;
        WriteSize=(UnpPtr-WrittenBorder)&MAXWINMASK;
      }
      if (BlockLength<=WriteSize)
      {
        unsigned int BlockEnd=(BlockStart+BlockLength)&MAXWINMASK;
        if (BlockStart<BlockEnd || BlockEnd==0)
          VM.SetMemory(0,Window+BlockStart,BlockLength);
        else
        {
          unsigned int FirstPartLength=MAXWINSIZE-BlockStart;
          VM.SetMemory(0,Window+BlockStart,FirstPartLength);
          VM.SetMemory(FirstPartLength,Window,BlockEnd);
        }
        VM_PreparedProgram *Prg=&flt->Prg;
        ExecuteCode(Prg);

        byte *FilteredData=Prg->FilteredData;
        unsigned int FilteredDataSize=Prg->FilteredDataSize;

        delete PrgStack[I];
        PrgStack[I]=NULL;
        while (I+1<PrgStack.Size())
        {
          UnpackFilter *NextFilter=PrgStack[I+1];
          if (NextFilter==NULL || NextFilter->BlockStart!=BlockStart ||
              NextFilter->BlockLength!=FilteredDataSize || NextFilter->NextWindow)
            break;
          VM.SetMemory(0,FilteredData,FilteredDataSize);
          VM_PreparedProgram *NextPrg=&PrgStack[I+1]->Prg;
          ExecuteCode(NextPrg);
          FilteredData=NextPrg->FilteredData;
          FilteredDataSize=NextPrg->FilteredDataSize;
          I++;
          delete PrgStack[I];
          PrgStack[I]=NULL;
        }
        UnpIO->UnpWrite(FilteredData,FilteredDataSize);
        UnpSomeRead=true;
        WrittenFileSize+=FilteredDataSize;
        WrittenBorder=BlockEnd;
        WriteSize=(UnpPtr-WrittenBorder)&MAXWINMASK;
      }
      else
      {
        for (int J=I;J<PrgStack.Size();J++)
        {
          UnpackFilter *flt=PrgStack[J];
          if (flt!=NULL && flt->NextWindow)
            flt->NextWindow=false;
        }
        WrPtr=WrittenBorder;
        return;
      }
    }
  }
   
  UnpWriteArea(WrittenBorder,UnpPtr);
  WrPtr=UnpPtr;
}


void Unpack::ExecuteCode(VM_PreparedProgram *Prg)
{
  if (Prg->GlobalData.Size()>0)
  {
    Prg->InitR[6]=int64to32(WrittenFileSize);
    VM.SetValue((uint *)&Prg->GlobalData[0x24],int64to32(WrittenFileSize));
    VM.SetValue((uint *)&Prg->GlobalData[0x28],int64to32(WrittenFileSize>>32));
    VM.Execute(Prg);
  }
}


void Unpack::UnpWriteArea(unsigned int StartPtr,unsigned int EndPtr)
{
  if (EndPtr!=StartPtr)
    UnpSomeRead=true;
  if (EndPtr<StartPtr)
  {
    UnpWriteData(&Window[StartPtr],-StartPtr & MAXWINMASK);
    UnpWriteData(Window,EndPtr);
    UnpAllBuf=true;
  }
  else
    UnpWriteData(&Window[StartPtr],EndPtr-StartPtr);
}


void Unpack::UnpWriteData(byte *Data,int Size)
{
  if (WrittenFileSize>=DestUnpSize)
    return;
  int WriteSize=Size;
  Int64 LeftToWrite=DestUnpSize-WrittenFileSize;
  if (WriteSize>LeftToWrite)
    WriteSize=int64to32(LeftToWrite);
  UnpIO->UnpWrite(Data,WriteSize);
  WrittenFileSize+=Size;
}


bool Unpack::ReadTables()
{
  byte BitLength[BC];
  unsigned char Table[HUFF_TABLE_SIZE];
  if (InAddr>ReadTop-25)
    if (!UnpReadBuf())
      return(false);
  faddbits((8-InBit)&7);
  unsigned int BitField=fgetbits();
  if (BitField & 0x8000)
  {
    UnpBlockType=BLOCK_PPM;
    return(PPM.DecodeInit(this,PPMEscChar));
  }
  UnpBlockType=BLOCK_LZ;
  
  PrevLowDist=0;
  LowDistRepCount=0;

  if (!(BitField & 0x4000))
    memset(UnpOldTable,0,sizeof(UnpOldTable));
  faddbits(2);

  for (uint I=0;I<BC;I++)
  {
    int Length=(byte)(fgetbits() >> 12);
    faddbits(4);
    if (Length==15)
    {
      int ZeroCount=(byte)(fgetbits() >> 12);
      faddbits(4);
      if (ZeroCount==0)
        BitLength[I]=15;
      else
      {
        ZeroCount+=2;
        while (ZeroCount-- > 0 && I<sizeof(BitLength)/sizeof(BitLength[0]))
          BitLength[I++]=0;
        I--;
      }
    }
    else
      BitLength[I]=Length;
  }
  MakeDecodeTables(BitLength,(struct Decode *)&BD,BC);

  const int TableSize=HUFF_TABLE_SIZE;
  for (int I=0;I<TableSize;)
  {
    if (InAddr>ReadTop-5)
      if (!UnpReadBuf())
        return(false);
    int Number=DecodeNumber((struct Decode *)&BD);
    if (Number<16)
    {
      Table[I]=(Number+UnpOldTable[I]) & 0xf;
      I++;
    }
    else
      if (Number<18)
      {
        int N;
        if (Number==16)
        {
          N=(fgetbits() >> 13)+3;
          faddbits(3);
        }
        else
        {
          N=(fgetbits() >> 9)+11;
          faddbits(7);
        }
        while (N-- > 0 && I<TableSize)
        {
          Table[I]=Table[I-1];
          I++;
        }
      }
      else
      {
        int N;
        if (Number==18)
        {
          N=(fgetbits() >> 13)+3;
          faddbits(3);
        }
        else
        {
          N=(fgetbits() >> 9)+11;
          faddbits(7);
        }
        while (N-- > 0 && I<TableSize)
          Table[I++]=0;
      }
  }
  TablesRead=true;
  if (InAddr>ReadTop)
    return(false);
  MakeDecodeTables(&Table[0],(struct Decode *)&LD,NC);
  MakeDecodeTables(&Table[NC],(struct Decode *)&DD,DC);
  MakeDecodeTables(&Table[NC+DC],(struct Decode *)&LDD,LDC);
  MakeDecodeTables(&Table[NC+DC+LDC],(struct Decode *)&RD,RC);
  memcpy(UnpOldTable,Table,sizeof(UnpOldTable));
  return(true);
}


void Unpack::UnpInitData(int Solid)
{
  if (!Solid)
  {
    TablesRead=false;
    memset(OldDist,0,sizeof(OldDist));
    OldDistPtr=0;
    LastDist=LastLength=0;
    if (UnpIO->UnpackToMemorySize > -1)
      memset(Window,0,MAXWINMEMSIZE);
    else
      memset(Window,0,MAXWINSIZE);
    memset(UnpOldTable,0,sizeof(UnpOldTable));
    UnpPtr=WrPtr=0;
    PPMEscChar=2;

    InitFilters();
  }
  InitBitInput();
  PPMError=false;
  WrittenFileSize=0;
  ReadTop=0;
  ReadBorder=0;
#ifndef SFX_MODULE
  UnpInitData20(Solid);
#endif
}


void Unpack::InitFilters()
{
  OldFilterLengths.Reset();
  LastFilter=0;

  for (int I=0;I<Filters.Size();I++)
    delete Filters[I];
  Filters.Reset();
  for (int I=0;I<PrgStack.Size();I++)
    delete PrgStack[I];
  PrgStack.Reset();
}


void Unpack::MakeDecodeTables(unsigned char *LenTab,struct Decode *Dec,int Size)
{
  int LenCount[16],TmpPos[16],I;
  long M,N;
  memset(LenCount,0,sizeof(LenCount));
  memset(Dec->DecodeNum,0,Size*sizeof(*Dec->DecodeNum));
  for (I=0;I<Size;I++)
    LenCount[LenTab[I] & 0xF]++;

  LenCount[0]=0;
  for (TmpPos[0]=Dec->DecodePos[0]=Dec->DecodeLen[0]=0,N=0,I=1;I<16;I++)
  {
    N=2*(N+LenCount[I]);
    M=N<<(15-I);
    if (M>0xFFFF)
      M=0xFFFF;
    Dec->DecodeLen[I]=(unsigned int)M;
    TmpPos[I]=Dec->DecodePos[I]=Dec->DecodePos[I-1]+LenCount[I-1];
  }

  for (I=0;I<Size;I++)
    if (LenTab[I]!=0)
      Dec->DecodeNum[TmpPos[LenTab[I] & 0xF]++]=I;
  Dec->MaxNum=Size;
}
