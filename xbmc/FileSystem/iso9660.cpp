/*
	Redbook			: CDDA	
	Yellowbook	: CDROM
ISO9660
 CD-ROM Mode 1 divides the 2352 byte data area into:
		-12		bytes of synchronisation 
		-4		bytes of header information 
		-2048 bytes of user information 
		-288	bytes of error correction and detection codes. 

 CD-ROM Mode 2 redefines the use of the 2352 byte data area as follows: 
		-12 bytes of synchronisation 
		-4 bytes of header information 
		-2336 bytes of user data. 



*/
#include "iso9660.h"
//#define _DEBUG_OUTPUT 1
int iso9660::m_iReferences=0;
HANDLE iso9660::m_hCDROM=NULL;
static CRITICAL_SECTION m_critSection;

#define RET_ERR -1
//******************************************************************************************************************
struct iso_dirtree *iso9660::ReadRecursiveDirFromSector( DWORD sector, const char *path )
{
	struct iso_dirtree* 			pDir=NULL;
	struct iso_dirtree* 			pFile_Pointer=NULL;
	char*											pCurr_dir_cache=NULL;
	DWORD											iso9660searchpointer;
	struct	iso9660_Directory isodir;
	struct	iso9660_Directory curr_dir;
	char		work[1024];

#ifdef _DEBUG_OUTPUT
	sprintf(work, "******************   Adding dir : %s\r",path);
	OutputDebugString( work );
#endif

	pDir = (struct iso_dirtree *)malloc(sizeof(struct iso_dirtree));
	pDir->next = NULL;
	pDir->path = NULL;
	pDir->name = NULL;
	pDir->dirpointer=NULL;
	pFile_Pointer = pDir;
	m_vecDirsAndFiles.push_back(pDir);


	::SetFilePointer( m_info.ISO_HANDLE, m_info.iso.wSectorSizeLE * sector,0,FILE_BEGIN );
	DWORD lpNumberOfBytesRead = 0;

	pCurr_dir_cache = (char*)malloc( m_info.iso.wSectorSizeLE );

	::ReadFile( m_info.ISO_HANDLE, pCurr_dir_cache, m_info.iso.wSectorSizeLE, &lpNumberOfBytesRead, NULL );
	memcpy( &isodir, pCurr_dir_cache, sizeof(isodir) );
	memcpy( &curr_dir, pCurr_dir_cache, sizeof(isodir) );
	
	if( curr_dir.dwFileLengthLE > m_info.iso.wSectorSizeLE )
	{
		free( pCurr_dir_cache );
		pCurr_dir_cache = (char*)malloc( isodir.dwFileLengthLE );
		::SetFilePointer( m_info.ISO_HANDLE, m_info.iso.wSectorSizeLE * sector,0,FILE_BEGIN );
		::ReadFile( m_info.ISO_HANDLE, pCurr_dir_cache , curr_dir.dwFileLengthLE, &lpNumberOfBytesRead, NULL );
	}
	iso9660searchpointer = 0;

	if(!m_lastpath)
	{
		m_lastpath = m_paths;
		if( !m_lastpath )
		{
			m_paths = (struct iso_directories *)malloc(sizeof(struct iso_directories));
			m_paths->path = NULL;
			m_paths->dir  = NULL;
			m_paths->next = NULL;
			m_lastpath = m_paths;
		}
		else
		{
			while( m_lastpath->next )
				m_lastpath = m_lastpath->next;
		}
	}
	m_lastpath->next = ( struct iso_directories *)malloc( sizeof(	struct iso_directories ) );
	m_lastpath				= m_lastpath->next;
	m_lastpath->next  = NULL;
	m_lastpath->dir   = pDir;
	m_lastpath->path  = (char *)malloc(strlen(path) + 1);
	strcpy( m_lastpath->path, path );

