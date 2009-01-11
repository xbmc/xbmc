#include "rar.hpp"

CommandData::CommandData()
{
  FileArgs=ExclArgs=InclArgs=StoreArgs=ArcNames=NULL;
  Init();
}


CommandData::~CommandData()
{
  Close();
}


void CommandData::Init()
{
  Close();

  *Command=0;
  *ArcName=0;
  *ArcNameW=0;
  FileLists=false;
  NoMoreSwitches=false;
  TimeConverted=false;

  FileArgs=new StringList;
  ExclArgs=new StringList;
  InclArgs=new StringList;
  StoreArgs=new StringList;
  ArcNames=new StringList;
}


void CommandData::Close()
{
  delete FileArgs;
  delete ExclArgs;
  delete InclArgs;
  delete StoreArgs;
  delete ArcNames;
  FileArgs=ExclArgs=InclArgs=StoreArgs=ArcNames=NULL;
  NextVolSizes.Reset();
}


#if !defined(SFX_MODULE) && !defined(_WIN_CE)
void CommandData::ParseArg(char *Arg,wchar *ArgW)
{
  if (IsSwitch(*Arg) && !NoMoreSwitches)
    if (Arg[1]=='-')
      NoMoreSwitches=true;
    else
      ProcessSwitch(&Arg[1]);
  else
    if (*Command==0)
    {
      strncpy(Command,Arg,sizeof(Command));
      if (ArgW!=NULL)
        strncpyw(CommandW,ArgW,sizeof(CommandW)/sizeof(CommandW[0]));
      if (toupper(*Command)=='S')
      {
        const char *SFXName=Command[1] ? Command+1:DefSFXName;
        if (PointToName(SFXName)!=SFXName || FileExist(SFXName))
          strcpy(SFXModule,SFXName);
        else
          GetConfigName(SFXName,SFXModule,true);
      }
#ifndef GUI
      *Command=toupper(*Command);
      if (*Command!='I' && *Command!='S')
        strupper(Command);
#endif
    }
    else
      if (*ArcName==0)
      {
        strncpy(ArcName,Arg,sizeof(ArcName));
        if (ArgW!=NULL)
          strncpyw(ArcNameW,ArgW,sizeof(ArcNameW)/sizeof(ArcNameW[0]));
      }
      else
      {
        int Length=strlen(Arg);
        char EndChar=Arg[Length-1];
        char CmdChar=toupper(*Command);
        bool Add=strchr("AFUM",CmdChar)!=NULL;
        bool Extract=CmdChar=='X' || CmdChar=='E';
        if ((IsDriveDiv(EndChar) || IsPathDiv(EndChar)) && !Add)
          strcpy(ExtrPath,Arg);
        else
          if ((Add || CmdChar=='T') && *Arg!='@')
            FileArgs->AddString(Arg);
          else
          {
            struct FindData FileData;
            bool Found=FindFile::FastFind(Arg,NULL,&FileData);
            if (!Found && *Arg=='@' && !IsWildcard(Arg))
            {
              ReadTextFile(Arg+1,FileArgs,false,true,true,true,true);
              FileLists=true;
            }
            else
              if (Found && FileData.IsDir && Extract && *ExtrPath==0)
              {
                strcpy(ExtrPath,Arg);
                AddEndSlash(ExtrPath);
              }
              else
                FileArgs->AddString(Arg);
          }
      }
}
#endif


void CommandData::ParseDone()
{
  if (FileArgs->ItemsCount()==0 && !FileLists)
    FileArgs->AddString(MASKALL);
  char CmdChar=toupper(*Command);
  bool Extract=CmdChar=='X' || CmdChar=='E';
  if (Test && Extract)
    Test=false;
  BareOutput=(CmdChar=='L' || CmdChar=='V') && Command[1]=='B';
}


#if !defined(SFX_MODULE) && !defined(_WIN_CE) && !defined(_XBOX) && !defined(_LINUX)
void CommandData::ParseEnvVar()
{
  char *EnvStr=getenv("RAR");
  if (EnvStr!=NULL)
    ProcessSwitchesString(EnvStr);
}
#endif


