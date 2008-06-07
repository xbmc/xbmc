/*****************************************************************
|
|   Neptune - Logging Support
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/
/** @file
* Implementation file for logging
*/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdarg.h>

#include "NptLogging.h"
#include "NptList.h"
#include "NptStreams.h"
#include "NptSockets.h"
#include "NptUtils.h"
#include "NptFile.h"
#include "NptSystem.h"
#include "NptConsole.h"
#include "NptThreads.h"
#include "NptDirectory.h"

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
class NPT_LogConsoleHandler : public NPT_LogHandler {
public:
    // class methods
    static NPT_Result Create(const char* logger_name, NPT_LogHandler*& handler);

    // methods
    void Log(const NPT_LogRecord& record);

private:
    // members
    bool      m_UseColors;
    NPT_Flags m_FormatFilter;
};

class NPT_LogFileHandler : public NPT_LogHandler {
public:
    // class methods
    static NPT_Result Create(const char* logger_name, NPT_LogHandler*& handler);

    // methods
    void Log(const NPT_LogRecord& record);

private:
    // members
    NPT_String                m_Filename;
    NPT_Flags                 m_FormatFilter;
    NPT_Mutex                 m_RecycleLock;
    NPT_UInt32                m_Recycle;
    NPT_OutputStreamReference m_Stream;
};

class NPT_LogTcpHandler : public NPT_LogHandler {
public:
    // class methods
    static NPT_Result Create(const char* logger_name, NPT_LogHandler*& handler);

    // methods
    void Log(const NPT_LogRecord& record);

private:
    // methods
    NPT_Result Connect();

    // members
    NPT_String                m_Host;
    NPT_UInt16                m_Port;
    NPT_OutputStreamReference m_Stream;
};

class NPT_LogNullHandler : public NPT_LogHandler {
public:
    // class methods
    static NPT_Result Create(NPT_LogHandler*& handler);

    // methods
    void Log(const NPT_LogRecord& record);
};

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
#define NPT_LOG_HEAP_BUFFER_INCREMENT 4096
#define NPT_LOG_STACK_BUFFER_MAX_SIZE 512
#define NPT_LOG_HEAP_BUFFER_MAX_SIZE  65536

#if !defined(NPT_LOG_CONFIG_ENV)
#define NPT_LOG_CONFIG_ENV "NEPTUNE_LOG_CONFIG"
#endif

#if !defined(NPT_LOG_DEFAULT_CONFIG_SOURCE)
#define NPT_LOG_DEFAULT_CONFIG_SOURCE "file:neptune-logging.properties"
#endif

#define NPT_LOG_ROOT_DEFAULT_LOG_LEVEL NPT_LOG_LEVEL_INFO
#define NPT_LOG_ROOT_DEFAULT_HANDLER   "ConsoleHandler"
#if !defined(NPT_LOG_ROOT_DEFAULT_FILE_HANDLER_FILENAME)
#define NPT_LOG_ROOT_DEFAULT_FILE_HANDLER_FILENAME "_neptune.log"
#endif

#define NPT_LOG_TCP_HANDLER_DEFAULT_PORT            7723
#define NPT_LOG_TCP_HANDLER_DEFAULT_CONNECT_TIMEOUT 5000 /* 5 seconds */

#if defined(_WIN32) || defined(_WIN32_WCE)
#define NPT_LOG_CONSOLE_HANDLER_DEFAULT_COLOR_MODE false
#else
#define NPT_LOG_CONSOLE_HANDLER_DEFAULT_COLOR_MODE true
#endif

#define NPT_LOG_FILE_HANDLER_MIN_RECYCLE_SIZE 20000000

#define NPT_LOG_FORMAT_FILTER_NO_SOURCE      1
#define NPT_LOG_FORMAT_FILTER_NO_TIMESTAMP   2
#define NPT_LOG_FORMAT_FILTER_NO_LOGGER_NAME 4

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
static NPT_LogManager LogManager;
const char*           LogManagerConfig = NULL;

