#include "..\..\..\stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <fcntl.h>
#include <direct.h>
#include <time.h>
#include <signal.h>
#include "..\..\..\util.h"
#include "..\..\..\filesystem\IDirectory.h"
#include "..\..\..\FileSystem\FactoryDirectory.h"

#include "emu_msvcrt.h"
#include "emu_dummy.h"

struct SDirData
{
  DIRECTORY::IDirectory* Directory;
  CFileItemList items;
  SDirData()
  {
    Directory = NULL;
  }
};

#define MAX_OPEN_FILES 50
#define MAX_OPEN_DIRS 10
static CFile* vecFilesOpen[MAX_OPEN_FILES];
static SDirData vecDirsOpen[MAX_OPEN_DIRS];
bool bVecFilesInited = false;
bool bVecDirsInited = false;
extern void update_cache_dialog(const char* tmp);

struct _env
{
  const char* name;
  char* value;
};

static struct _env environment[] = 
{
  // libdvdnav
  { "DVDREAD_NOKEYS", "1" },
  
  // libdvdcss
  { "DVDCSS_METHOD", "title" },
  { "DVDCSS_VERBOSE", "3" },
  { "DVDCSS_CACHE", "T:\\cache" },
  
  { NULL, NULL }
};

extern "C"
{
  char* dll_strdup( const char* str);

  void dll_sleep(unsigned long imSec)
  {
    Sleep(imSec);
  }

  void InitFiles()
  {
    if (bVecFilesInited) return ;
    memset(vecFilesOpen, 0, sizeof(vecFilesOpen));
    bVecFilesInited = true;
  }

  // FIXME, XXX, !!!!!!
  void dllReleaseAll( )
  {
    // close all open dirs...
    if (bVecDirsInited)
    {
      for (int i=0;i < MAX_OPEN_DIRS; ++i)
      {
        if (vecDirsOpen[i].Directory)
        {
          delete vecDirsOpen[i].Directory;
          vecDirsOpen[i].items.Clear();
          vecDirsOpen[i].Directory = NULL;
        }
      }
      bVecDirsInited = false;
    }
  }

  void* dllmalloc(size_t size)
  {
    void* pBlock = malloc(size);
    if (!pBlock)
    {
      char szTmp[129];
      sprintf(szTmp, "malloc %u bytes failed\n", size);
      OutputDebugString(szTmp);
    }
    return pBlock;
  }

  void dllfree( void* pPtr )
  {
    free(pPtr);
  }

  void* dllcalloc( size_t num, size_t size )
  {
    void* pBlock = calloc(num, size);
    return pBlock;
  }

  void* dllrealloc( void *memblock, size_t size )
  {
    return realloc(memblock, size);
  }

  void dllexit(int iCode)
  {
    not_implement("msvcrt.dll fake function exit() called\n");      //warning
  }

  void dllabort()
  {
    not_implement("msvcrt.dll fake function abort() called\n");     //warning
  }

  void* dll__dllonexit(PFV input, PFV ** start, PFV ** end)
  {
    //ported from WINE code
    PFV *tmp;
    int len;

    if (!start || !*start || !end || !*end)
    {
      //FIXME("bad table\n");
      return NULL;
    }

    len = (*end - *start);

    if (++len <= 0)
      return NULL;

    tmp = (PFV*) realloc (*start, len * sizeof(tmp) );
    if (!tmp)
      return NULL;
    *start = tmp;
    *end = tmp + len;
    tmp[len - 1] = input;
    return input;

    //wrong handling, this function is used for register functions
    //that called before exit use _initterm functions.

    //dllReleaseAll( );
    //return TRUE;
  }

  _onexit_t dll_onexit(_onexit_t func)
  {
    not_implement("msvcrt.dll fake function dll_onexit() called\n");

    // register to dll unload list
    // return func if succsesfully added to the dll unload list
    return NULL;
  }

  void dllputs(const char* szLine)
  {
    if (!szLine[0]) return ;
    if (szLine[strlen(szLine) - 1] != '\n')
      CLog::Log(LOGDEBUG,"  msg:%s", szLine);
    else
      CLog::Log(LOGDEBUG,"  msg:%s\n", szLine);
  }

  void dllprintf( const char *format, ... )
  {
    va_list va;
    static char tmp[2048];
    va_start(va, format);
    _vsnprintf(tmp, 2048, format, va);
    va_end(va);
    tmp[2048 - 1] = 0;
    update_cache_dialog(tmp);
    CLog::Log(LOGDEBUG, "  msg:%s", tmp);
  }

  char *_fullpath(char *absPath, const char *relPath, size_t maxLength)
  {
    unsigned int len = strlen(relPath);
    if (len > maxLength && absPath != NULL) return NULL;

    // dll has to make sure it uses the correct path for now
    if (len > 1 && relPath[1] == ':')
    {
      if (absPath == NULL) absPath = dll_strdup(relPath);
      else strcpy(absPath, relPath);
      return absPath;
    }
    if (!strncmp(relPath, "\\Device\\Cdrom0", 14))
    {
      // needed?
      if (absPath == NULL) absPath = strdup(relPath);
      else strcpy(absPath, relPath);
      /*
       if(absPath == NULL) absPath = malloc(strlen(relPath) - 12); // \\Device\\Cdrom0 needs 12 bytes less then D:
      strcpy(absPath, "D:");
      strcat(absPath, relPath + 14);*/ 
      return absPath;
    }

    not_implement("msvcrt.dll incomplete function _fullpath(...) called\n");      //warning
    return NULL;
  }

  FILE *_popen(const char *command, const char *mode)
  {
    not_implement("msvcrt.dll fake function _popen(...) called\n"); //warning
    return NULL;
  }

  int _pclose(FILE *stream)
  {
    not_implement("msvcrt.dll fake function _pclose(...) called\n");        //warning
    return 0;
  }

  FILE* dll_fdopen(int i, const char* file)
  {
    return _fdopen(i, file);
  }

  int dll_open(const char* szFileName, int iMode)
  {
    if (!bVecFilesInited) InitFiles();
    int fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; ++i)
    {
      if (vecFilesOpen[i] == NULL)
      {
        fd = i;
        break;
      }
    }
    if (fd < 0)
    {
      // too many open files
      VERIFY(0);
    }

    char str[XBMC_MAX_PATH];

    // move to CFile classes
    if (strncmp(szFileName, "\\Device\\Cdrom0", 14) == 0)
    {
      // replace "\\Device\\Cdrom0" with "D:"
      strcpy(str, "D:");
      strcat(str, szFileName + 14);
    }
    else strcpy(str, szFileName);

    CFile* pFile = new CFile();
    bool bBinary = false;
    if (iMode & O_BINARY)
      bBinary = true;
    bool bWrite = false;
    if (iMode & O_RDWR)
      bWrite = true;
    // currently always overwrites
    if ((bWrite && pFile->OpenForWrite(str, bBinary, true)) || pFile->Open(str, bBinary) )
    {
      vecFilesOpen[fd] = pFile;
      return fd;
    }
    delete pFile;
    return -1;
  }

  int dll_read(int fd, void* buffer, unsigned int uiSize)
  {
    if (!bVecFilesInited) InitFiles();
    if (fd < 0 || fd >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[fd];
    if (!pFile) return -1;

    return pFile->Read(buffer, uiSize);
  }

  int dll_write(int fd, const void* buffer, unsigned int uiSize)
  {
    if (!bVecFilesInited) InitFiles();
    if (fd < 0 || fd >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[fd];
    if (!pFile) return -1;
    return pFile->Write(buffer, uiSize);
  }

  int dll_close(int fd)
  {
    if (!bVecFilesInited) InitFiles();
    if (fd < 0 || fd >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[fd];
    if (!pFile) return -1;
    pFile->Close();
    delete pFile;
    vecFilesOpen[fd] = NULL;
    return 0;
  }

  __int64 dll_lseeki64(int fd, __int64 lPos, int iWhence)
  {
    if (!bVecFilesInited) InitFiles();
    if (fd < 0 || fd >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[fd];
    if (!pFile) return (__int64) - 1;

    /*
    char szTmp[128];
    _i64toa(lPos,szTmp,10);
    OutputDebugString("seek:");
    OutputDebugString(szTmp);
    OutputDebugString("  ret:");*/

    lPos = pFile->Seek(lPos, iWhence);
    /*
    _i64toa(lPos,szTmp,10);
    OutputDebugString(szTmp);
    OutputDebugString("\n");*/ 
    return lPos;
  }

  long dll_lseek(int fd, long lPos, int iWhence)
  {
    //OutputDebugString("dll_seek\n");
    return (long) dll_lseeki64(fd, (__int64)lPos, iWhence);
  }

  char* dll_getenv(const char* szKey)
  {
    int i = 0;
    while (environment[i].name)
    {
      if (strcmp(szKey, environment[i].name) == 0) return environment[i].value;
      i++;
    }
    
	  if (strcmp(szKey, "http_proxy") == 0) // needed by libdemux
	  {
        // Use a proxy, if the GUI was configured as such
        bool bProxyEnabled = g_guiSettings.GetBool("Network.UseHTTPProxy");
        if (bProxyEnabled)
        {
          CStdString& strProxyServer = g_guiSettings.GetString("Network.HTTPProxyServer");
          CStdString& strProxyPort = g_guiSettings.GetString("Network.HTTPProxyPort");
          // Should we check for valid strings here?
  static char opt_proxyurl[256];
          _snprintf( opt_proxyurl, 256, "http://%s:%s", strProxyServer.c_str(), strProxyPort.c_str() );
		  return opt_proxyurl;
        }
	  }

    return NULL;
  }

  //---------------------------------------------------------------------------------------------------------
  int dll_fclose (FILE * stream)
  {
    int iFile = (int)stream - 1;
    return dll_close(iFile);
  }

  // should be moved to CFile classes
  intptr_t dll_findfirst(const char *file, struct _finddata_t *data)
  {
    char str[XBMC_MAX_PATH];
    char* p;

    CURL url(file);
    if (url.GetProtocol() == "")
    {
      // move to CFile classes
      if (strncmp(file, "\\Device\\Cdrom0", 14) == 0)
      {
        // replace "\\Device\\Cdrom0" with "D:"
        strcpy(str, "D:");
        strcat(str, file + 14);
      }
      else strcpy(str, file);

      // convert '/' to '\\'
      p = str;
      while (p = strchr(p, '/')) *p = '\\';

      return _findfirst(str, data);
    }
    // non-local files. handle through IDirectory-class - only supports '*.bah' or '*.*'
    CStdString strURL(file);
    CStdString strMask;
    if (url.GetFileName().Find("*.*") != string::npos)
    {
      CStdString strReplaced = url.GetFileName();
      strReplaced.Replace("*.*","");
      url.SetFileName(strReplaced);
    }
    else if (url.GetFileName().Find("*.") != string::npos)
    {
      CUtil::GetExtension(url.GetFileName(),strMask);
      url.SetFileName(url.GetFileName().Left(url.GetFileName().Find("*.")));
    }
    int iDirSlot=0; // locate next free directory
    while ((vecDirsOpen[iDirSlot].Directory) && (iDirSlot<MAX_OPEN_DIRS)) iDirSlot++;
    if (iDirSlot > MAX_OPEN_DIRS)
      return 0xFFFF; // no free slots
    CStdString fName = url.GetFileName();
    url.SetFileName("");
    url.GetURL(strURL);
    bVecDirsInited = true;
    vecDirsOpen[iDirSlot].items.Clear();
    vecDirsOpen[iDirSlot].Directory = CFactoryDirectory::Create(strURL);
    vecDirsOpen[iDirSlot].Directory->SetMask(strMask);
    vecDirsOpen[iDirSlot].Directory->GetDirectory(strURL+fName,vecDirsOpen[iDirSlot].items);
    if (vecDirsOpen[iDirSlot].items.Size())
    {
      strcpy(data->name,vecDirsOpen[iDirSlot].items[0]->GetLabel().c_str());
      data->size = static_cast<_fsize_t>(vecDirsOpen[iDirSlot].items[0]->m_dwSize);
      data->time_write = iDirSlot; // save for later lookups
      data->time_access = 0;
      delete vecDirsOpen[iDirSlot].Directory;
      vecDirsOpen[iDirSlot].Directory = NULL;
      return NULL;
    }
    delete vecDirsOpen[iDirSlot].Directory;
    vecDirsOpen[iDirSlot].Directory = NULL;
    return 0xFFFF; // whatever != NULL
  }

  // should be moved to CFile classes
  int dll_findnext(intptr_t f, _finddata_t* data)
  {
    if ((data->time_write < 0) || (data->time_write > MAX_OPEN_DIRS)) // assume not one of our's
      return _findnext(f, data); // local dir

    // we have a valid data struture. get next item!
    int iItem=data->time_access;
    if (iItem+1 < vecDirsOpen[data->time_write].items.Size()) // we have a winner!
    {
      strcpy(data->name,vecDirsOpen[data->time_write].items[iItem+1]->GetLabel().c_str());
      data->size = static_cast<_fsize_t>(vecDirsOpen[data->time_write].items[iItem+1]->m_dwSize);
      data->time_access++;
      return 0;
    }

    vecDirsOpen[data->time_write].items.Clear();
    return -1;
  }

  char * dll_fgets (char* pszString, int num , FILE * stream)
  {
    int iFile = (int)stream - 1;
    if (!bVecFilesInited) InitFiles();
    if (iFile < 0 || iFile >= MAX_OPEN_FILES )
    {
      return NULL;
    }
    CFile* pFile = vecFilesOpen[iFile];
    if (!pFile)
    {
      return NULL;
    }
    if (pFile->GetPosition() >= pFile->GetLength())
    {
      return NULL;
    }
    bool bRead = pFile->ReadString(pszString, num);
    if (bRead)
    {
      return pszString;
    }
    return NULL;
  }

  int dll_feof (FILE * stream)
  {
    int iFile = (int)stream - 1;
    if (!bVecFilesInited) InitFiles();
    if (iFile < 0 || iFile >= MAX_OPEN_FILES ) return 1;
    CFile* pFile = vecFilesOpen[iFile];
    if (!pFile) return 1;
    if (pFile->GetPosition() >= pFile->GetLength() ) return 1;
    return 0;
  }

  int dll_fread (void * buffer, size_t size, size_t count, FILE * stream)
  {
    if (!size) return -1;
    int iFile = (int)stream - 1;
    int iItemsRead = dll_read(iFile, buffer, count * size);
    iItemsRead /= size;
    return iItemsRead;    
  }


  int dll_getc (FILE * stream)
  {
    char szString[10];
    if (dll_fread(&szString[0], 1, 1, stream) <= 0)
    {
      return -1;
    }
    if (dll_feof(stream))
    {
      return -1;
    }

    byte byKar = (byte)szString[0];
    int iKar = byKar;
    return iKar;
  }

  FILE * dll_fopen (const char * filename, const char * mode)
  {
    int iMode = O_TEXT;
    if (strchr(mode, 'b') )
      iMode = O_BINARY;
    if (strchr(mode, 'w'))
      iMode |= O_RDWR | O_CREAT;
    int iFile = dll_open(filename, iMode);
    if (iFile < 0)
    {
      return NULL;
    }
    return (FILE*)(iFile + 1); // add 1 as 0 is a valid fd
  }

  int dll_fputc (int character, FILE * stream)
  {
    not_implement("msvcrt.dll fake function dll_fputc() called\n");
    return 0;
  }

  int dll_fputs (const char * szLine , FILE* stream)
  {
    if (stream == stdout || stream == stderr )
    { //Stdout
      dllputs(szLine);
    }
    else
    {
      not_implement("msvcrt.dll fake function dll_fputs() called\n");
      OutputDebugString(szLine);
      OutputDebugString("\n");
    }

    return 1;
  }
  int dll_fseek ( FILE * stream , long offset , int origin )
  {
    int iFile = (int)stream - 1;
    dll_lseek(iFile, offset, origin);
    return 0;
  }

  int dll_ungetc (int c, FILE * stream)
  {
    char szString[10];
    if (dll_fseek(stream, -1, SEEK_CUR)!=0)
    {
      return -1;
    }
    if (dll_fread(&szString[0], 1, 1, stream) <= 0)
    {
      return -1;
    }
    if (dll_feof(stream))
    {
      return -1;
    }

    byte byKar = (byte)szString[0];
    int iKar = byKar;
    return iKar;
  }

  long dll_ftell ( FILE * stream )
  {
    int iFile = (int)stream - 1;
    if (!bVecFilesInited) InitFiles();
    if (iFile < 0 || iFile >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[iFile];
    if (!pFile) return -1;
    return (long)pFile->GetPosition();
  }

  long dll_tell ( int fd )
  {

    if (!bVecFilesInited) InitFiles();
    if (fd < 0 || fd >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[fd];
    if (!pFile) return -1;
    return (long)pFile->GetPosition();
  }

  __int64 dll_telli64 ( int fd )
  {

    if (!bVecFilesInited) InitFiles();
    if (fd < 0 || fd >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[fd];
    if (!pFile) return -1;
    return (__int64)pFile->GetPosition();
  }

  size_t dll_fwrite ( const void * buffer, size_t size, size_t count, FILE * stream )
  {
    int iFile = (int)stream - 1;
    int iItemsWritten = dll_write(iFile, buffer, count * size);
    iItemsWritten /= size;
    return iItemsWritten;
  }

  int dll_fflush (FILE * stream)
  {
    int iFile = (int)stream - 1;
    if (iFile < 0 || iFile >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[iFile];
    if (!pFile) return -1;
    pFile->Flush();
    return 0;
  }

  int dll_ferror (FILE * stream)
  {
    int iFile = (int)stream - 1;
    if (iFile < 0 || iFile >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[iFile];
    if (!pFile) return -1;
    // unimplemented
    return 0;
  }

  int dll_vfprintf(FILE *stream, const char *format, va_list va)
  {
    static char tmp[2048];
    
    if (stream == stdout || stream == stderr)
    {
      _vsnprintf(tmp, 2048, format, va);
      
      tmp[2048 - 1] = 0;
      CLog::Log(LOGINFO, "  msg:%s", tmp);

      return strlen(tmp) + 1;
    }

    int iFile = (int)stream - 1;
    if (!bVecFilesInited) InitFiles();
    if (iFile < 0 || iFile >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[iFile];
    if (!pFile) return -1;
    
    if (_vsnprintf(tmp, 2048, format, va) == -1)
      CLog::Log(LOGWARNING, "dll_fprintf: Data lost due to undersized buffer");

    tmp[2048 - 1] = 0;
    int len = strlen(tmp);
    // replace all '\n' occurences with '\r\n'...
    char tmp2[2048];
    int j = 0;
    for (int i = 0; i < len; i++)
    {
      if (j == 2047)
      { // out of space
        if (i != len-1)
          CLog::Log(LOGWARNING, "dll_fprintf: Data lost due to undersized buffer");
        break;
      }
      if (tmp[i] == '\n' && ((i > 0 && tmp[i-1] != '\r') || i == 0) && j < 2047 - 2)
      { // need to add a \r
        tmp2[j++] = '\r';
        tmp2[j++] = '\n';
      }
      else
      { // just add the character as-is
        tmp2[j++] = tmp[i];
      }
    }
    // terminate string
    tmp2[j] = 0;
    len = strlen(tmp2);
    pFile->Write(tmp2, len);
    return len;
  }
  
  int dll_fprintf(FILE* stream , const char * format, ...)
  {
    int res;
    va_list va;
    va_start(va, format);
    res = dll_vfprintf(stream, format, va);
    va_end(va);
    return res;
  }

  int dll_fgetpos(FILE* stream, fpos_t* pos)
  {
    int iFile = (int)stream - 1;
    if (!bVecFilesInited)
      InitFiles();
    if (iFile < 0 || iFile >= MAX_OPEN_FILES )
      return EINVAL;
    CFile* pFile = vecFilesOpen[iFile];
    if (!pFile)
      return EINVAL;
    *pos = pFile->GetPosition();
    return 0;
  }

  int dll_fsetpos(FILE* stream, const fpos_t* pos)
  {
    int iFile = (int)stream - 1;
    if (dll_lseeki64(iFile, *pos, SEEK_SET) >= 0)
      return 0;
    else
      return EINVAL;
  }

  int dll_fileno(FILE* stream)
  {
    return (int)stream;
  }

  char* dll_strdup( const char* str)
  {
    char* pdup;
    pdup = strdup(str);
    return pdup;
  }


  //Critical Section has been fixed in EMUkernel32.cpp

  int dll_initterm(PFV * start, PFV * end)        //pncrt.dll
  {
    PFV * temp;
    for (temp = start; temp < end; temp ++)
      if (*temp)
        (*temp)(); //call initial function table.
    return 0;
  }

  HANDLE dll_beginthreadex(LPSECURITY_ATTRIBUTES lpThreadAttributes, DWORD dwStackSize,
                           LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags,
                           LPDWORD lpThreadId)
  {
    // FIXME        --possible use xbox createthread function?
    HANDLE hThread = CreateThread(lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
    return hThread;
  }

  //SLOW CODE SHOULD BE REVISED
  int dll_stat(const char *path, struct _stat *buffer)
  {
    CLog::Log(LOGINFO, "Stating file %s", path);

    if (!strnicmp(path, "shout://", 8)) // don't stat shoutcast
      return -1;
    if (!strnicmp(path, "http://", 7)) // don't stat http
      return -1;
    if (!strnicmp(path, "mms://", 6)) // don't stat mms
      return -1;
    if (!_stricmp(path, "D:") || !_stricmp(path, "D:\\"))
    {
      buffer->st_mode = S_IFDIR;
      return 0;
    }
    if (!stricmp(path, "\\Device\\Cdrom0") || !stricmp(path, "\\Device\\Cdrom0\\"))
    {
      buffer->st_mode = _S_IFDIR;
      return 0;
    }

    struct __stat64 tStat;
    if (CFile::Stat(path, &tStat) == 0)
    {
      CUtil::Stat64ToStat(buffer, &tStat);
      return 0;
    }
    // errno is set by file.Stat(...)
    return -1;
  }

  int dll_stati64(const char *path, struct _stati64 *buffer)
  {
    CLog::Log(LOGINFO, "Stating file %s", path);

    if (!strnicmp(path, "shout://", 8)) // don't stat shoutcast
      return -1;
    if (!strnicmp(path, "http://", 7)) // don't stat http
      return -1;
    if (!strnicmp(path, "mms://", 6)) // don't stat mms
      return -1;
    if (!_stricmp(path, "D:") || !_stricmp(path, "D:\\"))
    {
      buffer->st_mode = _S_IFDIR;
      return 0;
    }
    if (!stricmp(path, "\\Device\\Cdrom0") || !stricmp(path, "\\Device\\Cdrom0\\"))
    {
      buffer->st_mode = _S_IFDIR;
      return 0;
    }

    struct __stat64 tStat;
    if (CFile::Stat(path, &tStat) == 0)
    {
      CUtil::Stat64ToStatI64(buffer, &tStat);
      return 0;
    }
    // errno is set by file.Stat(...)
    return -1;
  }


  int dll_fstat(FILE * stream, struct _stat *buffer)
  {
    CLog::Log(LOGINFO, "Stating open file");
    int iFile = (int)stream - 1;
    if (!bVecFilesInited) InitFiles();
    if (iFile < 0 || iFile >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[iFile];
    if (!pFile) return -1;

    __int64 size = pFile->GetLength();
    if (size <= LONG_MAX)
      buffer->st_size = (_off_t)size;
    else
    {
      buffer->st_size = 0;
      CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
    }
    buffer->st_mode = _S_IFREG;

    return 0;

  }

  int dll_fstati64(FILE * stream, struct _stati64 *buffer)
  {
    CLog::Log(LOGINFO, "Stating open file");
    int iFile = (int)stream - 1;
    if (!bVecFilesInited) InitFiles();
    if (iFile < 0 || iFile >= MAX_OPEN_FILES ) return -1;
    CFile* pFile = vecFilesOpen[iFile];
    if (!pFile) return -1;

    buffer->st_size = pFile->GetLength();
    buffer->st_mode = _S_IFREG;

    return 0;

  }

  int dll_setmode ( int handle, int mode )
  {
    not_implement("msvcrt.dll fake function dll_setmode() called\n");
    return -1;
  }

  void dllperror(const char* s)
  {
    OutputDebugString("dllperror\n");
    if (s)
      OutputDebugString(s);
  }

  char* dllstrerror(int iErr)
  {
    static char szError[128];
    sprintf(szError, "err:%i", iErr);
    return (char*)szError;
  }

  int dll_mkdir(const char* dir)
  {
    if (!dir) return -1;

    CStdString newDir(dir);
    newDir.Replace("/", "\\");
    newDir.Replace("\\\\", "\\");

    return mkdir(newDir.c_str());
  }

  char* dll_getcwd(char *buffer, int maxlen)
  {
    not_implement("msvcrt.dll fake function dll_getcwd() called\n");
    return "Q:";
  }

  int dll_putenv(const char* envstring)
  {
    not_implement("msvcrt.dll fake function dll_putenv() called\n");
    return 0;
  }

  int dll_ctype(int i)
  {
    not_implement("msvcrt.dll fake function dll_ctype() called\n");
    return 0;
  }

  int dll_system(const char *command)
  {
    not_implement("msvcrt.dll fake function dll_system() called\n");
    return NULL; //system(command);
  }

  void (__cdecl * dll_signal(int sig, void (__cdecl *func)(int)))(int)
  {
    // the xbox has a NSIG of 23 (+1), problem is when calling signal with 
    // one of the signals below the xbox wil crash. Just return SIG_ERR
    if (sig == SIGILL || sig == SIGFPE || sig == SIGSEGV) return SIG_ERR;
    
    return signal(sig, func);
  }

  int dll_getpid()
  {
    return 1;
  }
};
