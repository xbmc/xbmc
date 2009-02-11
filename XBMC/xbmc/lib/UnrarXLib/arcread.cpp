#include "rar.hpp"

int Archive::SearchBlock(int BlockType)
{
  int Size,Count=0;
  while ((Size=ReadHeader())!=0 &&
         (BlockType==ENDARC_HEAD || GetHeaderType()!=ENDARC_HEAD))
  {
    if ((++Count & 127)==0)
      Wait();
    if (GetHeaderType()==BlockType)
      return(Size);
    SeekToNext();
  }
  return(0);
}


int Archive::SearchSubBlock(const char *Type)
{
  int Size;
  while ((Size=ReadHeader())!=0 && GetHeaderType()!=ENDARC_HEAD)
  {
    if (GetHeaderType()==NEWSUB_HEAD && SubHead.CmpName(Type))
      return(Size);
    SeekToNext();
  }
  return(0);
}


int Archive::ReadHeader()
{
  CurBlockPos=Tell();

#ifndef SFX_MODULE
  if (OldFormat)
    return(ReadOldHeader());
#endif

  RawRead Raw(this);

  bool Decrypt=Encrypted && CurBlockPos>=SFXSize+SIZEOF_MARKHEAD+SIZEOF_NEWMHD;

  if (Decrypt)
  {
#if defined(SHELL_EXT) || defined(NOCRYPT)
    return(0);
#else
    if (Read(HeadersSalt,SALT_SIZE)!=SALT_SIZE)
      return(0);
    if (*Cmd->Password==0)
#ifdef RARDLL
      if (Cmd->Callback==NULL ||
          Cmd->Callback(UCM_NEEDPASSWORD,Cmd->UserData,(LONG)Cmd->Password,sizeof(Cmd->Password))==-1)
      {
        Close();
        ErrHandler.Exit(USER_BREAK);
      }

#else
      if (!GetPassword(PASSWORD_ARCHIVE,FileName,Cmd->Password,sizeof(Cmd->Password)))
      {
        Close();
        ErrHandler.Exit(USER_BREAK);
      }
#endif
    HeadersCrypt.SetCryptKeys(Cmd->Password,HeadersSalt,false);
    Raw.SetCrypt(&HeadersCrypt);
#endif
  }

  Raw.Read(SIZEOF_SHORTBLOCKHEAD);
  if (Raw.Size()==0)
  {
    Int64 ArcSize=FileLength();
    if (CurBlockPos>ArcSize || NextBlockPos>ArcSize)
    {
  #ifndef SHELL_EXT
      Log(FileName,St(MLogUnexpEOF));
  #endif
      ErrHandler.SetErrorCode(WARNING);
    }
    return(0);
  }

  Raw.Get(ShortBlock.HeadCRC);
  byte HeadType;
  Raw.Get(HeadType);
  ShortBlock.HeadType=(HEADER_TYPE)HeadType;
  Raw.Get(ShortBlock.Flags);
  Raw.Get(ShortBlock.HeadSize);
  if (ShortBlock.HeadSize<SIZEOF_SHORTBLOCKHEAD)
  {
#ifndef SHELL_EXT
    Log(FileName,St(MLogFileHead),"???");
#endif
    BrokenFileHeader=true;
    ErrHandler.SetErrorCode(CRC_ERROR);
    return(0);
  }

  if (ShortBlock.HeadType==COMM_HEAD)
    Raw.Read(SIZEOF_COMMHEAD-SIZEOF_SHORTBLOCKHEAD);
  else
    if (ShortBlock.HeadType==MAIN_HEAD && (ShortBlock.Flags & MHD_COMMENT)!=0)
      Raw.Read(SIZEOF_NEWMHD-SIZEOF_SHORTBLOCKHEAD);
    else
      Raw.Read(ShortBlock.HeadSize-SIZEOF_SHORTBLOCKHEAD);

  NextBlockPos=CurBlockPos+ShortBlock.HeadSize;

  switch(ShortBlock.HeadType)
  {
    case MAIN_HEAD:
      *(BaseBlock *)&NewMhd=ShortBlock;
      Raw.Get(NewMhd.HighPosAV);
      Raw.Get(NewMhd.PosAV);
      break;
    case ENDARC_HEAD:
      *(BaseBlock *)&EndArcHead=ShortBlock;
      if (EndArcHead.Flags & EARC_DATACRC)
        Raw.Get(EndArcHead.ArcDataCRC);
    if (EndArcHead.Flags & EARC_VOLNUMBER)
        Raw.Get(EndArcHead.VolNumber);
      break;
    case FILE_HEAD:
    case NEWSUB_HEAD:
      {
        FileHeader *hd=ShortBlock.HeadType==FILE_HEAD ? &NewLhd:&SubHead;
        *(BaseBlock *)hd=ShortBlock;
        Raw.Get(hd->PackSize);
        Raw.Get(hd->UnpSize);
        Raw.Get(hd->HostOS);
        Raw.Get(hd->FileCRC);
        Raw.Get(hd->FileTime);
        Raw.Get(hd->UnpVer);
        Raw.Get(hd->Method);
        Raw.Get(hd->NameSize);
        Raw.Get(hd->FileAttr);
        if (hd->Flags & LHD_LARGE)
        {
          Raw.Get(hd->HighPackSize);
          Raw.Get(hd->HighUnpSize);
        }
        else 
    {
          hd->HighPackSize=hd->HighUnpSize=0;
          if (hd->UnpSize==0xffffffff)
          {
            hd->UnpSize=int64to32(INT64MAX);
            hd->HighUnpSize=int64to32(INT64MAX>>32);
          }
        }
        hd->FullPackSize=int32to64(hd->HighPackSize,hd->PackSize);
        hd->FullUnpSize=int32to64(hd->HighUnpSize,hd->UnpSize);

        char FileName[NM*4];
        int NameSize=Min(hd->NameSize,sizeof(FileName)-1);
        Raw.Get((byte *)FileName,NameSize);
        FileName[NameSize]=0;

        strncpy(hd->FileName,FileName,sizeof(hd->FileName));
        hd->FileName[sizeof(hd->FileName)-1]=0;

        if (hd->HeadType==NEWSUB_HEAD)
        {
          int DataSize=hd->HeadSize-hd->NameSize-SIZEOF_NEWLHD;
          if (hd->Flags & LHD_SALT)
            DataSize-=SALT_SIZE;
          if (DataSize>0)
          {
            hd->SubData.Alloc(DataSize);
            Raw.Get(&hd->SubData[0],DataSize);
            if (hd->CmpName(SUBHEAD_TYPE_RR))
            {
              byte *D=&hd->SubData[8];
              RecoverySectors=D[0]+((uint)D[1]<<8)+((uint)D[2]<<16)+((uint)D[3]<<24);
            }
          }
        }
        else
          if (hd->HeadType==FILE_HEAD)
          {
            if (hd->Flags & LHD_UNICODE)
            {
              EncodeFileName NameCoder;
              int Length=strlen(FileName)+1;
              NameCoder.Decode(FileName,(byte *)FileName+Length,
                               hd->NameSize-Length,hd->FileNameW,
                               sizeof(hd->FileNameW)/sizeof(hd->FileNameW[0]));
              if (*hd->FileNameW==0)
                hd->Flags &= ~LHD_UNICODE;
            }
            else
              *hd->FileNameW=0;
#ifndef SFX_MODULE
            ConvertNameCase(hd->FileName);
            ConvertNameCase(hd->FileNameW);
#endif
            ConvertUnknownHeader();
          }
        if (hd->Flags & LHD_SALT)
          Raw.Get(hd->Salt,SALT_SIZE);
        hd->mtime.SetDos(hd->FileTime);
        hd->ctime.Reset();
        hd->atime.Reset();
        hd->arctime.Reset();
        if (hd->Flags & LHD_EXTTIME)
        {
          ushort Flags;
          Raw.Get(Flags);
          RarTime *tbl[4];
          tbl[0]=&NewLhd.mtime;
          tbl[1]=&NewLhd.ctime;
          tbl[2]=&NewLhd.atime;
          tbl[3]=&NewLhd.arctime;
          for (int I=0;I<4;I++)
          {
            RarTime *CurTime=tbl[I];
            uint rmode=Flags>>(3-I)*4;
            if ((rmode & 8)==0)
              continue;
            if (I!=0)
            {
              uint DosTime;
              Raw.Get(DosTime);
              CurTime->SetDos(DosTime);
            }
            RarLocalTime rlt;
            CurTime->GetLocal(&rlt);
            if (rmode & 4)
              rlt.Second++;
            rlt.Reminder=0;
            int count=rmode&3;
            for (int J=0;J<count;J++)
            {
              byte CurByte;
              Raw.Get(CurByte);
              rlt.Reminder|=(((uint)CurByte)<<((J+3-count)*8));
            }
            CurTime->SetLocal(&rlt);
          }
        }
        NextBlockPos+=hd->FullPackSize;
        bool CRCProcessedOnly=(hd->Flags & LHD_COMMENT)!=0;
        HeaderCRC = ~Raw.GetCRC(CRCProcessedOnly) & 0xffff;
        if (hd->HeadCRC!=HeaderCRC)
        {
          if (hd->HeadType==NEWSUB_HEAD)
            strcat(hd->FileName,"- ???");
          BrokenFileHeader=true;
          ErrHandler.SetErrorCode(WARNING);
#ifndef SHELL_EXT
          Log(Archive::FileName,St(MLogFileHead),IntNameToExt(hd->FileName));
          Alarm();
#endif
        }
      }
      break;
#ifndef SFX_MODULE
    case COMM_HEAD:
      *(BaseBlock *)&CommHead=ShortBlock;
      Raw.Get(CommHead.UnpSize);
      Raw.Get(CommHead.UnpVer);
      Raw.Get(CommHead.Method);
      Raw.Get(CommHead.CommCRC);
      break;
    case SIGN_HEAD:
      *(BaseBlock *)&SignHead=ShortBlock;
      Raw.Get(SignHead.CreationTime);
      Raw.Get(SignHead.ArcNameSize);
      Raw.Get(SignHead.UserNameSize);
      break;
    case AV_HEAD:
      *(BaseBlock *)&AVHead=ShortBlock;
      Raw.Get(AVHead.UnpVer);
      Raw.Get(AVHead.Method);
      Raw.Get(AVHead.AVVer);
      Raw.Get(AVHead.AVInfoCRC);
      break;
    case PROTECT_HEAD:
      *(BaseBlock *)&ProtectHead=ShortBlock;
      Raw.Get(ProtectHead.DataSize);
      Raw.Get(ProtectHead.Version);
      Raw.Get(ProtectHead.RecSectors);
      Raw.Get(ProtectHead.TotalBlocks);
      Raw.Get(ProtectHead.Mark,8);
      NextBlockPos+=ProtectHead.DataSize;
      RecoverySectors=ProtectHead.RecSectors;
      break;
    case SUB_HEAD:
      *(BaseBlock *)&SubBlockHead=ShortBlock;
      Raw.Get(SubBlockHead.DataSize);
      NextBlockPos+=SubBlockHead.DataSize;
      Raw.Get(SubBlockHead.SubType);
      Raw.Get(SubBlockHead.Level);
      switch(SubBlockHead.SubType)
      {
        case UO_HEAD:
          *(SubBlockHeader *)&UOHead=SubBlockHead;
          Raw.Get(UOHead.OwnerNameSize);
          Raw.Get(UOHead.GroupNameSize);
          if (UOHead.OwnerNameSize>NM-1)
            UOHead.OwnerNameSize=NM-1;
          if (UOHead.GroupNameSize>NM-1)
            UOHead.GroupNameSize=NM-1;
          Raw.Get((byte *)UOHead.OwnerName,UOHead.OwnerNameSize);
          Raw.Get((byte *)UOHead.GroupName,UOHead.GroupNameSize);
          UOHead.OwnerName[UOHead.OwnerNameSize]=0;
          UOHead.GroupName[UOHead.GroupNameSize]=0;
          break;
        case MAC_HEAD:
          *(SubBlockHeader *)&MACHead=SubBlockHead;
          Raw.Get(MACHead.fileType);
          Raw.Get(MACHead.fileCreator);
          break;
        case EA_HEAD:
        case BEEA_HEAD:
        case NTACL_HEAD:
          *(SubBlockHeader *)&EAHead=SubBlockHead;
          Raw.Get(EAHead.UnpSize);
          Raw.Get(EAHead.UnpVer);
          Raw.Get(EAHead.Method);
          Raw.Get(EAHead.EACRC);
          break;
        case STREAM_HEAD:
          *(SubBlockHeader *)&StreamHead=SubBlockHead;
          Raw.Get(StreamHead.UnpSize);
          Raw.Get(StreamHead.UnpVer);
          Raw.Get(StreamHead.Method);
          Raw.Get(StreamHead.StreamCRC);
          Raw.Get(StreamHead.StreamNameSize);
          if (StreamHead.StreamNameSize>NM-1)
            StreamHead.StreamNameSize=NM-1;
          Raw.Get((byte *)StreamHead.StreamName,StreamHead.StreamNameSize);
          StreamHead.StreamName[StreamHead.StreamNameSize]=0;
          break;
      }
      break;
#endif
    default:
      if (ShortBlock.Flags & LONG_BLOCK)
      {
        uint DataSize;
        Raw.Get(DataSize);
        NextBlockPos+=DataSize;
      }
      break;
  }
  HeaderCRC=~Raw.GetCRC(false)&0xffff;
  CurHeaderType=ShortBlock.HeadType;
  if (Decrypt)
  {
    NextBlockPos+=Raw.PaddedSize()+SALT_SIZE;

    if (ShortBlock.HeadCRC!=HeaderCRC)
    {
      bool Recovered=false;
      if (ShortBlock.HeadType==ENDARC_HEAD && (EndArcHead.Flags & EARC_REVSPACE)!=0)
      {
        SaveFilePos SavePos(*this);
        Int64 Length=Tell();
        Seek(Length-7,SEEK_SET);
        Recovered=true;
        for (int J=0;J<7;J++)
          if (GetByte()!=0)
            Recovered=false;
      }
      if (!Recovered)
      {
#ifndef SILENT
        Log(FileName,St(MEncrBadCRC),FileName);
#endif
        Close();

        BrokenFileHeader=true;
        ErrHandler.SetErrorCode(CRC_ERROR);
        return(0);
//        ErrHandler.Exit(CRC_ERROR);
      }
    }
  }

  if (NextBlockPos<=CurBlockPos)
  {
#ifndef SHELL_EXT
    Log(FileName,St(MLogFileHead),"???");
#endif
    BrokenFileHeader=true;
    ErrHandler.SetErrorCode(CRC_ERROR);
    return(0);
  }
  return(Raw.Size());
}


