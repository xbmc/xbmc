/*****************************************************************
|
|   Neptune - Logging Support
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/
/** @file
* Header file for logging
*/

#ifndef _NPT_LOGGING_H_
#define _NPT_LOGGING_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptDebug.h"
#include "NptTypes.h"
#include "NptTime.h"
#include "NptStrings.h"
#include "NptList.h"
#include "NptStreams.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
class NPT_LogRecord {
public:
    const char*   m_LoggerName;
    int           m_Level;
    const char*   m_Message;
    NPT_TimeStamp m_TimeStamp;
    const char*   m_SourceFile;
    unsigned int  m_SourceLine;
};

class NPT_LogHandler {
public:
    // class methods
    static NPT_Result Create(const char*      logger_name,
                             const char*      handler_name,
                             NPT_LogHandler*& handler);

    // methods
    virtual void Log(const NPT_LogRecord& record) = 0;
    virtual ~NPT_LogHandler() {}
};

class NPT_Logger {
public:
    // methods
    NPT_Logger(const char* name);
    ~NPT_Logger();
    void Log(int          level, 
             const char*  source_file,
             unsigned int source_line,
             const char*  msg, 
                          ...);

    NPT_Result AddHandler(NPT_LogHandler* handler);
    NPT_Result SetParent(NPT_Logger* parent);
    int        GetLevel() const { return m_Level; }

private:
    // members
    NPT_String                m_Name;
    int                       m_Level;
    bool                      m_LevelIsInherited;
    bool                      m_ForwardToParent;
    NPT_Logger*               m_Parent;
    NPT_List<NPT_LogHandler*> m_Handlers;

    // friends
    friend class NPT_LogManager;
};

typedef struct {
    NPT_Logger* logger;
    const char* name;
} NPT_LoggerReference;

class NPT_Log {
public:
    // class methods
    static int         GetLogLevel(const char* name);
    static const char* GetLogLevelName(int level);
    static const char* GetLogLevelAnsiColor(int level);
    static void        FormatRecordToStream(const NPT_LogRecord& record,
                                            NPT_OutputStream&    stream,
                                            bool                  use_colors,
                                            NPT_Flags            format_filter);
};

class NPT_LogConfigEntry {
public:
    NPT_LogConfigEntry(const char* key, const char* value) :
      m_Key(key), m_Value(value) {}
    NPT_String m_Key;
    NPT_String m_Value;
};

class NPT_LogManager {
public:
    // class methods
    static void SetConfig(const char* config);
    static bool ConfigValueIsBooleanTrue(NPT_String& value);
    static bool ConfigValueIsBooleanFalse(NPT_String& value);
    static NPT_Logger* GetLogger(const char* name);

    // methods
    NPT_LogManager();
    ~NPT_LogManager();
    NPT_String* GetConfigValue(const char* prefix, const char* suffix);
    NPT_Result  Configure();

private:
    // methods
    NPT_Result  SetConfigValue(const char* key, const char* value);
    NPT_Result  ParseConfig(const char* config, NPT_Size config_size);
    NPT_Result  ParseConfigSource(NPT_String& source);
    NPT_Result  ParseConfigFile(const char* filename);
    bool        HaveLoggerConfig(const char* name);
    NPT_Logger* FindLogger(const char* name);
    NPT_Result  ConfigureLogger(NPT_Logger* logger);

    // members
    bool                         m_Configured;
    NPT_List<NPT_LogConfigEntry> m_Config;
    NPT_List<NPT_Logger*>        m_Loggers;
    NPT_Logger*                  m_Root;
};

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define NPT_LOG_LEVEL_FATAL   700
#define NPT_LOG_LEVEL_SEVERE  600 
#define NPT_LOG_LEVEL_WARNING 500
#define NPT_LOG_LEVEL_INFO    400
#define NPT_LOG_LEVEL_FINE    300
#define NPT_LOG_LEVEL_FINER   200
#define NPT_LOG_LEVEL_FINEST  100 

#define NPT_LOG_LEVEL_OFF     32767
#define NPT_LOG_LEVEL_ALL     0

/*----------------------------------------------------------------------
|   macros
+---------------------------------------------------------------------*/
#define NPT_LOG_GET_LOGGER(_logger)                                   \
    if ((_logger).logger == NULL) {                                   \
        (_logger).logger = NPT_LogManager::GetLogger((_logger).name); \
    }

#define NPT_LOG(_level, _msg) NPT_LOG_L(_NPT_LocalLogger, (_level), (_msg))
#define NPT_LOG_1(_level, _msg, _arg1) \
    NPT_LOG_L1(_NPT_LocalLogger, (_level), (_msg), (_arg1)) 
#define NPT_LOG_2(_level, _msg, _arg1, _arg2) \
    NPT_LOG_L2(_NPT_LocalLogger, (_level), (_msg), (_arg1), (_arg2))
