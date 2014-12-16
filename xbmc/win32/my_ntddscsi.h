#ifndef MY_NTDDSCSI_H
#define MY_NTDDSCSI_H

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

//** Defines taken from ntddscsi.h in MS Windows DDK CD
#define SCSI_IOCTL_DATA_OUT             0 //Give data to SCSI device (e.g. for writing)
#define SCSI_IOCTL_DATA_IN              1 //Get data from SCSI device (e.g. for reading)
#define SCSI_IOCTL_DATA_UNSPECIFIED     2 //No data (e.g. for ejecting)

#define MAX_SENSE_LEN 18 //Sense data max length

#define IOCTL_SCSI_PASS_THROUGH         0x4D004
typedef struct _SCSI_PASS_THROUGH {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    ULONG_PTR DataBufferOffset;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
}SCSI_PASS_THROUGH, *PSCSI_PASS_THROUGH;

#define IOCTL_SCSI_PASS_THROUGH_DIRECT  0x4D014
typedef struct _SCSI_PASS_THROUGH_DIRECT {
    USHORT Length;
    UCHAR ScsiStatus;
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    UCHAR CdbLength;
    UCHAR SenseInfoLength;
    UCHAR DataIn;
    ULONG DataTransferLength;
    ULONG TimeOutValue;
    PVOID DataBuffer;
    ULONG SenseInfoOffset;
    UCHAR Cdb[16];
}SCSI_PASS_THROUGH_DIRECT, *PSCSI_PASS_THROUGH_DIRECT;
//** End of defines taken from ntddscsi.h from MS Windows DDK CD

typedef struct _SCSI_PASS_THROUGH_AND_BUFFERS {
    SCSI_PASS_THROUGH spt;
    BYTE DataBuffer[64*1024];
}T_SPT_BUFS;

typedef struct _SCSI_PASS_THROUGH_DIRECT_AND_SENSE_BUFFER {
    SCSI_PASS_THROUGH_DIRECT sptd;
    UCHAR SenseBuf[MAX_SENSE_LEN];
}T_SPDT_SBUF;
#endif