#include "rar.hpp"
#include "UnrarX.hpp"
#include "guilib/GUIWindowManager.h"
#include "dialogs/GUIDialogProgress.h"
#include "filesystem/File.h"

#include "smallfn.cpp"

using namespace std;

#ifdef _DJGPP
extern "C" char **__crt0_glob_function (char *arg) { return 0; }
extern "C" void   __crt0_load_environment_file (char *progname) { }
#endif

#if !defined(GUI) && !defined(RARDLL) && !defined(_LINUX) && !defined(_XBMC)
int main(int argc, char *argv[])
{
#ifdef _UNIX
  setlocale(LC_ALL,"");
#endif
#ifndef SFX_MODULE
  setbuf(stdout,NULL);

  #ifdef _EMX
    EnumConfigPaths(argv[0],-1);
  #endif
#endif

  ErrHandler.SetSignalHandlers(true);

  RARInitData();

#ifdef SFX_MODULE
  char ModuleName[NM];
#ifdef _WIN_32
  GetModuleFileName(NULL,ModuleName,sizeof(ModuleName));
#else
  strcpy(ModuleName,argv[0]);
#endif
#endif

#ifdef _WIN_32
  SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);


#endif

#ifdef ALLOW_EXCEPTIONS
  try 
#endif
  {
  
    CommandData Cmd;
#ifdef SFX_MODULE
    strcpy(Cmd.Command,"X");
    char *Switch=NULL;
#ifdef _SFX_RTL_
    char *CmdLine=GetCommandLine();
    if (CmdLine!=NULL && *CmdLine=='\"')
      CmdLine=strchr(CmdLine+1,'\"');
    if (CmdLine!=NULL && (CmdLine=strpbrk(CmdLine," /"))!=NULL)
    {
      while (isspace(*CmdLine))
        CmdLine++;
      Switch=CmdLine;
    }
#else
    Switch=argc>1 ? argv[1]:NULL;
#endif
    if (Switch!=NULL && Cmd.IsSwitch(Switch[0]))
    {
      int UpperCmd=toupper(Switch[1]);
      switch(UpperCmd)
      {
        case 'T':
        case 'V':
          Cmd.Command[0]=UpperCmd;
          break;
        case '?':
          Cmd.OutHelp();
          break;
      }
    }
    Cmd.AddArcName(ModuleName,NULL);
#else
    if (Cmd.IsConfigEnabled(argc,argv))
    {
      Cmd.ReadConfig(argc,argv);
      Cmd.ParseEnvVar();
    }
    for (int I=1;I<argc;I++)
      Cmd.ParseArg(argv[I],NULL);
#endif
    Cmd.ParseDone();


    InitConsoleOptions(Cmd.MsgStream,Cmd.Sound);
    InitSystemOptions(Cmd.SleepTime);
    InitLogOptions(Cmd.LogName);
    ErrHandler.SetSilent(Cmd.AllYes || Cmd.MsgStream==MSG_NULL);
    ErrHandler.SetShutdown(Cmd.Shutdown);

    Cmd.OutTitle();
    Cmd.ProcessCommand();
  }
#ifdef ALLOW_EXCEPTIONS
  catch (int ErrCode)
  {
    ErrHandler.SetErrorCode(ErrCode);
  }
#ifdef ENABLE_BAD_ALLOC
  catch (bad_alloc)
  {
    ErrHandler.SetErrorCode(MEMORY_ERROR);
  }
#endif
  catch (...)
  {
    ErrHandler.SetErrorCode(FATAL_ERROR);
  }
#endif
  File::RemoveCreated();
#if defined(SFX_MODULE) && defined(_DJGPP)
  _chmod(ModuleName,1,0x20);
#endif
  return(ErrHandler.GetErrorCode());
}
#endif