	while( 1 )
	{
		if( isodir.ucRecordLength )
			iso9660searchpointer += isodir.ucRecordLength;
		else 
		{
			iso9660searchpointer = (iso9660searchpointer - (iso9660searchpointer % m_info.iso.wSectorSizeLE)) + m_info.iso.wSectorSizeLE;
		}
		if( curr_dir.dwFileLengthLE <= iso9660searchpointer )
		{
			break;
		}
		memcpy( &isodir, pCurr_dir_cache + iso9660searchpointer, min(sizeof(isodir),sizeof(m_info.isodir)));
		if( !(isodir.byFlags & Flag_NotExist) )
		{
			if( (!( isodir.byFlags & Flag_Directory )) && ( isodir.Len_Fi ) )
			{
				string temp_text ;
				bool bContinue=false;
				if ( m_info.joliet &&  !isodir.FileName[0] )
				{
					bContinue=true;
					temp_text = GetThinText((WCHAR*)(isodir.FileName+1), isodir.Len_Fi );
					temp_text.resize(isodir.Len_Fi/2);
				}
				if (!m_info.joliet && isodir.FileName[0]>=0x20 )
				{
					temp_text=(char*)isodir.FileName;
					bContinue=true;
				}
				if (bContinue)
				{
					int semipos = temp_text.find(";",0);
					if (semipos >= 0)
 						temp_text.erase(semipos,temp_text.length()-semipos);


					pFile_Pointer->next = (struct iso_dirtree *)malloc(sizeof(struct iso_dirtree));
					m_vecDirsAndFiles.push_back(pFile_Pointer->next);
					pFile_Pointer = pFile_Pointer->next;
					pFile_Pointer->next = 0;
					pFile_Pointer->dirpointer=NULL;
					pFile_Pointer->path = (char *)malloc(strlen(path)+1);
					strcpy( pFile_Pointer->path, path );
					pFile_Pointer->name = (char *)malloc( temp_text.length()+1);
					
					strcpy( pFile_Pointer->name , temp_text.c_str());
					if( strstr(pFile_Pointer->name,".mp3" ) || strstr(pFile_Pointer->name,".MP3" ) )
						m_info.mp3++;

	#ifdef _DEBUG_OUTPUT
					char bufferX[1024];
					sprintf(bufferX,"adding sector : %X, File : %s     size = %u     pos = %x\r",sector,temp_text.c_str(), isodir.dwFileLengthLE, isodir.dwFileLocationLE );
					OutputDebugString( bufferX );
	#endif

					strcpy( work, path );
					if( strlen( path ) > 1 )
						strcat( work,"\\" );

					strcat( work,temp_text.c_str() );

					pFile_Pointer->Location = isodir.dwFileLocationLE;
					pFile_Pointer->dirpointer = NULL;
					pFile_Pointer ->Length = isodir.dwFileLengthLE;

					pFile_Pointer->type = 1;
				}
			}
		}
	}
	iso9660searchpointer = 0;
	memcpy( &curr_dir, pCurr_dir_cache, sizeof(isodir) );
	memcpy( &isodir, pCurr_dir_cache, sizeof(isodir) );
	while( 1 )
	{
		if( isodir.ucRecordLength )
			iso9660searchpointer += isodir.ucRecordLength;
		else 
		{
			iso9660searchpointer = (iso9660searchpointer - (iso9660searchpointer % m_info.iso.wSectorSizeLE)) + m_info.iso.wSectorSizeLE;
		}
		if( curr_dir.dwFileLengthLE <= iso9660searchpointer )
		{
			free( pCurr_dir_cache );
			return pDir;
		}
		memcpy( &isodir, pCurr_dir_cache + iso9660searchpointer, min(sizeof(isodir),sizeof(m_info.isodir)));
		if( !(isodir.byFlags & Flag_NotExist) )
		{
			if( (( isodir.byFlags & Flag_Directory )) && ( isodir.Len_Fi ) )
			{
				string temp_text ;
				bool bContinue=false;
				if ( m_info.joliet &&  !isodir.FileName[0] )
				{
					bContinue=true;
					temp_text = GetThinText((WCHAR*)(isodir.FileName+1), isodir.Len_Fi );
					temp_text.resize(isodir.Len_Fi/2);
				}
				if (!m_info.joliet && isodir.FileName[0]>=0x20 )
				{
					temp_text=(char*)isodir.FileName;
					bContinue=true;
				}
				if (bContinue)
				{

					int semipos = temp_text.find(";",0);
					if (semipos >= 0)
 						temp_text.erase(semipos,temp_text.length()-semipos);


					pFile_Pointer->next = (struct iso_dirtree *)malloc(sizeof(struct iso_dirtree));
					m_vecDirsAndFiles.push_back(pFile_Pointer->next);
					pFile_Pointer = pFile_Pointer->next;
					pFile_Pointer->next = 0;
					pFile_Pointer->dirpointer=NULL;
					pFile_Pointer->path = (char *)malloc(strlen(path)+1);
					strcpy( pFile_Pointer->path, path );
					pFile_Pointer->name = (char *)malloc( temp_text.length()+1);
					
					strcpy( pFile_Pointer->name , temp_text.c_str());
					if( strstr(pFile_Pointer->name,".mp3" ) || strstr(pFile_Pointer->name,".MP3" ) )
						m_info.mp3++;

	#ifdef _DEBUG_OUTPUT
					char bufferX[1024];
					sprintf(bufferX,"adding directory sector : %X, File : %s     size = %u     pos = %x\r",sector,temp_text.c_str(), isodir.dwFileLengthLE, isodir.dwFileLocationLE );
					OutputDebugString( bufferX );
	#endif

					strcpy( work, path );
					if( strlen( path ) > 1 )
						strcat( work,"\\" );

					strcat( work,temp_text.c_str() );

					pFile_Pointer->Location = isodir.dwFileLocationLE;
					pFile_Pointer->dirpointer = NULL;
					pFile_Pointer ->Length = isodir.dwFileLengthLE;

					pFile_Pointer->dirpointer = ReadRecursiveDirFromSector( isodir.dwFileLocationLE, work );
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
	m_bUseMode2=false;
	if (!m_iReferences)
	{
		m_hCDROM = m_IoSupport.OpenCDROM();
		InitializeCriticalSection(&m_critSection);
	}

	m_iReferences++;
	
	m_pCache           = new char[32768];
	m_paths = 0;
	m_lastpath = 0;
	memset(&m_info,0,sizeof(m_info));
	m_info.ISO_HANDLE = m_hCDROM ;
	m_info.Curr_dir_cache = 0;
	m_info.Curr_dir = (char*)malloc( 4096 );
	strcpy( m_info.Curr_dir, "\\" );


	EnterCriticalSection(&m_critSection);
	DWORD lpNumberOfBytesRead = 0;
	::SetFilePointer( m_info.ISO_HANDLE, 0x8000,0,FILE_BEGIN );
	::ReadFile( m_info.ISO_HANDLE, &m_info.iso, sizeof(m_info.iso), &lpNumberOfBytesRead, NULL );
	
	if(strncmp(m_info.iso.szSignature,"CD001",5))
	{
		m_IoSupport.CloseCDROM( m_info.ISO_HANDLE);
		m_info.ISO_HANDLE=NULL;
		m_hCDROM=NULL;
		m_info.iso9660 = 0;
		LeaveCriticalSection(&m_critSection);
		return;
	}
	else
	{
		m_info.iso9660 = 1;
		m_info.joliet = 0;

		m_info.HeaderPos = 0x8000;
		int current = 0x8000;
                    		
		while( m_info.iso.byOne != 255 )
		{
			if( ( m_info.iso.byZero3[0] == 0x25 ) && ( m_info.iso.byZero3[1] == 0x2f ) )
			{
				switch( m_info.iso.byZero3[2] )
				{
				case 0x45 :
				case 0x40 :
				case 0x43 : m_info.HeaderPos = current; 
							m_info.joliet = 1;
				}
//                        25 2f 45  or   25 2f 40   or 25 2f 43 = jouliet, and best fitted for reading
			}
			current += 0x800;
			::SetFilePointer( m_info.ISO_HANDLE, current,0,FILE_BEGIN );
			::ReadFile( m_info.ISO_HANDLE, &m_info.iso, sizeof(m_info.iso), &lpNumberOfBytesRead, NULL );
		}
		::SetFilePointer( m_info.ISO_HANDLE, m_info.HeaderPos,0,FILE_BEGIN );
		::ReadFile( m_info.ISO_HANDLE, &m_info.iso, sizeof(m_info.iso), &lpNumberOfBytesRead, NULL );
		memcpy( &m_info.isodir, m_info.iso.szRootDir, sizeof(m_info.isodir));
	}
    memcpy( &m_info.isodir, &m_info.iso.szRootDir, sizeof(m_info.isodir) );
	m_dirtree = ReadRecursiveDirFromSector( m_info.isodir.dwFileLocationLE, "\\" );
	LeaveCriticalSection(&m_critSection);
}

//******************************************************************************************************************  
iso9660::~iso9660(  )
{
	if (m_info.Curr_dir)
		free(m_info.Curr_dir);
	m_info.Curr_dir=NULL;

	if(m_info.Curr_dir_cache)
		free(m_info.Curr_dir_cache);
	m_info.Curr_dir_cache=NULL;

	if( m_pCache )
		delete [] m_pCache;
	m_pCache = NULL;

	
	struct iso_directories* nextpath;

	while( m_paths )
	{
		nextpath = m_paths->next;
		if (m_paths->path) free(m_paths->path);
		
		free (m_paths);
    m_paths = nextpath;
	}
	for (int i=0; i < (int)m_vecDirsAndFiles.size(); ++i)
	{
		struct iso_dirtree* pDir=m_vecDirsAndFiles[i];
		if (pDir->path) free(pDir->path);
		if (pDir->name) free(pDir->name);
		free(pDir);
	}

	m_iReferences--;
	if (!m_iReferences)
	{
		DeleteCriticalSection(&m_critSection);
		if (m_hCDROM)
		{
			m_IoSupport.CloseCDROM(m_hCDROM);
		}
		m_hCDROM=NULL;
	}
}
//******************************************************************************************************************
struct iso_dirtree *iso9660::FindFolder( char *Folder )
{
	char *work;
	
	work = (char *)malloc(m_info.iso.wSectorSizeLE);

	char *temp;
	struct iso_directories *lastpath;

	if( strpbrk(Folder,":") )
		strcpy(work, strpbrk(Folder,":")+1);
	else
		strcpy(work, Folder);

	temp = work+1;
	while( strpbrk( temp+1, "\\" ) )
		temp = strpbrk( temp+1, "\\" );
	
	if( work[ strlen(work)-1 ] == '*' ) 
	{
		work[ strlen(work)-1 ] = 0;
	}
	if( strlen( work ) > 2 )
		if( work[ strlen(work)-1 ] == '\\' ) 
			work[ strlen(work)-1 ] = 0;	

	lastpath = m_paths->next;
	while( lastpath )
	{
		if( !strcmp( lastpath->path, work))
			return lastpath->dir;
		lastpath = lastpath->next;
	}
	free ( work );
	return 0;
}

//******************************************************************************************************************
HANDLE iso9660::FindFirstFile( char *szLocalFolder, WIN32_FIND_DATA *wfdFile )
{
	if (m_info.ISO_HANDLE==0) return (HANDLE)0;
	memset( wfdFile, 0, sizeof(WIN32_FIND_DATA));

	m_searchpointer = FindFolder( szLocalFolder );

	if( m_searchpointer )
	{
		m_searchpointer = m_searchpointer->next;

		if( m_searchpointer )
		{
			strcpy(wfdFile->cFileName, m_searchpointer->name );

			if( m_searchpointer->type == 2 )
				wfdFile->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

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

	if( m_searchpointer )
		m_searchpointer = m_searchpointer->next;

	if( m_searchpointer )
	{
		strcpy(wfdFile->cFileName, m_searchpointer->name );

		if( m_searchpointer->type == 2 )
			wfdFile->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

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
HANDLE iso9660::OpenFile( const char* filename)
{
	if (m_info.ISO_HANDLE==NULL) return INVALID_HANDLE_VALUE;

	
	WIN32_FIND_DATA fileinfo;
	char *pointer,*pointer2;
	char work[512];
	m_bUseMode2=false;
	m_info.curr_filepos = 0;

	pointer = (char*)filename;
	while( strpbrk( pointer,"\\/" ) )
		pointer = strpbrk( pointer,"\\/" )+1;

	strcpy(work, filename );
	pointer2 = work;

	while( strpbrk(pointer2+1,"\\" ) )
		pointer2 = strpbrk(pointer2+1,"\\" );
		
	*(pointer2+1) = 0;

	int loop = (int)FindFirstFile( work, &fileinfo );
	while( loop > 0)	 
	{ 
		if( !strcmp(fileinfo.cFileName, pointer ) )
			loop = -1;
		else
			loop = FindNextFile( NULL, &fileinfo );
	}
	if( loop == 0 )
		return INVALID_HANDLE_VALUE;

//	DWORD calc = isodir.dwFileLocationLE * 0x800;
	//DWORD calc = iso.wSectorSizeLE * isodir.dwFileLocationLE;
	DWORD calc = m_info.iso.wSectorSizeLE * m_searchpointer->Location;
	
	m_info.curr_filesize = fileinfo.nFileSizeLow;
	m_dwStartSector = m_searchpointer->Location;

	EnterCriticalSection(&m_critSection);
	memcpy(&m_openfileinfo, m_searchpointer, sizeof( m_openfileinfo ));
	DWORD dwPos = ::SetFilePointer( m_info.ISO_HANDLE, calc ,0,FILE_BEGIN );
	
	DWORD dwBytesRead;
	int iRead=::ReadFile( m_info.ISO_HANDLE, m_pCache, m_info.iso.wSectorSizeLE, &dwBytesRead,NULL );
	if( iRead<=0 )
	{
		printf("using mode2 %i\n", m_info.curr_filesize);
		m_bUseMode2 = true;		
		m_info.iso.wSectorSizeLE=MODE2_DATA_SIZE;
		m_info.curr_filesize = (m_info.curr_filesize / 2048) * MODE2_DATA_SIZE;
	}

	LeaveCriticalSection(&m_critSection);
	return m_info.ISO_HANDLE;
}

//******************************************************************************************************************
void iso9660::CloseFile( HANDLE )
{
	if( m_pCache )
		delete [] m_pCache ;
	m_pCache=NULL;
}

//******************************************************************************************************************
DWORD iso9660::SetFilePointer(HANDLE hFile,  LONG lDistanceToMove,  PLONG lpDistanceToMoveHigh,  DWORD dwMoveMethod )
{
	switch( dwMoveMethod )
	{
		case( FILE_BEGIN ):
      m_info.curr_filepos = lDistanceToMove;
		break;


	case( FILE_CURRENT ):
    m_info.curr_filepos += lDistanceToMove;
		break;

	case( FILE_END ):
    m_info.curr_filepos = m_info.curr_filesize-lDistanceToMove;
		break;		
	}
	return m_info.curr_filepos;
}

//******************************************************************************************************************
DWORD iso9660::GetFileSize(HANDLE hFile,LPDWORD lpFileSizeHigh  )
{
	if( lpFileSizeHigh )
		lpFileSizeHigh = 0;
	return m_info.curr_filesize;
}

//******************************************************************************************************************
string iso9660::GetThinText(WCHAR* strTxt, int iLen )
{
	m_strReturn="";
	for (int i=0; i < iLen; i++)
	{
		m_strReturn += (char)(strTxt[i]&0xff);
	}
	return m_strReturn ;
}
//******************************************************************************************************************
DWORD iso9660::GetFilePosition(HANDLE hFile)
{
	return m_info.curr_filepos;
}

//******************************************************************************************************************
int iso9660::ReadFile( char * pBuffer, int * piSize, DWORD *totalread )
{
	DWORD dwPos=0;
	DWORD dwBytesRead=0;
	DWORD dwBytes2Read = *piSize;
	DWORD dwBytesLeft = m_info.curr_filesize;
	*totalread = 0;
	if (m_info.ISO_HANDLE==NULL) return -1;
	if (m_info.curr_filepos>=m_info.curr_filesize) 
	{
		return 0; // EOF
	}
	DWORD dwBytes=m_info.curr_filepos+dwBytes2Read;
	if (dwBytes >=m_info.curr_filesize) 
	{
		dwBytes2Read = (m_info.curr_filesize-m_info.curr_filepos);
	}

	while (dwBytes2Read> 0)
	{
		DWORD dwCurrentSector = (m_info.curr_filepos /  m_info.iso.wSectorSizeLE) + m_dwStartSector;
		int iAlignmentadjust = m_info.curr_filepos %  m_info.iso.wSectorSizeLE;
		if (iAlignmentadjust > 0)
		{
			int x=1;
		}
		for( int t=0; t<5; ++t )
		{
			int iResult;
			EnterCriticalSection(&m_critSection);
			if (m_bUseMode2)
			{
				iResult=m_IoSupport.ReadSectorMode2( m_info.ISO_HANDLE, dwCurrentSector, m_pCache);
				if ( iResult>0) dwBytesRead = iResult;
			}
			else
			{
				::SetFilePointer( m_info.ISO_HANDLE, dwCurrentSector*m_info.iso.wSectorSizeLE, 0,FILE_BEGIN);
				iResult=::ReadFile( m_info.ISO_HANDLE, m_pCache, m_info.iso.wSectorSizeLE, &dwBytesRead,NULL );
			}
			LeaveCriticalSection(&m_critSection);
			if (iResult <= 0)
			{
				if( t == 4 )
				{
					*totalread = 0;
					return ( int )RET_ERR;
				}
			}
			else
			{
				t = 5;
				dwBytesRead -= iAlignmentadjust;
				DWORD dwBytesToCopy=dwBytesRead;
				if (dwBytesToCopy > dwBytes2Read) dwBytesToCopy=dwBytes2Read;

				memcpy(&pBuffer[dwPos], &m_pCache[iAlignmentadjust], dwBytesToCopy);
				dwPos += dwBytesToCopy;
				*totalread = (*totalread) + dwBytesToCopy;
				dwBytes2Read-= dwBytesToCopy;
				m_info.curr_filepos +=dwBytesToCopy;
			}
		}
	}

	return *totalread;
}