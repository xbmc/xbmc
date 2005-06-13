#ifndef _RAR_ARCHIVE_
#define _RAR_ARCHIVE_

class Pack;

enum {EN_LOCK=1,EN_VOL=2,EN_FIRSTVOL=4};

class Archive:public File
{
  private:
    bool IsSignature(byte *D);
    void UpdateLatestTime(FileHeader *CurBlock);
    void Protect(int RecSectors);
    void ConvertNameCase(char *Name);
    void ConvertNameCase(wchar *Name);
    void ConvertUnknownHeader();
    bool AddArcComment(char *NameToShow);
    int ReadOldHeader();
    void PrepareExtraTime(FileHeader *hd,EXTTIME_MODE etm,EXTTIME_MODE etc,EXTTIME_MODE eta,EXTTIME_MODE etarc,Array<byte> &TimeData);

#if !defined(SHELL_EXT) && !defined(NOCRYPT)
    CryptData HeadersCrypt;
    byte HeadersSalt[SALT_SIZE];
#endif
#ifndef SHELL_EXT
    ComprDataIO SubDataIO;
    byte SubDataSalt[SALT_SIZE];
#endif
    RAROptions *Cmd,DummyCmd;

    MarkHeader MarkHead;
    OldMainHeader OldMhd;

    int RecoverySectors;
    Int64 RecoveryPos;

    RarTime LatestTime;
    int LastReadBlock;
    int CurHeaderType;

    bool SilentOpen;
  public:
    Archive(RAROptions *InitCmd=NULL);
    bool IsArchive(bool EnableBroken);
    int SearchBlock(int BlockType);
    int SearchSubBlock(const char *Type);
    int ReadBlock(int BlockType);
    void WriteBlock(int BlockType,BaseBlock *wb=NULL);
    int PrepareNamesToWrite(char *Name,wchar *NameW,char *DestName,byte *DestNameW);
    void SetLhdSize();
    int ReadHeader();
    void CheckArc(bool EnableBroken);
    void CheckOpen(char *Name,wchar *NameW=NULL);
    bool WCheckOpen(char *Name,wchar *NameW=NULL);
    bool TestLock(int Mode);
    void MakeTemp();
    void CopyMainHeader(Archive &Src,bool CopySFX=true,char *NameToDisplay=NULL);
    bool ProcessToFileHead(Archive &Src,bool LastBlockAdded,
      Pack *Pack=NULL,const char *SkipName=NULL);
    void TmpToArc(Archive &Src);
    void CloseNew(int AdjustRecovery,bool CloseVolume);
    void WriteEndBlock(bool CloseVolume);
    void CopyFileRecord(Archive &Src);
    void CopyArchiveData(Archive &Src);
    bool GetComment(Array<byte> &CmtData);
    void ViewComment();
    void ViewFileComment();
    void SetLatestTime(RarTime *NewTime);
    void SeekToNext();
    bool CheckAccess();
    bool IsArcDir();
    bool IsArcLabel();
    void ConvertAttributes();
    int LhdSize();
    int LhdExtraSize();
    int GetRecoverySize(bool Required);
    void VolSubtractHeaderSize(int SubSize);
    void AddSubData(byte *SrcData,int DataSize,File *SrcFile,char *Name,bool AllowSplit);
    bool ReadSubData(Array<byte> *UnpData,File *DestFile);
    int GetHeaderType() {return(CurHeaderType);};
    int ReadCommentData(Array<byte> &CmtData);
    void WriteCommentData(byte *Data,int DataSize,bool FileComment);
    RAROptions* GetRAROptions() {return(Cmd);}
    void SetSilentOpen(bool Mode) {SilentOpen=Mode;}

    BaseBlock ShortBlock;
    MainHeader NewMhd;
    FileHeader NewLhd;
    EndArcHeader EndArcHead;
    SubBlockHeader SubBlockHead;
    FileHeader SubHead;
    CommentHeader CommHead;
    ProtectHeader ProtectHead;
    AVHeader AVHead;
    SignHeader SignHead;
    UnixOwnersHeader UOHead;
    MacFInfoHeader MACHead;
    EAHeader EAHead;
    StreamHeader StreamHead;

    Int64 CurBlockPos;
    Int64 NextBlockPos;

    bool OldFormat;
    bool Solid;
    bool Volume;
    bool MainComment;
    bool Locked;
    bool Signed;
    bool NotFirstVolume;
    bool Protected;
    bool Encrypted;
    unsigned int SFXSize;
    bool BrokenFileHeader;

    bool Splitting;

    ushort HeaderCRC;

    Int64 VolWrite;
    Int64 AddingFilesSize;
    uint AddingHeadersSize;

    bool NewArchive;

    char FirstVolumeName[NM];
    wchar FirstVolumeNameW[NM];
};

#endif
