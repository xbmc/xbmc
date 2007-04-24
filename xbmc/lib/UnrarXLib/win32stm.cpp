// THIS FILE IS SLIGHTLY MODIFIED TO WORK WITH XBMC

#ifndef SFX_MODULE
void ExtractStreams(Archive &Arc,char *FileName,wchar *FileNameW)
{
  if (!WinNT())
    return;

  if (Arc.HeaderCRC!=Arc.StreamHead.HeadCRC)
  {
#ifndef SILENT
    Log(Arc.FileName,St(MStreamBroken),FileName);
#endif
    ErrHandler.SetErrorCode(CRC_ERROR);
    return;
  }

  if (Arc.StreamHead.Method<0x31 || Arc.StreamHead.Method>0x35 || Arc.StreamHead.UnpVer>PACK_VER)
  {
#ifndef SILENT
    Log(Arc.FileName,St(MStreamUnknown),FileName);
#endif
    ErrHandler.SetErrorCode(WARNING);
    return;
  }

  char StreamName[NM+2];
  if (FileName[0]!=0 && FileName[1]==0)
  {
    strcpy(StreamName,".\\");
    strcpy(StreamName+2,FileName);
  }
  else
    strcpy(StreamName,FileName);
  if (strlen(StreamName)+strlen((char *)Arc.StreamHead.StreamName)>=sizeof(StreamName))
  {
#ifndef SILENT
    Log(Arc.FileName,St(MStreamBroken),FileName);
#endif
    ErrHandler.SetErrorCode(CRC_ERROR);
    return;
  }

  strcat(StreamName,(char *)Arc.StreamHead.StreamName);

  FindData fd;
  bool Found=FindFile::FastFind(FileName,FileNameW,&fd);

  if (fd.FileAttr & FILE_ATTRIBUTE_READONLY)
    SetFileAttr(FileName,FileNameW,fd.FileAttr & ~FILE_ATTRIBUTE_READONLY);

  File CurFile;
  if (CurFile.WCreate(StreamName))
  {
    ComprDataIO DataIO;
    Unpack Unpack(&DataIO);
    Unpack.Init();

    Array<unsigned char> UnpData(Arc.StreamHead.UnpSize);
    DataIO.SetPackedSizeToRead(Arc.StreamHead.DataSize);
    DataIO.EnableShowProgress(false);
    DataIO.SetFiles(&Arc,&CurFile);
    Unpack.SetDestSize(Arc.StreamHead.UnpSize);
    Unpack.DoUnpack(Arc.StreamHead.UnpVer,false);

    if (Arc.StreamHead.StreamCRC!=~DataIO.UnpFileCRC)
    {
#ifndef SILENT
      Log(Arc.FileName,St(MStreamBroken),StreamName);
#endif
      ErrHandler.SetErrorCode(CRC_ERROR);
    }
    else
      CurFile.Close();
  }
  File HostFile;
  if (Found && HostFile.Open(FileName,FileNameW,true,true))
    /*SetFileTime(HostFile.GetHandle(),&fd.ftCreationTime,&fd.ftLastAccessTime,
                &fd.ftLastWriteTime);*/
  if (fd.FileAttr & FILE_ATTRIBUTE_READONLY)
    SetFileAttr(FileName,FileNameW,fd.FileAttr);
}
#endif


void ExtractStreamsNew(Archive &Arc,char *FileName,wchar *FileNameW)
{
  if (!WinNT())
    return;

  wchar NameW[NM];
  if (FileNameW!=NULL && *FileNameW!=0)
    strcpyw(NameW,FileNameW);
  else
    CharToWide(FileName,NameW);
  wchar StreamNameW[NM+2];
  if (NameW[0]!=0 && NameW[1]==0)
  {
    strcpyw(StreamNameW,L".\\");
    strcpyw(StreamNameW+2,NameW);
  }
  else
    strcpyw(StreamNameW,NameW);

  wchar *DestName=StreamNameW+strlenw(StreamNameW);
  byte *SrcName=&Arc.SubHead.SubData[0];
  int DestSize=Arc.SubHead.SubData.Size()/2;

  if (strlenw(StreamNameW)+DestSize>=sizeof(StreamNameW)/sizeof(StreamNameW[0]))
  {
#if !defined(SILENT) && !defined(SFX_MODULE)
    Log(Arc.FileName,St(MStreamBroken),FileName);
#endif
    ErrHandler.SetErrorCode(CRC_ERROR);
    return;
  }

  RawToWide(SrcName,DestName,DestSize);
  DestName[DestSize]=0;

  FindData fd;
  bool Found=FindFile::FastFind(FileName,FileNameW,&fd);

  if (fd.FileAttr & FILE_ATTRIBUTE_READONLY)
    SetFileAttr(FileName,FileNameW,fd.FileAttr & ~FILE_ATTRIBUTE_READONLY);
  char StreamName[NM];
  WideToChar(StreamNameW,StreamName);
  File CurFile;
  if (CurFile.WCreate(StreamName,StreamNameW) && Arc.ReadSubData(NULL,&CurFile))
    CurFile.Close();
  File HostFile;
  if (Found && HostFile.Open(FileName,FileNameW,true,true))
/*    SetFileTime(HostFile.GetHandle(),&fd.ftCreationTime,&fd.ftLastAccessTime,
                &fd.ftLastWriteTime);*/
  if (fd.FileAttr & FILE_ATTRIBUTE_READONLY)
    SetFileAttr(FileName,FileNameW,fd.FileAttr);
}