#define NPT_LOG_3(_level, _msg, _arg1, _arg2, _arg3) \
    NPT_LOG_L3(_NPT_LocalLogger, (_level), (_msg), (_arg1), (_arg2), (_arg3))
#define NPT_LOG_4(_level, _msg, _arg1, _arg2, _arg3, _arg4) \
    NPT_LOG_L4(_NPT_LocalLogger, (_level), (_msg), (_arg1), (_arg2), (_arg3), (_arg4))
#define NPT_LOG_5(_level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5) \
    NPT_LOG_L5(_NPT_LocalLogger, (_level), (_msg), (_arg1), (_arg2), (_arg3), (_arg4), (_arg5))
#define NPT_LOG_6(_level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6) \
    NPT_LOG_L6(_NPT_LocalLogger, (_level), (_msg), (_arg1), (_arg2), (_arg3), (_arg4), (_arg5), (_arg6))
#define NPT_LOG_7(_level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7) \
    NPT_LOG_L7(_NPT_LocalLogger, (_level), (_msg), (_arg1), (_arg2), (_arg3), (_arg4), (_arg5), (_arg6), (_arg7))
#define NPT_LOG_8(_level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7, _arg8) \
    NPT_LOG_L8(_NPT_LocalLogger, (_level), (_msg), (_arg1), (_arg2), (_arg3), (_arg4), (_arg5), (_arg6), (_arg7), (_arg8))
#define NPT_LOG_9(_level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7, _arg8, _arg9) \
    NPT_LOG_L9(_NPT_LocalLogger, (_level), (_msg), (_arg1), (_arg2), (_arg3), (_arg4), (_arg5), (_arg6), (_arg7), (_arg8), (_arg9))


#if defined(NPT_CONFIG_ENABLE_LOGGING)
#define NPT_DEFINE_LOGGER(_logger, _name) \
    static NPT_LoggerReference _logger = { NULL, (_name) };

#define NPT_LOG_LX(_logger, _level, _argsx)                             \
do {                                                                    \
    NPT_LOG_GET_LOGGER((_logger))                                       \
    if ((_logger).logger && (_level) >= (_logger).logger->GetLevel()) { \
        (_logger).logger->Log _argsx;                                   \
    }                                                                   \
} while(0)
#define NPT_LOG_L(_logger, _level, _msg) \
    NPT_LOG_LX(_logger, _level, ((_level), __FILE__, __LINE__, (_msg)) )
#define NPT_LOG_L1(_logger, _level, _msg, _arg1) \
    NPT_LOG_LX(_logger, _level, ((_level), __FILE__, __LINE__, (_msg), (_arg1)) )
#define NPT_LOG_L2(_logger, _level, _msg, _arg1, _arg2) \
    NPT_LOG_LX(_logger, _level, ((_level), __FILE__, __LINE__, (_msg), (_arg1), (_arg2)) )
#define NPT_LOG_L3(_logger, _level, _msg, _arg1, _arg2, _arg3) \
    NPT_LOG_LX(_logger, _level, ((_level), __FILE__, __LINE__, (_msg), (_arg1), (_arg2), (_arg3)) )
#define NPT_LOG_L4(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4) \
    NPT_LOG_LX(_logger, _level, ((_level), __FILE__, __LINE__, (_msg), (_arg1), (_arg2), (_arg3), (_arg4)) )
#define NPT_LOG_L5(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5) \
    NPT_LOG_LX(_logger, _level, ((_level), __FILE__, __LINE__, (_msg), (_arg1), (_arg2), (_arg3), (_arg4), (_arg5)) )
#define NPT_LOG_L6(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6) \
    NPT_LOG_LX(_logger, _level, ((_level), __FILE__, __LINE__, (_msg), (_arg1), (_arg2), (_arg3), (_arg4), (_arg5), (_arg6)) )
#define NPT_LOG_L7(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7) \
    NPT_LOG_LX(_logger, _level, ((_level), __FILE__, __LINE__, (_msg), (_arg1), (_arg2), (_arg3), (_arg4), (_arg5), (_arg6), (_arg7)) )
#define NPT_LOG_L8(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7, _arg8) \
    NPT_LOG_LX(_logger, _level, ((_level), __FILE__, __LINE__, (_msg), (_arg1), (_arg2), (_arg3), (_arg4), (_arg5), (_arg6), (_arg7), (_arg8)) )
#define NPT_LOG_L9(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7, _arg8, _arg9) \
    NPT_LOG_LX(_logger, _level, ((_level), __FILE__, __LINE__, (_msg), (_arg1), (_arg2), (_arg3), (_arg4), (_arg5), (_arg6), (_arg7), (_arg8), (_arg9)) )