#if !defined(GUI) && !defined(SFX_MODULE)
bool CommandData::IsConfigEnabled(int argc,char *argv[])
{
  bool ConfigEnabled=true;
  for (int I=1;I<argc;I++)
    if (IsSwitch(*argv[I]))
    {
      if (stricomp(&argv[I][1],"cfg-")==0)
        ConfigEnabled=false;
      if (strnicomp(&argv[I][1],"ilog",4)==0)
      {
        ProcessSwitch(&argv[I][1]);
        InitLogOptions(LogName);
      }
    }
  return(ConfigEnabled);}
#endif


#if !defined(GUI) && !defined(SFX_MODULE)
void CommandData::ReadConfig(int argc,char *argv[])
{
  StringList List;
  if (ReadTextFile((char*)DefConfigName,&List,true))
  {
    char *Str;
    while ((Str=List.GetString())!=NULL)
      if (strnicomp(Str,"switches=",9)==0)
        ProcessSwitchesString(Str+9);
  }
}
#endif


#if !defined(SFX_MODULE) && !defined(_WIN_CE)
void CommandData::ProcessSwitchesString(char *Str)
{
  while (*Str)
  {
    while (!IsSwitch(*Str) && *Str!=0)
      Str++;
    if (*Str==0)
      break;
    char *Next=Str;
    while (!(Next[0]==' ' && IsSwitch(Next[1])) && *Next!=0)
      Next++;
    char NextChar=*Next;
    *Next=0;
    ProcessSwitch(Str+1);
    *Next=NextChar;
    Str=Next;
  }
}
#endif