#ifndef SFX_MODULE
int Archive::ReadOldHeader()
{
  RawRead Raw(this);
  if (CurBlockPos<=SFXSize)
  {
    Raw.Read(SIZEOF_OLDMHD);
    Raw.Get(OldMhd.Mark,4);
    Raw.Get(OldMhd.HeadSize);
    Raw.Get(OldMhd.Flags);
    NextBlockPos=CurBlockPos+OldMhd.HeadSize;
    CurHeaderType=MAIN_HEAD;
  }
  else
  {
    OldFileHeader OldLhd;
    Raw.Read(SIZEOF_OLDLHD);
    NewLhd.HeadType=FILE_HEAD;
    Raw.Get(NewLhd.PackSize);
    Raw.Get(NewLhd.UnpSize);
    Raw.Get(OldLhd.FileCRC);
    Raw.Get(NewLhd.HeadSize);
    Raw.Get(NewLhd.FileTime);
    Raw.Get(OldLhd.FileAttr);
    Raw.Get(OldLhd.Flags);
    Raw.Get(OldLhd.UnpVer);
    Raw.Get(OldLhd.NameSize);
    Raw.Get(OldLhd.Method);

    NewLhd.Flags=OldLhd.Flags|LONG_BLOCK;
    NewLhd.UnpVer=(OldLhd.UnpVer==2) ? 13 : 10;
    NewLhd.Method=OldLhd.Method+0x30;
    NewLhd.NameSize=OldLhd.NameSize;
    NewLhd.FileAttr=OldLhd.FileAttr;
    NewLhd.FileCRC=OldLhd.FileCRC;
    NewLhd.FullPackSize=NewLhd.PackSize;
    NewLhd.FullUnpSize=NewLhd.UnpSize;

    NewLhd.mtime.SetDos(NewLhd.FileTime);
    NewLhd.ctime.Reset();
    NewLhd.atime.Reset();
    NewLhd.arctime.Reset();

    Raw.Read(OldLhd.NameSize);
    Raw.Get((byte *)NewLhd.FileName,OldLhd.NameSize);
    NewLhd.FileName[OldLhd.NameSize]=0;
    ConvertNameCase(NewLhd.FileName);
    *NewLhd.FileNameW=0;

    if (Raw.Size()!=0)
      NextBlockPos=CurBlockPos+NewLhd.HeadSize+NewLhd.PackSize;
    CurHeaderType=FILE_HEAD;
  }
  return(NextBlockPos>CurBlockPos ? Raw.Size():0);
}
#endif