#define NPT_CHECK_LL(_logger, _level, _result) do {                                    \
    NPT_Result _x = (_result);                                                         \
    if (_x != NPT_SUCCESS) {                                                           \
        NPT_LOG_L2(_logger, _level, "NPT_CHECK failed, result=%d [%s]", _x, #_result); \
        return _x;                                                                     \
    }                                                                                  \
} while(0)
#define NPT_CHECK_LABEL_LL(_logger, _level, _result, _label) do {                      \
    NPT_Result _x = (_result);                                                         \
    if (_x != NPT_SUCCESS) {                                                           \
        NPT_LOG_L2(_logger, _level, "NPT_CHECK failed, result=%d [%s]", _x, #_result); \
        goto _label;                                                                   \
    }                                                                                  \
} while(0)
#define NPT_CHECK_POINTER_LL(_logger, _level, _p) do {                                 \
    if ((_p) == NULL) {                                                                  \
        NPT_LOG_L(_logger, _level, "@@@ NULL pointer parameter");                        \
        return NPT_ERROR_INVALID_PARAMETERS;                                                     \
    }                                                                                  \
} while(0)
#define NPT_CHECK_POINTER_LABEL_LL(_logger, _level, _p, _label) do {                   \
    if ((_p) == NULL) {                                                                  \
        NPT_LOG_L(_logger, _level, "@@@ NULL pointer parameter");                        \
        goto _label;                                                                   \
    }                                                                                  \
} while(0)

#else /* NPT_CONFIG_ENABLE_LOGGING */
#define NPT_DEFINE_LOGGER(_logger, _name)
#define NPT_LOG_L(_logger, _level, _msg)
#define NPT_LOG_L1(_logger, _level, _msg, _arg1)
#define NPT_LOG_L2(_logger, _level, _msg, _arg1, _arg2)
#define NPT_LOG_L3(_logger, _level, _msg, _arg1, _arg2, _arg3)
#define NPT_LOG_L4(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4)
#define NPT_LOG_L5(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5)
#define NPT_LOG_L6(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6)
#define NPT_LOG_L7(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7)
#define NPT_LOG_L8(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7, _arg8)
#define NPT_LOG_L9(_logger, _level, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6, _arg7, _arg8, _arg9)

#define NPT_CHECK_LL(_logger, _level, _result) NPT_CHECK(_result)
#define NPT_CHECK_LABEL_LL(_logger, _level, _result, _label) NPT_CHECK_LABEL(_result, _label)
#define NPT_CHECK_POINTER_LL(_logger, _level, _p) NPT_CHECK_POINTER(_p)
#define NPT_CHECK_POINTER_LABEL_LL(_logger, _level, _p, _label) NPT_CHECK_POINTER_LABEL(_p, _label)

#endif /* NPT_CONFIG_ENABLE_LOGGING */

#define NPT_SET_LOCAL_LOGGER(_name) \
    NPT_DEFINE_LOGGER(_NPT_LocalLogger, (_name))