#if !defined(SFX_MODULE) && !defined(_WIN_CE)
void CommandData::ProcessSwitch(char *Switch)
{

  switch(toupper(Switch[0]))
  {
    case 'I':
      if (strnicomp(&Switch[1],"LOG",3)==0)
      {
        strncpy(LogName,Switch[4] ? Switch+4:DefLogName,sizeof(LogName));
        break;
      }
      if (stricomp(&Switch[1],"SND")==0)
      {
        Sound=true;
        break;
      }
      if (stricomp(&Switch[1],"ERR")==0)
      {
        MsgStream=MSG_STDERR;
        break;
      }
      if (strnicomp(&Switch[1],"EML",3)==0)
      {
        strncpy(EmailTo,Switch[4] ? Switch+4:"@",sizeof(EmailTo));
        EmailTo[sizeof(EmailTo)-1]=0;
        break;
      }
      if (stricomp(&Switch[1],"NUL")==0)
      {
        MsgStream=MSG_NULL;
        break;
      }
      if (toupper(Switch[1])=='D')
      {
        for (int I=2;Switch[I]!=0;I++)
          switch(toupper(Switch[I]))
          {
            case 'Q':
              MsgStream=MSG_ERRONLY;
              break;
            case 'C':
              DisableCopyright=true;
              break;
            case 'D':
              DisableDone=true;
              break;
            case 'P':
              DisablePercentage=true;
              break;
          }
        break;
      }
      if (stricomp(&Switch[1],"OFF")==0)
      {
        Shutdown=true;
        break;
      }
      break;
    case 'T':
      switch(toupper(Switch[1]))
      {
        case 'K':
          ArcTime=ARCTIME_KEEP;
          break;
        case 'L':
          ArcTime=ARCTIME_LATEST;
          break;
        case 'O':
          FileTimeBefore.SetAgeText(Switch+2);
          break;
        case 'N':
          FileTimeAfter.SetAgeText(Switch+2);
          break;
        case 'B':
          FileTimeBefore.SetIsoText(Switch+2);
          break;
        case 'A':
          FileTimeAfter.SetIsoText(Switch+2);
          break;
        case 'S':
          {
            EXTTIME_MODE Mode=EXTTIME_HIGH3;
            bool CommonMode=Switch[2]>='0' && Switch[2]<='4';
            if (CommonMode)
              Mode=(EXTTIME_MODE)(Switch[2]-'0');
            if (Switch[2]=='-')
              Mode=EXTTIME_NONE;
            if (CommonMode || Switch[2]=='-' || Switch[2]=='+' || Switch[2]==0)
              xmtime=xctime=xatime=Mode;
            else
            {
              if (Switch[3]>='0' && Switch[3]<='4')
                Mode=(EXTTIME_MODE)(Switch[3]-'0');
              if (Switch[3]=='-')
                Mode=EXTTIME_NONE;
              switch(toupper(Switch[2]))
              {
                case 'M':
                  xmtime=Mode;
                  break;
                case 'C':
                  xctime=Mode;
                  break;
                case 'A':
                  xatime=Mode;
                  break;
                case 'R':
                  xarctime=Mode;
                  break;
              }
            }
          }
          break;
        case '-':
          Test=false;
          break;
        case 0:
          Test=true;
          break;
        default:
          BadSwitch(Switch);
          break;
      }
      break;
    case 'A':
      switch(toupper(Switch[1]))
      {
        case 'C':
          ClearArc=true;
          break;
        case 'D':
          AppendArcNameToPath=true;
          break;
        case 'G':
          if (Switch[2]=='-' && Switch[3]==0)
            GenerateArcName=0;
          else
          {
            GenerateArcName=true;
            strncpy(GenerateMask,Switch+2,sizeof(GenerateMask));
          }
          break;
        case 'N': //reserved for archive name
          break;
        case 'O':
          AddArcOnly=true;
          break;
        case 'P':
          strcpy(ArcPath,Switch+2);
          break;
        case 'S':
          SyncFiles=true;
          break;
      }
      break;
    case 'D':
      if (Switch[2]==0)
        switch(toupper(Switch[1]))
        {
          case 'S':
            DisableSortSolid=true;
            break;
          case 'H':
            OpenShared=true;
            break;
          case 'F':
            DeleteFiles=true;
            break;
        }
      break;
    case 'O':
      switch(toupper(Switch[1]))
      {
        case '+':
          Overwrite=OVERWRITE_ALL;
          break;
        case '-':
          Overwrite=OVERWRITE_NONE;
          break;
        case 'W':
          ProcessOwners=true;
          break;
#ifdef SAVE_LINKS
        case 'L':
          SaveLinks=true;
          break;
#endif
#ifdef _WIN_32
        case 'S':
          SaveStreams=true;
          break;
		case 'C':
          SetCompressedAttr=true;
          break;
#endif
        default :
          BadSwitch(Switch);
          break;
      }
      break;
    case 'R':
      switch(toupper(Switch[1]))
      {
        case 0:
          Recurse=RECURSE_ALWAYS;
          break;
        case '-':
          Recurse=0;
          break;
        case '0':
          Recurse=RECURSE_WILDCARDS;
          break;
        case 'I':
          {
            Priority=atoi(Switch+2);
            char *ChPtr=strchr(Switch+2,':');
            if (ChPtr!=NULL)
			{
              SleepTime=atoi(ChPtr+1);
              InitSystemOptions(SleepTime);
            }
            SetPriority(Priority);
          }
          break;
      }
      break;
    case 'Y':
      AllYes=true;
      break;
    case 'N':
    case 'X':
      if (Switch[1]!=0)
      {
        StringList *Args=toupper(Switch[0])=='N' ? InclArgs:ExclArgs;
        if (Switch[1]=='@' && !IsWildcard(Switch))
          ReadTextFile(Switch+2,Args,false,true,true,true,true);
        else
          Args->AddString(Switch+1);
     }
     break;
    case 'E':
      switch(toupper(Switch[1]))
      {
        case 'P':
          switch(Switch[2])
          {
            case 0:
              ExclPath=EXCL_SKIPWHOLEPATH;
              break;
            case '1':
              ExclPath=EXCL_BASEPATH;
              break;
            case '2':
              ExclPath=EXCL_SAVEFULLPATH;
              break;
            case '3':
              ExclPath=EXCL_ABSPATH;
              break;
          }
          break;
        case 'D':
          ExclEmptyDir=true;
          break;
        case 'E':
          ProcessEA=false;
          break;
        case 'N':
          NoEndBlock=true;
          break;
        default:
          if (Switch[1]=='+')
          {
            InclFileAttr=GetExclAttr(&Switch[2]);
            InclAttrSet=true;
          }
          else
            ExclFileAttr=GetExclAttr(&Switch[1]);
          break;
      }
      break;
    case 'P':
      if (Switch[1]==0)
      {
        GetPassword(PASSWORD_GLOBAL,NULL,Password,sizeof(Password));
        eprintf("\n");
      }
      else
        strncpy(Password,Switch+1,sizeof(Password));
      break;
    case 'H':
      if (toupper(Switch[1])=='P')
      {
        EncryptHeaders=true;
        if (Switch[2]!=0)
          strncpy(Password,Switch+2,sizeof(Password));
        else
          if (*Password==0)
          {
            GetPassword(PASSWORD_GLOBAL,NULL,Password,sizeof(Password));
            eprintf("\n");
          }
      }
      break;
    case 'Z':
      strncpy(CommentFile,Switch[1]!=0 ? Switch+1:"stdin",sizeof(CommentFile));
      break;
    case 'M':
      switch(toupper(Switch[1]))
      {
        case 'C':
          {
            char *Str=Switch+2;
            if (*Str=='-')
              for (unsigned int I=0;I<sizeof(FilterModes)/sizeof(FilterModes[0]);I++)
                FilterModes[I].State=FILTER_DISABLE;
            else
              while (*Str)
              {
                int Param1=0,Param2=0;
                FilterState State=FILTER_AUTO;
                FilterType Type=FILTER_NONE;
                if (isdigit(*Str))
                {
                  Param1=atoi(Str);
                  while (isdigit(*Str))
                    Str++;
                }
                if (*Str==':' && isdigit(Str[1]))
                {
                  Param2=atoi(++Str);
                  while (isdigit(*Str))
                    Str++;
                }
                switch(toupper(*(Str++)))
                {
                  case 'T': Type=FILTER_PPM;         break;
                  case 'E': Type=FILTER_E8;          break;
                  case 'D': Type=FILTER_DELTA;       break;
                  case 'A': Type=FILTER_AUDIO;       break;
                  case 'C': Type=FILTER_RGB;         break;
                  case 'I': Type=FILTER_ITANIUM;     break;
                  case 'L': Type=FILTER_UPCASETOLOW; break;
                }
                if (*Str=='+' || *Str=='-')
                  State=*(Str++)=='+' ? FILTER_FORCE:FILTER_DISABLE;
                FilterModes[Type].State=State;
                FilterModes[Type].Param1=Param1;
                FilterModes[Type].Param2=Param2;
              }
            }
          break;
        case 'M':
          break;
        case 'D':
          {
            if ((WinSize=atoi(&Switch[2]))==0)
              WinSize=0x10000<<(toupper(Switch[2])-'A');
            else
              WinSize*=1024;
            if (!CheckWinSize())
              BadSwitch(Switch);
          }
          break;
        case 'S':
          {
            char *Names=Switch+2,DefNames[512];
            if (*Names==0)
            {
              strcpy(DefNames,DefaultStoreList);
              Names=DefNames;
            }
            while (*Names!=0)
            {
              char *End=strchr(Names,';');
              if (End!=NULL)
                *End=0;
              if (*Names=='.')
                Names++;
              char Mask[NM];
              if (strpbrk(Names,"*?.")==NULL)
                sprintf(Mask,"*.%s",Names);
              else
                strcpy(Mask,Names);
              StoreArgs->AddString(Mask);
              if (End==NULL)
                break;
              Names=End+1;
            }
          }
          break;
        default:
          Method=Switch[1]-'0';
          if (Method>5 || Method<0)
            BadSwitch(Switch);
          break;
      }
      break;
    case 'V':
      switch(toupper(Switch[1]))
      {
#ifdef _WIN_32
        case 'D':
          EraseDisk=true;
          break;
#endif
        case 'N':
          OldNumbering=true;
          break;
        case 'P':
          VolumePause=true;
          break;
        case 'E':
          if (toupper(Switch[2])=='R')
            VersionControl=atoi(Switch+3)+1;
          break;
        case '-':
          VolSize=0;
          break;
        default:
          {
            Int64 NewVolSize=atoil(&Switch[1]);

            if (NewVolSize==0)
              NewVolSize=INT64ERR;
            else
              switch (Switch[strlen(Switch)-1])
              {
                case 'f':
                case 'F':
                  switch(int64to32(NewVolSize))
                  {
                    case 360:
                      NewVolSize=362496;
                      break;
                    case 720:
                      NewVolSize=730112;
                      break;
                    case 1200:
                      NewVolSize=1213952;
                      break;
                    case 1440:
                      NewVolSize=1457664;
                      break;
                    case 2880:
                      NewVolSize=2915328;
                      break;
                  }
                  break;
                case 'k':
                  NewVolSize*=1024;
                  break;
                case 'm':
                  NewVolSize*=1024*1024;
                  break;
                case 'M':
                  NewVolSize*=1000*1000;
                  break;
                case 'g':
                  NewVolSize*=1024*1024;
                  NewVolSize*=1024;
                  break;
                case 'G':
                  NewVolSize*=1000*1000;
                  NewVolSize*=1000;
                  break;
                case 'b':
                case 'B':
                  break;
                default:
                  NewVolSize*=1000;
                  break;
              }
            if (VolSize==0)
              VolSize=NewVolSize;
            else
              NextVolSizes.Push(NewVolSize);
          }
          break;
      }
      break;
    case 'F':
      if (Switch[1]==0)
        FreshFiles=true;
      break;
    case 'U':
      if (Switch[1]==0)
        UpdateFiles=true;
      break;
    case 'W':
      strncpy(TempPath,&Switch[1],sizeof(TempPath)-1);
      AddEndSlash(TempPath);
      break;
    case 'S':
      if (strnicomp(Switch,"SFX",3)==0)
      {
        const char *SFXName=Switch[3] ? Switch+3:DefSFXName;
        if (PointToName(SFXName)!=SFXName || FileExist(SFXName))
          strcpy(SFXModule,SFXName);
        else
          GetConfigName(SFXName,SFXModule,true);
      }
      if (isdigit(Switch[1]))
      {
        Solid|=SOLID_COUNT;
        SolidCount=atoi(&Switch[1]);
      }
      else
        switch(toupper(Switch[1]))
        {
          case 0:
            Solid|=SOLID_NORMAL;
            break;
          case '-':
            Solid=SOLID_NONE;
            break;
          case 'E':
            Solid|=SOLID_FILEEXT;
            break;
          case 'V':
            Solid|=Switch[2]=='-' ? SOLID_VOLUME_DEPENDENT:SOLID_VOLUME_INDEPENDENT;
            break;
          case 'D':
            Solid|=SOLID_VOLUME_DEPENDENT;
            break;
        }
      break;
    case 'C':
      if (Switch[2]==0)
        switch(toupper(Switch[1]))
        {
          case '-':
            DisableComment=true;
            break;
          case 'U':
            ConvertNames=NAMES_UPPERCASE;
            break;
          case 'L':
            ConvertNames=NAMES_LOWERCASE;
            break;
        }
      break;
    case 'K':
      switch(toupper(Switch[1]))
      {
        case 'B':
          KeepBroken=true;
          break;
        case 0:
          Lock=true;
          break;
      }
      break;
#ifndef GUI
    case '?' :
      OutHelp();
      break;
#endif
    default :
      BadSwitch(Switch);
      break;
  }
}
#endif


