
#include "stdafx.h"
#include "log.h"
#ifndef _LINUX
#include <share.h>
#endif
#include "CriticalSection.h"
#include "SingleLock.h"
#include "StdString.h"
#include "../Settings.h"
#include "../Util.h"

FILE* CLog::fd = NULL;

static CCriticalSection critSec;

static char levelNames[][8] =
{"DEBUG", "INFO", "NOTICE", "WARNING", "ERROR", "SEVERE", "FATAL", "NONE"};


CLog::CLog()
{}

CLog::~CLog()
{}

void CLog::Close()
{
  CSingleLock waitLock(critSec);
  if (fd)
  {
    fclose(fd);
    fd = NULL;
  }
}


void CLog::Log(int loglevel, const char *format, ... )
{
  if (g_advancedSettings.m_logLevel > LOG_LEVEL_NORMAL || 
     (g_advancedSettings.m_logLevel > LOG_LEVEL_NONE && loglevel >= LOGNOTICE))
  {
    CSingleLock waitLock(critSec);
    if (!fd)
    {
      // g_stSettings.m_logFolder is initialized in the CSettings constructor to Q:
      // and if we are running from DVD, it's changed to T: in CApplication::Create()
      CStdString LogFile;
      CUtil::AddFileToFolder(g_stSettings.m_logFolder, "xbmc.log", LogFile);
#ifndef _LINUX      
      fd = _fsopen(LogFile, "a+", _SH_DENYWR);
#else
      fd = fopen(LogFile, "a+");
#endif
    }
      
    if (!fd)
      return ;

    SYSTEMTIME time;
    GetLocalTime(&time);

    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);

    CStdString strPrefix, strData;
   
    strPrefix.Format("%02.2d:%02.2d:%02.2d T:%lu M:%9u %7s: ", time.wHour, time.wMinute, time.wSecond, GetCurrentThreadId(), stat.dwAvailPhys, levelNames[loglevel]);

    strData.reserve(16384);
    va_list va;
    va_start(va, format);
    strData.FormatV(format,va);    
    va_end(va);
    

    int length = 0;
    while ( length != (int) strData.length() )
    {
      length = strData.length();
      strData.TrimRight(" ");
      strData.TrimRight('\n');      
      strData.TrimRight("\r");
    }

#if defined(_DEBUG) || defined(PROFILE)
    OutputDebugString(strData.c_str());
    OutputDebugString("\n");
#endif

    /* fixup newline alignment, number of spaces should equal prefix length */
    strData.Replace("\n", "\n                             ");
    strData += "\n";
       
    fwrite(strPrefix.c_str(), strPrefix.size(), 1, fd);
    fwrite(strData.c_str(), strData.size(), 1, fd);
    fflush(fd);
  }
#if defined(_DEBUG) || defined(PROFILE)
  else
  {
    // In debug mode dump everything to devstudio regardless of level
    CSingleLock waitLock(critSec);
    CStdString strData;
    strData.reserve(16384);

    va_list va;
    va_start(va, format);
    strData.FormatV(format, va);    
    va_end(va);
    
    OutputDebugString(strData.c_str());
    if( strData.Right(1) != "\n" )
      OutputDebugString("\n");
    
  }
#endif
}

void CLog::DebugLog(const char *format, ... )
{
#ifdef _DEBUG
  CSingleLock waitLock(critSec);

  CStdString strData;
  strData.reserve(16384);

  va_list va;
  va_start(va, format);
  strData.FormatV(format, va);    
  va_end(va);
  
  OutputDebugString(strData.c_str());
  if( strData.Right(1) != "\n" )
    OutputDebugString("\n");
#endif
}

void CLog::DebugLogMemory()
{
  CSingleLock waitLock(critSec);
  MEMORYSTATUS stat;
  CStdString strData;

  GlobalMemoryStatus(&stat);
  strData.Format("%i bytes free\n", stat.dwAvailPhys);
  OutputDebugString(strData.c_str());
}

void CLog::MemDump(BYTE *pData, int length)
{
  Log(LOGDEBUG, "MEM_DUMP: Dumping from %x", (unsigned int) pData);
  for (int i = 0; i < length; i+=16)
  {
    CStdString strLine;
    strLine.Format("MEM_DUMP: %04x ", i);
    BYTE *alpha = pData;
    for (int k=0; k < 4 && i + 4*k < length; k++)
    {
      for (int j=0; j < 4 && i + 4*k + j < length; j++)
      {
        CStdString strFormat;
        strFormat.Format(" %02x", *pData++);
        strLine += strFormat;
      }
      strLine += " ";
    }
    // pad with spaces
    while (strLine.size() < 13*4 + 16)
      strLine += " ";
    for (int j=0; j < 16 && i + j < length; j++)
    {
      CStdString strFormat;
      if (*alpha > 31 && *alpha < 128)
        strLine += *alpha;
      else
        strLine += '.';
      alpha++;
    }
    Log(LOGDEBUG, strLine.c_str());
  }
}


void _VerifyGLState(const char* szfile, const char* szfunction, int lineno){
#if defined(HAS_SDL_OPENGL) && defined(_DEBUG)
#define printMatrix(matrix)                                             \
  {                                                                     \
    for (int ixx = 0 ; ixx<4 ; ixx++)                                   \
      {                                                                 \
        CLog::Log(LOGDEBUG, "% 3.3f % 3.3f % 3.3f % 3.3f ",             \
                  matrix[ixx*4], matrix[ixx*4+1], matrix[ixx*4+2],      \
                  matrix[ixx*4+3]);                                     \
      }                                                                 \
  } 
  
  GLenum err = glGetError();
  if (err==GL_NO_ERROR)
    return;
  CLog::Log(LOGERROR, "GL ERROR: %s\n", gluErrorString(err));
  if (szfile && szfunction)
      CLog::Log(LOGERROR, "In file:%s function:%s line:%d", szfile, szfunction, lineno);
  GLboolean bools[16];
  GLfloat matrix[16];
  glGetFloatv(GL_SCISSOR_BOX, matrix);
  CLog::Log(LOGDEBUG, "Scissor box: %f, %f, %f, %f", matrix[0], matrix[1], matrix[2], matrix[3]);
  glGetBooleanv(GL_SCISSOR_TEST, bools);
  CLog::Log(LOGDEBUG, "Scissor test enabled: %d", (int)bools[0]);
  glGetFloatv(GL_VIEWPORT, matrix);
  CLog::Log(LOGDEBUG, "Viewport: %f, %f, %f, %f", matrix[0], matrix[1], matrix[2], matrix[3]);
  glGetFloatv(GL_PROJECTION_MATRIX, matrix);
  CLog::Log(LOGDEBUG, "Projection Matrix:");
  printMatrix(matrix);
  glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
  CLog::Log(LOGDEBUG, "Modelview Matrix:");
  printMatrix(matrix);
//  abort();
#endif
}
