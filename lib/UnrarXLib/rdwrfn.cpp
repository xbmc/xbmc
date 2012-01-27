#include "rar.hpp"
#include "URL.h"
#include "dialogs/GUIDialogProgress.h"

ComprDataIO::ComprDataIO()
{
  Init();
}


void ComprDataIO::Init()
{
  UnpackFromMemory=false;
  UnpackToMemory=false;
  UnpackToMemorySize=-1;
  UnpPackedSize=0;
  ShowProgress=true;
  TestMode=false;
  SkipUnpCRC=true;
  PackVolume=false;
  UnpVolume=false;
  NextVolumeMissing=false;
  SrcFile=NULL;
  DestFile=NULL;
  UnpWrSize=0;
  Command=NULL;
  Encryption=0;
  Decryption=0;
  TotalPackRead=0;
  CurPackRead=CurPackWrite=CurUnpRead=CurUnpWrite=CurUnpStart=0;
  PackFileCRC=UnpFileCRC=PackedCRC=0xffffffff;
  LastPercent=-1;
  SubHead=NULL;
  SubHeadPos=NULL;
  CurrentCommand=0;
  ProcessedArcSize=TotalArcSize=0;
  bQuit = false;
  m_pDlgProgress = NULL;
 }

int ComprDataIO::UnpRead(byte *Addr,uint Count)
{
  int RetCode=0,TotalRead=0;
  byte *ReadAddr;
  ReadAddr=Addr;
  while (Count > 0)
  {
    Archive *SrcArc=(Archive *)SrcFile;

    uint ReadSize=(Count>UnpPackedSize) ? int64to32(UnpPackedSize):Count;
    if (UnpackFromMemory)
    {
      memcpy(Addr,UnpackFromMemoryAddr,UnpackFromMemorySize);
      RetCode=UnpackFromMemorySize;
      UnpackFromMemorySize=0;
    }
    else
    {
      bool bRead = true;
      if (!SrcFile->IsOpened())
      {
        NextVolumeMissing = true;
        return(-1);
      }
      if (UnpackToMemory)
        if (hSeek->WaitMSec(1)) // we are seeking
        {
          if (m_iSeekTo > CurUnpStart+SrcArc->NewLhd.FullPackSize) // need to seek outside this block
          {
            TotalRead += (int)(SrcArc->NextBlockPos-SrcFile->Tell());
            CurUnpRead=CurUnpStart+SrcArc->NewLhd.FullPackSize;
            UnpPackedSize=0;
            RetCode = 0;
            bRead = false;
          }
          else
          {
            Int64 iStartOfFile = SrcArc->NextBlockPos-SrcArc->NewLhd.FullPackSize;
            m_iStartOfBuffer = CurUnpStart;
            Int64 iSeekTo=m_iSeekTo-CurUnpStart<MAXWINMEMSIZE/2?iStartOfFile:iStartOfFile+m_iSeekTo-CurUnpStart-MAXWINMEMSIZE/2;
            if (iSeekTo == iStartOfFile) // front
            {
              if (CurUnpStart+MAXWINMEMSIZE>SrcArc->NewLhd.FullUnpSize)
              {
                m_iSeekTo=iStartOfFile;
                UnpPackedSize = SrcArc->NewLhd.FullPackSize;
              }
              else 
              {
                m_iSeekTo=MAXWINMEMSIZE-(m_iSeekTo-CurUnpStart);
                UnpPackedSize = SrcArc->NewLhd.FullPackSize - (m_iStartOfBuffer - CurUnpStart);
              }
            }
            else
            {
              m_iStartOfBuffer = m_iSeekTo-MAXWINMEMSIZE/2; // front
              if (m_iSeekTo+MAXWINMEMSIZE/2>SrcArc->NewLhd.FullUnpSize)
              {
                iSeekTo = iStartOfFile+SrcArc->NewLhd.FullPackSize-MAXWINMEMSIZE;
                m_iStartOfBuffer = CurUnpStart+SrcArc->NewLhd.FullPackSize-MAXWINMEMSIZE;
                m_iSeekTo = MAXWINMEMSIZE-(m_iSeekTo-m_iStartOfBuffer);
                UnpPackedSize = MAXWINMEMSIZE;
              }
              else 
              {
                m_iSeekTo=MAXWINMEMSIZE/2;
                UnpPackedSize = SrcArc->NewLhd.FullPackSize - (m_iStartOfBuffer - CurUnpStart);
              }  
            }

            SrcFile->Seek(iSeekTo,SEEK_SET);
            TotalRead = 0;
            CurUnpRead = CurUnpStart + iSeekTo - iStartOfFile;
            CurUnpWrite = SrcFile->Tell() - iStartOfFile + CurUnpStart;
            
            hSeek->Reset();
            hSeekDone->Set();
          }
        }
      if (bRead)
      {
        ReadSize=(Count>UnpPackedSize) ? int64to32(UnpPackedSize):Count;
        RetCode=SrcFile->Read(ReadAddr,ReadSize);
        FileHeader *hd=SubHead!=NULL ? SubHead:&SrcArc->NewLhd;
        if (hd->Flags & LHD_SPLIT_AFTER)
        {
          PackedCRC=CRC(PackedCRC,ReadAddr,ReadSize);
        }
      }
    }
    CurUnpRead+=RetCode;
    ReadAddr+=RetCode;
    TotalRead+=RetCode;
    Count-=RetCode;
    UnpPackedSize-=RetCode;
    if (UnpPackedSize == 0 && UnpVolume)
    {
#ifndef NOVOLUME
      if (!MergeArchive(*SrcArc,this,true,CurrentCommand))
#endif
      {
        NextVolumeMissing=true;
        return(-1);
      }
      CurUnpStart = CurUnpRead;
      if (m_pDlgProgress)
      {
        CURL url(SrcArc->FileName);
        m_pDlgProgress->SetLine(0,url.GetWithoutUserDetails()); // update currently extracted rar file
        m_pDlgProgress->Progress();
      }
    }
    else
      break;
  }
  Archive *SrcArc=(Archive *)SrcFile;
  if (SrcArc!=NULL)
    ShowUnpRead(SrcArc->CurBlockPos+CurUnpRead,UnpArcSize);
  if (RetCode!=-1)
  {
    RetCode=TotalRead;
#ifndef NOCRYPT
    if (Decryption)
    {
#ifndef SFX_MODULE
      if (Decryption<20)
        Decrypt.Crypt(Addr,RetCode,(Decryption==15) ? NEW_CRYPT : OLD_DECODE);
      else if (Decryption==20)
        for (int I=0;I<RetCode;I+=16)
          Decrypt.DecryptBlock20(&Addr[I]);
      else
#endif
      {
        int CryptSize=(RetCode & 0xf)==0 ? RetCode:((RetCode & ~0xf)+16);
        Decrypt.DecryptBlock(Addr,CryptSize);
      }
    }
#endif
  }
  Wait();
  return(RetCode);
}