/*----------------------------------------------------------------------
|   NPT_LogHandler::Create
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogHandler::Create(const char*      logger_name,
                       const char*      handler_name,
                       NPT_LogHandler*& handler)
{
    handler = NULL;

    if (NPT_StringsEqual(handler_name, "NullHandler")) {
        return NPT_LogNullHandler::Create(handler);
    } else if (NPT_StringsEqual(handler_name, "FileHandler")) {
        return NPT_LogFileHandler::Create(logger_name, handler);
    } else if (NPT_StringsEqual(handler_name, "ConsoleHandler")) {
        return NPT_LogConsoleHandler::Create(logger_name, handler);
    } else if (NPT_StringsEqual(handler_name, "TcpHandler")) {
        return NPT_LogTcpHandler::Create(logger_name, handler);
    }

    return NPT_ERROR_NO_SUCH_CLASS;
}

/*----------------------------------------------------------------------
|   NPT_Log::GetLogLevel
+---------------------------------------------------------------------*/
int 
NPT_Log::GetLogLevel(const char* name)
{
    if (       NPT_StringsEqual(name, "FATAL")) {
        return NPT_LOG_LEVEL_SEVERE;
    } else if (NPT_StringsEqual(name, "SEVERE")) {
        return NPT_LOG_LEVEL_WARNING;
    } else if (NPT_StringsEqual(name, "WARNING")) {
        return NPT_LOG_LEVEL_WARNING;
    } else if (NPT_StringsEqual(name, "INFO")) {
        return NPT_LOG_LEVEL_INFO;
    } else if (NPT_StringsEqual(name, "FINE")) {
        return NPT_LOG_LEVEL_FINE;
    } else if (NPT_StringsEqual(name, "FINER")) {
        return NPT_LOG_LEVEL_FINER;
    } else if (NPT_StringsEqual(name, "FINEST")) {
        return NPT_LOG_LEVEL_FINEST;
    } else if (NPT_StringsEqual(name, "ALL")) {
        return NPT_LOG_LEVEL_ALL;
    } else if (NPT_StringsEqual(name, "OFF")) {
        return NPT_LOG_LEVEL_OFF;
    } else {
        return -1;
    }
}

/*----------------------------------------------------------------------
|   NPT_Log::GetLogLevelName
+---------------------------------------------------------------------*/
const char*
NPT_Log::GetLogLevelName(int level)
{
    switch (level) {
        case NPT_LOG_LEVEL_FATAL:   return "FATAL";
        case NPT_LOG_LEVEL_SEVERE:  return "SEVERE";
        case NPT_LOG_LEVEL_WARNING: return "WARNING";
        case NPT_LOG_LEVEL_INFO:    return "INFO";
        case NPT_LOG_LEVEL_FINE:    return "FINE";
        case NPT_LOG_LEVEL_FINER:   return "FINER";
        case NPT_LOG_LEVEL_FINEST:  return "FINEST";
        case NPT_LOG_LEVEL_OFF:     return "OFF";
        default:                    return "";
    }
}

/*----------------------------------------------------------------------
|   NPT_Log::GetLogLevelAnsiColor
+---------------------------------------------------------------------*/
const char*
NPT_Log::GetLogLevelAnsiColor(int level)
{
    switch (level) {
        case NPT_LOG_LEVEL_FATAL:   return "31";
        case NPT_LOG_LEVEL_SEVERE:  return "31";
        case NPT_LOG_LEVEL_WARNING: return "33";
        case NPT_LOG_LEVEL_INFO:    return "32";
        case NPT_LOG_LEVEL_FINE:    return "34";
        case NPT_LOG_LEVEL_FINER:   return "35";
        case NPT_LOG_LEVEL_FINEST:  return "36";
        default:                    return NULL;
    }
}

