#include "iso9660.h"
//#define _DEBUG_OUTPUT 1

#define RET_ERR -1



struct iso_dirtree *iso9660::ReadRecursiveDirFromSector( DWORD sector, char *path )
{
	struct iso_dirtree *dir;

	struct iso_dirtree *file_pointer;


	char *Curr_dir_cache;
	DWORD iso9660searchpointer;
	struct iso9660_Directory isodir;
	struct iso9660_Directory curr_dir;
	char work[512];

#ifdef _DEBUG_OUTPUT
	sprintf(work, "******************   Adding dir : %s\r",path);
	OutputDebugString( work );
#endif

	dir = (struct iso_dirtree *)malloc(sizeof(struct iso_dirtree));
	dir->next = 0;
	file_pointer = dir;

	::SetFilePointer( info.ISO_HANDLE, info.iso.wSectorSizeLE * sector,0,FILE_BEGIN );
	DWORD lpNumberOfBytesRead = 0;

	Curr_dir_cache = (char*)malloc( info.iso.wSectorSizeLE );

	::ReadFile( info.ISO_HANDLE, Curr_dir_cache, info.iso.wSectorSizeLE, &lpNumberOfBytesRead, NULL );
	memcpy( &isodir, Curr_dir_cache, sizeof(isodir) );
	memcpy( &curr_dir, Curr_dir_cache, sizeof(isodir) );
	
	if( curr_dir.dwFileLengthLE > info.iso.wSectorSizeLE )
	{
		free( Curr_dir_cache );
		Curr_dir_cache = (char*)malloc( isodir.dwFileLengthLE );
		::SetFilePointer( info.ISO_HANDLE, info.iso.wSectorSizeLE * sector,0,FILE_BEGIN );
		::ReadFile( info.ISO_HANDLE, Curr_dir_cache , curr_dir.dwFileLengthLE, &lpNumberOfBytesRead, NULL );
	}
	iso9660searchpointer = 0;

	if(!lastpath)
	{
		lastpath = paths;
		if( !lastpath )
		{
			paths = (struct iso_directories *)malloc(sizeof(struct iso_directories));
			paths->path = 0;
			paths->dir = 0;
			paths->next = 0;
			lastpath = paths;
		}
		else
		{
			while( lastpath->next )
				lastpath = lastpath->next;
		}
	}
	lastpath->next = ( struct iso_directories *)malloc( sizeof(	struct iso_directories ) );
	lastpath = lastpath->next;
	lastpath->next = 0;
	lastpath->dir = dir;
	lastpath->path = (char *)malloc(strlen(path) + 1);
	strcpy( lastpath->path, path );

