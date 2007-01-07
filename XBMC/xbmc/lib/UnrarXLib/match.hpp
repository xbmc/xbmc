#ifndef _RAR_MATCH_
#define _RAR_MATCH_

enum {MATCH_NAMES,MATCH_PATH,MATCH_EXACTPATH,MATCH_SUBPATH,MATCH_WILDSUBPATH};

#define MATCH_MODEMASK           0x0000ffff

bool CmpName(char *Wildcard,char *Name,int CmpPath);
bool CmpName(wchar *Wildcard,wchar *Name,int CmpPath);

int stricompc(const char *Str1,const char *Str2);
int stricompcw(const wchar *Str1,const wchar *Str2);
int strnicompc(const char *Str1,const char *Str2,int N);
int strnicompcw(const wchar *Str1,const wchar *Str2,int N);

#endif