/* NOTE: the following are machine-generated, do not edit */
#define NPT_LOG_FATAL_L(_logger, _msg) NPT_LOG_L((_logger), NPT_LOG_LEVEL_FATAL, (_msg))
#define NPT_LOG_FATAL(_msg) NPT_LOG_FATAL_L(_NPT_LocalLogger, (_msg))
#define NPT_LOG_FATAL_L1(_logger, _msg,_arg1) NPT_LOG_L1((_logger), NPT_LOG_LEVEL_FATAL, (_msg),(_arg1))
#define NPT_LOG_FATAL_1(_msg,_arg1) NPT_LOG_FATAL_L1(_NPT_LocalLogger, (_msg),(_arg1))
#define NPT_LOG_FATAL_L2(_logger, _msg,_arg1,_arg2) NPT_LOG_L2((_logger), NPT_LOG_LEVEL_FATAL, (_msg),(_arg1),(_arg2))
#define NPT_LOG_FATAL_2(_msg,_arg1,_arg2) NPT_LOG_FATAL_L2(_NPT_LocalLogger, (_msg),(_arg1),(_arg2))
#define NPT_LOG_FATAL_L3(_logger, _msg,_arg1,_arg2,_arg3) NPT_LOG_L3((_logger), NPT_LOG_LEVEL_FATAL, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_FATAL_3(_msg,_arg1,_arg2,_arg3) NPT_LOG_FATAL_L3(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_FATAL_L4(_logger, _msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_L4((_logger), NPT_LOG_LEVEL_FATAL, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_FATAL_4(_msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_FATAL_L4(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_FATAL_L5(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_L5((_logger), NPT_LOG_LEVEL_FATAL, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_FATAL_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_FATAL_L5(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_FATAL_L6(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_L6((_logger), NPT_LOG_LEVEL_FATAL, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_FATAL_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_FATAL_L6(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_FATAL_L7(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_L7((_logger), NPT_LOG_LEVEL_FATAL, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_FATAL_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_FATAL_L7(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_FATAL_L8(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_L8((_logger), NPT_LOG_LEVEL_FATAL, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_FATAL_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_FATAL_L8(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_SEVERE_L(_logger, _msg) NPT_LOG_L((_logger), NPT_LOG_LEVEL_SEVERE, (_msg))
#define NPT_LOG_SEVERE(_msg) NPT_LOG_SEVERE_L(_NPT_LocalLogger, (_msg))
#define NPT_LOG_SEVERE_L1(_logger, _msg,_arg1) NPT_LOG_L1((_logger), NPT_LOG_LEVEL_SEVERE, (_msg),(_arg1))
#define NPT_LOG_SEVERE_1(_msg,_arg1) NPT_LOG_SEVERE_L1(_NPT_LocalLogger, (_msg),(_arg1))
#define NPT_LOG_SEVERE_L2(_logger, _msg,_arg1,_arg2) NPT_LOG_L2((_logger), NPT_LOG_LEVEL_SEVERE, (_msg),(_arg1),(_arg2))
#define NPT_LOG_SEVERE_2(_msg,_arg1,_arg2) NPT_LOG_SEVERE_L2(_NPT_LocalLogger, (_msg),(_arg1),(_arg2))
#define NPT_LOG_SEVERE_L3(_logger, _msg,_arg1,_arg2,_arg3) NPT_LOG_L3((_logger), NPT_LOG_LEVEL_SEVERE, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_SEVERE_3(_msg,_arg1,_arg2,_arg3) NPT_LOG_SEVERE_L3(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_SEVERE_L4(_logger, _msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_L4((_logger), NPT_LOG_LEVEL_SEVERE, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_SEVERE_4(_msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_SEVERE_L4(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_SEVERE_L5(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_L5((_logger), NPT_LOG_LEVEL_SEVERE, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_SEVERE_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_SEVERE_L5(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_SEVERE_L6(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_L6((_logger), NPT_LOG_LEVEL_SEVERE, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_SEVERE_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_SEVERE_L6(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_SEVERE_L7(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_L7((_logger), NPT_LOG_LEVEL_SEVERE, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_SEVERE_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_SEVERE_L7(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_SEVERE_L8(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_L8((_logger), NPT_LOG_LEVEL_SEVERE, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_SEVERE_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_SEVERE_L8(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_WARNING_L(_logger, _msg) NPT_LOG_L((_logger), NPT_LOG_LEVEL_WARNING, (_msg))
#define NPT_LOG_WARNING(_msg) NPT_LOG_WARNING_L(_NPT_LocalLogger, (_msg))
#define NPT_LOG_WARNING_L1(_logger, _msg,_arg1) NPT_LOG_L1((_logger), NPT_LOG_LEVEL_WARNING, (_msg),(_arg1))
#define NPT_LOG_WARNING_1(_msg,_arg1) NPT_LOG_WARNING_L1(_NPT_LocalLogger, (_msg),(_arg1))
#define NPT_LOG_WARNING_L2(_logger, _msg,_arg1,_arg2) NPT_LOG_L2((_logger), NPT_LOG_LEVEL_WARNING, (_msg),(_arg1),(_arg2))
#define NPT_LOG_WARNING_2(_msg,_arg1,_arg2) NPT_LOG_WARNING_L2(_NPT_LocalLogger, (_msg),(_arg1),(_arg2))
#define NPT_LOG_WARNING_L3(_logger, _msg,_arg1,_arg2,_arg3) NPT_LOG_L3((_logger), NPT_LOG_LEVEL_WARNING, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_WARNING_3(_msg,_arg1,_arg2,_arg3) NPT_LOG_WARNING_L3(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_WARNING_L4(_logger, _msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_L4((_logger), NPT_LOG_LEVEL_WARNING, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_WARNING_4(_msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_WARNING_L4(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_WARNING_L5(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_L5((_logger), NPT_LOG_LEVEL_WARNING, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_WARNING_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_WARNING_L5(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_WARNING_L6(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_L6((_logger), NPT_LOG_LEVEL_WARNING, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_WARNING_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_WARNING_L6(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_WARNING_L7(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_L7((_logger), NPT_LOG_LEVEL_WARNING, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_WARNING_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_WARNING_L7(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_WARNING_L8(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_L8((_logger), NPT_LOG_LEVEL_WARNING, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_WARNING_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_WARNING_L8(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_INFO_L(_logger, _msg) NPT_LOG_L((_logger), NPT_LOG_LEVEL_INFO, (_msg))
#define NPT_LOG_INFO(_msg) NPT_LOG_INFO_L(_NPT_LocalLogger, (_msg))
#define NPT_LOG_INFO_L1(_logger, _msg,_arg1) NPT_LOG_L1((_logger), NPT_LOG_LEVEL_INFO, (_msg),(_arg1))
#define NPT_LOG_INFO_1(_msg,_arg1) NPT_LOG_INFO_L1(_NPT_LocalLogger, (_msg),(_arg1))
#define NPT_LOG_INFO_L2(_logger, _msg,_arg1,_arg2) NPT_LOG_L2((_logger), NPT_LOG_LEVEL_INFO, (_msg),(_arg1),(_arg2))
#define NPT_LOG_INFO_2(_msg,_arg1,_arg2) NPT_LOG_INFO_L2(_NPT_LocalLogger, (_msg),(_arg1),(_arg2))
#define NPT_LOG_INFO_L3(_logger, _msg,_arg1,_arg2,_arg3) NPT_LOG_L3((_logger), NPT_LOG_LEVEL_INFO, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_INFO_3(_msg,_arg1,_arg2,_arg3) NPT_LOG_INFO_L3(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_INFO_L4(_logger, _msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_L4((_logger), NPT_LOG_LEVEL_INFO, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_INFO_4(_msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_INFO_L4(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_INFO_L5(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_L5((_logger), NPT_LOG_LEVEL_INFO, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_INFO_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_INFO_L5(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_INFO_L6(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_L6((_logger), NPT_LOG_LEVEL_INFO, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_INFO_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_INFO_L6(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_INFO_L7(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_L7((_logger), NPT_LOG_LEVEL_INFO, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_INFO_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_INFO_L7(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_INFO_L8(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_L8((_logger), NPT_LOG_LEVEL_INFO, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_INFO_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_INFO_L8(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_FINE_L(_logger, _msg) NPT_LOG_L((_logger), NPT_LOG_LEVEL_FINE, (_msg))
#define NPT_LOG_FINE(_msg) NPT_LOG_FINE_L(_NPT_LocalLogger, (_msg))
#define NPT_LOG_FINE_L1(_logger, _msg,_arg1) NPT_LOG_L1((_logger), NPT_LOG_LEVEL_FINE, (_msg),(_arg1))
#define NPT_LOG_FINE_1(_msg,_arg1) NPT_LOG_FINE_L1(_NPT_LocalLogger, (_msg),(_arg1))
#define NPT_LOG_FINE_L2(_logger, _msg,_arg1,_arg2) NPT_LOG_L2((_logger), NPT_LOG_LEVEL_FINE, (_msg),(_arg1),(_arg2))
#define NPT_LOG_FINE_2(_msg,_arg1,_arg2) NPT_LOG_FINE_L2(_NPT_LocalLogger, (_msg),(_arg1),(_arg2))
#define NPT_LOG_FINE_L3(_logger, _msg,_arg1,_arg2,_arg3) NPT_LOG_L3((_logger), NPT_LOG_LEVEL_FINE, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_FINE_3(_msg,_arg1,_arg2,_arg3) NPT_LOG_FINE_L3(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_FINE_L4(_logger, _msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_L4((_logger), NPT_LOG_LEVEL_FINE, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_FINE_4(_msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_FINE_L4(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_FINE_L5(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_L5((_logger), NPT_LOG_LEVEL_FINE, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_FINE_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_FINE_L5(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_FINE_L6(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_L6((_logger), NPT_LOG_LEVEL_FINE, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_FINE_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_FINE_L6(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_FINE_L7(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_L7((_logger), NPT_LOG_LEVEL_FINE, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_FINE_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_FINE_L7(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_FINE_L8(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_L8((_logger), NPT_LOG_LEVEL_FINE, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_FINE_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_FINE_L8(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_FINER_L(_logger, _msg) NPT_LOG_L((_logger), NPT_LOG_LEVEL_FINER, (_msg))
#define NPT_LOG_FINER(_msg) NPT_LOG_FINER_L(_NPT_LocalLogger, (_msg))
#define NPT_LOG_FINER_L1(_logger, _msg,_arg1) NPT_LOG_L1((_logger), NPT_LOG_LEVEL_FINER, (_msg),(_arg1))
#define NPT_LOG_FINER_1(_msg,_arg1) NPT_LOG_FINER_L1(_NPT_LocalLogger, (_msg),(_arg1))
#define NPT_LOG_FINER_L2(_logger, _msg,_arg1,_arg2) NPT_LOG_L2((_logger), NPT_LOG_LEVEL_FINER, (_msg),(_arg1),(_arg2))
#define NPT_LOG_FINER_2(_msg,_arg1,_arg2) NPT_LOG_FINER_L2(_NPT_LocalLogger, (_msg),(_arg1),(_arg2))
#define NPT_LOG_FINER_L3(_logger, _msg,_arg1,_arg2,_arg3) NPT_LOG_L3((_logger), NPT_LOG_LEVEL_FINER, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_FINER_3(_msg,_arg1,_arg2,_arg3) NPT_LOG_FINER_L3(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_FINER_L4(_logger, _msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_L4((_logger), NPT_LOG_LEVEL_FINER, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_FINER_4(_msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_FINER_L4(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_FINER_L5(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_L5((_logger), NPT_LOG_LEVEL_FINER, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_FINER_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_FINER_L5(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_FINER_L6(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_L6((_logger), NPT_LOG_LEVEL_FINER, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_FINER_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_FINER_L6(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_FINER_L7(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_L7((_logger), NPT_LOG_LEVEL_FINER, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_FINER_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_FINER_L7(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_FINER_L8(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_L8((_logger), NPT_LOG_LEVEL_FINER, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_FINER_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_FINER_L8(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_FINEST_L(_logger, _msg) NPT_LOG_L((_logger), NPT_LOG_LEVEL_FINEST, (_msg))
#define NPT_LOG_FINEST(_msg) NPT_LOG_FINEST_L(_NPT_LocalLogger, (_msg))
#define NPT_LOG_FINEST_L1(_logger, _msg,_arg1) NPT_LOG_L1((_logger), NPT_LOG_LEVEL_FINEST, (_msg),(_arg1))
#define NPT_LOG_FINEST_1(_msg,_arg1) NPT_LOG_FINEST_L1(_NPT_LocalLogger, (_msg),(_arg1))
#define NPT_LOG_FINEST_L2(_logger, _msg,_arg1,_arg2) NPT_LOG_L2((_logger), NPT_LOG_LEVEL_FINEST, (_msg),(_arg1),(_arg2))
#define NPT_LOG_FINEST_2(_msg,_arg1,_arg2) NPT_LOG_FINEST_L2(_NPT_LocalLogger, (_msg),(_arg1),(_arg2))
#define NPT_LOG_FINEST_L3(_logger, _msg,_arg1,_arg2,_arg3) NPT_LOG_L3((_logger), NPT_LOG_LEVEL_FINEST, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_FINEST_3(_msg,_arg1,_arg2,_arg3) NPT_LOG_FINEST_L3(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3))
#define NPT_LOG_FINEST_L4(_logger, _msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_L4((_logger), NPT_LOG_LEVEL_FINEST, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_FINEST_4(_msg,_arg1,_arg2,_arg3,_arg4) NPT_LOG_FINEST_L4(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4))
#define NPT_LOG_FINEST_L5(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_L5((_logger), NPT_LOG_LEVEL_FINEST, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_FINEST_5(_msg,_arg1,_arg2,_arg3,_arg4,_arg5) NPT_LOG_FINEST_L5(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5))
#define NPT_LOG_FINEST_L6(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_L6((_logger), NPT_LOG_LEVEL_FINEST, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_FINEST_6(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6) NPT_LOG_FINEST_L6(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6))
#define NPT_LOG_FINEST_L7(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_L7((_logger), NPT_LOG_LEVEL_FINEST, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_FINEST_7(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7) NPT_LOG_FINEST_L7(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7))
#define NPT_LOG_FINEST_L8(_logger, _msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_L8((_logger), NPT_LOG_LEVEL_FINEST, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))
#define NPT_LOG_FINEST_8(_msg,_arg1,_arg2,_arg3,_arg4,_arg5,_arg6,_arg7,_arg8) NPT_LOG_FINEST_L8(_NPT_LocalLogger, (_msg),(_arg1),(_arg2),(_arg3),(_arg4),(_arg5),(_arg6),(_arg7),(_arg8))