	while( 1 )
	{
		if( isodir.ucRecordLength )
			iso9660searchpointer += isodir.ucRecordLength;
		else 
		{
			iso9660searchpointer = (iso9660searchpointer - (iso9660searchpointer % info.iso.wSectorSizeLE)) + info.iso.wSectorSizeLE;
		}
		if( curr_dir.dwFileLengthLE <= iso9660searchpointer )
		{
			break;
		}
		memcpy( &isodir, Curr_dir_cache + iso9660searchpointer, min(sizeof(isodir),sizeof(info.isodir)));
		if( !(isodir.byFlags & Flag_NotExist) )
		{
			if( (!( isodir.byFlags & Flag_Directory )) && ( isodir.Len_Fi ) )
			{
				string temp_text ;
				bool bContinue=false;
				if ( info.joliet &&  !isodir.FileName[0] )
				{
					bContinue=true;
					temp_text = GetThinText((WCHAR*)(isodir.FileName+1), isodir.Len_Fi );
					temp_text.resize(isodir.Len_Fi/2);
				}
				if (!info.joliet && isodir.FileName[0]>=0x20 )
				{
					temp_text=(char*)isodir.FileName;
					bContinue=true;
				}
				if (bContinue)
				{
					int semipos = temp_text.find(";",0);
					if (semipos >= 0)
 						temp_text.erase(semipos,temp_text.length()-semipos);


					file_pointer->next = (struct iso_dirtree *)malloc(sizeof(struct iso_dirtree));
					file_pointer = file_pointer->next;
					file_pointer->next = 0;
					file_pointer->path = (char *)malloc(strlen(path)+1);
					strcpy( file_pointer->path, path );
					file_pointer->name = (char *)malloc( temp_text.length()+1);
					
					strcpy( file_pointer->name , temp_text.c_str());
					if( strstr(file_pointer->name,".mp3" ) || strstr(file_pointer->name,".MP3" ) )
						info.mp3++;

	#ifdef _DEBUG_OUTPUT
					char bufferX[1024];
					sprintf(bufferX,"adding sector : %X, File : %s     size = %u     pos = %x\r",sector,temp_text.c_str(), isodir.dwFileLengthLE, isodir.dwFileLocationLE );
					OutputDebugString( bufferX );
	#endif

					strcpy( work, path );
					if( strlen( path ) > 1 )
						strcat( work,"\\" );

					strcat( work,temp_text.c_str() );

					file_pointer->Location = isodir.dwFileLocationLE;
					file_pointer->dirpointer = 0;
					file_pointer ->Length = isodir.dwFileLengthLE;

					file_pointer->type = 1;
				}
			}
		}
	}
	iso9660searchpointer = 0;
	memcpy( &curr_dir, Curr_dir_cache, sizeof(isodir) );
	memcpy( &isodir, Curr_dir_cache, sizeof(isodir) );
	while( 1 )
	{
		if( isodir.ucRecordLength )
			iso9660searchpointer += isodir.ucRecordLength;
		else 
		{
			iso9660searchpointer = (iso9660searchpointer - (iso9660searchpointer % info.iso.wSectorSizeLE)) + info.iso.wSectorSizeLE;
		}
		if( curr_dir.dwFileLengthLE <= iso9660searchpointer )
		{
			free( Curr_dir_cache );
			return dir;
		}
		memcpy( &isodir, Curr_dir_cache + iso9660searchpointer, min(sizeof(isodir),sizeof(info.isodir)));
		if( !(isodir.byFlags & Flag_NotExist) )
		{
			if( (( isodir.byFlags & Flag_Directory )) && ( isodir.Len_Fi ) )
			{
				string temp_text ;
				bool bContinue=false;
				if ( info.joliet &&  !isodir.FileName[0] )
				{
					bContinue=true;
					temp_text = GetThinText((WCHAR*)(isodir.FileName+1), isodir.Len_Fi );
					temp_text.resize(isodir.Len_Fi/2);
				}
				if (!info.joliet && isodir.FileName[0]>=0x20 )
				{
					temp_text=(char*)isodir.FileName;
					bContinue=true;
				}
				if (bContinue)
				{

					int semipos = temp_text.find(";",0);
					if (semipos >= 0)
 						temp_text.erase(semipos,temp_text.length()-semipos);


					file_pointer->next = (struct iso_dirtree *)malloc(sizeof(struct iso_dirtree));
					file_pointer = file_pointer->next;
					file_pointer->next = 0;
					file_pointer->path = (char *)malloc(strlen(path)+1);
					strcpy( file_pointer->path, path );
					file_pointer->name = (char *)malloc( temp_text.length()+1);
					
					strcpy( file_pointer->name , temp_text.c_str());
					if( strstr(file_pointer->name,".mp3" ) || strstr(file_pointer->name,".MP3" ) )
						info.mp3++;

	#ifdef _DEBUG_OUTPUT
					char bufferX[1024];
					sprintf(bufferX,"adding directory sector : %X, File : %s     size = %u     pos = %x\r",sector,temp_text.c_str(), isodir.dwFileLengthLE, isodir.dwFileLocationLE );
					OutputDebugString( bufferX );
	#endif

					strcpy( work, path );
					if( strlen( path ) > 1 )
						strcat( work,"\\" );

					strcat( work,temp_text.c_str() );

					file_pointer->Location = isodir.dwFileLocationLE;
					file_pointer->dirpointer = 0;
					file_pointer ->Length = isodir.dwFileLengthLE;

					file_pointer->dirpointer = ReadRecursiveDirFromSector( isodir.dwFileLocationLE, work );
					file_pointer->type = 2;
				}
			}
		}
	}
}

