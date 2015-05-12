/*
 *      Copyright (C) 2003 by The Joker / Avalaunch team
 *      Copyright (C) 2003-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <algorithm>
#include "system.h"
#include "utils/log.h"
#include <stdlib.h>

/*
 Redbook   : CDDA
 Yellowbook : CDROM
ISO9660
 CD-ROM Mode 1 divides the 2352 byte data area into:
  -12  bytes of synchronisation
  -4  bytes of header information
  -2048 bytes of user information
  -288 bytes of error correction and detection codes.

 CD-ROM Mode 2 redefines the use of the 2352 byte data area as follows:
  -12 bytes of synchronisation
  -4 bytes of header information
  -2336 bytes of user data.

 ISO9660 specs:
 http://www.ecma-international.org/publications/files/ECMA-ST/Ecma-119.pdf


*/
#include "iso9660.h"
#include "storage/IoSupport.h"
#include "utils/CharsetConverter.h"
#include "threads/SingleLock.h"
#include "IFile.h"

#ifndef TARGET_WINDOWS
#include "storage/DetectDVDType.h"  // for MODE2_DATA_SIZE etc.
#endif
#include <cdio/bytesex.h>
//#define _DEBUG_OUTPUT 1

static CCriticalSection m_critSection;
class iso9660 m_isoReader;
#define BUFFER_SIZE MODE2_DATA_SIZE
#define RET_ERR -1

using namespace std;

#ifndef HAS_DVD_DRIVE
extern "C"
{
  void cdio_warn(const char* msg, ...) { CLog::Log(LOGWARNING, "%s", msg); }
}
#endif

//******************************************************************************************************************
const string iso9660::ParseName(struct iso9660_Directory& isodir)
{
  string temp_text = (char*)isodir.FileName;
  temp_text.resize(isodir.Len_Fi);
  int iPos = isodir.Len_Fi;

  if (isodir.FileName[iPos] == 0)
  {
    iPos++;
  }

  if (isodir.FileName[iPos] == 'R' && isodir.FileName[iPos + 1] == 'R')
  {
    // rockridge
    iPos += 5;
    do
    {
      if (isodir.FileName[iPos] == 'N' && isodir.FileName[iPos + 1] == 'M')
      {
        // altername name
        // "N" "M"  LEN_NM  1  FLAGS  NAMECONTENT
        // BP1 BP2    BP3      BP4  BP5     BP6-LEN_NM
        int iNameLen = isodir.FileName[iPos + 2] - 5;
        temp_text = (char*) & isodir.FileName[iPos + 5];
        temp_text.resize(iNameLen);
        iPos += (iNameLen + 5);
      }
      if ( isascii(isodir.FileName[iPos]) && isascii(isodir.FileName[iPos + 1]))
      {
        // ??
        // "?" "?"  LEN
        // BP1 BP2  BP3
        iPos += isodir.FileName[iPos + 2];
      }
    }
    while (33 + iPos < isodir.ucRecordLength && isodir.FileName[iPos + 2] != 0);
    // when this isodir.FileName[iPos+2] is equal to 0 it should break out
    // as it has finished the loop
    // this is the fix for rockridge support
  }
  return temp_text;
}

bool iso9660::IsRockRidge(struct iso9660_Directory& isodir)
{
  int iPos = isodir.Len_Fi;

  if (isodir.FileName[iPos] == 0)
  {
    iPos++;
  }

  // found rock ridge in system use field
  if (isodir.FileName[iPos] == 'R' && isodir.FileName[iPos + 1] == 'R')
    return true;

  return false;
}

//******************************************************************************************************************
struct iso_dirtree *iso9660::ReadRecursiveDirFromSector( DWORD sector, const char *path )
{
  struct iso_dirtree* pDir = NULL;
  struct iso_dirtree* pFile_Pointer = NULL;
  char* pCurr_dir_cache = NULL;
  DWORD iso9660searchpointer;
  struct iso9660_Directory isodir;
  struct iso9660_Directory curr_dir;
  WORD wSectorSize = from_723(m_info.iso.logical_block_size);


  struct iso_directories *point = m_lastpath;
  if (point)
  {
    while ( point->next )
    {
      if (strcmp(path, point->path) == 0) return NULL;
      point = point->next;
    }
  }


#ifdef _DEBUG_OUTPUT
  std::string strTmp;
  strTmp = StringUtils::Format("******************   Adding dir : %s\r", path);
  OutputDebugString( strTmp.c_str() );
#endif

  pDir = (struct iso_dirtree *)malloc(sizeof(struct iso_dirtree));
  if (!pDir)
    return NULL;

  pDir->next = NULL;
  pDir->path = NULL;
  pDir->name = NULL;
  pDir->dirpointer = NULL;
  pFile_Pointer = pDir;
  m_vecDirsAndFiles.push_back(pDir);