#if !defined(SFX_MODULE) && !defined(_WIN_CE)
void CommandData::BadSwitch(char *Switch)
{
  mprintf(St(MUnknownOption),Switch);
  ErrHandler.Exit(USER_ERROR);
}
#endif


#ifndef GUI
void CommandData::OutTitle()
{
  if (BareOutput || DisableCopyright)
    return;
#if defined(__GNUC__) && defined(SFX_MODULE)
  mprintf(St(MCopyrightS));
#else
#ifndef SILENT
  static bool TitleShown=false;
  if (TitleShown)
    return;
  TitleShown=true;
  char Version[50];
  int Beta=RARVER_BETA;
  if (Beta!=0)
    sprintf(Version,"%d.%02d %s %d",RARVER_MAJOR,RARVER_MINOR,St(MBeta),RARVER_BETA);
  else
    sprintf(Version,"%d.%02d",RARVER_MAJOR,RARVER_MINOR);
#ifdef UNRAR
  mprintf(St(MUCopyright),Version,RARVER_YEAR);
#else
#endif
#endif
#endif
}
#endif


void CommandData::OutHelp()
{
#if !defined(GUI) && !defined(SILENT)
  OutTitle();
  static MSGID Help[]={
#ifdef SFX_MODULE
    MCHelpCmd,MSHelpCmdE,MSHelpCmdT,MSHelpCmdV
#elif defined(UNRAR)
    MUNRARTitle1,MRARTitle2,MCHelpCmd,MCHelpCmdE,MCHelpCmdL,MCHelpCmdP,
    MCHelpCmdT,MCHelpCmdV,MCHelpCmdX,MCHelpSw,MCHelpSwm,MCHelpSwAC,MCHelpSwAD,
    MCHelpSwAP,MCHelpSwAVm,MCHelpSwCm,MCHelpSwCFGm,MCHelpSwCL,MCHelpSwCU,
    MCHelpSwDH,MCHelpSwEP,MCHelpSwEP3,MCHelpSwF,MCHelpSwIDP,MCHelpSwIERR,
    MCHelpSwINUL,MCHelpSwIOFF,MCHelpSwKB,MCHelpSwN,MCHelpSwNa,MCHelpSwNal,
    MCHelpSwOp,MCHelpSwOm,MCHelpSwOC,MCHelpSwOW,MCHelpSwP,MCHelpSwPm,
    MCHelpSwR,MCHelpSwRI,MCHelpSwTA,MCHelpSwTB,MCHelpSwTN,MCHelpSwTO,
    MCHelpSwTS,MCHelpSwU,MCHelpSwVUnr,MCHelpSwVER,MCHelpSwVP,MCHelpSwX,
    MCHelpSwXa,MCHelpSwXal,MCHelpSwY
#else
    MRARTitle1,MRARTitle2,MCHelpCmd,MCHelpCmdA,MCHelpCmdC,MCHelpCmdCF,
    MCHelpCmdCW,MCHelpCmdD,MCHelpCmdE,MCHelpCmdF,MCHelpCmdI,MCHelpCmdK,
    MCHelpCmdL,MCHelpCmdM,MCHelpCmdP,MCHelpCmdR,MCHelpCmdRC,MCHelpCmdRN,
    MCHelpCmdRR,MCHelpCmdRV,MCHelpCmdS,MCHelpCmdT,MCHelpCmdU,MCHelpCmdV,
    MCHelpCmdX,MCHelpSw,MCHelpSwm,MCHelpSwAC,MCHelpSwAD,MCHelpSwAG,
    MCHelpSwAO,MCHelpSwAP,MCHelpSwAS,MCHelpSwAV,MCHelpSwAVm,MCHelpSwCm,
    MCHelpSwCFGm,MCHelpSwCL,MCHelpSwCU,MCHelpSwDF,MCHelpSwDH,MCHelpSwDS,
    MCHelpSwEa,MCHelpSwED,MCHelpSwEE,MCHelpSwEN,MCHelpSwEP,MCHelpSwEP1,
    MCHelpSwEP2,MCHelpSwEP3,MCHelpSwF,MCHelpSwHP,MCHelpSwIDP,MCHelpSwIEML,
    MCHelpSwIERR,MCHelpSwILOG,MCHelpSwINUL,MCHelpSwIOFF,MCHelpSwISND,
    MCHelpSwK,MCHelpSwKB,MCHelpSwMn,MCHelpSwMC,MCHelpSwMD,MCHelpSwMS,
    MCHelpSwN,MCHelpSwNa,MCHelpSwNal,MCHelpSwOp,MCHelpSwOm,MCHelpSwOC,
    MCHelpSwOL,MCHelpSwOS,MCHelpSwOW,MCHelpSwP,MCHelpSwPm,MCHelpSwR,
    MCHelpSwR0,MCHelpSwRI,MCHelpSwRR,MCHelpSwRV,MCHelpSwS,MCHelpSwSm,
    MCHelpSwSFX,MCHelpSwSI,MCHelpSwT,MCHelpSwTA,MCHelpSwTB,MCHelpSwTK,
    MCHelpSwTL,MCHelpSwTN,MCHelpSwTO,MCHelpSwTS,MCHelpSwU,MCHelpSwV,
    MCHelpSwVn,MCHelpSwVD,MCHelpSwVER,MCHelpSwVN,MCHelpSwVP,MCHelpSwW,
    MCHelpSwX,MCHelpSwXa,MCHelpSwXal,MCHelpSwY,MCHelpSwZ
#endif
  };

  for (int I=0;I<sizeof(Help)/sizeof(Help[0]);I++)
  {
#ifndef SFX_MODULE
#ifdef DISABLEAUTODETECT
    if (Help[I]==MCHelpSwV)
      continue;
#endif
#ifndef _WIN_32
    static MSGID Win32Only[]={
      MCHelpSwIEML,MCHelpSwVD,MCHelpSwAC,MCHelpSwAO,MCHelpSwOS,MCHelpSwIOFF,
      MCHelpSwEP2,MCHelpSwOC
    };
    bool Found=false;
    for (int J=0;J<sizeof(Win32Only)/sizeof(Win32Only[0]);J++)
      if (Help[I]==Win32Only[J])
      {
        Found=true;
        break;
      }
    if (Found)      continue;
#endif
#if !defined(_UNIX) && !defined(_WIN_32)
    if (Help[I]==MCHelpSwOW)
      continue;
#endif
#ifndef SAVE_LINKS
    if (Help[I]==MCHelpSwOL)
      continue;
#endif
#if defined(_WIN_32)
    if (Help[I]==MCHelpSwRI)
      continue;
#endif
#if !defined(_BEOS)
    if (Help[I]==MCHelpSwEE)
    {
#if defined(_EMX) && !defined(_DJGPP)
      if (_osmode != OS2_MODE)
        continue;
#else
      continue;
#endif
    }
#endif
#endif
    mprintf(St(Help[I]));
  }
  mprintf("\n");
  ErrHandler.Exit(0);
#endif
}


