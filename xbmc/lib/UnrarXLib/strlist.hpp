#ifndef _RAR_STRLIST_
#define _RAR_STRLIST_

class StringList
{
  private:
    Array<char> StringData;
    unsigned int CurPos;

    Array<wchar> StringDataW;
    unsigned int CurPosW;

    Array<int> PosDataW;
    uint PosDataItem;

    uint StringsCount;

    uint SaveCurPos[16],SaveCurPosW[16],SavePosDataItem[16],SavePosNumber;
  public:
    StringList();
    ~StringList();
    void Reset();
    unsigned int AddString(const char *Str);
    unsigned int AddString(const char *Str,const wchar *StrW);
    bool GetString(char *Str,int MaxLength);
    bool GetString(char *Str,wchar *StrW,int MaxLength);
    bool GetString(char *Str,wchar *StrW,int MaxLength,int StringNum);
    char* GetString();
    bool GetString(char **Str,wchar **StrW);
    char* GetString(unsigned int StringPos);
    void Rewind();
    unsigned int ItemsCount() {return(StringsCount);};
    int GetBufferSize();
    bool Search(char *Str,wchar *StrW,bool CaseSensitive);
    void SavePosition();
    void RestorePosition();
};

#endif