void Archive::ConvertNameCase(char *Name)
{
  if (Cmd->ConvertNames==NAMES_UPPERCASE)
  {
    IntToExt(Name,Name);
    strupper(Name);
    ExtToInt(Name,Name);
  }
  if (Cmd->ConvertNames==NAMES_LOWERCASE)
  {
    IntToExt(Name,Name);
    strlower(Name);
    ExtToInt(Name,Name);
  }
}


#ifndef SFX_MODULE
void Archive::ConvertNameCase(wchar *Name)
{
  if (Cmd->ConvertNames==NAMES_UPPERCASE)
    strupperw(Name);
  if (Cmd->ConvertNames==NAMES_LOWERCASE)
    strlowerw(Name);
}
#endif


bool Archive::IsArcDir()
{
  return((NewLhd.Flags & LHD_WINDOWMASK)==LHD_DIRECTORY);
}


bool Archive::IsArcLabel()
{
  return(NewLhd.HostOS<=HOST_WIN32 && (NewLhd.FileAttr & 8));
}


void Archive::ConvertAttributes()
{
#if defined(_WIN_32) || defined(_EMX)
  switch(NewLhd.HostOS)
  {
    case HOST_MSDOS:
    case HOST_OS2:
    case HOST_WIN32:
      break;
    case HOST_UNIX:
    case HOST_BEOS:
      if ((NewLhd.Flags & LHD_WINDOWMASK)==LHD_DIRECTORY)
        NewLhd.FileAttr=0x10;
      else
        NewLhd.FileAttr=0x20;
      break;
    default:
      if ((NewLhd.Flags & LHD_WINDOWMASK)==LHD_DIRECTORY)
        NewLhd.FileAttr=0x10;
      else
        NewLhd.FileAttr=0x20;
      break;
  }
#endif
#ifdef _UNIX
  static mode_t mask = (mode_t) -1;

  if (mask == (mode_t) -1)
  {
    mask = umask(022);
    umask(mask);
  }
  switch(NewLhd.HostOS)
  {
    case HOST_MSDOS:
    case HOST_OS2:
    case HOST_WIN32:
      if (NewLhd.FileAttr & 0x10)
        NewLhd.FileAttr=0x41ff & ~mask;
      else
        if (NewLhd.FileAttr & 1)
          NewLhd.FileAttr=0x8124 & ~mask;
        else
          NewLhd.FileAttr=0x81b6 & ~mask;
      break;
    case HOST_UNIX:
    case HOST_BEOS:
      break;
    default:
      if ((NewLhd.Flags & LHD_WINDOWMASK)==LHD_DIRECTORY)
        NewLhd.FileAttr=0x41ff & ~mask;
      else
        NewLhd.FileAttr=0x81b6 & ~mask;
      break;
  }
#endif
}