bool CommandData::ExclCheckArgs(StringList *Args,char *CheckName,bool CheckFullPath,int MatchMode)
{
  char *Name=ConvertPath(CheckName,NULL);
  char FullName[NM],*CurName;
  *FullName=0;
  Args->Rewind();
  while ((CurName=Args->GetString())!=NULL)
#ifndef SFX_MODULE
    if (CheckFullPath && IsFullPath(CurName))
    {
      if (*FullName==0)
        ConvertNameToFull(CheckName,FullName);
      if (CmpName(CurName,FullName,MatchMode))
        return(true);
    }
    else
#endif
      if (CmpName(ConvertPath(CurName,NULL),Name,MatchMode))
        return(true);
  return(false);
}


bool CommandData::ExclCheck(char *CheckName,bool CheckFullPath)
{
  if (ExclCheckArgs(ExclArgs,CheckName,CheckFullPath,MATCH_WILDSUBPATH))
    return(true);
  if (InclArgs->ItemsCount()==0)
    return(false);
  if (ExclCheckArgs(InclArgs,CheckName,CheckFullPath,MATCH_NAMES))
    return(false);
  return(true);
}




#ifndef SFX_MODULE
bool CommandData::TimeCheck(RarTime &ft)
{
  if (FileTimeBefore.IsSet() && ft>=FileTimeBefore)
    return(true);
  if (FileTimeAfter.IsSet() && ft<=FileTimeAfter)
    return(true);
/*
  if (FileTimeOlder!=0 || FileTimeNewer!=0)
  {
    if (!TimeConverted)
    {
      if (FileTimeOlder!=0)
        FileTimeOlder=SecondsToDosTime(FileTimeOlder);
      if (FileTimeNewer!=0)
        FileTimeNewer=SecondsToDosTime(FileTimeNewer);
      TimeConverted=true;
    }
    if (FileTimeOlder!=0 && ft>=FileTimeOlder)
      return(true);
    if (FileTimeNewer!=0 && ft<=FileTimeNewer)
      return(true);

  }
*/
  return(false);
}
#endif


