#ifndef _RAR_CONSIO_
#define _RAR_CONSIO_

enum {ALARM_SOUND,ERROR_SOUND,QUESTION_SOUND};

enum PASSWORD_TYPE {PASSWORD_GLOBAL,PASSWORD_FILE,PASSWORD_ARCHIVE};

void InitConsoleOptions(MESSAGE_TYPE MsgStream,bool Sound);

#ifndef SILENT
void mprintf(const char *fmt,...);
void eprintf(const char *fmt,...);
void Alarm();
void GetPasswordText(char *Str,int MaxLength);
unsigned int GetKey();
bool GetPassword(PASSWORD_TYPE Type,const char *FileName,char *Password,int MaxLength);
int Ask(const char *AskStr);
#endif

int KbdAnsi(char *Addr,int Size);
void OutComment(char *Comment,int Size);

#ifdef SILENT
inline void mprintf(const char *fmt,byte *a) {}
inline void mprintf(const char *fmt,char a,char b,char c,char d,char e,char f,char g=0,char h=0,char i=0,char j=0) {}
inline void mprintf(const char *fmt,int a,int b,int c=0,int d=0) {}
inline void mprintf(const char *fmt,const char *a=NULL,const char *b=NULL,const char *c=NULL) {}
inline void eprintf(const char *fmt,const char *a=NULL,const char *b=NULL,const char *c=NULL) {}
inline void mprintf(const char *fmt,int b,const char *c=NULL,const char *d=NULL,int e=0) {}
inline void eprintf(const char *fmt,int b,const char *c=NULL,const char *d=NULL,int e=0) {}
inline void mprintf(const char *fmt,const char *a,int b) {}
inline void eprintf(const char *fmt,const char *a,int b) {}
inline void Alarm() {}
inline void GetPasswordText(char *Str,int MaxLength) {}
inline unsigned int GetKey() {return(0);}
inline bool GetPassword(PASSWORD_TYPE Type,const char *FileName,char *Password,int MaxLength) {return(false);}
inline int Ask(const char *AskStr) {return(0);}
#endif

#endif
