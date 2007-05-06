#include "rar.hpp"

static void GetFirstNewVolName(const char *ArcName,char *VolName,
  Int64 VolSize,Int64 TotalSize);



bool MergeArchive(Archive &Arc,ComprDataIO *DataIO,bool ShowFileName,char Command)
{
  RAROptions *Cmd=Arc.GetRAROptions();

  int HeaderType=Arc.GetHeaderType();
  FileHeader *hd=HeaderType==NEWSUB_HEAD ? &Arc.SubHead:&Arc.NewLhd;
  bool SplitHeader=(HeaderType==FILE_HEAD || HeaderType==NEWSUB_HEAD) &&
                   (hd->Flags & LHD_SPLIT_AFTER)!=0;

  if (DataIO!=NULL && SplitHeader && hd->UnpVer>=20 &&
      hd->FileCRC!=0xffffffff && DataIO->PackedCRC!=~hd->FileCRC)
  {
    Log(Arc.FileName,St(MDataBadCRC),hd->FileName,Arc.FileName);
  }

  Int64 PosBeforeClose=Arc.Tell();
  Arc.Close();

  char NextName[NM];
  strcpy(NextName,Arc.FileName);
  NextVolumeName(NextName,(Arc.NewMhd.Flags & MHD_NEWNUMBERING)==0 || Arc.OldFormat);

#if !defined(SFX_MODULE) && !defined(RARDLL)
  bool RecoveryDone=false;
#endif
  bool FailedOpen=false,OldSchemeTested=false;

  while (!Arc.Open(NextName))
  {
    if (!OldSchemeTested)
    {
      char AltNextName[NM];
      strcpy(AltNextName,Arc.FileName);
      NextVolumeName(AltNextName,true);
      OldSchemeTested=true;
      if (Arc.Open(AltNextName))
      {
        strcpy(NextName,AltNextName);
        break;
      }
    }
#ifdef RARDLL
    if (Cmd->Callback==NULL && Cmd->ChangeVolProc==NULL ||
        Cmd->Callback!=NULL && Cmd->Callback(UCM_CHANGEVOLUME,Cmd->UserData,(LONG)NextName,RAR_VOL_ASK)==-1)
    {
      Cmd->DllError=ERAR_EOPEN;
      FailedOpen=true;
      break;
    }
    if (Cmd->ChangeVolProc!=NULL)
    {
#ifdef _WIN_32
      _EBX=_ESP;
#endif
      int RetCode=Cmd->ChangeVolProc(NextName,RAR_VOL_ASK);
#ifdef _WIN_32
      _ESP=_EBX;
#endif
      if (RetCode==0)
      {
        Cmd->DllError=ERAR_EOPEN;
        FailedOpen=true;
        break;
      }
    }
#else

#if !defined(SFX_MODULE) && !defined(_WIN_CE)
    if (!RecoveryDone)
    {
      RecVolumes RecVol;
      RecVol.Restore(Cmd,Arc.FileName,Arc.FileNameW,true);
      RecoveryDone=true;
      continue;
    }
#endif

#ifndef GUI
    if (!Cmd->VolumePause && !IsRemovable(NextName))
    {
      Log(Arc.FileName,St(MAbsNextVol),NextName);
      FailedOpen=true;
      break;
    }
#endif
#ifndef SILENT
    if (Cmd->AllYes || !AskNextVol(NextName))
#endif
    {
      FailedOpen=true;
      break;
    }
#endif
  }
  if (FailedOpen)
  {
    Arc.Open(Arc.FileName,Arc.FileNameW);
    Arc.Seek(PosBeforeClose,SEEK_SET);
    return(false);
  }
  Arc.CheckArc(true);
#ifdef RARDLL
  if (Cmd->Callback!=NULL &&
      Cmd->Callback(UCM_CHANGEVOLUME,Cmd->UserData,(LONG)NextName,RAR_VOL_NOTIFY)==-1)
    return(false);
  if (Cmd->ChangeVolProc!=NULL)
  {
#ifdef _WIN_32
    _EBX=_ESP;
#endif
    int RetCode=Cmd->ChangeVolProc(NextName,RAR_VOL_NOTIFY);
#ifdef _WIN_32
    _ESP=_EBX;
#endif
    if (RetCode==0)
      return(false);
  }
#endif

  if (Command=='T' || Command=='X' || Command=='E')
    mprintf(St(Command=='T' ? MTestVol:MExtrVol),Arc.FileName);
  if (SplitHeader)
    Arc.SearchBlock(HeaderType);
  else
    Arc.ReadHeader();
  if (Arc.GetHeaderType()==FILE_HEAD)
  {
    Arc.ConvertAttributes();
    Arc.Seek(Arc.NextBlockPos-Arc.NewLhd.FullPackSize,SEEK_SET);
  }
#ifndef GUI
  if (ShowFileName)
  {
    mprintf(St(MExtrPoints),IntNameToExt(Arc.NewLhd.FileName));
    if (!Cmd->DisablePercentage)
      mprintf("     ");
  }
#endif
  if (DataIO!=NULL)
  {
    if (HeaderType==ENDARC_HEAD)
      DataIO->UnpVolume=false;
    else
    {
      DataIO->UnpVolume=(hd->Flags & LHD_SPLIT_AFTER);
      DataIO->SetPackedSizeToRead(hd->FullPackSize);
    }
#ifdef SFX_MODULE
    DataIO->UnpArcSize=Arc.FileLength();
    DataIO->CurUnpRead=0;
#endif
    DataIO->PackedCRC=0xffffffff;
//    DataIO->SetFiles(&Arc,NULL);
  }
  return(true);
}






#ifndef SILENT
bool AskNextVol(char *ArcName)
{
  eprintf(St(MAskNextVol),ArcName);
  if (Ask(St(MContinueQuit))==2)
    return(false);
  return(true);
}
#endif