int CommandData::IsProcessFile(FileHeader &NewLhd,bool *ExactMatch,int MatchType)
{
  if (strlen(NewLhd.FileName)>=NM || strlenw(NewLhd.FileNameW)>=NM)
    return(0);
  if (ExclCheck(NewLhd.FileName,false))
    return(0);
#ifndef SFX_MODULE
  if (TimeCheck(NewLhd.mtime))
    return(0);
#endif
  char *ArgName;
  wchar *ArgNameW;
  FileArgs->Rewind();
  for (int StringCount=1;FileArgs->GetString(&ArgName,&ArgNameW);StringCount++)
  {
#ifndef SFX_MODULE
    bool Unicode=(NewLhd.Flags & LHD_UNICODE) || ArgNameW!=NULL;
    if (Unicode)
    {
      wchar NameW[NM],ArgW[NM],*NamePtr=NewLhd.FileNameW;
      bool CorrectUnicode=true;
      if (ArgNameW==NULL)
      {
        if (!CharToWide(ArgName,ArgW) || *ArgW==0)
          CorrectUnicode=false;
        ArgNameW=ArgW;
      }
      if ((NewLhd.Flags & LHD_UNICODE)==0)
      {
        if (!CharToWide(NewLhd.FileName,NameW) || *NameW==0)
          CorrectUnicode=false;
        NamePtr=NameW;
      }
      if (CmpName(ArgNameW,NamePtr,MatchType))
      {
        if (ExactMatch!=NULL)
          *ExactMatch=stricompcw(ArgNameW,NamePtr)==0;
        return(StringCount);
      }
      if (CorrectUnicode)
        continue;
    }
#endif
    if (CmpName(ArgName,NewLhd.FileName,MatchType))
    {
      if (ExactMatch!=NULL)
        *ExactMatch=stricompc(ArgName,NewLhd.FileName)==0;
      return(StringCount);
    }
  }
  return(0);
}