  ::SetFilePointer( m_info.ISO_HANDLE, wSectorSize * sector, 0, FILE_BEGIN );
  DWORD lpNumberOfBytesRead = 0;

  pCurr_dir_cache = (char*)malloc( 16*wSectorSize );
  if (!pCurr_dir_cache )
    return NULL;

  BOOL bResult = ::ReadFile( m_info.ISO_HANDLE, pCurr_dir_cache, wSectorSize, &lpNumberOfBytesRead, NULL );
  if (!bResult || lpNumberOfBytesRead != wSectorSize)
  {
    CLog::Log(LOGERROR, "%s: unable to read", __FUNCTION__);
    free(pCurr_dir_cache);
    return NULL;
  }
  memcpy( &isodir, pCurr_dir_cache, sizeof(isodir) );
  memcpy( &curr_dir, pCurr_dir_cache, sizeof(isodir) );

  DWORD curr_dirSize = from_733(curr_dir.size);
  if ( curr_dirSize > wSectorSize )
  {
    free( pCurr_dir_cache );
    pCurr_dir_cache = (char*)malloc( 16 * from_733(isodir.size) );
    if (!pCurr_dir_cache )
      return NULL;

    ::SetFilePointer( m_info.ISO_HANDLE, wSectorSize * sector, 0, FILE_BEGIN );
    bResult = ::ReadFile( m_info.ISO_HANDLE, pCurr_dir_cache , curr_dirSize, &lpNumberOfBytesRead, NULL );
    if (!bResult || lpNumberOfBytesRead != curr_dirSize)
    {
      CLog::Log(LOGERROR, "%s: unable to read", __FUNCTION__);
      free(pCurr_dir_cache);
      return NULL;
    }
  }
  iso9660searchpointer = 0;

  if (!m_lastpath)
  {
    m_lastpath = m_paths;
    if ( !m_lastpath )
    {
      m_paths = (struct iso_directories *)malloc(sizeof(struct iso_directories));
      if (!m_paths )
        return NULL;

      m_paths->path = NULL;
      m_paths->dir = NULL;
      m_paths->next = NULL;
      m_lastpath = m_paths;
    }
    else
    {
      while ( m_lastpath->next )
        m_lastpath = m_lastpath->next;
    }
  }
  m_lastpath->next = ( struct iso_directories *)malloc( sizeof( struct iso_directories ) );
  if (!m_lastpath->next )
  {
    free(pCurr_dir_cache);
    return NULL;
  }

  m_lastpath = m_lastpath->next;
  m_lastpath->next = NULL;
  m_lastpath->dir = pDir;
  m_lastpath->path = (char *)malloc(strlen(path) + 1);
  if (!m_lastpath->path )
  {
    free(pCurr_dir_cache);
    return NULL;
  }

  strcpy( m_lastpath->path, path );