iso9660::iso9660( char *filename )
{
	char temp[10];

	alignmentadjust=0;
	Cached_Sector=0;
	Cache=0;

	paths = 0;
	lastpath = 0;
	memset(&info,0,sizeof(info));
	info.Curr_dir_cache = 0;
	info.Curr_dir = (char*)malloc( info.iso.wSectorSizeLE );
	strcpy( info.Curr_dir, "\\" );

	if( strlen( filename ) != 3 )
		info.ISO_HANDLE = CreateFile( filename,FILE_LIST_DIRECTORY,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL );
	else
	{
		memcpy( temp, filename, 2 );
		temp[2] = 0;
		info.ISO_HANDLE = CreateFile( temp,FILE_LIST_DIRECTORY,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL );
	}

	DWORD lpNumberOfBytesRead = 0;
	::SetFilePointer( info.ISO_HANDLE, 0x8000,0,FILE_BEGIN );
	::ReadFile( info.ISO_HANDLE, &info.iso, sizeof(info.iso), &lpNumberOfBytesRead, NULL );
	if(strncmp(info.iso.szSignature,"CD001",5))
	{
		CloseHandle( info.ISO_HANDLE );
		info.iso9660 = 0;
		return;
	}
	else
	{
		info.iso9660 = 1;
		info.joliet = 0;

		info.HeaderPos = 0x8000;
		int current = 0x8000;
                    		
		while( info.iso.byOne != 255 )
		{
			if( ( info.iso.byZero3[0] == 0x25 ) && ( info.iso.byZero3[1] == 0x2f ) )
			{
				switch( info.iso.byZero3[2] )
				{
				case 0x45 :
				case 0x40 :
				case 0x43 : info.HeaderPos = current; 
							info.joliet = 1;
				}
//                        25 2f 45  or   25 2f 40   or 25 2f 43 = jouliet, and best fitted for reading
			}
			current += 0x800;
			::SetFilePointer( info.ISO_HANDLE, current,0,FILE_BEGIN );
			::ReadFile( info.ISO_HANDLE, &info.iso, sizeof(info.iso), &lpNumberOfBytesRead, NULL );
		}
		::SetFilePointer( info.ISO_HANDLE, info.HeaderPos,0,FILE_BEGIN );
		::ReadFile( info.ISO_HANDLE, &info.iso, sizeof(info.iso), &lpNumberOfBytesRead, NULL );
		memcpy( &info.isodir, info.iso.szRootDir, sizeof(info.isodir));
	}
    memcpy( &info.isodir, &info.iso.szRootDir, sizeof(info.isodir) );
	dirtree = ReadRecursiveDirFromSector( info.isodir.dwFileLocationLE, "\\" );
}

  
iso9660::~iso9660(  )
{
	free(info.Curr_dir);
	if(info.Curr_dir_cache)
		free(info.Curr_dir_cache);
	if( Cache )
		free( Cache );
	CloseHandle( info.ISO_HANDLE );
	if( Cache )
		free( Cache );
}

struct iso_dirtree *iso9660::FindFolder( char *Folder )
{
	char *work;
	
	work = (char *)malloc(info.iso.wSectorSizeLE);

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

	lastpath = paths->next;
	while( lastpath )
	{
		if( !strcmp( lastpath->path, work))
			return lastpath->dir;
		lastpath = lastpath->next;
	}
	free ( work );
	return 0;
}

