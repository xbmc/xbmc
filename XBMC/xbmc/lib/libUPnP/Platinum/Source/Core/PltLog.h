/*****************************************************************
|
|   Platinum - Log Utilities
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#ifndef _PLT_LOG_H_
#define _PLT_LOG_H_

/*----------------------------------------------------------------------
|   PLT_LOG_LEVELS
+---------------------------------------------------------------------*/
#define PLT_LOG_LEVEL_1     0x00000001
#define PLT_LOG_LEVEL_2     0x00000010
#define PLT_LOG_LEVEL_3     0x00000100
#define PLT_LOG_LEVEL_4     0x00001000
#define PLT_LOG_LEVEL_MAX   0x11111111

extern unsigned long LogLevel;

/*----------------------------------------------------------------------
|   PLT_Log
+---------------------------------------------------------------------*/
void PLT_Print(const char* message);
void PLT_Print(unsigned long level, const char* message);
void PLT_Log(unsigned long level, const char* format, ...);

/*----------------------------------------------------------------------
|   NPT_Log
+---------------------------------------------------------------------*/
void PLT_SetLogLevel(unsigned long level);


#endif /* _PLT_LOG_H_ */