  while ( 1 )
  {
    if ( isodir.ucRecordLength )
      iso9660searchpointer += isodir.ucRecordLength;
    else
    {
      iso9660searchpointer = (iso9660searchpointer - (iso9660searchpointer % wSectorSize)) + wSectorSize;
    }
    if ( curr_dirSize <= iso9660searchpointer )
    {
      break;
    }
    int isize = min(sizeof(isodir), sizeof(m_info.isodir));
    memcpy( &isodir, pCurr_dir_cache + iso9660searchpointer, isize);
    if (!isodir.ucRecordLength)
      continue;
    if ( !(isodir.byFlags & Flag_NotExist) )
    {
      if ( (!( isodir.byFlags & Flag_Directory )) && ( isodir.Len_Fi > 1) )
      {
        string temp_text ;
        bool bContinue = false;

        if ( m_info.joliet )
        {
          bContinue = true;
          isodir.FileName[isodir.Len_Fi] = isodir.FileName[isodir.Len_Fi + 1] = 0; //put terminator by its length
          temp_text = GetThinText(isodir.FileName, isodir.Len_Fi );
          //     temp_text.resize(isodir.Len_Fi);
        }

        if (!m_info.joliet && isodir.FileName[0] >= 0x20 )
        {
          temp_text = ParseName(isodir);
          bContinue = true;
        }
        if (bContinue)
        {
          int semipos = temp_text.find(";", 0);
          if (semipos >= 0)
            temp_text.erase(semipos, temp_text.length() - semipos);


          pFile_Pointer->next = (struct iso_dirtree *)malloc(sizeof(struct iso_dirtree));
          if (!pFile_Pointer->next)
            break;

          m_vecDirsAndFiles.push_back(pFile_Pointer->next);
          pFile_Pointer = pFile_Pointer->next;
          pFile_Pointer->next = 0;
          pFile_Pointer->dirpointer = NULL;
          pFile_Pointer->path = (char *)malloc(strlen(path) + 1);
          if (!pFile_Pointer->path)
          {
            free(pCurr_dir_cache);
            return NULL;
          }

          strcpy( pFile_Pointer->path, path );
          pFile_Pointer->name = (char *)malloc( temp_text.length() + 1);
          if (!pFile_Pointer->name)
          {
            free(pCurr_dir_cache);
            return NULL;
          }

          strcpy( pFile_Pointer->name , temp_text.c_str());
#ifdef _DEBUG_OUTPUT
          //std::string strTmp;
          //strTmp = StringUtils::Format("adding sector : %X, File : %s     size = %u     pos = %x\r",sector,temp_text.c_str(), isodir.dwFileLengthLE, isodir.dwFileLocationLE );
          //OutputDebugString( strTmp.c_str());
#endif

          pFile_Pointer->Location = from_733(isodir.extent);
          pFile_Pointer->dirpointer = NULL;
          pFile_Pointer ->Length = from_733(isodir.size);

          IsoDateTimeToFileTime(&isodir.DateTime, &pFile_Pointer->filetime);

          pFile_Pointer->type = 1;
        }
      }
    }
  }
  iso9660searchpointer = 0;
  memcpy( &curr_dir, pCurr_dir_cache, sizeof(isodir) );
  memcpy( &isodir, pCurr_dir_cache, sizeof(isodir) );
  while ( 1 )
  {
    if ( isodir.ucRecordLength )
      iso9660searchpointer += isodir.ucRecordLength;


    else
    {
      iso9660searchpointer = (iso9660searchpointer - (iso9660searchpointer % wSectorSize)) + wSectorSize;
    }
    if ( from_733(curr_dir.size) <= iso9660searchpointer )
    {
      free( pCurr_dir_cache );
      pCurr_dir_cache = NULL;
      return pDir;
    }
    memcpy( &isodir, pCurr_dir_cache + iso9660searchpointer, min(sizeof(isodir), sizeof(m_info.isodir)));
    if (!isodir.ucRecordLength)
      continue;
    if ( !(isodir.byFlags & Flag_NotExist) )
    {
      if ( (( isodir.byFlags & Flag_Directory )) && ( isodir.Len_Fi > 1) )
      {
        string temp_text ;
        bool bContinue = false;
        if ( m_info.joliet )
        {
          bContinue = true;
          isodir.FileName[isodir.Len_Fi] = isodir.FileName[isodir.Len_Fi + 1] = 0; //put terminator by its length
          temp_text = GetThinText(isodir.FileName, isodir.Len_Fi);
          //     temp_text.resize(isodir.Len_Fi);
        }
        if (!m_info.joliet && isodir.FileName[0] >= 0x20 )
        {
          temp_text = ParseName(isodir);
          bContinue = true;
        }
        if (bContinue)
        {

          //     int semipos = temp_text.find(";",0); //the directory is not seperate by ";",but by its length
          //     if (semipos >= 0)
          //       temp_text.erase(semipos,temp_text.length()-semipos);

          pFile_Pointer->next = (struct iso_dirtree *)malloc(sizeof(struct iso_dirtree));
          if (!pFile_Pointer->next)
            return NULL;

          m_vecDirsAndFiles.push_back(pFile_Pointer->next);
          pFile_Pointer = pFile_Pointer->next;
          pFile_Pointer->next = 0;
          pFile_Pointer->dirpointer = NULL;
          pFile_Pointer->path = (char *)malloc(strlen(path) + 1);

          if (!pFile_Pointer->path)
          {
            free(pCurr_dir_cache);
            return NULL;
          }

          strcpy( pFile_Pointer->path, path );
          pFile_Pointer->name = (char *)malloc( temp_text.length() + 1);

          if (!pFile_Pointer->name)
          {
            free(pCurr_dir_cache);
            return NULL;
          }

          strcpy( pFile_Pointer->name , temp_text.c_str());

          DWORD dwFileLocation = from_733(isodir.extent);
#ifdef _DEBUG_OUTPUT
          std::string strTmp;
          strTmp = StringUtils::Format("adding directory sector : %X, File : %s     size = %u     pos = %x\r", sector, temp_text.c_str(), from_733(isodir.size), dwFileLocation );
          OutputDebugString( strTmp.c_str());
#endif

          pFile_Pointer->Location = dwFileLocation;
          pFile_Pointer->dirpointer = NULL;
          pFile_Pointer->Length = from_733(isodir.size);

          IsoDateTimeToFileTime(&isodir.DateTime, &pFile_Pointer->filetime);

          string strPath = path;
          if ( strlen( path ) > 1 ) strPath += "\\";
          strPath += temp_text;

          pFile_Pointer->dirpointer = ReadRecursiveDirFromSector( dwFileLocation, strPath.c_str() );

          pFile_Pointer->type = 2;
        }
      }
    }
  }
  return NULL;
}
//******************************************************************************************************************
iso9660::iso9660( )
{
  memset(m_isoFiles, 0, sizeof(m_isoFiles));
  m_hCDROM = NULL;
  m_lastpath = NULL;
  m_searchpointer = NULL;
  m_paths = 0;
  Reset();
}