HANDLE iso9660::FindFirstFile( char *szLocalFolder, WIN32_FIND_DATA *wfdFile )
{
	memset( wfdFile, 0, sizeof(WIN32_FIND_DATA));

	searchpointer = FindFolder( szLocalFolder );

	if( searchpointer )
	{
		searchpointer = searchpointer->next;

		if( searchpointer )
		{
			strcpy(wfdFile->cFileName, searchpointer->name );

			if( searchpointer->type == 2 )
				wfdFile->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

			wfdFile->nFileSizeLow = searchpointer->Length;
			return (HANDLE)1;
		}
	}
	return (HANDLE)0;
}


int iso9660::FindNextFile( HANDLE szLocalFolder, WIN32_FIND_DATA *wfdFile )
{
	memset( wfdFile, 0, sizeof(WIN32_FIND_DATA));

	if( searchpointer )
		searchpointer = searchpointer->next;

	if( searchpointer )
	{
		strcpy(wfdFile->cFileName, searchpointer->name );

		if( searchpointer->type == 2 )
			wfdFile->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

		wfdFile->nFileSizeLow = searchpointer->Length;
		return 1;
	}

	return 0;
}


bool iso9660::FindClose( HANDLE szLocalFolder )
{
	searchpointer = 0;
	free(info.Curr_dir_cache);
	info.Curr_dir_cache = 0;
	return true;
}


HANDLE iso9660::OpenFile( char* filename, DWORD location )
{
	WIN32_FIND_DATA fileinfo;
	char *pointer,*pointer2;
	char work[512];

	if( !location )
	{
		info.curr_filepos = 0;

		pointer = filename;
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
		DWORD calc = info.iso.wSectorSizeLE * searchpointer->Location;

		info.curr_filesize = searchpointer->Length;

		memcpy(&openfileinfo, searchpointer, sizeof( openfileinfo ));
		::SetFilePointer( info.ISO_HANDLE, calc ,0,FILE_BEGIN );
	 
		return info.ISO_HANDLE;
	}
	DWORD calc = info.iso.wSectorSizeLE * location;
	::SetFilePointer( info.ISO_HANDLE, calc ,0,FILE_BEGIN );	
	return info.ISO_HANDLE;
}


void iso9660::CloseFile( HANDLE )
{
	if( Cache )
		free( Cache );
}


DWORD iso9660::SetFilePointer(HANDLE hFile,  LONG lDistanceToMove,  PLONG lpDistanceToMoveHigh,  DWORD dwMoveMethod )
{

	// doesn't really care about handle
	int calc;

	switch( dwMoveMethod )
	{
	case( FILE_BEGIN ):
		calc = 0-(info.curr_filepos - lDistanceToMove);   // if we're going to pos 2 from beginning, and current file-read is at 1000 - we're going 998 back.

		alignmentadjust = calc % info.iso.wSectorSizeLE;
		if( alignmentadjust )
		{
			calc -= alignmentadjust;
		}
        ::SetFilePointer( info.ISO_HANDLE, calc,0,FILE_CURRENT);
        info.curr_filepos = lDistanceToMove;
		break;


	case( FILE_CURRENT ):

		calc = info.curr_filepos + lDistanceToMove;
		alignmentadjust = calc % info.iso.wSectorSizeLE;
		calc = lDistanceToMove;
		if( alignmentadjust )
		{
			calc -= alignmentadjust;
		}
		::SetFilePointer( info.ISO_HANDLE, calc,0,FILE_CURRENT);  // pretty straightforward
        info.curr_filepos += lDistanceToMove;
		break;

	case( FILE_END ):
		calc = (openfileinfo.dwFileLengthLE + lDistanceToMove ) -  info.curr_filepos;
		alignmentadjust = calc % info.iso.wSectorSizeLE;
		if( alignmentadjust )
		{
			calc -= alignmentadjust;
		}
        ::SetFilePointer( info.ISO_HANDLE, calc,0,FILE_CURRENT);  // pretty straightforward
        info.curr_filepos = calc;
		break;		
	}
	return info.curr_filepos;
}

DWORD iso9660::GetFileSize(HANDLE hFile,LPDWORD lpFileSizeHigh  )
{
	if( lpFileSizeHigh )
		lpFileSizeHigh = 0;
	return info.curr_filesize;
}


