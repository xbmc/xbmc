#include "rar.hpp"

#define RECVOL_BUFSIZE  0x800

RecVolumes::RecVolumes()
{
  Buf.Alloc(RECVOL_BUFSIZE*256);
  memset(SrcFile,0,sizeof(SrcFile));
}


RecVolumes::~RecVolumes()
{
  for (int I=0;I<sizeof(SrcFile)/sizeof(SrcFile[0]);I++)
    delete SrcFile[I];
}




bool RecVolumes::Restore(RAROptions *Cmd,const char *Name,
                         const wchar *NameW,bool Silent)
{
  char ArcName[NM];
  wchar ArcNameW[NM];
  strcpy(ArcName,Name);
  strcpyw(ArcNameW,NameW);
  char *Ext=GetExt(ArcName);
  bool NewStyle=false;
  bool RevName=Ext!=NULL && stricomp(Ext,".rev")==0;
  if (RevName)
  {
    for (int DigitGroup=0;Ext>ArcName && DigitGroup<3;Ext--)
      if (!isdigit(*Ext))
        if (isdigit(*(Ext-1)) && (*Ext=='_' || DigitGroup<2))
          DigitGroup++;
        else
          if (DigitGroup<2)
          {
            NewStyle=true;
            break;
          }
    while (isdigit(*Ext) && Ext>ArcName+1)
      Ext--;
    strcpy(Ext,"*.*");
    FindFile Find;
    Find.SetMask(ArcName);
    struct FindData FD;
    while (Find.Next(&FD))
    {
      Archive Arc(Cmd);
      if (Arc.WOpen(FD.Name,FD.NameW) && Arc.IsArchive(true))
      {
        strcpy(ArcName,FD.Name);
        *ArcNameW=0;
        break;
      }
    }
  }

  Archive Arc(Cmd);
  if (!Arc.WCheckOpen(ArcName,ArcNameW))
    return(false);
  if (!Arc.Volume)
  {
#ifndef SILENT
    Log(ArcName,St(MNotVolume),ArcName);
#endif
    return(false);
  }
  bool NewNumbering=(Arc.NewMhd.Flags & MHD_NEWNUMBERING);
  Arc.Close();
  char *VolNumStart=VolNameToFirstName(ArcName,ArcName,NewNumbering);
  char RecVolMask[NM];
  strcpy(RecVolMask,ArcName);
  int BaseNamePartLength=VolNumStart-ArcName;
  strcpy(RecVolMask+BaseNamePartLength,"*.rev");

#ifndef SILENT
  Int64 RecFileSize=0;
#endif
  FindFile Find;
  Find.SetMask(RecVolMask);
  struct FindData RecData;
  int FileNumber=0,RecVolNumber=0,FoundRecVolumes=0,MissingVolumes=0;
  char PrevName[NM];
  while (Find.Next(&RecData))
  {
    char *Name=RecData.Name;
    int P[3];
    if (!RevName && !NewStyle)
    {
      NewStyle=true;
      char *Dot=GetExt(Name);
      if (Dot!=NULL)
      {
        int LineCount=0;
        Dot--;
        while (Dot>Name && *Dot!='.')
        {
          if (*Dot=='_')
            LineCount++;
          Dot--;
        }
        if (LineCount==2)
          NewStyle=false;
      }
    }
    if (NewStyle)
    {
      File CurFile;
      CurFile.TOpen(Name);
      CurFile.Seek(0,SEEK_END);
      Int64 Length=CurFile.Tell();
      CurFile.Seek(Length-7,SEEK_SET);
      for (int I=0;I<3;I++)
        P[2-I]=CurFile.GetByte()+1;
      uint FileCRC=0;
      for (int I=0;I<4;I++)
        FileCRC|=CurFile.GetByte()<<(I*8);
      if (FileCRC!=CalcFileCRC(&CurFile,Length-4))
      {
#ifndef SILENT
        mprintf(St(MCRCFailed),Name);
#endif
        continue;
      }
    }
    else
    {
      char *Dot=GetExt(Name);
      if (Dot==NULL)
        continue;
      bool WrongParam=false;
      for (int I=0;I<sizeof(P)/sizeof(P[0]);I++)
      {
        do
        {
          Dot--;
        } while (isdigit(*Dot) && Dot>=Name+BaseNamePartLength);
        P[I]=atoi(Dot+1);
        if (P[I]==0 || P[I]>255)
          WrongParam=true;
      }
      if (WrongParam)
        continue;
    }
    if (P[1]+P[2]>255)
      continue;
    if (RecVolNumber!=0 && RecVolNumber!=P[1] || FileNumber!=0 && FileNumber!=P[2])
    {
#ifndef SILENT
      Log(NULL,St(MRecVolDiffSets),Name,PrevName);
#endif
      return(false);
    }
    RecVolNumber=P[1];
    FileNumber=P[2];
    strcpy(PrevName,Name);
    File *NewFile=new File;
    NewFile->TOpen(Name);
    SrcFile[FileNumber+P[0]-1]=NewFile;
    FoundRecVolumes++;
#ifndef SILENT
    if (RecFileSize==0)
      RecFileSize=NewFile->FileLength();
#endif
  }
#ifndef SILENT
  if (!Silent || FoundRecVolumes!=0)
  {
    mprintf(St(MRecVolFound),FoundRecVolumes);
  }
#endif
  if (FoundRecVolumes==0)
    return(false);

  bool WriteFlags[256];
  memset(WriteFlags,0,sizeof(WriteFlags));

  char LastVolName[NM];
  *LastVolName=0;

  for (int CurArcNum=0;CurArcNum<FileNumber;CurArcNum++)
  {
    Archive *NewFile=new Archive;
    bool ValidVolume=FileExist(ArcName);
    if (ValidVolume)
    {
      NewFile->TOpen(ArcName);
      ValidVolume=NewFile->IsArchive(false);
      if (ValidVolume)
      {
        bool EndFound=false,EndBlockRequired=false;
        while (!EndFound && NewFile->ReadHeader()!=0)
        {
          if (NewFile->GetHeaderType()==FILE_HEAD)
          {
            if (NewFile->NewLhd.UnpVer>=29)
              EndBlockRequired=true;
            if (!EndBlockRequired && (NewFile->NewLhd.Flags & LHD_SPLIT_AFTER))
              EndFound=true;
          }
          if (NewFile->GetHeaderType()==ENDARC_HEAD)
          {
            if ((NewFile->EndArcHead.Flags&EARC_DATACRC)!=0 && 
                NewFile->EndArcHead.ArcDataCRC!=CalcFileCRC(NewFile,NewFile->CurBlockPos))
            {
              ValidVolume=false;
#ifndef SILENT
              mprintf(St(MCRCFailed),ArcName);
#endif
            }
            EndFound=true;
          }
          NewFile->SeekToNext();
        }
        if (!EndFound)
          ValidVolume=false;
      }
      if (!ValidVolume)
      {
        NewFile->Close();
        char NewName[NM];
        strcpy(NewName,ArcName);
        strcat(NewName,".bad");
#ifndef SILENT
        mprintf(St(MBadArc),ArcName);
        mprintf(St(MRenaming),ArcName,NewName);
#endif
        rename(ArcName,NewName);
      }
      NewFile->Seek(0,SEEK_SET);
    }
    if (!ValidVolume)
    {
      NewFile->TCreate(ArcName);
      WriteFlags[CurArcNum]=true;
      MissingVolumes++;

      if (CurArcNum==FileNumber-1)
        strcpy(LastVolName,ArcName);

#ifndef SILENT
      mprintf(St(MAbsNextVol),ArcName);
#endif
    }
    SrcFile[CurArcNum]=(File*)NewFile;
    NextVolumeName(ArcName,!NewNumbering);
  }

#ifndef SILENT
  mprintf(St(MRecVolMissing),MissingVolumes);
#endif

  if (MissingVolumes==0)
  {
#ifndef SILENT
    mprintf(St(MRecVolAllExist));
#endif
    return(false);
  }

  if (MissingVolumes>FoundRecVolumes)
  {
#ifndef SILENT
    mprintf(St(MRecVolCannotFix));
#endif
    return(false);
  }
#ifndef SILENT
  mprintf(St(MReconstructing));
#endif

  RSCoder RSC(RecVolNumber);

  int TotalFiles=FileNumber+RecVolNumber;
  int Erasures[256],EraSize=0;

  for (int I=0;I<TotalFiles;I++)
    if (WriteFlags[I] || SrcFile[I]==NULL)
      Erasures[EraSize++]=I;

#ifndef SILENT
  Int64 ProcessedSize=0;
#ifndef GUI
  int LastPercent=-1;
  mprintf("     ");
#endif
#endif
  int RecCount=0;

  while (true)
  {
    if ((++RecCount & 15)==0)
      Wait();
    int MaxRead=0;
    for (int I=0;I<TotalFiles;I++)
      if (WriteFlags[I] || SrcFile[I]==NULL)
        memset(&Buf[I*RECVOL_BUFSIZE],0,RECVOL_BUFSIZE);
      else
      {
        int ReadSize=SrcFile[I]->Read(&Buf[I*RECVOL_BUFSIZE],RECVOL_BUFSIZE);
        if (ReadSize!=RECVOL_BUFSIZE)
          memset(&Buf[I*RECVOL_BUFSIZE+ReadSize],0,RECVOL_BUFSIZE-ReadSize);
        if (ReadSize>MaxRead)
          MaxRead=ReadSize;
      }
    if (MaxRead==0)
      break;
#ifndef SILENT
    int CurPercent=ToPercent(ProcessedSize,RecFileSize);
    if (!Cmd->DisablePercentage && CurPercent!=LastPercent)
    {
      mprintf("\b\b\b\b%3d%%",CurPercent);
      LastPercent=CurPercent;
    }
    ProcessedSize+=MaxRead;
#endif
    for (int BufPos=0;BufPos<MaxRead;BufPos++)
    {
      byte Data[256];
      for (int I=0;I<TotalFiles;I++)
        Data[I]=Buf[I*RECVOL_BUFSIZE+BufPos];
      RSC.Decode(Data,TotalFiles,Erasures,EraSize);
      for (int I=0;I<EraSize;I++)
        Buf[Erasures[I]*RECVOL_BUFSIZE+BufPos]=Data[Erasures[I]];
/*
      for (int I=0;I<FileNumber;I++)
        Buf[I*RECVOL_BUFSIZE+BufPos]=Data[I];
*/
    }
    for (int I=0;I<FileNumber;I++)
      if (WriteFlags[I])
        SrcFile[I]->Write(&Buf[I*RECVOL_BUFSIZE],MaxRead);
  }
  for (int I=0;I<RecVolNumber+FileNumber;I++)
    if (SrcFile[I]!=NULL)
    {
      File *CurFile=SrcFile[I];
      if (NewStyle && WriteFlags[I])
      {
        Int64 Length=CurFile->Tell();
        CurFile->Seek(Length-7,SEEK_SET);
        for (int J=0;J<7;J++)
          CurFile->PutByte(0);
      }
      CurFile->Close();
      SrcFile[I]=NULL;
    }
  if (*LastVolName)
  {
    Archive Arc(Cmd);
    if (Arc.Open(LastVolName,NULL,false,true) && Arc.IsArchive(true) &&
        Arc.SearchBlock(ENDARC_HEAD))
    {
      Arc.Seek(Arc.NextBlockPos,SEEK_SET);
      char Buf[8192];
      int ReadSize=Arc.Read(Buf,sizeof(Buf));
      int ZeroCount=0;
      while (ZeroCount<ReadSize && Buf[ZeroCount]==0)
        ZeroCount++;
      if (ZeroCount==ReadSize)
      {
        Arc.Seek(Arc.NextBlockPos,SEEK_SET);
        Arc.Truncate();
      }
    }
  }
#if !defined(GUI) && !defined(SILENT)
  if (!Cmd->DisablePercentage)
    mprintf("\b\b\b\b100%%");
  if (!Silent && !Cmd->DisableDone)
    mprintf(St(MDone));
#endif
  return(true);
}