void iso9660::Scan()
{
  if (m_hCDROM != NULL)
    return ;

  m_hCDROM = CIoSupport::OpenCDROM();
  CIoSupport::AllocReadBuffer();

  m_paths = 0;
  m_lastpath = 0;
  memset(&m_info, 0, sizeof(m_info));
  m_info.ISO_HANDLE = m_hCDROM ;
  m_info.Curr_dir_cache = 0;
  m_info.Curr_dir = (char*)malloc( 4096 );
  strcpy( m_info.Curr_dir, "\\" );

  CSingleLock lock(m_critSection);

  DWORD lpNumberOfBytesRead = 0;
  ::SetFilePointer( m_info.ISO_HANDLE, 0x8000, 0, FILE_BEGIN );

  ::ReadFile( m_info.ISO_HANDLE, &m_info.iso, sizeof(m_info.iso), &lpNumberOfBytesRead, NULL );

  if (strncmp(m_info.iso.szSignature, "CD001", 5))
  {
    CIoSupport::CloseCDROM( m_info.ISO_HANDLE);
    CIoSupport::FreeReadBuffer();
    m_info.ISO_HANDLE = NULL;
    m_hCDROM = NULL;
    m_info.iso9660 = 0;
    return ;
  }
  else
  {
    m_info.iso9660 = 1;
    m_info.joliet = 0;

    m_info.HeaderPos = 0x8000;
    int current = 0x8000;

    WORD wSectorSize = from_723(m_info.iso.logical_block_size);

    // first check if first file in the current VD has a rock-ridge NM. if it has, disable joliet
    iso9660_Directory *dirPointer = reinterpret_cast<iso9660_Directory*>(&m_info.iso.szRootDir);
    ::SetFilePointer( m_info.ISO_HANDLE, wSectorSize * from_733(dirPointer->extent), 0, FILE_BEGIN );

    DWORD lpNumberOfBytesRead;
    char* pCurr_dir_cache = (char*)malloc( 16*wSectorSize );
    iso9660_Directory isodir;
    BOOL bResult = ::ReadFile( m_info.ISO_HANDLE, pCurr_dir_cache, wSectorSize, &lpNumberOfBytesRead, NULL );
    memcpy( &isodir, pCurr_dir_cache, sizeof(isodir));

    int iso9660searchpointer=0;
    if ( isodir.ucRecordLength )
      iso9660searchpointer += isodir.ucRecordLength;
    else
      iso9660searchpointer = (iso9660searchpointer - (iso9660searchpointer % wSectorSize)) + wSectorSize;

    memcpy( &isodir, pCurr_dir_cache + iso9660searchpointer,min(sizeof(isodir), sizeof(m_info.isodir)));
    free(pCurr_dir_cache);
    if (bResult && lpNumberOfBytesRead == wSectorSize)
      bResult = IsRockRidge(isodir);
    while ( m_info.iso.byOne != 255)
    {
      if ( ( m_info.iso.byZero3[0] == 0x25 ) && ( m_info.iso.byZero3[1] == 0x2f ) && !bResult )
      {
        switch ( m_info.iso.byZero3[2] )
        {
        case 0x45 :
        case 0x40 :
        case 0x43 : m_info.HeaderPos = current;
          m_info.joliet = 1;
        }
        //                        25 2f 45  or   25 2f 40   or 25 2f 43 = jouliet, and best fitted for reading
      }
      current += 0x800;
      ::SetFilePointer( m_info.ISO_HANDLE, current, 0, FILE_BEGIN );
      ::ReadFile( m_info.ISO_HANDLE, &m_info.iso, sizeof(m_info.iso), &lpNumberOfBytesRead, NULL );
    }
    ::SetFilePointer( m_info.ISO_HANDLE, m_info.HeaderPos, 0, FILE_BEGIN );
    ::ReadFile( m_info.ISO_HANDLE, &m_info.iso, sizeof(m_info.iso), &lpNumberOfBytesRead, NULL );
    memcpy( &m_info.isodir, m_info.iso.szRootDir, sizeof(m_info.isodir));
  }

  memcpy( &m_info.isodir, &m_info.iso.szRootDir, sizeof(m_info.isodir) );
  ReadRecursiveDirFromSector( from_733(m_info.isodir.extent), "\\" );
}