#ifndef _WIN_CE
void CommandData::ProcessCommand()
{
#ifndef SFX_MODULE
  if ((Command[1] && (strchr("FUADPXETK",*Command)!=NULL)) || *ArcName==0)
    OutHelp();

#ifdef _UNIX
  if (GetExt(ArcName)==NULL && (!FileExist(ArcName) || IsDir(GetFileAttr(ArcName))))
    strcat(ArcName,".rar");
#else
  if (GetExt(ArcName)==NULL)
    strcat(ArcName,".rar");
#endif

  if (strchr("AFUMD",*Command)==NULL)
  {
    StringList ArcMasks;
    ArcMasks.AddString(ArcName);
    ScanTree Scan(&ArcMasks,Recurse,SaveLinks,SCAN_SKIPDIRS);
    FindData FindData;
    while (Scan.GetNext(&FindData)==SCAN_SUCCESS)
      AddArcName(FindData.Name,FindData.NameW);
  }
  else
    AddArcName(ArcName,NULL);
#endif

  switch(Command[0])
  {
    case 'P':
    case 'X':
    case 'E':
    case 'T':
    case 'I':
      {
        CmdExtract Extract;
        Extract.DoExtract(this);
      }
      break;
#if !defined(GUI) && !defined(SILENT)
    case 'V':
    case 'L':
      ListArchive(this);
      break;
    default:
      OutHelp();
#endif
  }
#ifndef GUI
  if (!BareOutput)
  {
    mprintf("\n");
  }
#endif
}
#endif


