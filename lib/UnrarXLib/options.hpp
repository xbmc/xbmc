#ifndef _RAR_OPTIONS_
#define _RAR_OPTIONS_

#define DEFAULT_RECOVERY    -1

#define DEFAULT_RECVOLUMES  -10

enum PathExclMode {
  EXCL_NONE,EXCL_BASEPATH,EXCL_SKIPWHOLEPATH,EXCL_SAVEFULLPATH,
  EXCL_SKIPABSPATH,EXCL_ABSPATH};
enum {SOLID_NONE=0,SOLID_NORMAL=1,SOLID_COUNT=2,SOLID_FILEEXT=4,
      SOLID_VOLUME_DEPENDENT=8,SOLID_VOLUME_INDEPENDENT=16};
enum {ARCTIME_NONE,ARCTIME_KEEP,ARCTIME_LATEST};
enum EXTTIME_MODE {
  EXTTIME_NONE,EXTTIME_1S,EXTTIME_HIGH1,EXTTIME_HIGH2,EXTTIME_HIGH3
};
enum {NAMES_ORIGINALCASE,NAMES_UPPERCASE,NAMES_LOWERCASE};
enum MESSAGE_TYPE {MSG_STDOUT,MSG_STDERR,MSG_ERRONLY,MSG_NULL};
enum OVERWRITE_MODE { OVERWRITE_ASK,OVERWRITE_ALL,OVERWRITE_NONE};

#define     MAX_FILTERS           16
enum FilterState {FILTER_DEFAULT=0,FILTER_AUTO,FILTER_FORCE,FILTER_DISABLE};

struct FilterMode
{
  FilterState State;
  int Param1;
  int Param2;
};


class RAROptions
{
  public:
    RAROptions();
    ~RAROptions();
    void Init();

    uint ExclFileAttr;
    uint InclFileAttr;
    bool InclAttrSet;
    uint WinSize;
    char TempPath[NM];
    char SFXModule[NM];
    char ExtrPath[NM+16];
    wchar ExtrPathW[NM];
    char CommentFile[NM];
    char ArcPath[NM];
    char Password[MAXPASSWORD];
    bool EncryptHeaders;
    char LogName[NM];
    MESSAGE_TYPE MsgStream;
    bool Sound;
    OVERWRITE_MODE Overwrite;
    int Method;
    int Recovery;
    int RecVolNumber;
    bool DisablePercentage;
    bool DisableCopyright;
    bool DisableDone;
    int Solid;
    int SolidCount;
    bool ClearArc;
    bool AddArcOnly;
    bool AV;
    bool DisableComment;
    bool FreshFiles;
    bool UpdateFiles;
    PathExclMode ExclPath;
    int Recurse;
    Int64 VolSize;
    Array<Int64> NextVolSizes;
    int CurVolNum;
    bool AllYes;
    bool DisableViewAV;
    bool DisableSortSolid;
    int ArcTime;
    int ConvertNames;
    bool ProcessOwners;
    bool SaveLinks;
    int Priority;
    int SleepTime;
    bool KeepBroken;
    bool EraseDisk;
    bool OpenShared;
    bool ExclEmptyDir;
    bool DeleteFiles;
    bool SyncFiles;
    bool GenerateArcName;
    char GenerateMask[80];
    bool ProcessEA;
    bool SaveStreams;
    bool SetCompressedAttr;
    uint FileTimeOlder;
    uint FileTimeNewer;
    RarTime FileTimeBefore;
    RarTime FileTimeAfter;
    bool OldNumbering;
    bool Lock;
    bool Test;
    bool VolumePause;
    FilterMode FilterModes[MAX_FILTERS];
    char EmailTo[NM];
    int VersionControl;
    bool NoEndBlock;
    bool AppendArcNameToPath;
    bool Shutdown;
    EXTTIME_MODE xmtime;
    EXTTIME_MODE xctime;
    EXTTIME_MODE xatime;
    EXTTIME_MODE xarctime;
    char CompressStdin[NM];



#if defined(RARDLL)
    char DllDestName[NM];
    wchar DllDestNameW[NM];
    int DllOpMode;
    int DllError;
    LONG UserData;
    UNRARCALLBACK Callback;
    CHANGEVOLPROC ChangeVolProc;
    PROCESSDATAPROC ProcessDataProc;
#endif
};
#endif