//******************************************************************************************************************
iso9660::~iso9660( )
{
  Reset();
}

void iso9660::Reset()
{

  if (m_info.Curr_dir)
    free(m_info.Curr_dir);
  m_info.Curr_dir = NULL;

  if (m_info.Curr_dir_cache)
    free(m_info.Curr_dir_cache);
  m_info.Curr_dir_cache = NULL;



  struct iso_directories* nextpath;

  while ( m_paths )
  {
    nextpath = m_paths->next;
    if (m_paths->path) free(m_paths->path);

    free (m_paths);
    m_paths = nextpath;
  }
  for (int i = 0; i < (int)m_vecDirsAndFiles.size(); ++i)
  {
    struct iso_dirtree* pDir = m_vecDirsAndFiles[i];
    if (pDir->path) free(pDir->path);
    if (pDir->name) free(pDir->name);
    free(pDir);
  }
  m_vecDirsAndFiles.erase(m_vecDirsAndFiles.begin(), m_vecDirsAndFiles.end());

  for (intptr_t i = 0; i < MAX_ISO_FILES;++i)
  {
    FreeFileContext( (HANDLE)i);
  }

  if (m_hCDROM)
  {
    CIoSupport::CloseCDROM(m_hCDROM);
    CIoSupport::FreeReadBuffer();
  }
  m_hCDROM = NULL;
}

//******************************************************************************************************************
struct iso_dirtree *iso9660::FindFolder( char *Folder )
{
  char *work;

  work = (char *)malloc(from_723(m_info.iso.logical_block_size));

  char *temp;
  struct iso_directories *lastpath = NULL;;

  if ( strpbrk(Folder, ":") )
    strcpy(work, strpbrk(Folder, ":") + 1);
  else
    strcpy(work, Folder);

  temp = work + 1;
  while ( strlen( temp ) > 1 && strpbrk( temp + 1, "\\" ) )
    temp = strpbrk( temp + 1, "\\" );

  if ( strlen( work ) > 1 && work[ strlen(work) - 1 ] == '*' )
  {
    work[ strlen(work) - 1 ] = 0;
  }
  if ( strlen( work ) > 2 )
    if ( work[ strlen(work) - 1 ] == '\\' )
      work[ strlen(work) - 1 ] = 0;

  if (m_paths)
    lastpath = m_paths->next;
  while ( lastpath )
  {
    if ( !stricmp( lastpath->path, work))
    {
      free ( work );
      return lastpath->dir;
    }
    lastpath = lastpath->next;
  }
  free ( work );
  return 0;
}

//******************************************************************************************************************
HANDLE iso9660::FindFirstFile( char *szLocalFolder, WIN32_FIND_DATA *wfdFile )
{
  if (m_info.ISO_HANDLE == 0) return (HANDLE)0;
  memset( wfdFile, 0, sizeof(WIN32_FIND_DATA));

  m_searchpointer = FindFolder( szLocalFolder );

  if ( m_searchpointer )
  {
    m_searchpointer = m_searchpointer->next;

    if ( m_searchpointer )
    {
      strcpy(wfdFile->cFileName, m_searchpointer->name );

      if ( m_searchpointer->type == 2 )
        wfdFile->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

      wfdFile->ftLastWriteTime = m_searchpointer->filetime;
      wfdFile->ftLastAccessTime = m_searchpointer->filetime;
      wfdFile->ftCreationTime = m_searchpointer->filetime;

      wfdFile->nFileSizeLow = m_searchpointer->Length;
      return (HANDLE)1;
    }
  }
  return (HANDLE)0;
}

//******************************************************************************************************************
int iso9660::FindNextFile( HANDLE szLocalFolder, WIN32_FIND_DATA *wfdFile )
{
  memset( wfdFile, 0, sizeof(WIN32_FIND_DATA));

  if ( m_searchpointer )
    m_searchpointer = m_searchpointer->next;

  if ( m_searchpointer )
  {
    strcpy(wfdFile->cFileName, m_searchpointer->name );

    if ( m_searchpointer->type == 2 )
      wfdFile->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

    wfdFile->ftLastWriteTime = m_searchpointer->filetime;
    wfdFile->ftLastAccessTime = m_searchpointer->filetime;
    wfdFile->ftCreationTime = m_searchpointer->filetime;

    wfdFile->nFileSizeLow = m_searchpointer->Length;
    return 1;
  }

  return 0;
}

//******************************************************************************************************************
bool iso9660::FindClose( HANDLE szLocalFolder )
{
  m_searchpointer = 0;
  if (m_info.Curr_dir_cache) free(m_info.Curr_dir_cache);
  m_info.Curr_dir_cache = 0;
  return true;
}



