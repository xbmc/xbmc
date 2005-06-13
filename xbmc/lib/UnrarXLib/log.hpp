#ifndef _RAR_LOG_
#define _RAR_LOG_

void InitLogOptions(char *LogName);

#ifndef SILENT
void Log(const char *ArcName,const char *Format,...);
#endif

#ifdef SILENT
inline void Log(const char *a,const char *b,const char *c=NULL,const char *d=NULL) {}
#endif

#endif
