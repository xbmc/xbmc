// ISO9660.cpp: implementation of the CISO9660 class.
//
//////////////////////////////////////////////////////////////////////
#pragma code_seg( "ISO9660" )
#pragma data_seg( "ISO9660_RW" )
#pragma bss_seg( "ISO9660_RW" )
#pragma const_seg( "ISO9660_RD" )
#pragma comment(linker, "/merge:ISO9660_RW=ISO9660")
#pragma comment(linker, "/merge:ISO9660_RD=ISO9660")

#include "ISO9660.h"
using namespace XISO9660;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define	ISO_BOOT_BLOCK			0x10			/*location on CD*/

#define BUFFER_SIZE MODE2_DATA_SIZE
//************************************************************************************
CISO9660::CISO9660(CIoSupport& cdrom)
:m_cdrom(cdrom)
{
	m_pBuffer=NULL;
	m_bJoliet=false;
	memset(&m_volDescriptor,0,sizeof(m_volDescriptor));
	m_hDevice=m_cdrom.OpenCDROM();
}
//************************************************************************************
CISO9660::~CISO9660()
{
	m_cdrom.CloseCDROM(m_hDevice);

	if (m_pBuffer)
	{
		delete [] m_pBuffer;
		m_pBuffer=NULL;
	}
}
//************************************************************************************
bool CISO9660::OpenDisc()
{
	m_bJoliet=false;
	memset(&m_volDescriptor,0,sizeof(m_volDescriptor));
	if ( !ReadVolumeDescriptor() ) return false;
	
	struct stDirectory* pRootDir= (struct stDirectory*)(&m_volDescriptor.szRootDir[0]);
	
	ReadDirectoryEntries(pRootDir->dwFileLocationLE,1);
//	m_dirs.Dump();
	return true;
}

//************************************************************************************
void CISO9660::ReadDirectoryEntries(DWORD dwSector, DWORD dwParent)
{
	char buffer[2048];
	
	bool bFound=false;
	bool bAdded=false;
	do
	{
		bFound=false;
		if ( m_cdrom.ReadSector(m_hDevice,dwSector ,( char*)buffer) < 0) 	return ;
		int iPos;
		iPos=0;
		do
		{
			stDirectory* pDir = (stDirectory*)&buffer[iPos];
			if (0==pDir->ucRecordLength) break;
			if (pDir->Len_Fi>0)
			{
				if (pDir->Len_Fi==1 && pDir->FileName[0]==0)
				{
					// .
					if ( bAdded) return;
				}
				else if (pDir->Len_Fi==1 && pDir->FileName[0]==1)
				{
					// ..
					if ( bAdded) return;
				}
				else
				{
					if (pDir->byFlags & Flag_Directory)
					{
						if (dwSector != pDir->dwFileLocationLE)
						{
							if (!m_dirs.DirExists(pDir->dwFileLocationLE))
							{
								if ( (pDir->byFlags & Flag_NotExist)==0)
								{
                  char pszDir[2048];
									ConvertFilename(pszDir,(char*)pDir->FileName, pDir->Len_Fi);
									m_dirs.AddDir(pDir->dwFileLocationLE,
																dwParent,
																pszDir);
									ReadDirectoryEntries( pDir->dwFileLocationLE,pDir->dwFileLocationLE);
									bFound=true;
									bAdded=true;
								}
							}
						}
					}
					if ( (pDir->byFlags & (Flag_Directory| Flag_NotExist))==0)
					{
						// normal file
            char pszFile[2048];
						ConvertFilename(pszFile,(char*)pDir->FileName, pDir->Len_Fi-2);
						m_dirs.AddFile( dwParent,
														pszFile,
														pDir->dwFileLengthLE,
														pDir->dwFileLocationLE);
						bFound=true;
						bAdded=true;
					}
				}
			}
			iPos+=pDir->ucRecordLength;
		} while (1);
		dwSector ++;
	} while (bFound);

	return;
}
//************************************************************************************
void CISO9660::ConvertFilename(char* pszFile,const char *strFileName, int iFileNameLength)
{
	char* dst = (char* )&pszFile[0];
	for(int i=m_bJoliet ; i < iFileNameLength; i += m_bJoliet+1 )
	{
		*dst = strFileName[i];		
		if (*dst != ';') dst++;
	}
	*dst = 0;
}