//******************************************************************************************************************
string iso9660::GetThinText(BYTE* strTxt, int iLen )
{
  // convert from "fat" text (UTF-16BE) to "thin" text (UTF-8)
  std::u16string strTxtUnicode((char16_t*)strTxt, iLen / 2);
  std::string utf8String;

  g_charsetConverter.utf16BEtoUTF8(strTxtUnicode, utf8String);

  return utf8String;
}

//************************************************************************************
HANDLE iso9660::OpenFile(const char *filename)
{
  if (m_info.ISO_HANDLE == NULL) return INVALID_HANDLE_VALUE;
  HANDLE hContext = AllocFileContext();
  if (hContext == INVALID_HANDLE_VALUE) return hContext;

  iso9660::isofile* pContext = GetFileContext(hContext);
  if (!pContext)
    return INVALID_HANDLE_VALUE;

  WIN32_FIND_DATA fileinfo;
  char *pointer, *pointer2;
  char work[512];
  pContext->m_bUseMode2 = false;
  m_info.curr_filepos = 0;

  pointer = (char*)filename;
  while ( strpbrk( pointer, "\\/" ) )
    pointer = strpbrk( pointer, "\\/" ) + 1;

  strcpy(work, filename );
  pointer2 = work;

  while ( strpbrk(pointer2 + 1, "\\" ) )
    pointer2 = strpbrk(pointer2 + 1, "\\" );

  *(pointer2 + 1) = 0;

  intptr_t loop = (intptr_t)FindFirstFile( work, &fileinfo );
  while ( loop > 0)
  {
    if ( !stricmp(fileinfo.cFileName, pointer ) )
      loop = -1;
    else
      loop = FindNextFile( NULL, &fileinfo );
  }
  if ( loop == 0 )
  {
    FreeFileContext(hContext);
    return INVALID_HANDLE_VALUE;
  }

  pContext->m_dwCurrentBlock = m_searchpointer->Location;
  pContext->m_dwFileSize = m_info.curr_filesize = fileinfo.nFileSizeLow;
  pContext->m_pBuffer = new uint8_t[CIRC_BUFFER_SIZE * BUFFER_SIZE];
  pContext->m_dwStartBlock = pContext->m_dwCurrentBlock;
  pContext->m_dwFilePos = 0;
  pContext->m_dwCircBuffBegin = 0;
  pContext->m_dwCircBuffEnd = 0;
  pContext->m_dwCircBuffSectorStart = 0;
  pContext->m_bUseMode2 = false;

  bool bError;

  CSingleLock lock(m_critSection);
  bError = (CIoSupport::ReadSector(m_info.ISO_HANDLE, pContext->m_dwStartBlock, (char*) & (pContext->m_pBuffer[0])) < 0);
  if ( bError )
  {
    bError = (CIoSupport::ReadSectorMode2(m_info.ISO_HANDLE, pContext->m_dwStartBlock, (char*) & (pContext->m_pBuffer[0])) < 0);
    if ( !bError )
      pContext->m_bUseMode2 = true;
  }
  if (pContext->m_bUseMode2)
    pContext->m_dwFileSize = (pContext->m_dwFileSize / 2048) * MODE2_DATA_SIZE;

  return hContext;
}

