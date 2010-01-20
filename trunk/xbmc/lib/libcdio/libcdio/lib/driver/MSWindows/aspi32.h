/* Win32 aspi specific */
/*
    $Id: aspi32.h,v 1.5 2006/03/17 03:10:53 rocky Exp $

    Copyright (C) 2003, 2004, 2005 Rocky Bernstein <rocky@panix.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#define ASPI_HAID           0
#define ASPI_TARGET         0
#define DTYPE_CDROM         0x05

#define SENSE_LEN           0x0E
#define SC_HA_INQUIRY       0x00
#define SC_GET_DEV_TYPE     0x01
#define SC_EXEC_SCSI_CMD    0x02
#define SC_GET_DISK_INFO    0x06

//*****************************************************************************
//      %%% SRB Status %%%
//*****************************************************************************

#define SS_PENDING                0x00  // SRB being processed
#define SS_COMP                   0x01  // SRB completed without error
#define SS_ABORTED                0x02  // SRB aborted
#define SS_ABORT_FAIL             0x03  // Unable to abort SRB
#define SS_ERR                    0x04  // SRB completed with error

#define SS_INVALID_CMD            0x80  // Invalid ASPI command
#define SS_INVALID_HA             0x81  // Invalid host adapter number
#define SS_NO_DEVICE              0x82  // SCSI device not installed

#define SS_INVALID_SRB            0xE0  // Invalid parameter set in SRB
#define SS_OLD_MANAGER            0xE1  // ASPI manager doesn't support Windows
#define SS_BUFFER_ALIGN           0xE1  // Buffer not aligned (replaces 
                                        // OLD_MANAGER in Win32)
#define SS_ILLEGAL_MODE           0xE2  // Unsupported Windows mode
#define SS_NO_ASPI                0xE3  // No ASPI managers resident
#define SS_FAILED_INIT            0xE4  // ASPI for windows failed init
#define SS_ASPI_IS_BUSY           0xE5  // No resources available to execute 
                                        // cmd
#define SS_BUFFER_TOO_BIG         0xE6  // Buffer size to big to handle!
#define SS_MISMATCHED_COMPONENTS  0xE7  // The DLLs/EXEs of ASPI don't version 
                                        // check
#define SS_NO_ADAPTERS            0xE8  // No host adapters to manage
#define SS_INSUFFICIENT_RESOURCES 0xE9  // Couldn't allocate resources needed 
                                        // to init
#define SS_ASPI_IS_SHUTDOWN       0xEA  // Call came to ASPI after 
                                        // PROCESS_DETACH
#define SS_BAD_INSTALL            0xEB  // The DLL or other components are installed wrong

//*****************************************************************************
//      %%% Host Adapter Status %%%
//*****************************************************************************

#define HASTAT_OK                 0x00    // Host adapter did not detect an
                                          // error
#define HASTAT_SEL_TO             0x11    // Selection Timeout
#define HASTAT_DO_DU              0x12    // Data overrun data underrun
#define HASTAT_BUS_FREE           0x13    // Unexpected bus free
#define HASTAT_PHASE_ERR          0x14    // Target bus phase sequence
                                          // failure
#define HASTAT_TIMEOUT            0x09    // Timed out while SRB was
                                          // waiting to beprocessed.
#define HASTAT_COMMAND_TIMEOUT    0x0B    // Adapter timed out processing SRB.
#define HASTAT_MESSAGE_REJECT     0x0D    // While processing SRB, the
                                          // adapter received a MESSAGE
#define HASTAT_BUS_RESET            0x0E  // A bus reset was detected.
#define HASTAT_PARITY_ERROR         0x0F  // A parity error was detected.
#define HASTAT_REQUEST_SENSE_FAILED 0x10  // The adapter failed in issuing
#define SS_NO_ADAPTERS      0xE8
#define SRB_DIR_IN          0x08
#define SRB_DIR_OUT         0x10
#define SRB_EVENT_NOTIFY    0x40

#define SECTOR_TYPE_MODE2 0x14
#define READ_CD_USERDATA_MODE2 0x10

#define READ_TOC 0x43
#define READ_TOC_FORMAT_TOC 0x0

#pragma pack(1)

struct SRB_GetDiskInfo
{
    unsigned char   SRB_Cmd;
    unsigned char   SRB_Status;
    unsigned char   SRB_HaId;
    unsigned char   SRB_Flags;
    unsigned long   SRB_Hdr_Rsvd;
    unsigned char   SRB_Target;
    unsigned char   SRB_Lun;
    unsigned char   SRB_DriveFlags;
    unsigned char   SRB_Int13HDriveInfo;
    unsigned char   SRB_Heads;
    unsigned char   SRB_Sectors;
    unsigned char   SRB_Rsvd1[22];
};

struct SRB_GDEVBlock
{
    unsigned char SRB_Cmd;
    unsigned char SRB_Status;
    unsigned char SRB_HaId;
    unsigned char SRB_Flags;
    unsigned long SRB_Hdr_Rsvd;
    unsigned char SRB_Target;
    unsigned char SRB_Lun;
    unsigned char SRB_DeviceType;
    unsigned char SRB_Rsvd1;
};

struct SRB_ExecSCSICmd
{
    unsigned char   SRB_Cmd;
    unsigned char   SRB_Status;
    unsigned char   SRB_HaId;
    unsigned char   SRB_Flags;
    unsigned long   SRB_Hdr_Rsvd;
    unsigned char   SRB_Target;
    unsigned char   SRB_Lun;
    unsigned short  SRB_Rsvd1;
    unsigned long   SRB_BufLen;
    unsigned char   *SRB_BufPointer;
    unsigned char   SRB_SenseLen;
    unsigned char   SRB_CDBLen;
    unsigned char   SRB_HaStat;
    unsigned char   SRB_TargStat;
    unsigned long   *SRB_PostProc;
    unsigned char   SRB_Rsvd2[20];
    unsigned char   CDBByte[16];
    unsigned char   SenseArea[SENSE_LEN+2];
};

/*****************************************************************************
          %%% SRB - HOST ADAPTER INQUIRY - SC_HA_INQUIRY (0) %%%
*****************************************************************************/

