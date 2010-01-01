/********************************************************************
 * Copyright(c) 2008 Broadcom Corporation.
 *
 *  Name: decif.h
 *
 *  Description: Devic Interface API.
 *
 *  AU
 *
 *  HISTORY:
 *
 *******************************************************************
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 *
 *******************************************************************/
#ifndef __BCMDECIF_H__
#define __BCMDECIF_H__

#include "bc_dts_defs.h"
#include "bc_ldil_if.h"

#define PROC_TIMEOUT 2000
#define ALIGN_BUF_SIZE	(512*1024)

struct _DecIf
{
	HANDLE	hdev;
	guint8* aligned_buf;
};
typedef struct _DecIf BcmDecIF;

BC_STATUS
decif_open(BcmDecIF * decif);

BC_STATUS
decif_close(BcmDecIF * decif);

BC_STATUS
decif_prepare_play(BcmDecIF * decif,guint8 comp);

BC_STATUS
decif_start_play(BcmDecIF * decif);

BC_STATUS
decif_pause(BcmDecIF * decif,gboolean pause);

BC_STATUS
decif_stop(BcmDecIF * decif);

BC_STATUS
decif_flush_dec(BcmDecIF * decif,gint8 flush_type);

BC_STATUS
decif_flush_rxbuf(BcmDecIF * decif,gboolean discard_only);

BC_STATUS
decif_send_buffer(BcmDecIF * decif,guint8* buffer,guint32 size,GstClockTime time_stamp,guint8 flags);

BC_STATUS
decif_set422_mode(BcmDecIF * decif,guint8 mode);

BC_STATUS 
decif_get_drv_status(BcmDecIF * decif, gboolean* suspended);

BC_STATUS
decif_decode_catchup(BcmDecIF * decif, gboolean catchup);

#endif