//************************************************************************************
void iso9660::CloseFile(HANDLE hFile)
{
  iso9660::isofile* pContext = GetFileContext(hFile);
  if (pContext)
  {
    if (pContext->m_pBuffer)
    {
      delete [] pContext->m_pBuffer;
      pContext->m_pBuffer = NULL;
    }
  }
  FreeFileContext(hFile);
}
//************************************************************************************
bool iso9660::ReadSectorFromCache(iso9660::isofile* pContext, DWORD sector, uint8_t** ppBuffer)
{

  DWORD StartSectorInCircBuff = pContext->m_dwCircBuffSectorStart;
  DWORD SectorsInCircBuff;

  if ( pContext->m_dwCircBuffEnd >= pContext->m_dwCircBuffBegin )
    SectorsInCircBuff = pContext->m_dwCircBuffEnd - pContext->m_dwCircBuffBegin;
  else
    SectorsInCircBuff = CIRC_BUFFER_SIZE - (pContext->m_dwCircBuffBegin - pContext->m_dwCircBuffEnd);

  // If our sector is already in the circular buffer
  if ( sector >= StartSectorInCircBuff &&
       sector < (StartSectorInCircBuff + SectorsInCircBuff) &&
       SectorsInCircBuff > 0 )
  {
    // Just retrieve it
    DWORD SectorInCircBuff = (sector - StartSectorInCircBuff) +
                             pContext->m_dwCircBuffBegin;
    if ( SectorInCircBuff >= CIRC_BUFFER_SIZE )
      SectorInCircBuff -= CIRC_BUFFER_SIZE;

    *ppBuffer = &(pContext->m_pBuffer[SectorInCircBuff]);
  }
  else
  {
    // Sector is not cache.  Read it in.
    bool SectorIsAdjacentInBuffer =
      (StartSectorInCircBuff + SectorsInCircBuff) == sector;
    if ( SectorsInCircBuff == CIRC_BUFFER_SIZE - 1 ||
         !SectorIsAdjacentInBuffer)
    {
      // The cache is full. (Or its not an adjacent request in which we'll
      // also flush the cache)

      // If its adjacent, just get rid of the first sector.
      if ( SectorIsAdjacentInBuffer )
      {
        // Release the first sector in cache
        pContext->m_dwCircBuffBegin++;
        if ( pContext->m_dwCircBuffBegin >= CIRC_BUFFER_SIZE )
          pContext->m_dwCircBuffBegin -= CIRC_BUFFER_SIZE;
        pContext->m_dwCircBuffSectorStart++;
      }
      else
      {
        pContext->m_dwCircBuffBegin = pContext->m_dwCircBuffEnd = 0;
        pContext->m_dwCircBuffSectorStart = 0;
      }
    }
    // Ok, we're ready to read the sector into the cache
    bool bError;
    {
      CSingleLock lock(m_critSection);
      if ( pContext->m_bUseMode2 )
      {
        bError = (CIoSupport::ReadSectorMode2(m_info.ISO_HANDLE, sector, (char*) & (pContext->m_pBuffer[pContext->m_dwCircBuffEnd])) < 0);
      }
      else
      {
        bError = (CIoSupport::ReadSector(m_info.ISO_HANDLE, sector, (char*) & (pContext->m_pBuffer[pContext->m_dwCircBuffEnd])) < 0);
      }
    }
    if ( bError )
      return false;
    *ppBuffer = &(pContext->m_pBuffer[pContext->m_dwCircBuffEnd]);
    if ( pContext->m_dwCircBuffEnd == pContext->m_dwCircBuffBegin )
      pContext->m_dwCircBuffSectorStart = sector;
    pContext->m_dwCircBuffEnd++;
    if ( pContext->m_dwCircBuffEnd >= CIRC_BUFFER_SIZE )
      pContext->m_dwCircBuffEnd -= CIRC_BUFFER_SIZE;
  }
  return true;
}
//************************************************************************************
void iso9660::ReleaseSectorFromCache(iso9660::isofile* pContext, DWORD sector)
{

  DWORD StartSectorInCircBuff = pContext->m_dwCircBuffSectorStart;
  DWORD SectorsInCircBuff;

  if ( pContext->m_dwCircBuffEnd >= pContext->m_dwCircBuffBegin )
    SectorsInCircBuff = pContext->m_dwCircBuffEnd - pContext->m_dwCircBuffBegin;
  else
    SectorsInCircBuff = CIRC_BUFFER_SIZE - (pContext->m_dwCircBuffBegin - pContext->m_dwCircBuffEnd);

  // If our sector is in the circular buffer
  if ( sector >= StartSectorInCircBuff &&
       sector < (StartSectorInCircBuff + SectorsInCircBuff) &&
       SectorsInCircBuff > 0 )
  {
    DWORD SectorsToFlush = sector - StartSectorInCircBuff + 1;
    pContext->m_dwCircBuffBegin += SectorsToFlush;

    pContext->m_dwCircBuffSectorStart += SectorsToFlush;
    if ( pContext->m_dwCircBuffBegin >= CIRC_BUFFER_SIZE )
      pContext->m_dwCircBuffBegin -= CIRC_BUFFER_SIZE;
  }
}
//************************************************************************************
long iso9660::ReadFile(HANDLE hFile, uint8_t *pBuffer, long lSize)
{
  bool bError;
  long iBytesRead = 0;
  DWORD sectorSize = 2048;
  iso9660::isofile* pContext = GetFileContext(hFile);
  if (!pContext) return -1;

  if ( pContext->m_bUseMode2 )
    sectorSize = MODE2_DATA_SIZE;

  while (lSize > 0 && pContext->m_dwFilePos < pContext->m_dwFileSize)
  {
    pContext->m_dwCurrentBlock = (DWORD) (pContext->m_dwFilePos / sectorSize);
    int64_t iOffsetInBuffer = pContext->m_dwFilePos - (sectorSize * pContext->m_dwCurrentBlock);
    pContext->m_dwCurrentBlock += pContext->m_dwStartBlock;

    // CLog::Log(LOGDEBUG, "pos:%li cblk:%li sblk:%li off:%li",(long)pContext->m_dwFilePos, (long)pContext->m_dwCurrentBlock,(long)pContext->m_dwStartBlock,(long)iOffsetInBuffer);

    uint8_t* pSector;
    bError = !ReadSectorFromCache(pContext, pContext->m_dwCurrentBlock, &pSector);
    if (!bError)
    {
      DWORD iBytes2Copy = lSize;
      if (iBytes2Copy > (sectorSize - iOffsetInBuffer) )
        iBytes2Copy = (DWORD) (sectorSize - iOffsetInBuffer);


      memcpy( &pBuffer[iBytesRead], &pSector[iOffsetInBuffer], iBytes2Copy);
      iBytesRead += iBytes2Copy;
      lSize -= iBytes2Copy;
      pContext->m_dwFilePos += iBytes2Copy;

      if ( iBytes2Copy + iOffsetInBuffer == sectorSize )
        ReleaseSectorFromCache(pContext, pContext->m_dwCurrentBlock);

      // Why is this done?  It is recalculated at the beginning of the loop
      pContext->m_dwCurrentBlock += BUFFER_SIZE / MODE2_DATA_SIZE;

    }
    else
    {
      CLog::Log(LOGDEBUG, "iso9660::ReadFile() hit EOF");
      break;
    }
  }
  if (iBytesRead == 0) return -1;
  return iBytesRead;
}
//************************************************************************************
int64_t iso9660::Seek(HANDLE hFile, int64_t lOffset, int whence)
{
  iso9660::isofile* pContext = GetFileContext(hFile);
  if (!pContext) return -1;

  int64_t dwFilePos = pContext->m_dwFilePos;
  switch (whence)
  {
  case SEEK_SET:
    // cur = pos
    pContext->m_dwFilePos = lOffset;
    break;

  case SEEK_CUR:
    // cur += pos
    pContext->m_dwFilePos += lOffset;
    break;
  case SEEK_END:
    // end += pos
    pContext->m_dwFilePos = pContext->m_dwFileSize + lOffset;
    break;
  default:
    return -1;
  }

  if (pContext->m_dwFilePos > pContext->m_dwFileSize || pContext->m_dwFilePos < 0)
  {
    pContext->m_dwFilePos = dwFilePos;
    return pContext->m_dwFilePos;
  }


  return pContext->m_dwFilePos;
}