//************************************************************************************
bool CISO9660::ReadVolumeDescriptor()
{
	BYTE i;
	bool bIsIso(false);
	unsigned char buffer[4096];
	int iJolietLevel=0;
	for(i=0; i<5; i++)
	{
		if ( m_cdrom.ReadSector(m_hDevice,ISO_BOOT_BLOCK + i ,( char*)buffer) < 0) 
			return false;
	 
		if (strncmp((char*)(&buffer[1]),"CD001",5)==0)
		{
			if (!bIsIso && buffer[0]==0x1)
			{
				// ISO_VD_PRIMARY
				memcpy(&m_volDescriptor,buffer,sizeof(m_volDescriptor));
				bIsIso=true;
			}
			else if (bIsIso && buffer[0]==0x02)
			{
				// ISO_VD_SUPPLEMENTARY
				// JOLIET extension  
				// test escape sequence for level of UCS-2 characterset
				if (buffer[88] == 0x25 && buffer[89] == 0x2f)
				{
					switch(buffer[90])
					{
						case 0x40: iJolietLevel = 1; break;
						case 0x43: iJolietLevel = 2; break;
						case 0x45: iJolietLevel = 3; break;
					}			
					// Because Joliet-stuff starts at other sector,
					// update root directory record.
					if (iJolietLevel > 0)
					{
						WORD dwDirectoryStartSector;
						memcpy(&dwDirectoryStartSector,&buffer[156], 2);
						

						
						memcpy(&m_volDescriptor,buffer,sizeof(m_volDescriptor));
						m_bJoliet=true;
					}
				}
			}
			else if (buffer[0] == 0xff) // ISO_VD_END
			{
				break;
			}
		}
	}
	return bIsIso;
}
//************************************************************************************
bool CISO9660::IsJoliet()
{
	return m_bJoliet;
}


//************************************************************************************
HANDLE CISO9660::FindFirstFile(char* lpFileName,  LPWIN32_FIND_DATA lpFindFileData)
{
	return m_dirs.FindFirstFile(lpFileName,  lpFindFileData);
}
//************************************************************************************
BOOL CISO9660::FindNextFile(HANDLE hFindFile,  LPWIN32_FIND_DATA lpFindFileData)
{
	return m_dirs.FindNextFile(hFindFile,  lpFindFileData);
}



