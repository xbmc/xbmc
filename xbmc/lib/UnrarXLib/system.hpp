#ifndef _RAR_SYSTEM_
#define _RAR_SYSTEM_

void InitSystemOptions(int SleepTime);
void SetPriority(int Priority);
void Wait();
bool EmailFile(char *FileName,char *MailTo);
void Shutdown();

#endif