void Archive::ConvertUnknownHeader()
{
  if (NewLhd.UnpVer<20 && (NewLhd.FileAttr & 0x10))
    NewLhd.Flags|=LHD_DIRECTORY;
  if (NewLhd.HostOS>=HOST_MAX)
  {
    if ((NewLhd.Flags & LHD_WINDOWMASK)==LHD_DIRECTORY)
      NewLhd.FileAttr=0x10;
    else
      NewLhd.FileAttr=0x20;
  }
  for (char *s=NewLhd.FileName;*s!=0;s=charnext(s))
  {
    if (*s=='/' || *s=='\\')
      *s=CPATHDIVIDER;
#if defined(_APPLE) && !defined(UNICODE_SUPPORTED)
    if ((byte)*s<32 || (byte)*s>127)
      *s='_';
#endif
  }
  for (wchar *s=NewLhd.FileNameW;*s!=0;s++)
    if (*s=='/' || *s=='\\')
      *s=CPATHDIVIDER;
}

#ifndef SHELL_EXT
bool Archive::ReadSubData(Array<byte> *UnpData,File *DestFile)
{
  if (HeaderCRC!=SubHead.HeadCRC)
  {
#ifndef SHELL_EXT
    Log(FileName,St(MSubHeadCorrupt));
#endif
    ErrHandler.SetErrorCode(CRC_ERROR);
    return(false);
  }
  if (SubHead.Method<0x30 || SubHead.Method>0x35 || SubHead.UnpVer>PACK_VER)
  {
#ifndef SHELL_EXT
    Log(FileName,St(MSubHeadUnknown));
#endif
    return(false);
  }

  if (SubHead.PackSize==0 && (SubHead.Flags & LHD_SPLIT_AFTER)==0)
    return(true);

  SubDataIO.Init();
  Unpack Unpack(&SubDataIO);
  Unpack.Init();

  if (DestFile==NULL)
  {
    UnpData->Alloc(SubHead.UnpSize);
    SubDataIO.SetUnpackToMemory(&(*UnpData)[0],SubHead.UnpSize);
  }
  if (SubHead.Flags & LHD_PASSWORD)
  {
    if (*Cmd->Password)
      SubDataIO.SetEncryption(SubHead.UnpVer,Cmd->Password,
             (SubHead.Flags & LHD_SALT) ? SubHead.Salt:NULL,false);
    else
      return(false);
  }
  SubDataIO.SetPackedSizeToRead(SubHead.PackSize);
  SubDataIO.EnableShowProgress(false);
  SubDataIO.SetFiles(this,DestFile);
  SubDataIO.UnpVolume=(SubHead.Flags & LHD_SPLIT_AFTER);
  SubDataIO.SetSubHeader(&SubHead,NULL);
  Unpack.SetDestSize(SubHead.UnpSize);
  if (SubHead.Method==0x30)
    CmdExtract::UnstoreFile(SubDataIO,SubHead.UnpSize);
  else
    Unpack.DoUnpack(SubHead.UnpVer,false);

  if (SubHead.FileCRC!=~SubDataIO.UnpFileCRC)
  {
#ifndef SHELL_EXT
    Log(FileName,St(MSubHeadDataCRC),SubHead.FileName);
#endif
    ErrHandler.SetErrorCode(CRC_ERROR);
    if (UnpData!=NULL)
      UnpData->Reset();
    return(false);
  }
  return(true);
}
#endif
