bool Archive::GetComment(Array<byte> &CmtData)
{
  if (!MainComment)
    return(false);
  SaveFilePos SavePos(*this);

  ushort CmtLength;
#ifndef SFX_MODULE
  if (OldFormat)
  {
    Seek(SFXSize+SIZEOF_OLDMHD,SEEK_SET);
    CmtLength=GetByte()+(GetByte()<<8);
  }
  else
#endif
  {
    if (NewMhd.Flags & MHD_COMMENT)
    {
      Seek(SFXSize+SIZEOF_MARKHEAD+SIZEOF_NEWMHD,SEEK_SET);
      ReadHeader();
    }
    else
    {
      Seek(SFXSize+SIZEOF_MARKHEAD+NewMhd.HeadSize,SEEK_SET);
      return(SearchSubBlock(SUBHEAD_TYPE_CMT)!=0 && ReadCommentData(CmtData)!=0);
    }
#ifndef SFX_MODULE
    if (CommHead.HeadCRC!=HeaderCRC)
    {
      Log(FileName,St(MLogCommHead));
      Alarm();
      return(false);
    }
    CmtLength=CommHead.HeadSize-SIZEOF_COMMHEAD;
#endif
  }
#ifndef SFX_MODULE
  if ((OldFormat && (OldMhd.Flags & MHD_PACK_COMMENT)) || (!OldFormat && CommHead.Method!=0x30))
  {
    if (!OldFormat && (CommHead.UnpVer < 15 || CommHead.UnpVer > UNP_VER || CommHead.Method > 0x35))
      return(false);
    ComprDataIO DataIO;
    Unpack Unpack(&DataIO);
    Unpack.Init();
    DataIO.SetTestMode(true);
    uint UnpCmtLength;
    if (OldFormat)
    {
      UnpCmtLength=GetByte()+(GetByte()<<8);
      CmtLength-=2;
      DataIO.SetCmt13Encryption();
    }
    else
      UnpCmtLength=CommHead.UnpSize;
    DataIO.SetFiles(this,NULL);
    DataIO.EnableShowProgress(false);
    DataIO.SetPackedSizeToRead(CmtLength);
    Unpack.SetDestSize(UnpCmtLength);
    Unpack.DoUnpack(CommHead.UnpVer,false);

    if (!OldFormat && ((~DataIO.UnpFileCRC)&0xffff)!=CommHead.CommCRC)
    {
      Log(FileName,St(MLogCommBrk));
      Alarm();
      return(false);
    }
    else
    {
      unsigned char *UnpData;
      uint UnpDataSize;
      DataIO.GetUnpackedData(&UnpData,&UnpDataSize);
      CmtData.Alloc(UnpDataSize);
      memcpy(&CmtData[0],UnpData,UnpDataSize);
    }
  }
  else
  {
    CmtData.Alloc(CmtLength);
    
    Read(&CmtData[0],CmtLength);
    if (!OldFormat && CommHead.CommCRC!=(~CRC(0xffffffff,&CmtData[0],CmtLength)&0xffff))
    {
      Log(FileName,St(MLogCommBrk));
      Alarm();
      CmtData.Reset();
      return(false);
    }
  }
#endif
#if defined(_WIN_32) && !defined(_WIN_CE) && !defined(_XBOX) && !defined(_LINUX)
  //if (CmtData.Size()>0)
  //  OemToCharBuff((char*)&CmtData[0],(char*)&CmtData[0],CmtData.Size());
#endif
  return(CmtData.Size()>0);
}


int Archive::ReadCommentData(Array<byte> &CmtData)
{
  bool Unicode=SubHead.SubFlags & SUBHEAD_FLAGS_CMT_UNICODE;
  if (!ReadSubData(&CmtData,NULL))
    return(0);
  int CmtSize=CmtData.Size();
  if (Unicode)
  {
    CmtSize/=2;
    Array<wchar> CmtDataW(CmtSize+1);
    RawToWide(&CmtData[0],&CmtDataW[0],CmtSize);
    CmtDataW[CmtSize]=0;
    CmtData.Alloc(CmtSize*2);
    WideToChar(&CmtDataW[0],(char *)&CmtData[0]);
    CmtSize=strlen((char *)&CmtData[0]);
    CmtData.Alloc(CmtSize);
  }
  return(CmtSize);
}


void Archive::ViewComment()
{
#ifndef GUI
  if (Cmd->DisableComment)
    return;
  Array<byte> CmtBuf;
  if (GetComment(CmtBuf))
  {
    int CmtSize=CmtBuf.Size();
    char *ChPtr=(char *)memchr(&CmtBuf[0],0x1A,CmtSize);
    if (ChPtr!=NULL)
      CmtSize=ChPtr-(char *)&CmtBuf[0];
    mprintf("\n");
    OutComment((char *)&CmtBuf[0],CmtSize);
  }
#endif
}


#ifndef SFX_MODULE
void Archive::ViewFileComment()
{
  if (!(NewLhd.Flags & LHD_COMMENT) || Cmd->DisableComment || OldFormat)
    return;
#ifndef GUI
  mprintf(St(MFileComment));
#endif
  const int MaxSize=0x8000;
  Array<char> CmtBuf(MaxSize);
  SaveFilePos SavePos(*this);
  Seek(CurBlockPos+SIZEOF_NEWLHD+NewLhd.NameSize,SEEK_SET);
  Int64 SaveCurBlockPos=CurBlockPos;
  Int64 SaveNextBlockPos=NextBlockPos;

  int Size=ReadHeader();

  CurBlockPos=SaveCurBlockPos;
  NextBlockPos=SaveNextBlockPos;

  if (Size<7 || CommHead.HeadType!=COMM_HEAD)
    return;
  if (CommHead.HeadCRC!=HeaderCRC)
  {
    #ifndef GUI
    Log(FileName,St(MLogCommHead));
#endif
    return;
  }
  if (CommHead.UnpVer < 15 || CommHead.UnpVer > UNP_VER ||
      CommHead.Method > 0x30 || CommHead.UnpSize > MaxSize)
    return;
  Read(&CmtBuf[0],CommHead.UnpSize);
  if (CommHead.CommCRC!=((~CRC(0xffffffff,&CmtBuf[0],CommHead.UnpSize)&0xffff)))
  {
    Log(FileName,St(MLogBrokFCmt));
  }
  else
  {
    OutComment(&CmtBuf[0],CommHead.UnpSize);
#ifndef GUI
    mprintf("\n");
#endif
  }
}
#endif