/*----------------------------------------------------------------------
|   NPT_Log::FormatRecordToStream
+---------------------------------------------------------------------*/
void
NPT_Log::FormatRecordToStream(const NPT_LogRecord& record,
                              NPT_OutputStream&    stream,
                              bool                 use_colors,
                              NPT_Flags            format_filter)
{
    const char* level_name = GetLogLevelName(record.m_Level);
    NPT_String  level_string;

    /* format and emit the record */
    if (level_name[0] == '\0') {
        level_string = NPT_String::FromInteger(record.m_Level);
        level_name = level_string;
    }
    if ((format_filter & NPT_LOG_FORMAT_FILTER_NO_SOURCE) == 0) {
        stream.WriteString(record.m_SourceFile);
        stream.Write("(", 1, NULL);
        stream.WriteString(NPT_String::FromIntegerU(record.m_SourceLine));
        stream.Write("): ", 3, NULL);
    }
    if ((format_filter & NPT_LOG_FORMAT_FILTER_NO_LOGGER_NAME) == 0) {
        stream.Write("[", 1, NULL);
        stream.WriteString(record.m_LoggerName);
        stream.Write("] ", 2, NULL);
    }
    if ((format_filter & NPT_LOG_FORMAT_FILTER_NO_TIMESTAMP) == 0) {
        stream.WriteString(NPT_String::FromIntegerU(record.m_TimeStamp.m_Seconds));
        stream.WriteString(":");
        NPT_String ms = NPT_String::FromIntegerU(record.m_TimeStamp.m_NanoSeconds/1000000L);
        if (ms.GetLength() < 3) stream.Write("0", 1);
        if (ms.GetLength() < 2) stream.Write("0", 1);
        stream.WriteString(ms);
        stream.Write(" ", 1);
    }
    const char* ansi_color = NULL;
    if (use_colors) {
        ansi_color = GetLogLevelAnsiColor(record.m_Level);
        if (ansi_color) {
            stream.Write("\033[", 2, NULL);
            stream.WriteString(ansi_color);
            stream.Write(";1m", 3, NULL);
        }
    }
    stream.WriteString(level_name);
    if (use_colors && ansi_color) {
        stream.Write("\033[0m", 4, NULL);
    }
    stream.Write(": ", 2, NULL);
    stream.WriteString(record.m_Message);
    stream.Write("\n", 1, NULL);
}

/*----------------------------------------------------------------------
|   NPT_LogManager::NPT_LogManager
+---------------------------------------------------------------------*/
NPT_LogManager::NPT_LogManager() :
    m_Configured(false),
    m_Root(NULL)
{
}