#define NPT_CHECK_L(_level, _result) NPT_CHECK_LL(_NPT_LocalLogger, _level, _result)
#define NPT_CHECK_FATAL_L(_logger, _result) NPT_CHECK_LL(_logger, NPT_LOG_LEVEL_FATAL, _result)
#define NPT_CHECK_FATAL(_result) NPT_CHECK_L(NPT_LOG_LEVEL_FATAL, _result)
#define NPT_CHECK_SEVERE_L(_logger, _result) NPT_CHECK_LL(_logger, NPT_LOG_LEVEL_SEVERE, _result)
#define NPT_CHECK_SEVERE(_result) NPT_CHECK_L(NPT_LOG_LEVEL_SEVERE, _result)
#define NPT_CHECK_WARNING_L(_logger, _result) NPT_CHECK_LL(_logger, NPT_LOG_LEVEL_WARNING, _result)
#define NPT_CHECK_WARNING(_result) NPT_CHECK_L(NPT_LOG_LEVEL_WARNING, _result)
#define NPT_CHECK_INFO_L(_logger, _result) NPT_CHECK_LL(_logger, NPT_LOG_LEVEL_INFO, _result)
#define NPT_CHECK_INFO(_result) NPT_CHECK_L(NPT_LOG_LEVEL_INFO, _result)
#define NPT_CHECK_FINE_L(_logger, _result) NPT_CHECK_LL(_logger, NPT_LOG_LEVEL_FINE, _result)
#define NPT_CHECK_FINE(_result) NPT_CHECK_L(NPT_LOG_LEVEL_FINE, _result)
#define NPT_CHECK_FINER_L(_logger, _result) NPT_CHECK_LL(_logger, NPT_LOG_LEVEL_FINER, _result)
#define NPT_CHECK_FINER(_result) NPT_CHECK_L(NPT_LOG_LEVEL_FINER, _result)
#define NPT_CHECK_FINEST_L(_logger, _result) NPT_CHECK_LL(_logger, NPT_LOG_LEVEL_FINEST, _result)
#define NPT_CHECK_FINEST(_result) NPT_CHECK_L(NPT_LOG_LEVEL_FINEST, _result)

