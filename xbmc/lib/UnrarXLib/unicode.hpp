#ifndef _RAR_UNICODE_
#define _RAR_UNICODE_

#ifndef _EMX
#define MBFUNCTIONS
#endif

#if defined(MBFUNCTIONS) || defined(_WIN_32) || defined(_EMX) && !defined(_DJGPP)
#define UNICODE_SUPPORTED
#endif

#ifdef _WIN_32
#define DBCS_SUPPORTED
#endif

bool WideToChar(const wchar *Src,char *Dest,int DestSize=0x10000000);
bool CharToWide(const char *Src,wchar *Dest,int DestSize=0x10000000);
byte* WideToRaw(const wchar *Src,byte *Dest,int DestSize=0x10000000);
wchar* RawToWide(const byte *Src,wchar *Dest,int DestSize=0x10000000);
void WideToUtf(const wchar *Src,char *Dest,int DestSize);
void UtfToWide(const char *Src,wchar *Dest,int DestSize);
bool UnicodeEnabled();

int strlenw(const wchar *str);
wchar* strcpyw(wchar *dest,const wchar *src);
wchar* strncpyw(wchar *dest,const wchar *src,int n);
wchar* strcatw(wchar *dest,const wchar *src);
wchar* strncatw(wchar *dest,const wchar *src,int n);
int strcmpw(const wchar *s1,const wchar *s2);
int strncmpw(const wchar *s1,const wchar *s2,int n);
int stricmpw(const wchar *s1,const wchar *s2);
int strnicmpw(const wchar *s1,const wchar *s2,int n);
wchar *strchrw(const wchar *s,int c);
wchar* strrchrw(const wchar *s,int c);
wchar* strpbrkw(const wchar *s1,const wchar *s2);
wchar* strlowerw(wchar *Str);
wchar* strupperw(wchar *Str);
int toupperw(int ch);
int atoiw(const wchar *s);

#ifdef DBCS_SUPPORTED
class SupportDBCS
{
  public:
    SupportDBCS();
    void Init();

    char* charnext(const char *s);
    uint strlend(const char *s);
    char *strchrd(const char *s, int c);
    char *strrchrd(const char *s, int c);
    void copychrd(char *dest,const char *src);

    bool IsLeadByte[256];
    bool DBCSMode;
};

extern SupportDBCS gdbcs;

inline char* charnext(const char *s) {return (char *)(gdbcs.DBCSMode ? gdbcs.charnext(s):s+1);}
inline uint strlend(const char *s) {return (uint)(gdbcs.DBCSMode ? gdbcs.strlend(s):strlen(s));}
inline char* strchrd(const char *s, int c) {return (char *)(gdbcs.DBCSMode ? gdbcs.strchrd(s,c):strchr(s,c));}
inline char* strrchrd(const char *s, int c) {return (char *)(gdbcs.DBCSMode ? gdbcs.strrchrd(s,c):strrchr(s,c));}
inline void copychrd(char *dest,const char *src) {if (gdbcs.DBCSMode) gdbcs.copychrd(dest,src); else *dest=*src;}
inline bool IsDBCSMode() {return(gdbcs.DBCSMode);}
inline void InitDBCS() {gdbcs.Init();}

#else
#define charnext(s) ((s)+1)
#define strlend strlen
#define strchrd strchr
#define strrchrd strrchr
#define IsDBCSMode() (true)
inline void copychrd(char *dest,const char *src) {*dest=*src;}
#endif

#endif
