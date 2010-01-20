#ifndef _RAR_FINDDATA_
#define _RAR_FINDDATA_

struct FindData
{
  char Name[NM];
  wchar NameW[NM];
  Int64 Size;
  uint FileAttr;
  uint FileTime;
  bool IsDir;
  RarTime mtime;
  RarTime ctime;
  RarTime atime;
#ifdef _WIN_32
  char ShortName[NM];
  FILETIME ftCreationTime; 
  FILETIME ftLastAccessTime; 
  FILETIME ftLastWriteTime; 
#endif
  bool Error;
};

class FindFile
{
  private:
#ifdef _WIN_32
    static HANDLE Win32Find(HANDLE hFind,const char *Mask,const wchar *MaskW,struct FindData *fd);
#endif

    char FindMask[NM];
    wchar FindMaskW[NM];
    int FirstCall;
#ifdef _WIN_32
    HANDLE hFind;
#else
    DIR *dirp;
#endif
  public:
    FindFile();
    ~FindFile();
    void SetMask(const char *FindMask);
    void SetMaskW(const wchar *FindMaskW);
    bool Next(struct FindData *fd,bool GetSymLink=false);
    static bool FastFind(const char *FindMask,const wchar *FindMaskW,struct FindData *fd,bool GetSymLink=false);
};

#endif
