// ISO9660.h: interface for the CISO9660 class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ISO9660_H__E14E96F8_255B_42D3_9BE2_89FFDD6DFB67__INCLUDED_)
#define AFX_ISO9660_H__E14E96F8_255B_42D3_9BE2_89FFDD6DFB67__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <xtl.h>
#include "../xbox/iosupport.h"
#include "IsoDir.h"	// Added by ClassView

namespace XISO9660
{

#pragma pack(1)
struct stVolumeDescriptor
{
	unsigned char byOne;												//0
	char					szSignature[6];								//1-6
	unsigned char byZero;												//7
	char          szSystemIdentifier[32];				//8-39
	char          szVolumeIdentifier[32];				//40-71
	unsigned char byZero2[8];										//72-79
	DWORD				  dwTotalSectorsLE;							//80-83
	DWORD				  dwTotalSectorsBE;							//84-87
	unsigned char byZero3[32];									//88-119
	WORD          wVolumeSetSizeLE;							//120-121
	WORD          wVolumeSetSizeBE;							//122-123
	WORD          wVolumeSequenceNumberLE;			//124-125
	WORD          wVolumeSequenceNumberBE;			//126-127
	WORD					wSectorSizeLE;								//128-129
	WORD					wSectorSizeBE;								//130-131
	DWORD  			  dwPathTableLengthLE;					//132-135
	DWORD  			  dwPathTableLengthBE;					//136-139
	DWORD					wFirstPathTableStartSectorLE;	//140-143
	DWORD					wSecondPathTableStartSectorLE;//144-147
	DWORD					wFirstPathTableStartSectorBE;	//148-151
	DWORD					wSecondPathTableStartSectorBE;//152-155
	unsigned char szRootDir[34];
	unsigned char szVolumeSetIdentifier[128];
	unsigned char szPublisherIdentifier[128];
	unsigned char szDataPreparerIdentifier[128];
	unsigned char szApplicationIdentifier[128];
	unsigned char szCopyRightFileIdentifier[37];
	unsigned char szAbstractFileIdentifier[37];
	unsigned char szBibliographicalFileIdentifier[37];
	unsigned char tDateTimeVolumeCreation[17];
	unsigned char tDateTimeVolumeModification[17];
	unsigned char tDateTimeVolumeExpiration[17];
	unsigned char tDateTimeVolumeEffective[17];
	unsigned char byOne2;
	unsigned char byZero4;
	unsigned char byZero5[512];
	unsigned char byZero6[653];
} ;


struct stDirectory 
{

		#define Flag_NotExist		0x01     /* 1-file not exists */
		#define Flag_Directory	0x02     /* 0-normal file, 1-directory */
		#define Flag_Associated	0x03     /* 0-not associated file */
		#define Flag_Protection	0x04     /* 0-normal acces */
		#define Flag_Multi			0x07     /* 0-final Directory Record for the file */

		BYTE 	ucRecordLength;						//0      the number of bytes in the record (which must be even)
		BYTE	ucExtendAttributeSectors;	//1      [number of sectors in extended attribute record]
		DWORD	dwFileLocationLE;					//2..5   number of the first sector of file data or directory
		DWORD	dwFileLocationBE;					//6..9
		DWORD	dwFileLengthLE;						//10..13 number of bytes of file data or length of directory
		DWORD	dwFileLengthBE;           //14..17
		BYTE	byDateTime[7];						//18..24 date
		BYTE	byFlags;									//25     flags
		BYTE	UnitSize;									//26     file unit size for an interleaved file
		BYTE	InterleaveGapSize;				//27     interleave gap size for an interleaved file
		WORD	VolSequenceLE;						//28..29 volume sequence number
		WORD	VolSequenceBE;            //30..31
		BYTE	Len_Fi;										//32     N, the identifier length
		BYTE	FileName[128];						//33     identifier

};


#pragma pack()

class CISO9660  
{
public:
	CISO9660(CIoSupport& cdrom);
	virtual ~CISO9660();
	
	bool		OpenDisc();
	HANDLE	FindFirstFile(char* lpFileName,  LPWIN32_FIND_DATA lpFindFileData);
	BOOL		FindNextFile(HANDLE hFindFile,  LPWIN32_FIND_DATA lpFindFileData);


	int			OpenFile(const char *strFileName);
	void		CloseFile(int fd);	
	long		ReadFile(int fd, byte *pBuffer, long lSize);	
	INT64		Seek(int fd, INT64 lOffset, int whence);	
	INT64		GetFileSize();
	INT64		GetFilePosition();

protected:
	bool				ReadSectorFromCache(DWORD sector, byte** ppBuffer);
	void				ReleaseSectorFromCache(DWORD sector);
	bool				ReadVolumeDescriptor();
	bool				IsJoliet();
	void        ConvertFilename(char*	pszFile,const char* strFileName, int iFileNameLength);
	void				ReadDirectoryEntries(DWORD dwSector, DWORD dwParent);

	CIoSupport								m_cdrom;
	struct stVolumeDescriptor m_volDescriptor;
	bool											m_bJoliet;
	CIsoDir										m_dirs;

	
	DWORD				m_dwStartBlock;
	DWORD				m_dwCurrentBlock;				// Current being read Block
	INT64				m_dwFilePos;
	BYTE*       m_pBuffer;
	DWORD				m_dwFileSize;

#define CIRC_BUFFER_SIZE 10
	DWORD				m_dwCircBuffBegin;
	DWORD				m_dwCircBuffEnd;
	DWORD				m_dwCircBuffSectorStart;
	bool				m_bUsingMode2;
private:
	HANDLE m_hDevice;
};
};
#endif // !defined(AFX_ISO9660_H__E14E96F8_255B_42D3_9BE2_89FFDD6DFB67__INCLUDED_)