void ComprDataIO::UnpWrite(byte *Addr,uint Count)
{
#ifdef RARDLL
  RAROptions *Cmd=((Archive *)SrcFile)->GetRAROptions();
  if (Cmd->DllOpMode!=RAR_SKIP)
  {
    if (Cmd->Callback!=NULL &&
        Cmd->Callback(UCM_PROCESSDATA,Cmd->UserData,(LONG)Addr,Count)==-1)
      ErrHandler.Exit(USER_BREAK);
    if (Cmd->ProcessDataProc!=NULL)
    {
#ifdef _WIN_32
      _EBX=_ESP;
#endif
      int RetCode=Cmd->ProcessDataProc(Addr,Count);
#ifdef _WIN_32
      _ESP=_EBX;
#endif
      if (RetCode==0)
        ErrHandler.Exit(USER_BREAK);
    }
  }
#endif
  UnpWrAddr=Addr;
  UnpWrSize=Count;
  if (UnpackToMemory)
  {
    while(UnpackToMemorySize < (int)Count)
    {
      hBufferEmpty->Set();
      while(! hBufferFilled->WaitMSec(1)) 
        if (hQuit->WaitMSec(1))
          return;
    }
    
    if (! hSeek->WaitMSec(1)) // we are seeking
    {
      memcpy(UnpackToMemoryAddr,Addr,Count);
      UnpackToMemoryAddr+=Count;
      UnpackToMemorySize-=Count;
    }
    else
      return;
  }
  else
    if (!TestMode)
      DestFile->Write(Addr,Count);
  
  CurUnpWrite+=Count;
  if (!SkipUnpCRC)
  {
#ifndef SFX_MODULE
    if (((Archive *)SrcFile)->OldFormat)
      UnpFileCRC=OldCRC((ushort)UnpFileCRC,Addr,Count);
    else
#endif
      UnpFileCRC=CRC(UnpFileCRC,Addr,Count);
  }
  ShowUnpWrite();
  Wait();
  if (m_pDlgProgress)
  {
    m_pDlgProgress->ShowProgressBar(true);
    m_pDlgProgress->SetPercentage(int(float(CurUnpWrite)/float(((Archive*)SrcFile)->NewLhd.FullUnpSize)*100));
    m_pDlgProgress->Progress();
    if (m_pDlgProgress->IsCanceled()) 
      bQuit = true;
  }
}






