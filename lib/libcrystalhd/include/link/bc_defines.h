/********************************************************************
 * Copyright(c) 2006-2009 Broadcom Corporation.
 *
 *  Name: bc_defines.h
 *
 *  Description: Driver Interface library Internal.
 *
 *  AU
 *
 *  HISTORY:
 *
 ********************************************************************
 * This header is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License.
 *
 * This header is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with this header.  If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************/

#ifndef _BC_DEFINES_
#define _BC_DEFINES_
//The AES and DCI H/W engines are big endian and hence the DATA needs to be
//byte swapped when loading the data registers in this block
#define rotr32_1(x,n)   (((x) >> n) | ((x) << (32 - n)))
#define bswap_32_1(x) (rotr32_1((x), 24) & 0x00ff00ff | rotr32_1((x), 8) & 0xff00ff00)

#define DCI_INITIATE_FW_DOWNLOAD	(0x1) //bit 0
#define DCI_DOWNLOAD_READY		(0x1<<4) //bit 4
#define DCI_DOWNLOAD_COMPLETE		(0x1<<1)  //bit 2
#define DCI_FIRMWARE_VALIDATED		(0x1) //bit 0
#define DCI_SIGNATURE_MATCHED		(0x1<<9) //bit 9
#define DCI_SIGNATURE_MISMATCH		(0x1<<8) //bit 8
#define DCI_START_PROCESSOR		(0x10) //bit 4

#define OTP_KEYS_AVAIL			(0x1<<1)

#define AES_PREPARE_ENCRYPTION		(0x1<<4)
#define AES_PREPARE_DONE		(0x1<<4)
#define AES_WRITE_EEPROM		(0x1<<12)
#define AES_WRITE_DONE			(0x1<<12)
#define AES_RANDOM_READY		(0x1<<20)

#endif