//************************************************************************************
int64_t iso9660::GetFileSize(HANDLE hFile)
{
  iso9660::isofile* pContext = GetFileContext(hFile);
  if (!pContext) return -1;
  return pContext->m_dwFileSize;
}

//************************************************************************************
int64_t iso9660::GetFilePosition(HANDLE hFile)
{
  iso9660::isofile* pContext = GetFileContext(hFile);
  if (!pContext) return -1;
  return pContext->m_dwFilePos;
}

//************************************************************************************
void iso9660::FreeFileContext(HANDLE hFile)
{
  intptr_t iFile = (intptr_t)hFile;
  if (iFile >= 1 && iFile < MAX_ISO_FILES)
  {
    if (m_isoFiles[iFile ]) delete m_isoFiles[iFile ];
    m_isoFiles[iFile ] = NULL;
  }
}

//************************************************************************************
HANDLE iso9660::AllocFileContext()
{
  for (intptr_t i = 1; i < MAX_ISO_FILES; ++i)
  {
    if (m_isoFiles[i] == NULL)
    {
      m_isoFiles[i] = new isofile;
      return (HANDLE)i;
    }
  }
  return INVALID_HANDLE_VALUE;
}

//************************************************************************************
iso9660::isofile* iso9660::GetFileContext(HANDLE hFile)
{
  intptr_t iFile = (intptr_t)hFile;
  if (iFile >= 1 && iFile < MAX_ISO_FILES)
  {
    return m_isoFiles[iFile];
  }
  return NULL;
}

//************************************************************************************
bool iso9660::IsScanned()
{
  return (m_hCDROM != NULL);
}

//************************************************************************************
void iso9660::IsoDateTimeToFileTime(iso9660_Datetime* isoDateTime, FILETIME* filetime)
{
  tm t;
  ZeroMemory(&t, sizeof(tm));
  t.tm_year=isoDateTime->year;
  t.tm_mon=isoDateTime->month-1;
  t.tm_mday=isoDateTime->day;
  t.tm_hour=isoDateTime->hour;
  t.tm_min=isoDateTime->minute;
  t.tm_sec=isoDateTime->second + (isoDateTime->gmtoff * (15 * 60));
  t.tm_isdst=-1;
  mktime(&t);

  SYSTEMTIME time;
  time.wYear=t.tm_year+1900;
  time.wMonth=t.tm_mon+1;
  time.wDayOfWeek=t.tm_wday;
  time.wDay=t.tm_mday;
  time.wHour=t.tm_hour;
  time.wMinute=t.tm_min;
  time.wSecond=t.tm_sec;
  time.wMilliseconds=0;
  SystemTimeToFileTime(&time, filetime);
}
