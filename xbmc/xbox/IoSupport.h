/*
 * XBoxMediaPlayer
 * Copyright (c) 2002 d7o3g4q and RUNTiME
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// IoSupport.h: interface for the CIoSupport class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IOSUPPORT_H__F084A488_BD6E_49D5_8CD3_0BE62149DB40__INCLUDED_)
#define AFX_IOSUPPORT_H__F084A488_BD6E_49D5_8CD3_0BE62149DB40__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define TRAY_OPEN					16
#define TRAY_CLOSED_NO_MEDIA		64
#define TRAY_CLOSED_MEDIA_PRESENT	96

#define DRIVE_OPEN						0 // Open...
#define DRIVE_NOT_READY					1 // Opening.. Closing... 
#define DRIVE_READY						2  
#define DRIVE_CLOSED_NO_MEDIA			3 // CLOSED...but no media in drive
#define DRIVE_CLOSED_MEDIA_PRESENT		4 // Will be send once when the drive just have closed

#define MODE2_DATA_START			24  	// Mode2 raw sector has 24 bytes before the data payload
#define MODE2_DATA_SIZE				2324	// And has 2324 usable bytes
#define RAW_SECTOR_SIZE				2352	// Raw sector size




// This flag (part of PARTITION_ENTRY.pe_flags) tells you whether/not a 
// partition is being used (whether/not drive G is active, for example)
#define	PE_PARTFLAGS_IN_USE										0x80000000



#define	IOCTL_CMD_LBA48_ACCESS								0xcafebabe
#define	IOCTL_SUBCMD_GET_INFO									0

#define	LBA48_GET_INFO_MAGIC1_IDX							0
#define	LBA48_GET_INFO_MAGIC1_VAL							0xcafebabe
#define	LBA48_GET_INFO_MAGIC2_IDX							1
#define	LBA48_GET_INFO_MAGIC2_VAL							0xbabeface
#define	LBA48_GET_INFO_PATCHCODE_VERSION_IDX	2
#define	LBA48_GET_INFO_LOWCODE_BASE_IDX				3
#define	LBA48_GET_INFO_HIGHCODE_BASE_IDX			4
#define	LBA48_GET_INFO_PATCHSEG_SIZE_IDX			5
#define	LBA48_GET_INFO_PART_TABLE_OFS_IDX			6


#define     MAX_PARTITIONS									 14

typedef struct tagPARTITION_ENTRY 
{
	char					pe_name[16];
	unsigned long	pe_flags;	// bitmask
	unsigned long	pe_lba_start;
	unsigned long	pe_lba_size;
	unsigned long	pe_reserved;
} PARTITION_ENTRY;

typedef struct tagPARTITION_TABLE 
{
	char						pt_magic[16];
	unsigned char		pt_reserved[32];
	PARTITION_ENTRY	pt_entries[MAX_PARTITIONS];
} PARTITION_TABLE;


class CIoSupport  
{
public:

	CIoSupport();
	CIoSupport(CIoSupport& other);
	virtual ~CIoSupport();
  VOID					GetXbePath(char* szDest);

	HRESULT 			Mount(const char* szDrive, char* szDevice);
	HRESULT 			Unmount(const char* szDrive);

	HRESULT 			Remount(const char* szDrive, char* szDevice);
	HRESULT 			Remap(char* szMapping);

	DWORD					GetTrayState();
	HRESULT 			EjectTray();
	HRESULT 			CloseTray();

	string				GetDrive(const string& szPartition);
	VOID					GetPartition(LPCSTR strFilename, LPSTR strPartition);
	VOID					RemountDrive(LPCSTR szDrive);
	VOID					UpdateDvdrom();
	HANDLE				OpenCDROM();
	INT						ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
	INT 					ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
	INT 					ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
	VOID					CloseCDROM(HANDLE hDevice);
	BOOL					IsDebug();
	
  static HRESULT Shutdown();
	static VOID		 SpindownHarddisk();
	bool					IsDrivePresent( const char* cDrive );

protected:
	static VOID			IdexWritePortUchar(USHORT port, UCHAR data);
  static UCHAR    IdexReadPortUchar(USHORT port);
  unsigned int	  read_active_partition_table(PARTITION_TABLE *p_table);
	bool						DriveExists(const char* szDrive);
	bool						PartitionExists(const char* szPartition);
private:
	static PARTITION_TABLE* m_partitionTable;
	HGLOBAL					m_gmXferBuffer;
	PVOID						m_rawXferBuffer;
	DWORD 					m_dwTrayState;
	DWORD 					m_dwTrayCount;
	DWORD 					m_dwLastTrayState;
};

#endif // !defined(AFX_IOSUPPORT_H__F084A488_BD6E_49D5_8CD3_0BE62149DB40__INCLUDED_)