/*----------------------------------------------------------------------
|   NPT_LogManager::~NPT_LogManager
+---------------------------------------------------------------------*/
NPT_LogManager::~NPT_LogManager()
{
    /* destroy everything we've created */
    for (NPT_List<NPT_Logger*>::Iterator i = m_Loggers.GetFirstItem();
         i;
         ++i) {
        NPT_Logger* logger = *i;
        delete logger;
    }

    /* destroy the root logger */
    delete m_Root;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::SetConfig
+---------------------------------------------------------------------*/
void
NPT_LogManager::SetConfig(const char* config) 
{
    LogManagerConfig = config;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::Configure
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogManager::Configure() 
{
    NPT_String  config_sources_env;
    const char* config_sources = NPT_LOG_DEFAULT_CONFIG_SOURCE;

    // exit if we're already initialized
    if (m_Configured) return NPT_SUCCESS;

    /* set some default config values */
    SetConfigValue(".handlers", NPT_LOG_ROOT_DEFAULT_HANDLER);

    if (!LogManagerConfig) {
        /* see if the config sources have been set to non-default values */
        if (NPT_SUCCEEDED(NPT_GetEnvironment(NPT_LOG_CONFIG_ENV, config_sources_env))) {
            config_sources = config_sources_env;
        }
    } else {
        config_sources = LogManagerConfig;
    }

    /* load all configs */
    NPT_String config_source;
    const char* cursor = config_sources; 
    const char* source = config_sources;
    for (;;) {
        if (*cursor == '\0' || *cursor == '|') {
            if (cursor != source) {
                config_source.Assign(source, (NPT_Size)(cursor-source));
                config_source.Trim(" \t");
                ParseConfigSource(config_source);
            }
            if (*cursor == '\0') break;
        }
        cursor++;
    }

    /* create the root logger */
    LogManager.m_Root = new NPT_Logger("");
    LogManager.m_Root->m_Level = NPT_LOG_ROOT_DEFAULT_LOG_LEVEL;
    LogManager.m_Root->m_LevelIsInherited = false;
    ConfigureLogger(LogManager.m_Root);

    // we're initialized now
    m_Configured = true;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::ConfigValueIsBooleanTrue
+---------------------------------------------------------------------*/
bool
NPT_LogManager::ConfigValueIsBooleanTrue(NPT_String& value)
{
    return 
        value.Compare("true", true) == 0 ||
        value.Compare("yes",  true) == 0 ||
        value.Compare("on",   true) == 0 ||
        value.Compare("1",    true) == 0;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::ConfigValueIsBooleanFalse
+---------------------------------------------------------------------*/
bool
NPT_LogManager::ConfigValueIsBooleanFalse(NPT_String& value)
{
    return 
        value.Compare("false", true) == 0  ||
        value.Compare("no",    true) == 0  ||
        value.Compare("off",   true) == 0  ||
        value.Compare("0",     true) == 0;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::GetConfigValue
+---------------------------------------------------------------------*/
NPT_String*
NPT_LogManager::GetConfigValue(const char* prefix, const char* suffix)
{
    NPT_Size prefix_length = prefix?NPT_StringLength(prefix):0;
    NPT_Size suffix_length = suffix?NPT_StringLength(suffix):0;
    NPT_Size key_length    = prefix_length+suffix_length;
    for (NPT_List<NPT_LogConfigEntry>::Iterator i = LogManager.m_Config.GetFirstItem();
         i;
         ++i) {
        NPT_LogConfigEntry& entry = *i;
        if ((entry.m_Key.GetLength() == key_length) &&
            (prefix == NULL || entry.m_Key.StartsWith(prefix)) &&
            (suffix == NULL || entry.m_Key.EndsWith(suffix  )) ) {
            return &entry.m_Value;
        }
    }

    // not found
    return NULL;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::SetConfigValue
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogManager::SetConfigValue(const char* key, const char* value)
{
    NPT_String* value_string = GetConfigValue(key, NULL);
    if (value_string) {
        /* the key already exists, replace the value */
        *value_string = value;
    } else {
        /* the value does not already exist, create a new one */
        NPT_CHECK(LogManager.m_Config.Add(NPT_LogConfigEntry(key, value)));
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::ParseConfig
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogManager::ParseConfig(const char* config,
                            NPT_Size    config_size) 
{
    const char* cursor    = config;
    const char* line      = config;
    const char* separator = NULL;
    NPT_String  key;
    NPT_String  value;

    /* parse all entries */
    while (cursor <= config+config_size) {
        /* separators are newlines, ';' or end of buffer */
        if ( cursor == config+config_size ||
            *cursor == '\n'              || 
            *cursor == '\r'              || 
            *cursor == ';') {
            /* newline or end of buffer */
            if (separator && line[0] != '#') {
                /* we have a property */
                key.Assign(line,                    (NPT_Size)(separator-line));
                value.Assign(line+(separator+1-line), (NPT_Size)(cursor-(separator+1)));
                key.Trim(" \t");
                value.Trim(" \t");
            
                SetConfigValue((const char*)key, (const char*)value);
            }
            line = cursor+1;
            separator = NULL;
        } else if (*cursor == '=' && separator == NULL) {
            separator = cursor;
        }
        cursor++;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::ParseConfigFile
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogManager::ParseConfigFile(const char* filename) 
{
    NPT_Result result;

    /* load the file */
    NPT_DataBuffer buffer;
    result = NPT_File::Load(filename, buffer);
    if (NPT_FAILED(result)) return result;

    /* parse the config */
    return ParseConfig((const char*)buffer.GetData(), buffer.GetDataSize());
}

/*----------------------------------------------------------------------
|   NPT_LogManager::ParseConfigSource
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogManager::ParseConfigSource(NPT_String& source) 
{
    if (source.StartsWith("file:")) {
        /* file source */
        ParseConfigFile(source.GetChars()+5);
    } else if (source.StartsWith("plist:")) {
        /* property list source */
        ParseConfig(source.GetChars()+6, source.GetLength()-6);
    } else {
        return NPT_ERROR_INVALID_SYNTAX;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::HaveLoggerConfig
+---------------------------------------------------------------------*/
bool
NPT_LogManager::HaveLoggerConfig(const char* name)
{
    NPT_Size name_length = NPT_StringLength(name);
    for (NPT_List<NPT_LogConfigEntry>::Iterator i = m_Config.GetFirstItem();
         i;
         ++i) {
        NPT_LogConfigEntry& entry = *i;
        if (entry.m_Key.StartsWith(name)) {
            const char* suffix = entry.m_Key.GetChars()+name_length;
            if (NPT_StringsEqual(suffix, ".level") ||
                NPT_StringsEqual(suffix, ".handlers") ||
                NPT_StringsEqual(suffix, ".forward")) {
                return true;
            }
        }
    }

    /* no config found */
    return false;

}

/*----------------------------------------------------------------------
|   NPT_LogManager::ConfigureLogger
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogManager::ConfigureLogger(NPT_Logger* logger)
{
    /* configure the level */
    NPT_String* level_value = GetConfigValue(logger->m_Name,".level");
    if (level_value) {
        long value;
        /* try a symbolic name */
        value = NPT_Log::GetLogLevel(*level_value);
        if (value < 0) {
            /* try a numeric value */
            if (NPT_FAILED(level_value->ToInteger(value, false))) {
                value = -1;
            }
        }
        if (value >= 0) {
            logger->m_Level = value;
            logger->m_LevelIsInherited = false;
        }
    }

    /* configure the handlers */
    NPT_String* handlers = GetConfigValue(logger->m_Name,".handlers");
    if (handlers) {
        const char*     handlers_list = handlers->GetChars();
        const char*     cursor = handlers_list;
        const char*     name_start = handlers_list;
        NPT_String      handler_name;
        NPT_LogHandler* handler;
        for (;;) {
            if (*cursor == '\0' || *cursor == ',') {
                if (cursor != name_start) {
                    handler_name.Assign(name_start, (NPT_Size)(cursor-name_start));
                    handler_name.Trim(" \t");
                    
                    /* create a handler */
                    if (NPT_SUCCEEDED(
                        NPT_LogHandler::Create(logger->m_Name, handler_name, handler))) {
                        logger->AddHandler(handler);
                    }

                }
                if (*cursor == '\0') break;
                name_start = cursor+1;
            }
            ++cursor;
        }
    }

    /* configure the forwarding */
    NPT_String* forward = GetConfigValue(logger->m_Name,".forward");
    if (forward && !ConfigValueIsBooleanTrue(*forward)) {
        logger->m_ForwardToParent = false;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::FindLogger
+---------------------------------------------------------------------*/
NPT_Logger*
NPT_LogManager::FindLogger(const char* name)
{
    for (NPT_List<NPT_Logger*>::Iterator i = LogManager.m_Loggers.GetFirstItem();
         i;
         ++i) {
        NPT_Logger* logger = *i;
        if (logger->m_Name == name) {
            return logger;
        }
    }

    return NULL;
}

/*----------------------------------------------------------------------
|   NPT_LogManager::GetLogger
+---------------------------------------------------------------------*/
NPT_Logger*
NPT_LogManager::GetLogger(const char* name)
{
    NPT_Logger* logger;

    /* check that the manager is initialized */
    if (!LogManager.m_Configured) {
        /* init the manager */
        LogManager.Configure();
        NPT_ASSERT(LogManager.m_Configured);
    }

    /* check if this logger is already configured */
    logger = LogManager.FindLogger(name);
    if (logger) return logger;

    /* create a new logger */
    logger = new NPT_Logger(name);
    if (logger == NULL) return NULL;

    /* configure the logger */
    LogManager.ConfigureLogger(logger);

    /* find which parent to attach to */
    NPT_Logger* parent = LogManager.m_Root;
    NPT_String  parent_name = name;
    for (;;) {
        NPT_Logger* candidate_parent;

        /* find the last dot */
        int dot = parent_name.ReverseFind('.');
        if (dot < 0) break;
        parent_name.SetLength(dot);
        
        /* see if the parent exists */
        candidate_parent = LogManager.FindLogger(parent_name);
        if (candidate_parent) {
            parent = candidate_parent;
            break;
        }

        /* this parent name does not exist, see if we need to create it */
        if (LogManager.HaveLoggerConfig(parent_name)) {
            parent = GetLogger(parent_name);
            break;
        }
    }

    /* attach to the parent */
    logger->SetParent(parent);

    /* add this logger to the list */
    LogManager.m_Loggers.Add(logger);

    return logger;
}

/*----------------------------------------------------------------------
|   NPT_Logger::NPT_Logger
+---------------------------------------------------------------------*/
NPT_Logger::NPT_Logger(const char* name) :
    m_Name(name),
    m_Level(NPT_LOG_LEVEL_OFF),
    m_LevelIsInherited(true),
    m_ForwardToParent(true),
    m_Parent(NULL)
{
}

/*----------------------------------------------------------------------
|   NPT_Logger::~NPT_Logger
+---------------------------------------------------------------------*/
NPT_Logger::~NPT_Logger()
{
    /* destroy all handlers */
    for (NPT_List<NPT_LogHandler*>::Iterator i = m_Handlers.GetFirstItem();
         i;
         ++i) {
        NPT_LogHandler* handler = *i;
        delete handler;
    }
}

/*----------------------------------------------------------------------
|   NPT_Logger::Log
+---------------------------------------------------------------------*/
void
NPT_Logger::Log(int          level, 
                const char*  source_file,
                unsigned int source_line,
                const char*  msg, 
                             ...)
{
    char     buffer[NPT_LOG_STACK_BUFFER_MAX_SIZE];
    NPT_Size buffer_size = sizeof(buffer);
    char*    message = buffer;
    int      result;
    va_list  args;

    va_start(args, msg);

    /* check the log level (in case filtering has not already been done) */
    if (level < m_Level) return;
        
    for(;;) {
        /* try to format the message (it might not fit) */
        result = NPT_FormatStringVN(message, buffer_size-1, msg, args);
        if (result >= (int)(buffer_size-1)) result = -1;
        message[buffer_size-1] = 0; /* force a NULL termination */
        if (result >= 0) break;

        /* the buffer was too small, try something bigger */
        buffer_size = (buffer_size+NPT_LOG_HEAP_BUFFER_INCREMENT)*2;
        if (buffer_size > NPT_LOG_HEAP_BUFFER_MAX_SIZE) break;
        if (message != buffer) delete[] message;
        message = new char[buffer_size];
        if (message == NULL) return;
    }

    /* the message is formatted, publish it to the handlers */
    NPT_LogRecord record;
    NPT_Logger*   logger = this;
    
    /* setup the log record */
    record.m_LoggerName = logger->m_Name,
    record.m_Level      = level;
    record.m_Message    = message;
    record.m_SourceFile = source_file;
    record.m_SourceLine = source_line;
    NPT_System::GetCurrentTimeStamp(record.m_TimeStamp);

    /* call all handlers for this logger and parents */
    while (logger) {
        /* call all handlers for the current logger */
        for (NPT_List<NPT_LogHandler*>::Iterator i = logger->m_Handlers.GetFirstItem();
             i;
             ++i) {
            NPT_LogHandler* handler = *i;
            handler->Log(record);
        }

        /* forward to the parent unless this logger does not forward */
        if (logger->m_ForwardToParent) {
            logger = logger->m_Parent;
        } else {
            break;
        }
    }

    /* free anything we may have allocated */
    if (message != buffer) delete[] message;

    va_end(args);
}

/*----------------------------------------------------------------------
|   NPT_Logger::AddHandler
+---------------------------------------------------------------------*/
NPT_Result
NPT_Logger::AddHandler(NPT_LogHandler* handler)
{
    /* check parameters */
    if (handler == NULL) return NPT_ERROR_INVALID_PARAMETERS;

    return m_Handlers.Add(handler);
}

/*----------------------------------------------------------------------
|   NPT_Logger::SetParent
+---------------------------------------------------------------------*/
NPT_Result
NPT_Logger::SetParent(NPT_Logger* parent)
{
    /* set our new parent */
    m_Parent = parent;

    /* find the first ancestor with its own log level */
    NPT_Logger* logger = this;
    while (logger->m_LevelIsInherited && logger->m_Parent) {
        logger = logger->m_Parent;
    }
    if (logger != this) m_Level = logger->m_Level;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_LogNullHandler::Create
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogNullHandler::Create(NPT_LogHandler*& handler)
{
    handler = new NPT_LogNullHandler();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_LogNullHandler::Log
+---------------------------------------------------------------------*/
void
NPT_LogNullHandler::Log(const NPT_LogRecord& /*record*/)
{
}

/*----------------------------------------------------------------------
|   NPT_LogConsoleHandler::Create
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogConsoleHandler::Create(const char*      logger_name,
                              NPT_LogHandler*& handler)
{
    /* compute a prefix for the configuration of this handler */
    NPT_String logger_prefix = logger_name;
    logger_prefix += ".ConsoleHandler";

    /* allocate a new object */
    NPT_LogConsoleHandler* instance = new NPT_LogConsoleHandler();
    handler = instance;

    /* configure the object */
    NPT_String* colors;
    instance->m_UseColors = NPT_LOG_CONSOLE_HANDLER_DEFAULT_COLOR_MODE;
    colors = LogManager.GetConfigValue(logger_prefix,".colors");
    if (colors) {
        if (NPT_LogManager::ConfigValueIsBooleanTrue(*colors)) {
            instance->m_UseColors = true;
        } else if (NPT_LogManager::ConfigValueIsBooleanFalse(*colors)) {
            instance->m_UseColors = false;
        }
    }
    NPT_String* filter;
    instance->m_FormatFilter = 0;
    filter = LogManager.GetConfigValue(logger_prefix,".filter");
    if (filter) {
        long flags = 0;
        filter->ToInteger(flags, true);
        instance->m_FormatFilter = flags;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_LogConsoleHandler::Log
+---------------------------------------------------------------------*/
void
NPT_LogConsoleHandler::Log(const NPT_LogRecord& record)
{
    NPT_MemoryStream memory_stream(4096);

    NPT_Log::FormatRecordToStream(record, memory_stream, m_UseColors, m_FormatFilter);
    memory_stream.Write("\0", 1);
    NPT_Console::Output((const char*)memory_stream.GetData());
}

/*----------------------------------------------------------------------
|   NPT_LogFileHandler::Log
+---------------------------------------------------------------------*/
void
NPT_LogFileHandler::Log(const NPT_LogRecord& record)
{
    if (m_Recycle > 0) {
        m_RecycleLock.Lock();

        /* get log size */
        NPT_Position position;
        m_Stream->Tell(position);

        /* time to recycle ? */
        if (position > m_Recycle) {
            /* release stream */
            m_Stream = NULL;

            /* move file */
            NPT_TimeStamp now;
            NPT_System::GetCurrentTimeStamp(now);

            NPT_String path;
            NPT_String name;
            NPT_DirectorySplitFilePath(m_Filename, path, name);
            NPT_DirectoryAppendToPath(path, "veodia-" + NPT_String::FromIntegerU(now.m_Seconds) + ".log");

            NPT_Directory::Move(m_Filename, path);

            /* re-open the log file */
            NPT_File file(m_Filename);
            NPT_Result result = file.Open(NPT_FILE_OPEN_MODE_CREATE |
                                          NPT_FILE_OPEN_MODE_WRITE);
            if (NPT_FAILED(result)) {
                NPT_Debug("NPT_LogFileHandler::Create - cannot open log file '%s' (%d)\n", 
                    m_Filename.GetChars(), result);
            }

            file.GetOutputStream(m_Stream);
        }
    }    
    
    if (m_Stream.AsPointer()) {
        NPT_Log::FormatRecordToStream(record, *m_Stream, false, m_FormatFilter);

        /* force flushing for file handler only */
        m_Stream->Flush();
    }

    if (m_Recycle > 0) m_RecycleLock.Unlock();
}

/*----------------------------------------------------------------------
|   NPT_LogFileHandler::Create
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogFileHandler::Create(const char*      logger_name,
                           NPT_LogHandler*& handler)
{
    /* compute a prefix for the configuration of this handler */
    NPT_String logger_prefix = logger_name;
    logger_prefix += ".FileHandler";

    /* allocate a new object */
    NPT_LogFileHandler* instance = new NPT_LogFileHandler();
    handler = instance;

    /* filename */
    NPT_String* filename_conf = LogManager.GetConfigValue(logger_prefix, ".filename");
    if (filename_conf) {
        instance->m_Filename = *filename_conf;
    } else if (logger_name[0]) {
        NPT_String filename_synth = logger_name;
        filename_synth += ".log";
        instance->m_Filename = filename_synth;
    } else {
        /* default name for the root logger */
        instance->m_Filename = NPT_LOG_ROOT_DEFAULT_FILE_HANDLER_FILENAME;
    }

    /* append mode */
    bool append = true;
    NPT_String* append_mode = LogManager.GetConfigValue(logger_prefix, ".append");
    if (append_mode && NPT_LogManager::ConfigValueIsBooleanFalse(*append_mode)) {
        append = false;
    }

    /* filter */
    NPT_String* filter;
    instance->m_FormatFilter = 0;
    filter = LogManager.GetConfigValue(logger_prefix,".filter");
    if (filter) {
        long flags = 0;
        filter->ToInteger(flags, true);
        instance->m_FormatFilter = flags;
    }

    /* recycle */
    NPT_String* recycle;
    instance->m_Recycle = 0;
    recycle = LogManager.GetConfigValue(logger_prefix,".recycle");
    if (recycle) {
        long size = 0;
        recycle->ToInteger(size, true);
        if (size > NPT_LOG_FILE_HANDLER_MIN_RECYCLE_SIZE) {
            instance->m_Recycle = size;
        }
    }

    /* open the log file */
    NPT_File file(instance->m_Filename);
    NPT_Result result = file.Open(NPT_FILE_OPEN_MODE_CREATE |
                                  NPT_FILE_OPEN_MODE_WRITE  |
                                  (append?NPT_FILE_OPEN_MODE_APPEND:NPT_FILE_OPEN_MODE_TRUNCATE));
    if (NPT_FAILED(result)) {
        return result;
    }

    return file.GetOutputStream(instance->m_Stream);
}

/*----------------------------------------------------------------------
|   NPT_LogTcpHandler::Create
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogTcpHandler::Create(const char* logger_name, NPT_LogHandler*& handler)
{
    /* compute a prefix for the configuration of this handler */
    NPT_String logger_prefix = logger_name;
    logger_prefix += ".TcpHandler";

    /* allocate a new object */
    NPT_LogTcpHandler* instance = new NPT_LogTcpHandler();
    handler = instance;

    /* configure the object */
    const NPT_String* hostname = LogManager.GetConfigValue(logger_prefix, ".hostname");
    if (hostname) {
        instance->m_Host = *hostname;
    } else {
        /* default hostname */
        instance->m_Host = "localhost";
    }
    const NPT_String* port = LogManager.GetConfigValue(logger_prefix, ".port");
    if (port) {
        long port_int;
        if (NPT_SUCCEEDED(port->ToInteger(port_int, true))) {
            instance->m_Port = (NPT_UInt16)port_int;
        } else {
            instance->m_Port = NPT_LOG_TCP_HANDLER_DEFAULT_PORT;
        }
    } else {
        /* default port */
        instance->m_Port = NPT_LOG_TCP_HANDLER_DEFAULT_PORT;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_LogTcpHandler::Connect
+---------------------------------------------------------------------*/
NPT_Result
NPT_LogTcpHandler::Connect()
{
    /* create a socket */
    NPT_Socket tcp_socket = new NPT_TcpClientSocket();

    /* connect to the host */
    NPT_IpAddress ip_address;
    NPT_CHECK(ip_address.ResolveName(m_Host));
    NPT_Result result = tcp_socket.Connect(NPT_SocketAddress(ip_address, m_Port), 
                                           NPT_LOG_TCP_HANDLER_DEFAULT_CONNECT_TIMEOUT);
    if (NPT_FAILED(result)) {
        return result;
    }

    /* get the stream */
    return tcp_socket.GetOutputStream(m_Stream);
}

/*----------------------------------------------------------------------
|   NPT_LogTcpHandler::Log
+---------------------------------------------------------------------*/
void
NPT_LogTcpHandler::Log(const NPT_LogRecord& record)
{
    /* ensure we're connected */
    if (m_Stream.IsNull()) {
        if (NPT_FAILED(Connect())) return;
    }

    /* format the record */
    NPT_String msg;
    const char* level_name = NPT_Log::GetLogLevelName(record.m_Level);
    NPT_String  level_string;

    /* format and emit the record */
    if (level_name[0] == '\0') {
        level_string = NPT_String::FromIntegerU(record.m_Level);
        level_name = level_string;
    }
    msg.Reserve(2048);
    msg += "Logger: ";
    msg += record.m_LoggerName;
    msg += "\r\nLevel: ";
    msg += level_name;
    msg += "\r\nSource-File: ";
    msg += record.m_SourceFile;
    msg += "\r\nSource-Line: ";
    msg += NPT_String::FromIntegerU(record.m_SourceLine);
    msg += "\r\nTimeStamp: ";
    msg += NPT_String::FromIntegerU(record.m_TimeStamp.m_Seconds);
    msg += ":";
    msg += NPT_String::FromIntegerU(record.m_TimeStamp.m_NanoSeconds/1000000L);
    msg += "\r\nContent-Length: ";
    msg += NPT_String::FromIntegerU(NPT_StringLength(record.m_Message));
    msg += "\r\n\r\n";
    msg += record.m_Message;

    /* emit the formatted record */
    if (NPT_FAILED(m_Stream->WriteString(msg))) {
        m_Stream = NULL;
    }
}
