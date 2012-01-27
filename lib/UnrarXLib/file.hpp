#ifndef _RAR_FILE_
#define _RAR_FILE_

#ifdef _WIN_32
typedef HANDLE FileHandle;
#define BAD_HANDLE INVALID_HANDLE_VALUE
#else
typedef FILE* FileHandle;
#define BAD_HANDLE NULL
#endif

class RAROptions;

enum FILE_HANDLETYPE {FILE_HANDLENORMAL,FILE_HANDLESTD,FILE_HANDLEERR};

enum FILE_ERRORTYPE {FILE_SUCCESS,FILE_NOTFOUND,FILE_READERROR};

struct FileStat
{
  uint FileAttr;
  uint FileTime;
  Int64 FileSize;
  bool IsDir;
};

namespace XFILE {
  class CFile;
};

class File
{
  private:
    //void AddFileToList(FileHandle hFile);
    void AddFileToList();

    //FileHandle hFile;
    XFILE::CFile &m_File;

    bool LastWrite;
    FILE_HANDLETYPE HandleType;
    bool SkipClose;
    bool IgnoreReadErrors;
    bool NewFile;
    bool AllowDelete;
    bool AllowExceptions;
  protected:
    bool OpenShared;
  public:
    char FileName[NM];
    wchar FileNameW[NM];

    FILE_ERRORTYPE ErrorType;

    uint CloseCount;
  public:
    File();
    virtual ~File();
    void operator = (File &SrcFile);
    bool Open(const char *Name,const wchar *NameW=NULL,bool OpenShared=false,bool Update=false);
    void TOpen(const char *Name,const wchar *NameW=NULL);
    bool WOpen(const char *Name,const wchar *NameW=NULL);
    bool Create(const char *Name,const wchar *NameW=NULL);
    void TCreate(const char *Name,const wchar *NameW=NULL);
    bool WCreate(const char *Name,const wchar *NameW=NULL);
    bool Close();
    void Flush();
    bool Delete();
    bool Rename(const char *NewName);
    void Write(const void *Data,int Size);
    int Read(void *Data,int Size);
    int DirectRead(void *Data,int Size);
    void Seek(Int64 Offset,int Method);
    bool RawSeek(Int64 Offset,int Method);
    Int64 Tell();
    void Prealloc(Int64 Size);
    byte GetByte();
    void PutByte(byte Byte);
    bool Truncate();
    void SetOpenFileTime(RarTime *ftm,RarTime *ftc=NULL,RarTime *fta=NULL);
    void SetCloseFileTime(RarTime *ftm,RarTime *fta=NULL);
    static void SetCloseFileTimeByName(const char *Name,RarTime *ftm,RarTime *fta);
    void SetOpenFileStat(RarTime *ftm,RarTime *ftc,RarTime *fta);
    void SetCloseFileStat(RarTime *ftm,RarTime *fta,uint FileAttr);
    void GetOpenFileTime(RarTime *ft);
    //bool IsOpened() {return(hFile!=BAD_HANDLE);};
    bool IsOpened() {return true;}; // wtf
    Int64 FileLength();
    void SetHandleType(FILE_HANDLETYPE Type);
    FILE_HANDLETYPE GetHandleType() {return(HandleType);};
    bool IsDevice();
    void fprintf(const char *fmt,...);
    static bool RemoveCreated();
    //FileHandle GetHandle() {return(hFile);};
    void SetIgnoreReadErrors(bool Mode) {IgnoreReadErrors=Mode;};
    char *GetName() {return(FileName);}
    long Copy(File &Dest,Int64 Length=INT64ERR);
    void SetAllowDelete(bool Allow) {AllowDelete=Allow;}
    void SetExceptions(bool Allow) {AllowExceptions=Allow;}
};

#endif