//************************************************************************************
int CISO9660::OpenFile(const char *strFileName)
{
	char szFileName[1024];
	strcpy(szFileName,strFileName);
	char* pDir=strstr(strFileName,"iso9660:");
	if (pDir)
	{
		pDir+=strlen("iso9660:");
		strcpy(szFileName,pDir);
	}
	// skip any trailing /
	if (szFileName[0]=='/')
	{
		char szTmp[1024];
		strcpy(szTmp,szFileName);
		strcpy(szFileName,&szTmp[1]);
	}
	if (! m_dirs.GetFileInfo(szFileName, m_dwCurrentBlock, m_dwFileSize))
	{
		return -1;
	}
	m_pBuffer    = new byte[CIRC_BUFFER_SIZE*BUFFER_SIZE];
	m_dwStartBlock=m_dwCurrentBlock;
	m_dwFilePos=0;
	m_dwCircBuffBegin = 0;
	m_dwCircBuffEnd = 0;
	m_dwCircBuffSectorStart = 0;
	m_bUsingMode2 = false;

	bool bError;

	bError = (m_cdrom.ReadSector(m_hDevice,m_dwStartBlock, (char*)&(m_pBuffer[0])) <0);
	if( bError )
	{
		bError = (m_cdrom.ReadSectorMode2(m_hDevice,m_dwStartBlock, (char*)&(m_pBuffer[0])) <0);
		if( !bError )
			m_bUsingMode2 = true;		
	}

	if (m_bUsingMode2)
		m_dwFileSize = (m_dwFileSize / 2048) * MODE2_DATA_SIZE;

	return 1;		
}
//************************************************************************************
void CISO9660::CloseFile(int fd)
{
	if (m_pBuffer)
	{
		delete [] m_pBuffer;
		m_pBuffer=NULL;
	}
}
//************************************************************************************
bool CISO9660::ReadSectorFromCache(DWORD sector, byte** ppBuffer)
{
	DWORD StartSectorInCircBuff = m_dwCircBuffSectorStart;
	DWORD SectorsInCircBuff;

	if( m_dwCircBuffEnd >= m_dwCircBuffBegin )
		SectorsInCircBuff = m_dwCircBuffEnd - m_dwCircBuffBegin;
	else
		SectorsInCircBuff = CIRC_BUFFER_SIZE - (m_dwCircBuffBegin - m_dwCircBuffEnd);

	// If our sector is already in the circular buffer
	if( sector >= StartSectorInCircBuff &&
		sector < (StartSectorInCircBuff + SectorsInCircBuff) &&
		SectorsInCircBuff > 0 )
	{
		// Just retrieve it
		DWORD SectorInCircBuff = (sector - StartSectorInCircBuff) +
									m_dwCircBuffBegin;
		if( SectorInCircBuff >= CIRC_BUFFER_SIZE )
			SectorInCircBuff -= CIRC_BUFFER_SIZE;

		*ppBuffer = &(m_pBuffer[SectorInCircBuff]);
	}
	else
	{
		// Sector is not cache.  Read it in.
		bool SectorIsAdjacentInBuffer =
			(StartSectorInCircBuff + SectorsInCircBuff) == sector;
		if( SectorsInCircBuff == CIRC_BUFFER_SIZE - 1 ||
			!SectorIsAdjacentInBuffer)
		{
			// The cache is full. (Or its not an adjacent request in which we'll
			// also flush the cache)

			// If its adjacent, just get rid of the first sector.
			if( SectorIsAdjacentInBuffer )
			{
				// Release the first sector in cache
				m_dwCircBuffBegin++;
				if( m_dwCircBuffBegin >= CIRC_BUFFER_SIZE )
					m_dwCircBuffBegin -= CIRC_BUFFER_SIZE;
				m_dwCircBuffSectorStart++;
				SectorsInCircBuff--;
			}
			else
			{
				m_dwCircBuffBegin = m_dwCircBuffEnd = 0;
				m_dwCircBuffSectorStart = 0;
				SectorsInCircBuff = 0;
				SectorIsAdjacentInBuffer = 0;
			}
		}
		// Ok, we're ready to read the sector into the cache
		bool bError;

		if( m_bUsingMode2 )
		{
			bError = (m_cdrom.ReadSectorMode2(m_hDevice,sector, (char*)&(m_pBuffer[m_dwCircBuffEnd])) <0);
		}
		else
		{
			bError = (m_cdrom.ReadSector(m_hDevice,sector, (char*)&(m_pBuffer[m_dwCircBuffEnd])) <0);
		}
		if( bError )
			return false;
		*ppBuffer = &(m_pBuffer[m_dwCircBuffEnd]);
		if( m_dwCircBuffEnd == m_dwCircBuffBegin )
			m_dwCircBuffSectorStart = sector;
		m_dwCircBuffEnd++;
		if( m_dwCircBuffEnd >= CIRC_BUFFER_SIZE )
			m_dwCircBuffEnd -= CIRC_BUFFER_SIZE;
	}
	return true;
}
//************************************************************************************
void CISO9660::ReleaseSectorFromCache(DWORD sector)
{
	DWORD StartSectorInCircBuff = m_dwCircBuffSectorStart;
	DWORD SectorsInCircBuff;

	if( m_dwCircBuffEnd >= m_dwCircBuffBegin )
		SectorsInCircBuff = m_dwCircBuffEnd - m_dwCircBuffBegin;
	else
		SectorsInCircBuff = CIRC_BUFFER_SIZE - (m_dwCircBuffBegin - m_dwCircBuffEnd);

	// If our sector is in the circular buffer
	if( sector >= StartSectorInCircBuff &&
		sector < (StartSectorInCircBuff + SectorsInCircBuff) &&
		SectorsInCircBuff > 0 )
	{
		DWORD SectorsToFlush = sector - StartSectorInCircBuff + 1;
		m_dwCircBuffBegin += SectorsToFlush;

		m_dwCircBuffSectorStart += SectorsToFlush;
		if( m_dwCircBuffBegin >= CIRC_BUFFER_SIZE )
			m_dwCircBuffBegin -= CIRC_BUFFER_SIZE;
	}
}
//************************************************************************************
long CISO9660::ReadFile(int fd, byte *pBuffer, long lSize)
{
	bool bError;
	long iBytesRead=0;
	DWORD sectorSize = 2048;

	if( m_bUsingMode2 )
		sectorSize = MODE2_DATA_SIZE;
	
	while (lSize > 0)
	{
		m_dwCurrentBlock  = (DWORD) (m_dwFilePos/sectorSize);
		INT64 iOffsetInBuffer= m_dwFilePos - (sectorSize*m_dwCurrentBlock);
		m_dwCurrentBlock += m_dwStartBlock;

		//char szBuf[256];
		//sprintf(szBuf,"pos:%i cblk:%i sblk:%i off:%i",(long)m_dwFilePos, (long)m_dwCurrentBlock,(long)m_dwStartBlock,(long)iOffsetInBuffer);
		//DBG(szBuf);

		byte* pSector;
		bError = !ReadSectorFromCache(m_dwCurrentBlock, &pSector);
		if (!bError)
		{
			DWORD iBytes2Copy =lSize;
			if (iBytes2Copy > (sectorSize-iOffsetInBuffer) )
				iBytes2Copy = (DWORD) (sectorSize-iOffsetInBuffer);


			memcpy( &pBuffer[iBytesRead], &pSector[iOffsetInBuffer], iBytes2Copy);
			iBytesRead += iBytes2Copy;
			lSize      -= iBytes2Copy;
			m_dwFilePos += iBytes2Copy;

			if( iBytes2Copy + iOffsetInBuffer == sectorSize )
				ReleaseSectorFromCache(m_dwCurrentBlock);
			
			// Why is this done?  It is recalculated at the beginning of the loop
			m_dwCurrentBlock += BUFFER_SIZE / MODE2_DATA_SIZE;
			
		}
		else 
		{
			//DBG("EOF");
			break;
		}
	}
	if (iBytesRead ==0) return -1;
	return iBytesRead;
}
//************************************************************************************
INT64 CISO9660::Seek(int fd, INT64 lOffset, int whence)
{
	switch(whence)  
	{
		case SEEK_SET:
			// cur = pos
			m_dwFilePos = lOffset;
			break;
 
		case SEEK_CUR: 
			// cur += pos
			m_dwFilePos += lOffset;
			break;
		case SEEK_END:
			// end -= pos
			m_dwFilePos = m_dwFileSize - lOffset;
			break;
	}

	if (m_dwFilePos < 0)
		return 0;

	if (m_dwFilePos > m_dwFileSize)
		return m_dwFileSize;

	return m_dwFilePos;
}


//************************************************************************************
INT64 CISO9660::GetFileSize()
{
	return m_dwFileSize;
}
//************************************************************************************
INT64 CISO9660::GetFilePosition()
{
	return m_dwFilePos;
}
