#ifndef _RAR_STRFN_
#define _RAR_STRFN_

const char *NullToEmpty(const char *Str);
const wchar *NullToEmpty(const wchar *Str);
char *IntNameToExt(const char *Name);
void ExtToInt(const char *Src,char *Dest);
void IntToExt(const char *Src,char *Dest);
char* strlower(char *Str);
char* strupper(char *Str);
int stricomp(const char *Str1,const char *Str2);
int strnicomp(const char *Str1,const char *Str2,int N);
char* RemoveEOL(char *Str);
char* RemoveLF(char *Str);
unsigned int loctolower(byte ch);
unsigned int loctoupper(byte ch);



bool LowAscii(const char *Str);
bool LowAscii(const wchar *Str);


#endif