void ComprDataIO::ShowUnpRead(Int64 ArcPos,Int64 ArcSize)
{
  if (ShowProgress && SrcFile!=NULL)
  {
    Archive *SrcArc=(Archive *)SrcFile;
    RAROptions *Cmd=SrcArc->GetRAROptions();
    if (TotalArcSize!=0)
      ArcSize=TotalArcSize;
    ArcPos+=ProcessedArcSize;
    if (!SrcArc->Volume)
    {
      int CurPercent=ToPercent(ArcPos,ArcSize);
      if (!Cmd->DisablePercentage && CurPercent!=LastPercent)
      {
        mprintf("\b\b\b\b%3d%%",CurPercent);
        LastPercent=CurPercent;
      }
    }
  }
}


void ComprDataIO::ShowUnpWrite()
{
}








void ComprDataIO::SetFiles(File *SrcFile,File *DestFile)
{
  if (SrcFile!=NULL)
    ComprDataIO::SrcFile=SrcFile;
  if (DestFile!=NULL)
    ComprDataIO::DestFile=DestFile;
  LastPercent=-1;
}


void ComprDataIO::GetUnpackedData(byte **Data,uint *Size)
{
  *Data=UnpWrAddr;
  *Size=UnpWrSize;
}


void ComprDataIO::SetEncryption(int Method,char *Password,byte *Salt,bool Encrypt)
{
  if (Encrypt)
  {
    Encryption=*Password ? Method:0;
#ifndef NOCRYPT
    Crypt.SetCryptKeys(Password,Salt,Encrypt);
#endif
  }
  else
  {
    Decryption=*Password ? Method:0;
#ifndef NOCRYPT
    Decrypt.SetCryptKeys(Password,Salt,Encrypt,Method<29);
#endif
  }
}


#ifndef SFX_MODULE
void ComprDataIO::SetAV15Encryption()
{
  Decryption=15;
  Decrypt.SetAV15Encryption();
}
#endif


#ifndef SFX_MODULE
void ComprDataIO::SetCmt13Encryption()
{
  Decryption=13;
  Decrypt.SetCmt13Encryption();
}
#endif




void ComprDataIO::SetUnpackToMemory(byte *Addr,uint Size)
{
  UnpackToMemory=true;
  UnpackToMemoryAddr=Addr;
  UnpackToMemorySize=Size;
}