typedef struct                     // Offset
{                                  // HX/DEC
    BYTE        SRB_Cmd;           // 00/000 ASPI command code = SC_HA_INQUIRY
    BYTE        SRB_Status;        // 01/001 ASPI command status byte
    BYTE        SRB_HaId;          // 02/002 ASPI host adapter number
    BYTE        SRB_Flags;         // 03/003 ASPI request flags
    DWORD       SRB_Hdr_Rsvd;      // 04/004 Reserved, MUST = 0
    BYTE        HA_Count;          // 08/008 Number of host adapters present
    BYTE        HA_SCSI_ID;        // 09/009 SCSI ID of host adapter
    BYTE        HA_ManagerId[16];  // 0A/010 String describing the manager
    BYTE        HA_Identifier[16]; // 1A/026 String describing the host adapter
    BYTE        HA_Unique[16];     // 2A/042 Host Adapter Unique parameters
    WORD        HA_Rsvd1;          // 3A/058 Reserved, MUST = 0
}
SRB_HAInquiry;

/*! 
  Get disc type associated with cd object.
*/
discmode_t get_discmode_aspi (_img_private_t *p_env);

/*!
  Return the the kind of drive capabilities of device.

  Note: string is malloc'd so caller should free() then returned
  string when done with it.

 */
char * get_mcn_aspi (const _img_private_t *env);

/*!  
  Get the format (XA, DATA, AUDIO) of a track. 
*/
track_format_t get_track_format_aspi(const _img_private_t *env, 
				     track_t i_track); 

/*!
  Initialize internal structures for CD device.
 */
bool init_aspi (_img_private_t *env);

/*
  Read cdtext information for a CdIo object .
  
  return true on success, false on error or CD-TEXT information does
  not exist.
*/
bool init_cdtext_aspi (_img_private_t *env);

const char *is_cdrom_aspi(const char drive_letter);

/*!
   Reads an audio device using the DeviceIoControl method into data
   starting from lsn.  Returns 0 if no error.
 */
int read_audio_sectors_aspi (_img_private_t *obj, void *data, lsn_t lsn, 
			     unsigned int nblocks);
/*!
   Reads a single mode1 sector using the DeviceIoControl method into
   data starting from lsn. Returns 0 if no error.
 */
int read_mode1_sector_aspi (_img_private_t *env, void *data, 
			    lsn_t lsn, bool b_form2);
/*!
   Reads a single mode2 sector from cd device into data starting
   from lsn. Returns 0 if no error. 
 */
int read_mode2_sector_aspi (_img_private_t *env, void *data, lsn_t lsn, 
			    bool b_form2);

/*! 
  Read and cache the CD's Track Table of Contents and track info.
  Return true if successful or false if an error.
*/
bool read_toc_aspi (_img_private_t *env);

/*!
  Run a SCSI MMC command. 
 
  env	        private CD structure 
  i_timeout     time in milliseconds we will wait for the command
                to complete. If this value is -1, use the default 
		time-out value.
  p_buf	        Buffer for data, both sending and receiving
  i_buf	        Size of buffer
  e_direction	direction the transfer is to go.
  cdb	        CDB bytes. All values that are needed should be set on 
                input. We'll figure out what the right CDB length should be.

  Return 0 if command completed successfully.
 */
int run_mmc_cmd_aspi( void *p_user_data, 
		      unsigned int i_timeout,
		      unsigned int i_cdb, 
		      const mmc_cdb_t * p_cdb,
		      cdio_mmc_direction_t e_direction, 
		      unsigned int i_buf, /*in/out*/ void *p_buf );