void CommandData::AddArcName(char *Name,wchar *NameW)
{
  ArcNames->AddString(Name,NameW);
}


bool CommandData::GetArcName(char *Name,wchar *NameW,int MaxSize)
{
  if (!ArcNames->GetString(Name,NameW,NM))
    return(false);
  return(true);
}


bool CommandData::IsSwitch(int Ch)
{
#if defined(_WIN_32) || defined(_EMX)
  return(Ch=='-' || Ch=='/');
#else
  return(Ch=='-');
#endif
}


#ifndef SFX_MODULE
uint CommandData::GetExclAttr(char *Str)
{
  if (isdigit(*Str))
    return(strtol(Str,NULL,0));
  else
  {
    uint Attr;
    for (Attr=0;*Str;Str++)
      switch(toupper(*Str))
      {
#ifdef _UNIX
        case 'D':
          Attr|=S_IFDIR;
          break;
        case 'V':
          Attr|=S_IFCHR;
          break;
#elif defined(_WIN_32) || defined(_EMX)
        case 'R':
          Attr|=0x1;
          break;
        case 'H':
          Attr|=0x2;
          break;
        case 'S':
          Attr|=0x4;
          break;
        case 'D':
          Attr|=0x10;
          break;
        case 'A':
          Attr|=0x20;
          break;
#endif
      }
    return(Attr);
  }
}
#endif

#ifndef SFX_MODULE
bool CommandData::CheckWinSize()
{
  static unsigned int ValidSize[]={
    0x10000,0x20000,0x40000,0x80000,0x100000,0x200000,0x400000
  };
  for (unsigned int I=0;I<sizeof(ValidSize)/sizeof(ValidSize[0]);I++)
    if (WinSize==ValidSize[I])
      return(true);
  WinSize=0x400000;
  return(false);
}
#endif