#if defined(_LINUX) || defined(_XBMC)
/*-------------------------------------------------------------------------*\
                               XBOX interface
\*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*\
  Extract a RAR file
  rarfile      - Name of the RAR file to uncompress
  targetPath    - The path to which we want to uncompress
  fileToExtract - The file inside the archive we want to uncompress,
          or NULL for all files.
  libpassword   - Password (for encrypted archives)
\*-------------------------------------------------------------------------*/
int urarlib_get(char *rarfile, char *targetPath, char *fileToExtract, char *libpassword, int64_t* iOffset, bool bShowProgress)
{
  InitCRC();
  int bRes = 1;

  // Set the arguments for the extract command
  auto_ptr<CommandData> pCmd (new CommandData);

  if( pCmd.get() )
  {
    strcpy(pCmd->Command, "X");
    pCmd->AddArcName(rarfile,NULL);
    strncpy(pCmd->ExtrPath, targetPath, sizeof(pCmd->Command) - 2);
    pCmd->ExtrPath[sizeof(pCmd->Command) - 2] = '\0';
    AddEndSlash(pCmd->ExtrPath);
    pCmd->ParseArg((char*)"-va",NULL);
    if (fileToExtract)
    {
      if (*fileToExtract)
      {
        pCmd->FileArgs->AddString(fileToExtract);
        // Uncomment this if you want to extract a single file without the full path
        strcpy(pCmd->Command, "E");
      }
    }
    else
    {
      pCmd->FileArgs->AddString(MASKALL);
    }

    // Set password for encrypted archives
    if (libpassword)
      if (strlen(libpassword)!=0)
      {
        strncpy(pCmd->Password, libpassword, sizeof(pCmd->Password) - 1);
        pCmd->Password[sizeof(pCmd->Password) - 1] = '\0';
      }

    // Opent the archive    
    auto_ptr<Archive> pArc( new Archive(pCmd.get()) );
    
    if( pArc.get() )
    {
      if (!pArc->WOpen(rarfile,NULL))
        return 0;

      if (pArc->IsOpened() && pArc->IsArchive(true))
      {
        auto_ptr<CmdExtract> pExtract( new CmdExtract );
        
        if( pExtract.get() )
        {
          pExtract->GetDataIO().SetCurrentCommand(*(pCmd->Command));
          struct FindData FD;
          if (FindFile::FastFind(rarfile,NULL,&FD))
            pExtract->GetDataIO().TotalArcSize+=FD.Size;
          pExtract->ExtractArchiveInit(pCmd.get(),*pArc);

          if (bShowProgress)
          {
            pExtract->GetDataIO().m_pDlgProgress = (CGUIDialogProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
            if (pExtract->GetDataIO().m_pDlgProgress)
            {
              pExtract->GetDataIO().m_pDlgProgress->SetHeading(fileToExtract);
              pExtract->GetDataIO().m_pDlgProgress->SetCanCancel(false);
              pExtract->GetDataIO().m_pDlgProgress->StartModal();
            }
          }

          int64_t iOff=0;
          bool bSeeked = false;
          while (1)
          {
            iOff = pArc->Tell();
            int Size=pArc->ReadHeader();
            int Type=pArc->GetHeaderType();
          
            if (Type == ENDARC_HEAD)
              break;

            if (Type != FILE_HEAD)
            {
              pArc->SeekToNext();
              continue;
            }
            
            bool Repeat=false;           
            if (!pExtract->ExtractCurrentFile(pCmd.get(),*pArc,Size,Repeat))
            {
               bRes = FALSE;
                break;
            }
            
            if (pExtract->GetDataIO().bQuit) 
            {
              if (pExtract->GetDataIO().m_pDlgProgress)
                pExtract->GetDataIO().m_pDlgProgress->Close();
              bRes = 2;
              break;
            }

            if (fileToExtract)
            {
              if (*fileToExtract)
              {
                bool EqualNames=false;
                int MatchNumber=pCmd->IsProcessFile(pArc->NewLhd,&EqualNames);
                bool ExactMatch=MatchNumber!=0;
                if (ExactMatch)
                {
                  if (iOffset)
                    *iOffset = iOff;
                  break;
                }
              }
            }
            if (iOffset && !bSeeked && !pArc->Solid)
            {
              if (*iOffset > -1)
              {
                bSeeked = true;
                pArc->Seek(*iOffset,SEEK_SET);
              }
            }
          }

          pExtract->GetDataIO().ProcessedArcSize+=FD.Size;         
          if (pExtract->GetDataIO().m_pDlgProgress)
            pExtract->GetDataIO().m_pDlgProgress->ShowProgressBar(false);
        }
        if (bShowProgress)
          if (pExtract->GetDataIO().m_pDlgProgress)
            pExtract->GetDataIO().m_pDlgProgress->Close();
      }
    }
  }

  File::RemoveCreated();
  return bRes;
}

/*-------------------------------------------------------------------------*\
  List the files in a RAR file
  rarfile    - Name of the RAR file to uncompress
  list      - Output. A list of file data of the files in the archive.
                The list should be freed with urarlib_freelist().
  libpassword - Password (for encrypted archives)
\*-------------------------------------------------------------------------*/
int urarlib_list(char *rarfile, ArchiveList_struct **ppList, char *libpassword, bool stopattwo)
{
  if (!ppList)
    return 0;
  uint FileCount = 0;
  InitCRC();

  // Set the arguments for the extract command
  auto_ptr<CommandData> pCmd( new CommandData );

  {
    strcpy(pCmd->Command, "L");
    pCmd->AddArcName(rarfile, NULL);
    pCmd->FileArgs->AddString(MASKALL);
    pCmd->ParseArg((char*)"-va",NULL);

    // Set password for encrypted archives
    if (libpassword)
    {
      strncpy(pCmd->Password, libpassword, sizeof(pCmd->Password) - 1);
      pCmd->Password[sizeof(pCmd->Password) - 1] = '\0';
    }

    // Opent the archive
    auto_ptr<Archive> pArc( new Archive(pCmd.get()) );
    if ( pArc.get() )
    {
      if (!pArc->WOpen(rarfile,NULL))
        return 0;

      FileCount=0;
      *ppList = NULL;
      ArchiveList_struct *pPrev = NULL;
      int iArchive=0;
      while (1)
      {
        if (pArc->IsOpened() && pArc->IsArchive(true))
        {
          int64_t iOffset = pArc->NextBlockPos;
          while(pArc->ReadHeader()>0)
          {
            if (pArc->GetHeaderType() == FILE_HEAD)
            {
              if (pPrev)
                if (stricmp(pArc->NewLhd.FileName,pPrev->item.Name)==0)
                {
                  iOffset = pArc->NextBlockPos;
                  pArc->SeekToNext();
                  continue;
                }

              IntToExt(pArc->NewLhd.FileName,pArc->NewLhd.FileName);
              ArchiveList_struct *pCurr = (ArchiveList_struct *)malloc(sizeof(ArchiveList_struct));
              if (!pCurr)
                break;
              if (pPrev)
                pPrev->next = pCurr;
              if (!*ppList)
                *ppList = pCurr;
              pCurr->item.NameSize = strlen(pArc->NewLhd.FileName);
              // sanity check - if it fails the archive is likely corrupt
              if (pCurr->item.NameSize > NM)
              {
                File::RemoveCreated();
                return 0;
              }

              pCurr->item.Name = (char *)malloc(pCurr->item.NameSize + 1);
              strcpy(pCurr->item.Name, pArc->NewLhd.FileName);
              pCurr->item.NameW = (wchar *)malloc((pCurr->item.NameSize + 1)*sizeof(wchar));
              wcscpy(pCurr->item.NameW, pArc->NewLhd.FileNameW);
              pCurr->item.PackSize = pArc->NewLhd.PackSize;
              pCurr->item.UnpSize = int32to64(pArc->NewLhd.HighUnpSize,pArc->NewLhd.UnpSize);
              pCurr->item.HostOS = pArc->NewLhd.HostOS;
              pCurr->item.FileCRC = pArc->NewLhd.FileCRC;
              pCurr->item.FileTime = pArc->NewLhd.FileTime;
              pCurr->item.UnpVer = pArc->NewLhd.UnpVer;
              pCurr->item.Method = pArc->NewLhd.Method;
              pCurr->item.FileAttr = pArc->NewLhd.FileAttr;
              pCurr->item.iOffset = iOffset;
              pCurr->next = NULL;
              pPrev = pCurr;
              FileCount++;
              if (stopattwo && FileCount > 1)
                break;
            }
            iOffset = pArc->NextBlockPos;
            if (iOffset > pArc->FileLength())
            {
              File::RemoveCreated();
              return 0;
            }
            pArc->SeekToNext();
          }
          if (pCmd->VolSize!=0 && ((pArc->NewLhd.Flags & LHD_SPLIT_AFTER) || (pArc->GetHeaderType()==ENDARC_HEAD && (pArc->EndArcHead.Flags & EARC_NEXT_VOLUME)!=0)))
          {
            if (FileCount == 1 && iArchive==0)
            {
              char NextName[NM];
              char LastName[NM];
              strcpy(NextName,pArc->FileName);
              while (XFILE::CFile::Exists(NextName))
              {
                strcpy(LastName,NextName);
                NextVolumeName(NextName,(pArc->NewMhd.Flags & MHD_NEWNUMBERING)==0 || pArc->OldFormat);
              }
               Archive arc;
              if (arc.WOpen(LastName,NULL))
              {
                bool bBreak=false;
                while(arc.ReadHeader()>0)
                {
                  if (arc.GetHeaderType() == FILE_HEAD)
                    if (stricmp(arc.NewLhd.FileName,pPrev->item.Name)==0)
                    {
                      bBreak=true;
                      break;  
                    }
//                  iOffset = pArc->Tell();
                  arc.SeekToNext();
                }
                if (bBreak)
                {
                  break;
                }
              }
            }
            if (MergeArchive(*pArc,NULL,false,*pCmd->Command))
            {
              iArchive++;
              pArc->Seek(0,SEEK_SET); 
            }
            else
              break;
          }
          else
            break;
        }
        else
          break;
      }
    }
  }

  File::RemoveCreated();
  return FileCount;
}

/*-------------------------------------------------------------------------*\
  Free the file list returned by urarlib_list()
  list - The output from urarlib_list()
\*-------------------------------------------------------------------------*/
void urarlib_freelist(ArchiveList_struct *list)
{
  ArchiveList_struct *p;
  while (list)
  {
    p = list->next;
    free(list->item.Name);
    free(list->item.NameW);
    free(list);
    list = p;
  }
}


#endif
