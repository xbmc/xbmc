#include "rar.hpp"

RawRead::RawRead(File *SrcFile)
{
  RawRead::SrcFile=SrcFile;
  ReadPos=0;
  DataSize=0;
#ifndef SHELL_EXT
  Crypt=NULL;
#endif
}


void RawRead::Read(int Size)
{
#if !defined(SHELL_EXT) && !defined(NOCRYPT)
  if (Crypt!=NULL)
  {
    int CurSize=Data.Size();
    int SizeToRead=Size-(CurSize-DataSize);
    if (SizeToRead>0)
    {
      int AlignedReadSize=SizeToRead+((~SizeToRead+1)&0xf);
      Data.Add(AlignedReadSize);
      int ReadSize=SrcFile->Read(&Data[CurSize],AlignedReadSize);
      Crypt->DecryptBlock(&Data[CurSize],AlignedReadSize);
      DataSize+=ReadSize==0 ? 0:Size;
    }
    else
      DataSize+=Size;
  }
  else
#endif
    if (Size!=0)
    {
      Data.Add(Size);
      DataSize+=SrcFile->Read(&Data[DataSize],Size);
    }
}


void RawRead::Read(byte *SrcData,int Size)
{
  if (Size!=0)
  {
    Data.Add(Size);
    memcpy(&Data[DataSize],SrcData,Size);
    DataSize+=Size;
  }
}


void RawRead::Get(byte &Field)
{
  Field=Data[ReadPos];
  ReadPos++;
}


void RawRead::Get(ushort &Field)
{
  Field=Data[ReadPos]+(Data[ReadPos+1]<<8);
  ReadPos+=2;
}


void RawRead::Get(uint &Field)
{
  Field=Data[ReadPos]+(Data[ReadPos+1]<<8)+(Data[ReadPos+2]<<16)+
        (Data[ReadPos+3]<<24);
  ReadPos+=4;
}


void RawRead::Get8(Int64 &Field)
{
  uint Low,High;
  Get(Low);
  Get(High);
  Field=int32to64(High,Low);
}


void RawRead::Get(byte *Field,int Size)
{
  memcpy(Field,&Data[ReadPos],Size);
  ReadPos+=Size;
}


void RawRead::Get(wchar *Field,int Size)
{
  RawToWide(&Data[ReadPos],Field,Size);
  ReadPos+=2*Size;
}


uint RawRead::GetCRC(bool ProcessedOnly)
{
  return(DataSize>2 ? CRC(0xffffffff,&Data[2],(ProcessedOnly ? ReadPos:DataSize)-2):0xffffffff);
}
