#include "rar.hpp"
#include "unrarx.hpp"
#include "../../utils/log.h"

#include "smallfn.cpp"

#ifdef _DJGPP
extern "C" char **__crt0_glob_function (char *arg) { return 0; }
extern "C" void   __crt0_load_environment_file (char *progname) { }
#endif

#if defined(_XBOX) && defined(__XBOX__TEST__)
void main(int argc, char *argv[])
{
	ArchiveList_struct *list, *p;
	urarlib_list(argv[1], &list, NULL);

	printf("                   Name     Size  Packed   OS  FileTime    ");
	printf("CRC-32 Attr Ver Meth\n");
	printf("     ------------------------------------------------------");
	printf("--------------------\n");

	p = list;
	while (list)
	{
		if (list->item.NameSize < 23)
			printf("%23s", list->item.Name);
		else
			printf("%23s", list->item.Name + (list->item.NameSize - 23));

		printf("%9ld",  list->item.UnpSize);
		printf("%8ld",  list->item.PackSize);
		printf("%5d",  list->item.HostOS);
		printf("%10lx", list->item.FileTime);
		printf("%10lx", list->item.FileCRC);
		printf("%5ld",  list->item.FileAttr);
		printf("%4d",  list->item.UnpVer);
		printf("%5d",  list->item.Method);
		printf("\n");

		list = list->next;
	}
	urarlib_freelist(p);

	int res = urarlib_get(argv[1], argv[2], argv[3], NULL);
}
#else

#if !defined(GUI) && !defined(RARDLL) && !defined(_XBOX)
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

#endif /* __XBOX__TEST__ */

#ifdef _XBOX
/*-------------------------------------------------------------------------*\
                               XBOX interface
\*-------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------*\
	Extract a RAR file
	rarfile		  - Name of the RAR file to uncompress
	targetPath	  - The path to which we want to uncompress
	fileToExtract - The file inside the archive we want to uncompress,
					or NULL for all files.
	libpassword   - Password (for encrypted archives)
\*-------------------------------------------------------------------------*/
int urarlib_get(char *rarfile, char *targetPath, char *fileToExtract, char *libpassword)
{
	InitCRC();
	BOOL bRes = TRUE;

	// Set the arguments for the extract command
    CommandData * pCmd = NULL;

	pCmd = new CommandData;

	if ( pCmd )
	{
		strcpy(pCmd->Command, "X");
		pCmd->AddArcName(rarfile,NULL);
		strncpy(pCmd->ExtrPath, targetPath, sizeof(pCmd->Command) - 2);
		pCmd->ExtrPath[sizeof(pCmd->Command) - 2] = '\0';
		AddEndSlash(pCmd->ExtrPath);
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
		Archive * pArc = NULL;
		pArc = new Archive(pCmd);

		if ( pArc )
		{
			if (!pArc->WOpen(rarfile,NULL))
				return 0;

			if (pArc->IsOpened() && pArc->IsArchive(true))
			{
				CmdExtract * pExtract = NULL;

				pExtract = new CmdExtract;

				if ( pExtract )
				{
          pExtract->GetDataIO().SetCurrentCommand(*(pCmd->Command));
					struct FindData FD;
					if (FindFile::FastFind(rarfile,NULL,&FD))
						pExtract->GetDataIO().TotalArcSize+=FD.Size;
          pExtract->ExtractArchiveInit(pCmd,*pArc);
          while (1)
					{
            int Size=pArc->ReadHeader();
            
            if (pArc->GetHeaderType() == ENDARC_HEAD)
              break;

						bool Repeat=false;
            if (!pExtract->ExtractCurrentFile(pCmd,*pArc,Size,Repeat))
						{
               bRes = FALSE;
						 	 break;
						}
						if (fileToExtract)
              if (*fileToExtract)
                if(stricmp(pArc->NewLhd.FileName, fileToExtract) == 0)
					  			break;
					}
					pExtract->GetDataIO().ProcessedArcSize+=FD.Size;
					delete pExtract;
				}
			}
			delete pArc;
		}
		delete pCmd;
	}

	File::RemoveCreated();
  return bRes;
}

/*-------------------------------------------------------------------------*\
	List the files in a RAR file
	rarfile		- Name of the RAR file to uncompress
	list	    - Output. A list of file data of the files in the archive.
	              The list should be freed with urarlib_freelist().
	libpassword - Password (for encrypted archives)
\*-------------------------------------------------------------------------*/
int urarlib_list(char *rarfile, ArchiveList_struct **ppList, char *libpassword)
{
  if (!ppList)
		return 0;
	uint FileCount;
	InitCRC();

	// Set the arguments for the extract command
    CommandData * pCmd = new CommandData;

	if ( pCmd )
	{
		strcpy(pCmd->Command, "L");
		pCmd->AddArcName(rarfile, NULL);
		pCmd->FileArgs->AddString(MASKALL);

		// Set password for encrypted archives
		if (libpassword)
		{
			strncpy(pCmd->Password, libpassword, sizeof(pCmd->Password) - 1);
			pCmd->Password[sizeof(pCmd->Password) - 1] = '\0';
		}

		// Opent the archive
		Archive * pArc = new Archive(pCmd);
		if ( pArc )
		{
			if (!pArc->WOpen(rarfile,NULL))
				return 0;

			bool FileMatched=true;

			Int64 TotalPackSize=0,TotalUnpSize=0;
			FileCount=0;
			*ppList = NULL;
			if (pArc->IsOpened() && pArc->IsArchive(true))
			{
				ArchiveList_struct *pPrev = NULL;
				bool TitleShown=false;
				while(pArc->ReadHeader()>0)
				{
					if (pArc->GetHeaderType() == FILE_HEAD)
					{
            IntToExt(pArc->NewLhd.FileName,pArc->NewLhd.FileName);
						ArchiveList_struct *pCurr = (ArchiveList_struct *)malloc(sizeof(ArchiveList_struct));
						if (!pCurr)
							break;
						if (pPrev)
							pPrev->next = pCurr;
						if (!*ppList)
							*ppList = pCurr;
						pCurr->item.NameSize = strlen(pArc->NewLhd.FileName);
						pCurr->item.Name = (char *)malloc(pCurr->item.NameSize + 1);
						strcpy(pCurr->item.Name, pArc->NewLhd.FileName);
						pCurr->item.PackSize = pArc->NewLhd.PackSize;
            pCurr->item.UnpSize = int32to64(pArc->NewLhd.HighUnpSize,pArc->NewLhd.UnpSize);
						pCurr->item.HostOS = pArc->NewLhd.HostOS;
						pCurr->item.FileCRC = pArc->NewLhd.FileCRC;
						pCurr->item.FileTime = pArc->NewLhd.FileTime;
						pCurr->item.UnpVer = pArc->NewLhd.UnpVer;
						pCurr->item.Method = pArc->NewLhd.Method;
						pCurr->item.FileAttr = pArc->NewLhd.FileAttr;
						pCurr->next = NULL;
						pPrev = pCurr;
						FileCount++;
					}
					pArc->SeekToNext();
				}
			}
			delete pArc;
		}
		delete pCmd;
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
		free(list);
		list = p;
	}
}


#endif /* _XBOX */