int iso9660::ReadFile( char * pBuffer, int * piSize, DWORD *totalread )
{
	DWORD dwBytesRead=0;
	DWORD totalbytesread = 0;
	DWORD dwBytes2Read = *piSize;
	DWORD bufferpointer=0;

	// if first part of wanted block is unaligned....
	if( alignmentadjust )
	{
		// if cache exists, the desired block is already in memory - no need to load it
		if( !Cache )
		{
            Cache = (char*) malloc( info.iso.wSectorSizeLE );
			for( int t=0; t<5; ++t )
			{
				if(::ReadFile( info.ISO_HANDLE, Cache, info.iso.wSectorSizeLE, &dwBytesRead,NULL ) == ( BOOL )FALSE )
				{
					if( t == 4 )
					{
						*totalread = 0;
						return ( int )RET_ERR;
					}
				}
				else
					t = 5;
			}
		}
		if( ( dwBytes2Read + alignmentadjust ) < info.iso.wSectorSizeLE )  // actually.... next read will be in the block as well
		{
			memcpy( pBuffer, Cache + alignmentadjust, dwBytes2Read);
			alignmentadjust += dwBytes2Read;
			*totalread = dwBytes2Read;
			info.curr_filepos += dwBytes2Read;
			return dwBytes2Read;
		}
		bufferpointer = info.iso.wSectorSizeLE - alignmentadjust;
		memcpy( pBuffer, Cache + alignmentadjust, info.iso.wSectorSizeLE - alignmentadjust);
		free( Cache );
		Cache = 0;
		info.curr_filepos += info.iso.wSectorSizeLE-alignmentadjust;
		dwBytes2Read -= info.iso.wSectorSizeLE-alignmentadjust;
		alignmentadjust = 0;
		totalbytesread += bufferpointer;
	}



	alignmentadjust = dwBytes2Read % info.iso.wSectorSizeLE;
	if( alignmentadjust )
		dwBytes2Read -= alignmentadjust;

	Cache = (char*)malloc(dwBytes2Read);

	for( int t=0; t<5; ++t )
	{
		if(::ReadFile( info.ISO_HANDLE, Cache, dwBytes2Read, &dwBytesRead,NULL ) == ( BOOL )FALSE )
		{
			int error = GetLastError();
			if( t == 4 )
			{
				*totalread = 0;
				return ( int )RET_ERR;
			}
		}
		else
		{
			memcpy( pBuffer + bufferpointer, Cache,  dwBytesRead );
			free( Cache );
			Cache = 0;
			bufferpointer += dwBytesRead;
			DWORD dwBytesToRead=min((DWORD)info.curr_filesize - info.curr_filepos, (DWORD)*piSize );
			info.curr_filepos += dwBytesToRead;
			t = 5;
			*totalread = dwBytesToRead;
			totalbytesread += dwBytesRead;
		}
	}

	// if end of block is unaligned...
	if( alignmentadjust )
	{
		Cache = (char *)malloc( info.iso.wSectorSizeLE );
		for( int t=0; t<5; ++t )
		{
			if(::ReadFile( info.ISO_HANDLE,Cache , info.iso.wSectorSizeLE, &dwBytesRead,NULL ) == ( BOOL )FALSE )
			{
				if( t == 4 )
				{
					*totalread = 0;
					return ( int )RET_ERR;
				}
			}
			else
				t = 5;
		}
		memcpy( pBuffer + bufferpointer, Cache,  alignmentadjust );
		bufferpointer += alignmentadjust;
		info.curr_filepos += dwBytesRead;
		totalbytesread += dwBytesRead;
	}
	return bufferpointer;
}




string iso9660::GetThinText(WCHAR* strTxt, int iLen )
{
	m_strReturn="";
	for (int i=0; i < iLen; i++)
	{
		m_strReturn += (char)strTxt[i];
	}
	return m_strReturn ;
}