#define NPT_CHECK_LABEL_L(_level, _result, _label) NPT_CHECK_LABEL_LL(_NPT_LocalLogger, _level, _result, _label)
#define NPT_CHECK_LABEL_FATAL_L(_logger, _result, _label) NPT_CHECK_LABEL_LL(_logger, NPT_LOG_LEVEL_FATAL, _result, _label)
#define NPT_CHECK_LABEL_FATAL(_result, _label) NPT_CHECK_LABEL_L(NPT_LOG_LEVEL_FATAL, _result, _label)
#define NPT_CHECK_LABEL_SEVERE_L(_logger, _result, _label) NPT_CHECK_LABEL_LL(_logger, NPT_LOG_LEVEL_SEVERE, _result, _label)
#define NPT_CHECK_LABEL_SEVERE(_result, _label) NPT_CHECK_LABEL_L(NPT_LOG_LEVEL_SEVERE, _result, _label)
#define NPT_CHECK_LABEL_WARNING_L(_logger, _result, _label) NPT_CHECK_LABEL_LL(_logger, NPT_LOG_LEVEL_WARNING, _result, _label)
#define NPT_CHECK_LABEL_WARNING(_result, _label) NPT_CHECK_LABEL_L(NPT_LOG_LEVEL_WARNING, _result, _label)
#define NPT_CHECK_LABEL_INFO_L(_logger, _result, _label) NPT_CHECK_LABEL_LL(_logger, NPT_LOG_LEVEL_INFO, _result, _label)
#define NPT_CHECK_LABEL_INFO(_result, _label) NPT_CHECK_LABEL_L(NPT_LOG_LEVEL_INFO, _result, _label)
#define NPT_CHECK_LABEL_FINE_L(_logger, _result, _label) NPT_CHECK_LABEL_LL(_logger, NPT_LOG_LEVEL_FINE, _result, _label)
#define NPT_CHECK_LABEL_FINE(_result, _label) NPT_CHECK_LABEL_L(NPT_LOG_LEVEL_FINE, _result, _label)
#define NPT_CHECK_LABEL_FINER_L(_logger, _result, _label) NPT_CHECK_LABEL_LL(_logger, NPT_LOG_LEVEL_FINER, _result, _label)
#define NPT_CHECK_LABEL_FINER(_result, _label) NPT_CHECK_LABEL_L(NPT_LOG_LEVEL_FINER, _result, _label)
#define NPT_CHECK_LABEL_FINEST_L(_logger, _result, _label) NPT_CHECK_LABEL_LL(_logger, NPT_LOG_LEVEL_FINEST, _result, _label)
#define NPT_CHECK_LABEL_FINEST(_result, _label) NPT_CHECK_LABEL_L(NPT_LOG_LEVEL_FINEST, _result, _label)

