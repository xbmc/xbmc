#ifndef _RAR_ERRHANDLER_
#define _RAR_ERRHANDLER_

#if (defined(GUI) || !defined(_WIN_32)) && !defined(SFX_MODULE) && !defined(_WIN_CE) || defined(RARDLL)
#define ALLOW_EXCEPTIONS
#endif



#define rarmalloc malloc
#define rarcalloc calloc
#define rarrealloc realloc
#define rarfree free
#define rarstrdup strdup



enum { SUCCESS,WARNING,FATAL_ERROR,CRC_ERROR,LOCK_ERROR,WRITE_ERROR,
       OPEN_ERROR,USER_ERROR,MEMORY_ERROR,CREATE_ERROR,USER_BREAK=255};

class ErrorHandler
{
  private:
    void ErrMsg(const char *ArcName,const char *fmt,...);

    int ExitCode;
    int ErrCount;
    bool EnableBreak;
    bool Silent;
    bool DoShutdown;
  public:
    ErrorHandler();
    void Clean();
    void MemoryError();
    void OpenError(const char *FileName);
    void CloseError(const char *FileName);
    void ReadError(const char *FileName);
    bool AskRepeatRead(const char *FileName);
    void WriteError(const char *ArcName,const char *FileName);
    void WriteErrorFAT(const char *FileName);
    bool AskRepeatWrite(const char *FileName);
    void SeekError(const char *FileName);
    void MemoryErrorMsg();
    void OpenErrorMsg(const char *FileName);
    void OpenErrorMsg(const char *ArcName,const char *FileName);
    void CreateErrorMsg(const char *FileName);
    void CreateErrorMsg(const char *ArcName,const char *FileName);
    void ReadErrorMsg(const char *ArcName,const char *FileName);
    void WriteErrorMsg(const char *ArcName,const char *FileName);
    void Exit(int ExitCode);
    void SetErrorCode(int Code);
    int GetErrorCode() {return(ExitCode);}
    int GetErrorCount() {return(ErrCount);}
    void SetSignalHandlers(bool Enable);
    void Throw(int Code);
    void SetSilent(bool Mode) {Silent=Mode;};
    void SetShutdown(bool Mode) {DoShutdown=Mode;};
    void SysErrMsg();
};

#endif
