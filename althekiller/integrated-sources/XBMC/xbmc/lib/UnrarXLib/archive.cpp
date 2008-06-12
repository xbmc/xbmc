#include "rar.hpp"

#ifndef SHELL_EXT
#include "arccmt.cpp"
#endif


Archive::Archive(RAROptions *InitCmd)
{
  Cmd=InitCmd==NULL ? &DummyCmd:InitCmd;
  OpenShared=Cmd->OpenShared;
  OldFormat=false;
  Solid=false;
  Volume=false;
  MainComment=false;
  Locked=false;
  Signed=false;
  NotFirstVolume=false;
  SFXSize=0;
  LatestTime.Reset();
  Protected=false;
  Encrypted=false;
  BrokenFileHeader=false;
  LastReadBlock=0;

  CurBlockPos=0;
  NextBlockPos=0;

  RecoveryPos=SIZEOF_MARKHEAD;
  RecoverySectors=-1;

  memset(&NewMhd,0,sizeof(NewMhd));
  NewMhd.HeadType=MAIN_HEAD;
  NewMhd.HeadSize=SIZEOF_NEWMHD;
  HeaderCRC=0;
  VolWrite=0;
  AddingFilesSize=0;
  AddingHeadersSize=0;
#if !defined(SHELL_EXT) && !defined(NOCRYPT)
  *HeadersSalt=0;
  *SubDataSalt=0;
#endif
  *FirstVolumeName=0;
  *FirstVolumeNameW=0;

  Splitting=false;
  NewArchive=false;

  SilentOpen=false;
}


#ifndef SHELL_EXT
void Archive::CheckArc(bool EnableBroken)
{
  if (!IsArchive(EnableBroken))
  {
    Log(FileName,St(MBadArc),FileName);
    ErrHandler.Exit(FATAL_ERROR);
  }
}
#endif


#if !defined(SHELL_EXT) && !defined(SFX_MODULE)
void Archive::CheckOpen(char *Name,wchar *NameW)
{
  TOpen(Name,NameW);
  CheckArc(false);
}
#endif


bool Archive::WCheckOpen(char *Name,wchar *NameW)
{
  if (!WOpen(Name,NameW))
    return(false);
  if (!IsArchive(false))
  {
#ifndef SHELL_EXT
    Log(FileName,St(MNotRAR),FileName);
#endif
    Close();
    return(false);
  }
  return(true);
}


bool Archive::IsSignature(byte *D)
{
  bool Valid=false;
  if (D[0]==0x52)
  {
#ifndef SFX_MODULE
    if (D[1]==0x45 && D[2]==0x7e && D[3]==0x5e)
    {
      OldFormat=true;
      Valid=true;
    }
    else
#endif
    {
      if (D[1]==0x61 && D[2]==0x72 && D[3]==0x21 && D[4]==0x1a && D[5]==0x07 && D[6]==0x00)
      {
        OldFormat=false;
        Valid=true;
      }
    }
  }
  return(Valid);
}


bool Archive::IsArchive(bool EnableBroken)
{
  Encrypted=false;
#ifndef SFX_MODULE
  if (IsDevice())
  {
#ifndef SHELL_EXT
    Log(FileName,St(MInvalidName),FileName);
#endif
    return(false);
  }
#endif
  if (Read(MarkHead.Mark,SIZEOF_MARKHEAD)!=SIZEOF_MARKHEAD)
    return(false);
    
  SFXSize=0;
  if (IsSignature(MarkHead.Mark))
  {
    if (OldFormat)
      Seek(0,SEEK_SET);
  }
  else
  {
    Array<char> Buffer(0x80000);
    long CurPos=int64to32(Tell());
    int ReadSize=Read(&Buffer[0],Buffer.Size()-16);
    for (int I=0;I<ReadSize;I++)
      if (Buffer[I]==0x52 && IsSignature((byte *)&Buffer[I]))
      {
        if (OldFormat && I>0 && CurPos<28 && ReadSize>31)
        {
          char *D=&Buffer[28-CurPos];
          if (D[0]!=0x52 || D[1]!=0x53 || D[2]!=0x46 || D[3]!=0x58)
            continue;
        }
        SFXSize=CurPos+I;
        Seek(SFXSize,SEEK_SET);
        if (!OldFormat)
          Read(MarkHead.Mark,SIZEOF_MARKHEAD);
        break;
      }
    if (SFXSize==0)
      return(false);
  }
  ReadHeader();
  SeekToNext();
#ifndef SFX_MODULE
  if (OldFormat)
  {
    NewMhd.Flags=OldMhd.Flags & 0x3f;
    NewMhd.HeadSize=OldMhd.HeadSize;
  }
  else
#endif
  {
    if (HeaderCRC!=NewMhd.HeadCRC)
    {
#ifndef SHELL_EXT
      Log(FileName,St(MLogMainHead));
#endif
      Alarm();
      if (!EnableBroken)
        return(false);
    }
  }
  Volume=(NewMhd.Flags & MHD_VOLUME);
  Solid=(NewMhd.Flags & MHD_SOLID)!=0;
  MainComment=(NewMhd.Flags & MHD_COMMENT)!=0;
  Locked=(NewMhd.Flags & MHD_LOCK)!=0;
  Signed=(NewMhd.PosAV!=0);
  Protected=(NewMhd.Flags & MHD_PROTECT)!=0;
  Encrypted=(NewMhd.Flags & MHD_PASSWORD)!=0;

#ifdef RARDLL
  SilentOpen=true;
#endif
  if (!SilentOpen || !Encrypted)
  {
    SaveFilePos SavePos(*this);
    Int64 SaveCurBlockPos=CurBlockPos,SaveNextBlockPos=NextBlockPos;

    NotFirstVolume=false;
    while (ReadHeader())
    {
      int HeaderType=GetHeaderType();
      if (HeaderType==NEWSUB_HEAD)
      {
        if (SubHead.CmpName(SUBHEAD_TYPE_CMT))
          MainComment=true;
        if ((SubHead.Flags & LHD_SPLIT_BEFORE) ||
            (Volume && (NewMhd.Flags & MHD_FIRSTVOLUME)==0))
          NotFirstVolume=true;
      }
      else
      {
        if ((HeaderType==FILE_HEAD && ((NewLhd.Flags & LHD_SPLIT_BEFORE)!=0)) ||
            (Volume && NewLhd.UnpVer>=29 && (NewMhd.Flags & MHD_FIRSTVOLUME)==0))
          NotFirstVolume=true;
        break;
      }
      SeekToNext();
    }
    CurBlockPos=SaveCurBlockPos;
    NextBlockPos=SaveNextBlockPos;
  }
  return(true);
}




void Archive::SeekToNext()
{
  Seek(NextBlockPos,SEEK_SET);
}


#ifndef SFX_MODULE
int Archive::GetRecoverySize(bool Required)
{
  if (!Protected)
    return(0);
  if (RecoverySectors!=-1 || !Required)
    return(RecoverySectors);
  SaveFilePos SavePos(*this);
  Seek(SFXSize,SEEK_SET);
  SearchSubBlock(SUBHEAD_TYPE_RR);
  return(RecoverySectors);
}
#endif