#define NPT_CHECK_POINTER_L(_level, _p) NPT_CHECK_POINTER_LL(_NPT_LocalLogger, _level, _p)
#define NPT_CHECK_POINTER_FATAL_L(_logger, _p) NPT_CHECK_POINTER_LL(_logger, NPT_LOG_LEVEL_FATAL, _p)
#define NPT_CHECK_POINTER_FATAL(_p) NPT_CHECK_POINTER_L(NPT_LOG_LEVEL_FATAL, _p)
#define NPT_CHECK_POINTER_SEVERE_L(_logger, _p) NPT_CHECK_POINTER_LL(_logger, NPT_LOG_LEVEL_SEVERE, _p)
#define NPT_CHECK_POINTER_SEVERE(_p) NPT_CHECK_POINTER_L(NPT_LOG_LEVEL_SEVERE, _p)
#define NPT_CHECK_POINTER_WARNING_L(_logger, _p) NPT_CHECK_POINTER_LL(_logger, NPT_LOG_LEVEL_WARNING, _p)
#define NPT_CHECK_POINTER_WARNING(_p) NPT_CHECK_POINTER_L(NPT_LOG_LEVEL_WARNING, _p)
#define NPT_CHECK_POINTER_INFO_L(_logger, _p) NPT_CHECK_POINTER_LL(_logger, NPT_LOG_LEVEL_INFO, _p)
#define NPT_CHECK_POINTER_INFO(_p) NPT_CHECK_POINTER_L(NPT_LOG_LEVEL_INFO, _p)
#define NPT_CHECK_POINTER_FINE_L(_logger, _p) NPT_CHECK_POINTER_LL(_logger, NPT_LOG_LEVEL_FINE, _p)
#define NPT_CHECK_POINTER_FINE(_p) NPT_CHECK_POINTER_L(NPT_LOG_LEVEL_FINE, _p)
#define NPT_CHECK_POINTER_FINER_L(_logger, _p) NPT_CHECK_POINTER_LL(_logger, NPT_LOG_LEVEL_FINER, _p)
#define NPT_CHECK_POINTER_FINER(_p) NPT_CHECK_POINTER_L(NPT_LOG_LEVEL_FINER, _p)
#define NPT_CHECK_POINTER_FINEST_L(_logger, _p) NPT_CHECK_POINTER_LL(_logger, NPT_LOG_LEVEL_FINEST, _p)
#define NPT_CHECK_POINTER_FINEST(_p) NPT_CHECK_POINTER_L(NPT_LOG_LEVEL_FINEST, _p)

#define NPT_CHECK_POINTER_LABEL_L(_level, _p, _label) NPT_CHECK_POINTER_LABEL_LL(_NPT_LocalLogger, _level, _p, _label)
#define NPT_CHECK_POINTER_LABEL_FATAL_L(_logger, _p, _label) NPT_CHECK_POINTER_LABEL_LL(_logger, NPT_LOG_LEVEL_FATAL, _p, _label)
#define NPT_CHECK_POINTER_LABEL_FATAL(_p, _label) NPT_CHECK_POINTER_LABEL_L(NPT_LOG_LEVEL_FATAL, _p, _label)
#define NPT_CHECK_POINTER_LABEL_SEVERE_L(_logger, _p, _label) NPT_CHECK_POINTER_LABEL_LL(_logger, NPT_LOG_LEVEL_SEVERE, _p, _label)
#define NPT_CHECK_POINTER_LABEL_SEVERE(_p, _label) NPT_CHECK_POINTER_LABEL_L(NPT_LOG_LEVEL_SEVERE, _p, _label)
#define NPT_CHECK_POINTER_LABEL_WARNING_L(_logger, _p, _label) NPT_CHECK_POINTER_LABEL_LL(_logger, NPT_LOG_LEVEL_WARNING, _p, _label)
#define NPT_CHECK_POINTER_LABEL_WARNING(_p, _label) NPT_CHECK_POINTER_LABEL_L(NPT_LOG_LEVEL_WARNING, _p, _label)
#define NPT_CHECK_POINTER_LABEL_INFO_L(_logger, _p, _label) NPT_CHECK_POINTER_LABEL_LL(_logger, NPT_LOG_LEVEL_INFO, _p, _label)
#define NPT_CHECK_POINTER_LABEL_INFO(_p, _label) NPT_CHECK_POINTER_LABEL_L(NPT_LOG_LEVEL_INFO, _p, _label)
#define NPT_CHECK_POINTER_LABEL_FINE_L(_logger, _p, _label) NPT_CHECK_POINTER_LABEL_LL(_logger, NPT_LOG_LEVEL_FINE, _p, _label)
#define NPT_CHECK_POINTER_LABEL_FINE(_p, _label) NPT_CHECK_POINTER_LABEL_L(NPT_LOG_LEVEL_FINE, _p, _label)
#define NPT_CHECK_POINTER_LABEL_FINER_L(_logger, _p, _label) NPT_CHECK_POINTER_LABEL_LL(_logger, NPT_LOG_LEVEL_FINER, _p, _label)
#define NPT_CHECK_POINTER_LABEL_FINER(_p, _label) NPT_CHECK_POINTER_LABEL_L(NPT_LOG_LEVEL_FINER, _p, _label)
#define NPT_CHECK_POINTER_LABEL_FINEST_L(_logger, _p, _label) NPT_CHECK_POINTER_LABEL_LL(_logger, NPT_LOG_LEVEL_FINEST, _p, _label)
#define NPT_CHECK_POINTER_LABEL_FINEST(_p, _label) NPT_CHECK_POINTER_LABEL_L(NPT_LOG_LEVEL_FINEST, _p, _label)

#endif /* _NPT_LOGGING_H_ */
