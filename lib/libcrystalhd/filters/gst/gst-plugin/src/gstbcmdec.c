/********************************************************************
 * Copyright(c) 2008 Broadcom Corporation.
 *
 *  Name: gstbcmdec.c
 *
 *  Description: Broadcom 70012 decoder plugin
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <pthread.h>
#include <sys/times.h>
#include <semaphore.h>
#include <getopt.h>
#include <errno.h>
#include <time.h>
#include <glib.h>
#include <gst/base/gstadapter.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>

#include "decif.h"
#include "parse.h"
#include "gstbcmdec.h"

GST_DEBUG_CATEGORY_STATIC (gst_bcmdec_debug);
#define GST_CAT_DEFAULT gst_bcmdec_debug

//#define YV12__

//#define FILE_DUMP__ 1

static BC_STATUS bcmdec_send_buffer(GstBcmDec *bcmdec, guint8* pbuffer,
				    guint32 size, guint32 offset,
				    GstClockTime tCurrent, guint8 flags)
{
	BC_STATUS sts;

	if (bcmdec->enable_spes) {
		if (offset == 0) {
			sts = decif_send_buffer(&bcmdec->decif, pbuffer, size, tCurrent, flags);
			if (sts != BC_STS_SUCCESS)
				GST_DEBUG_OBJECT(bcmdec, "decif_send_buffer -- failed...0");
		} else {
			sts = decif_send_buffer(&bcmdec->decif, pbuffer, offset, 0, flags);
			if (sts != BC_STS_SUCCESS)
				GST_DEBUG_OBJECT(bcmdec, "decif_send_buffer -- failed...1");
			sts = decif_send_buffer(&bcmdec->decif, pbuffer + offset, size - offset, tCurrent, flags);
			if (sts != BC_STS_SUCCESS)
				GST_DEBUG_OBJECT(bcmdec, "decif_send_buffer -- failed...2");
		}
	} else {
		sts = decif_send_buffer(&bcmdec->decif, pbuffer, size, 0, flags);
	}

	return sts;
}

static GstFlowReturn bcmdec_send_buff_detect_error(GstBcmDec *bcmdec, GstBuffer *buf,
						   guint8* pbuffer, guint32 size,
						   guint32 offset, GstClockTime tCurrent,
						   guint8 flags)
{
	BC_STATUS sts, suspend_sts = BC_STS_SUCCESS;
	gboolean suspended = FALSE;

	sts = bcmdec_send_buffer(bcmdec, pbuffer, size, offset, tCurrent, flags);
	if (sts != BC_STS_SUCCESS) {
		GST_ERROR_OBJECT(bcmdec, "proc input failed sts = %d", sts);
		GST_ERROR_OBJECT(bcmdec, "Chain: timeStamp = %llu size = %d data = %p",
				 GST_BUFFER_TIMESTAMP(buf), GST_BUFFER_SIZE(buf),
				 GST_BUFFER_DATA (buf));
		if ((sts == BC_STS_IO_USER_ABORT) || (sts == BC_STS_ERROR)) {
			suspend_sts = decif_get_drv_status(&bcmdec->decif,&suspended);
			if (suspend_sts == BC_STS_SUCCESS) {
				if (suspended) {
					GST_INFO_OBJECT(bcmdec, "suspend ststus recv");
					if (!bcmdec->suspend_mode)  {
						bcmdec_suspend_callback(bcmdec);
						bcmdec->suspend_mode = TRUE;
						GST_INFO_OBJECT(bcmdec, "suspend done", sts);
					}
					if (bcmdec_resume_callback(bcmdec) == BC_STS_SUCCESS) {
						GST_INFO_OBJECT(bcmdec, "resume done", sts);
						bcmdec->suspend_mode = FALSE;
						sts = bcmdec_send_buffer(bcmdec, pbuffer, size, offset, tCurrent, flags);
						GST_ERROR_OBJECT(bcmdec, "proc input..2 sts = %d", sts);
					} else {
						GST_INFO_OBJECT(bcmdec, "resume failed", sts);
					}
				}
				else if (sts == BC_STS_ERROR) {
					GST_INFO_OBJECT(bcmdec, "device is not suspended");
					//gst_buffer_unref (buf);
					return GST_FLOW_ERROR;
				}
			} else {
				GST_INFO_OBJECT(bcmdec, "decif_get_drv_status -- failed %d", sts);
			}
		}
	}

	return GST_FLOW_OK;
}


#ifdef WMV_FILE_HANDLING
static const guint8 b_asf_vc1_frame_scode[4] = { 0x00, 0x00, 0x01, 0x0D };
static const uint8_t b_asf_vc1_sm_frame_scode[4] = { 0x00, 0x00, 0x01, 0xE0 };
static const uint8_t b_asf_vc1_sm_codein_scode[4] = { 0x5a, 0x5a, 0x5a, 0x5a };
static const uint8_t b_asf_vc1_sm_codein_data_suffix[1] = { 0x0D };
static const uint8_t b_asf_vc1_sm_codein_sl_suffix[1] = { 0x0F };
static const uint8_t b_asf_vc1_sm_codein_pts_suffix[1] = { 0xBD };


#define GET_INTERLACED_INFORMATION
guint64 swap64(guint64 x)
{
	return ((guint64)(((x) << 56) | ((x) >> 56) |
			 (((x) & 0xFF00LL) << 40) |
			 (((x) & 0xFF000000000000LL) >> 40) |
			 (((x) & 0xFF0000LL) << 24) |
			 (((x) & 0xFF0000000000LL) >> 24) |
			 (((x) & 0xFF000000LL) << 8) |
			 (((x) & 0xFF00000000LL) >> 8)));
}


void
print_buffer(guint8 *pbuffer,guint32 sz)
{
	guint32 i = 0;
	printf("\nprinting BufferSz %x\n", sz);
	for (i = 0; i < sz; i++)
		printf("0x%x ", pbuffer[i]);
	printf("\n=====\n");
}


guint32 swap32(guint32 x)
{
	return ((guint32)(((x) << 24) |
			  ((x) >> 24) |
			 (((x) & 0xFF00) << 8) |
			 (((x) & 0xFF0000) >> 8)));
}

guint16 swap16(guint16 x)
{
	return ((guint16)(((x) << 8) | ((x) >> 8)));
}


static BC_STATUS parse_VC1SeqHdr(GstBcmDec *bcmdec, void *pBuffer, guint32 buff_sz)
{
	guint8 *pdata, index = 0;
	guint32 seq_hdr_sz;
	pdata = (guint8 *)pBuffer;

	if (!pdata || !buff_sz)
		return BC_STS_INV_ARG;

	/*Scan for VC1 Sequence Header SC for Advacned profile*/
	bcmdec->adv_profile = FALSE;
	bcmdec->vc1_seq_header_sz = 0;

	for (index = 0; index < buff_sz; index++) {
		pdata += index;
		if (((buff_sz - index) >= 4) && (*pdata == 0x00) && (*(pdata + 1) == 0x00) &&
		    (*(pdata + 2) == 0x01) && (*(pdata + 3) == 0x0f)) {
			GST_INFO_OBJECT(bcmdec, "VC1 Seqeucne Header Found for AdvProfile");
			bcmdec->adv_profile = TRUE;
			seq_hdr_sz = buff_sz - index + 1;

			if (seq_hdr_sz > MAX_ADV_PROF_SEQ_HDR_SZ)
				seq_hdr_sz = MAX_ADV_PROF_SEQ_HDR_SZ;
			bcmdec->vc1_seq_header_sz = seq_hdr_sz;
			memcpy(bcmdec->vc1_advp_seq_header, pdata, bcmdec->vc1_seq_header_sz);
			break;
		}
	}

	if (bcmdec->adv_profile) {
		GST_INFO_OBJECT(bcmdec, "Setting Input format to Adv Profile");
		bcmdec->input_format = BC_VID_ALGO_VC1;
	} else {

		GST_INFO_OBJECT(bcmdec, "Parsing VC-1 SI/MA Seq Header [Sz:%x]\n", buff_sz);
		bcmdec->vc1_seq_header_sz = buff_sz;
		memcpy(bcmdec->vc1_advp_seq_header, pBuffer, bcmdec->vc1_seq_header_sz);

		bcmdec->input_format = BC_VID_ALGO_VC1MP;
		bcmdec->proc_in_flags |= 0x02;
		guint32 dwSH = swap32(*(guint32 *)(pBuffer));
		bcmdec->bRangered    = 0x00000080 & dwSH;
		bcmdec->bMaxbFrames  = 0x00000070 & dwSH;
		bcmdec->bFinterpFlag = 0x00000002 & dwSH;
	}

	return BC_STS_SUCCESS;
}

static BC_STATUS connect_wmv_file(GstBcmDec *bcmdec, GstStructure *structure)
{
	GstBuffer *buffer;
	guint8 *data;
	guint size;

	const GValue *g_value;

	if (!structure || !bcmdec)
		return BC_STS_INV_ARG;

	bcmdec->wmv_file = TRUE;

	if (!bcmdec->vc1_dest_buffer) {
		bcmdec->vc1_dest_buffer = (guint8 *)malloc(MAX_VC1_INPUT_DATA_SZ);
		if (!bcmdec->vc1_dest_buffer) {
			GST_ERROR_OBJECT(bcmdec, "Failed to Allocate VC-1 Destination Buffer");
			return BC_STS_ERROR;
		}
	}

	gst_structure_get_int(structure, "width", &bcmdec->frame_width);
	gst_structure_get_int(structure, "height", &bcmdec->frame_height);
	g_value = gst_structure_get_value(structure, "codec_data");
	if (g_value && (G_VALUE_TYPE(g_value) == GST_TYPE_BUFFER)) {
		buffer = gst_value_get_buffer(g_value);
		data = GST_BUFFER_DATA(buffer);
		size = GST_BUFFER_SIZE(buffer);
		return parse_VC1SeqHdr(bcmdec, data, size);
	}

	return BC_STS_ERROR;
}

static guint8 get_VC1_SPMP_frame_type(GstBcmDec *bcmdec, guint8 *pBuffer)
{
	guint8 checks[] = { 0x20, 0x10, 0x10, 0x08, 0x08, 0x04 };
	guint check_offset = 0, frm_type = 0x0f;

	if (bcmdec->bFinterpFlag)
		check_offset += 2;
	if (bcmdec->bRangered)
		check_offset += 2;
	/* The first check will be at check offset and
	 * the second check at check offset + 1 */
	if (pBuffer[0] & checks[check_offset])
		frm_type = P_FRAME;
	else if (!bcmdec->bMaxbFrames)
		frm_type = I_FRAME;
	else if (pBuffer[0] & checks[check_offset + 1])
		frm_type = I_FRAME;
	else
		frm_type = B_FRAME;

	return frm_type;
}

static guint8 get_VC1_adv_prof_frame_type(GstBcmDec *bcmdec, guint8 *pBuffer)
{
	guint8 frm_type = 0x0f;

	if (!pBuffer || !bcmdec->adv_profile)
		return 0xFF;

	if (bcmdec->interlace) {
		if ((pBuffer[0] & 0x20) == 0)
			frm_type = P_FRAME;
		else if ((pBuffer[0] & 0x10) == 0)
			frm_type = B_FRAME;
		else if ((pBuffer[0] & 0x08) == 0)
			frm_type = I_FRAME;
		else if ((pBuffer[0] & 0x04) == 0)
			frm_type = BI_FRAME;
	} else {
		if ((pBuffer[0] & 0x80) == 0)
			frm_type = P_FRAME;
		else if ((pBuffer[0] & 0x40) == 0)
			frm_type = B_FRAME;
		else if ((pBuffer[0] & 0x20) == 0)
			frm_type = I_FRAME;
		else if ((pBuffer[0] & 0x10) == 0)
			frm_type = BI_FRAME;
	}

	return frm_type;
}

/*
 * insert_asf_header: 16 byte header + 1 byte suffix.
 * ------------------------------------------------------------------
 *  | 5a5a5a5a | total_pkt_sz | last_data_loc | 0x5a5a5a5a | 0x0D |
 * ------------------------------------------------------------------
 * total_pkt_sz = payload_hdr_sz [17] + proc_in_buff_sz + zero_padding_Sz[for alignment]
 * last_data_loc = proc_in_buff_sz - 1;
 *
 * ** NOTE: This header should be sent only once for every payload
 * **       that you create for hardware.
 */
guint32 insert_asf_header(guint8 *pHWbuffer, guint32 total_pkt_sz, guint32 last_data_loc)
{
	guint8 *pbuff_start = pHWbuffer;
	memcpy(pHWbuffer, b_asf_vc1_sm_codein_scode, MAX_SC_SZ);
	pHWbuffer += MAX_SC_SZ;
	(*(guint32 *)pHWbuffer) = swap32((guint32)total_pkt_sz);
	pHWbuffer += 4;
	(*(guint32 *)pHWbuffer) = swap32((guint32)last_data_loc);
	pHWbuffer += 4;
	memcpy(pHWbuffer, b_asf_vc1_sm_codein_scode, MAX_SC_SZ);
	pHWbuffer += MAX_SC_SZ;
	memcpy(pHWbuffer, b_asf_vc1_sm_codein_data_suffix, 1);
	pHWbuffer += 1;
	//printf("INSERT-PALYLOAD \n");
	//printf("TotalPktSz:0x%x LastDataLoc:0x%x\n",total_pkt_sz,last_data_loc);
	//print_buffer(pbuff_start,(guint32)(pHWbuffer - pbuff_start));

	return ((guint32)(pHWbuffer - pbuff_start));
}

/*
 * insert_pes_header: 9 byte header.
 * ------------------------------------------------------------------
 *  | 000001e0 | total_pes_pkt_len | 0x81    | 0     | pes_hdr_len |
 *  | 4-bytes  | 2-bytes	   | 1-byte  | 1byte | 1 byte      |
 * ------------------------------------------------------------------
 * total_pes_pkt_len =  payload_hdr_sz + current_xfer_sz + zero_pad_sz;
 * pes_hdr_len = 0 - data packets.
 *			     9 - PTS packets and sequence header packets
 *
 * ** NOTE: total_pes_pkt_len cannot be more than 0x7FFF [2 bytes]
 * ** If the data is more than this it should be broken in packets.
*/

guint32 insert_pes_header(guint8 *pHWbuffer, guint32 pes_pkt_len)
{
	guint8 *pbuff_start = pHWbuffer;

	if (pes_pkt_len > MAX_RE_PES_BOUND) {
		printf("Not Inserting PES Hdr %x\n",pes_pkt_len);
		return 0;
	}

	memcpy(pHWbuffer, b_asf_vc1_sm_frame_scode, MAX_SC_SZ);
	pHWbuffer += MAX_SC_SZ;
	/*
	 * The optional three bytes that we are using to tell that there is no
	 * extension.
	 */
	(*(guint16 *)pHWbuffer) = swap16((guint16)pes_pkt_len);
	pHWbuffer += 2;
	(*(guint8 *)pHWbuffer) = 0x81;
	pHWbuffer += 1;
	(*(guint8 *)pHWbuffer) = 0x0;
	pHWbuffer += 1;
	(*(guint8 *)pHWbuffer) = 0;
	pHWbuffer += 1;
	//printf("INSERT-PES \n");
	//print_buffer(pbuff_start,(guint32)(pHWbuffer - pbuff_start));
	//printf("PesLen:0x%x \n",pes_pkt_len);

	return ((guint32)(pHWbuffer - pbuff_start));
}

static void PTS2MakerBit5Bytes(guint *p_mrk_bit_loc, GstClockTime llPTS)
{
	//4 Bits: '0010'
	//3 Bits: PTS[32:30]
	//1 Bit: '1'
	//15 Bits: PTS[29:15]
	//1 Bit: '1'
	//15 Bits: PTS[14:0]
	//1 Bit : '1'
	//0010 xxx1 xxxx xxxx xxxx xxx1 xxxx xxxx xxxx xxx1

	//Swap
	//guint64 ullSwap = swap64(llPTS);

	guint64 ullSwap = swap32(llPTS);
	guint8 *pPTS = (guint8 *)(&ullSwap);

	//Reserved the last 5 bytes, using the last 33 bits of PTS
	//0000 000x xxxx xxxx xxxx xxxx xxxx xxxx xxxx xxxx
	pPTS = pPTS + 3;

	//1st Byte: 0010 xxx1
	*(p_mrk_bit_loc) = 0x21 | ((*(pPTS) & 0x01) << 3) | ((*(pPTS + 1) & 0xC0) >> 5);

	//2nd Byte: xxxx xxxx
	*(p_mrk_bit_loc + 1) = ((*(pPTS + 1) & 0x3F) << 2) | ((*(pPTS + 2) & 0xC0) >> 6);

	//3rd Byte: xxxx xxx1
	*(p_mrk_bit_loc + 2) = 0x01 | ((*(pPTS + 2) & 0x3F) << 2) | ((*(pPTS + 3) & 0x80) >> 6);

	//4th Byte: xxxx xxxx
	*(p_mrk_bit_loc + 3) = ((*(pPTS + 3) & 0x7F) << 1) | ((*(pPTS + 4) & 0x80) >> 7);

	//5th Byte: xxxx xxx1
	*(p_mrk_bit_loc + 4) = 0x01 | ((*(pPTS + 4) & 0x7F) << 1);
}

static guint32 send_VC1_SPMP_seq_hdr(GstBcmDec *bcmdec, GstBuffer *buf,
				     guint8 *pBuffer) /* Destination Buffer for hardware */
{
	guint32 used_sz = 0;
	guint8 *pcopy_loc = pBuffer;
	guint32 CIN_HEADER_SZ = 17, CIN_DATA_SZ = 8, CIN_ZERO_PAD_SZ = 7, PES_HEADER_LEN = 5;
	guint32 CIN_PKT_SZ = CIN_HEADER_SZ + CIN_DATA_SZ + CIN_ZERO_PAD_SZ;
	guint32 PES_PKT_LEN = CIN_PKT_SZ + PES_HEADER_LEN + 3;
	GstFlowReturn fret;
#ifdef FILE_DUMP__
	FILE *spmpfiledump;
	spmpfiledump = fopen("spmpfiledump", "a+");
#endif

	memcpy(pcopy_loc, b_asf_vc1_sm_frame_scode, MAX_FRSC_SZ);
	pcopy_loc += MAX_FRSC_SZ;
	used_sz += MAX_FRSC_SZ;

	*((guint16 *)pcopy_loc) = swap16((guint16)PES_PKT_LEN);
	pcopy_loc += 2;
	used_sz += 2;

	*pcopy_loc = 0x81;
	pcopy_loc += 1;
	used_sz += 1;

	*pcopy_loc = 0x80;
	pcopy_loc += 1;
	used_sz += 1;

	*pcopy_loc = PES_HEADER_LEN;
	pcopy_loc += 1;
	used_sz += 1;

	PTS2MakerBit5Bytes((guint *)pcopy_loc, 0);
	pcopy_loc += 5;
	used_sz += 5;

	memcpy(pcopy_loc, b_asf_vc1_sm_codein_scode, MAX_SC_SZ);
	pcopy_loc += MAX_SC_SZ;
	used_sz += MAX_SC_SZ;

	*((guint32 *)pcopy_loc) = swap32(CIN_PKT_SZ);
	pcopy_loc += 4;
	used_sz += 4;

	*((guint32 *)pcopy_loc) = swap32(CIN_DATA_SZ - 1);
	pcopy_loc += 4;
	used_sz += 4;

	memcpy(pcopy_loc, b_asf_vc1_sm_codein_scode, MAX_SC_SZ);
	pcopy_loc += MAX_SC_SZ;
	used_sz += MAX_SC_SZ;

	memcpy(pcopy_loc, b_asf_vc1_sm_codein_sl_suffix, 1);
	pcopy_loc += 1;
	used_sz += 1;

	*((guint16 *)pcopy_loc) = swap16(bcmdec->frame_width);
	pcopy_loc += 2;
	used_sz += 2;

	*((guint16 *)pcopy_loc) = swap16(bcmdec->frame_height);
	pcopy_loc += 2;
	used_sz += 2;

	memcpy(pcopy_loc, bcmdec->vc1_advp_seq_header, bcmdec->vc1_seq_header_sz);
	pcopy_loc += bcmdec->vc1_seq_header_sz;
	used_sz += bcmdec->vc1_seq_header_sz;

	memset(pcopy_loc, 0, CIN_ZERO_PAD_SZ);
	pcopy_loc += CIN_ZERO_PAD_SZ;
	used_sz += CIN_ZERO_PAD_SZ;

#ifdef FILE_DUMP__
	fwrite(pBuffer, used_sz, 1, spmpfiledump);
	fclose(spmpfiledump);
#endif

	fret = bcmdec_send_buff_detect_error(bcmdec, buf, pBuffer, used_sz,
					     0, 0, bcmdec->proc_in_flags);
	if (fret != GST_FLOW_OK) {
		GST_ERROR_OBJECT(bcmdec, "Failed to Send the SPMP Sequence Header");
		return 0;
	}

	return used_sz;
}

static guint32 send_VC1_SPMP_PES_payload_PTS(GstBcmDec *bcmdec, GstBuffer *buf,
					     guint8 *pBuffer,	//Destination Buffer for hardware
					     GstClockTime tCurrent)
{
	guint32 used_sz = 0;
	guint8 *pcopy_loc = pBuffer;
	guint32 cin_hdr_sz = 17, cin_zero_pad_sz = 5, cin_data_sz = 10;
	guint32 pes_hdr_len = 0;
	guint32 cin_pkt_sz = cin_hdr_sz + cin_zero_pad_sz + cin_data_sz;
	guint32 cin_last_data_loc = cin_data_sz - 1;
	guint32 pes_pkt_len = cin_pkt_sz + pes_hdr_len + 3;
	GstFlowReturn fret;
#ifdef FILE_DUMP__
	FILE *spmpfiledump;
	spmpfiledump = fopen("spmpfiledump", "a+");
#endif

	memcpy(pcopy_loc, b_asf_vc1_sm_frame_scode, MAX_FRSC_SZ);
	pcopy_loc += MAX_FRSC_SZ;
	used_sz += MAX_FRSC_SZ;

	*((guint16 *) pcopy_loc) = swap16((guint16)pes_pkt_len);
	pcopy_loc += 2;
	used_sz += 2;

	*pcopy_loc = 0x81;
	pcopy_loc += 1;
	used_sz += 1;

	*pcopy_loc=0x0;
	pcopy_loc += 1;
	used_sz += 1;

	*pcopy_loc = pes_hdr_len;
	pcopy_loc += 1;
	used_sz += 1;

	memcpy(pcopy_loc, b_asf_vc1_sm_codein_scode, MAX_SC_SZ);
	pcopy_loc += MAX_SC_SZ;
	used_sz += MAX_SC_SZ;

	*((guint32 *)pcopy_loc) = swap32(cin_pkt_sz);
	pcopy_loc += 4;
	used_sz += 4;

	*((guint32 *)pcopy_loc) = swap32(cin_last_data_loc);
	pcopy_loc += 4;
	used_sz += 4;

	memcpy(pcopy_loc, b_asf_vc1_sm_codein_scode, MAX_SC_SZ);
	pcopy_loc += MAX_SC_SZ;
	used_sz += MAX_SC_SZ;

	memcpy(pcopy_loc, b_asf_vc1_sm_codein_pts_suffix, 1);
	pcopy_loc += 1;
	used_sz += 1;

	*pcopy_loc = 0x01;
	pcopy_loc += 1;
	used_sz += 1;

	if (tCurrent < 0)
		tCurrent = 0;

	PTS2MakerBit5Bytes((guint *)pcopy_loc, tCurrent);
	pcopy_loc += 5;
	used_sz += 5;

	*((guint32 *)pcopy_loc) = 0xffffffff;
	pcopy_loc += 4;
	used_sz += 4;

	memset(pcopy_loc, 0, cin_zero_pad_sz);
	pcopy_loc += cin_zero_pad_sz;
	used_sz += cin_zero_pad_sz;

#ifdef FILE_DUMP__
	fwrite(pBuffer, used_sz, 1, spmpfiledump);
	fclose(spmpfiledump);
#endif

	fret = bcmdec_send_buff_detect_error(bcmdec, buf, pBuffer, used_sz,
					     0, 0, bcmdec->proc_in_flags);
	if (fret != GST_FLOW_OK) {
		GST_ERROR_OBJECT(bcmdec, "Failed to Send the SPMP PTS Header");
		return 0;
	}

	return used_sz;
}

static guint32 send_VC1_SPMP_data(GstBcmDec *bcmdec, GstBuffer *buf,
				  guint8 *pbuffer_out, guint8 *pinput_data,
				  guint32 input_data_sz, GstClockTime tCurrent)
{
	guint32 used_sz = 0, current_xfer_sz = 0;
	guint32 zero_pad_sz = GET_ZERO_PAD_SZ(input_data_sz);
	guint32 sz_in_asf_hdr, rem_xfer_sz = input_data_sz;
	sz_in_asf_hdr = input_data_sz + zero_pad_sz + PAYLOAD_HDR_SZ_WITH_SUFFIX;
	current_xfer_sz = input_data_sz;
	GstFlowReturn fret;
#ifdef FILE_DUMP__
	FILE *spmpfiledump;
	spmpfiledump = fopen("spmpfiledump", "a+");
#endif

	if (input_data_sz > MAX_FIRST_CHUNK_SZ) {
		current_xfer_sz = MAX_FIRST_CHUNK_SZ;
		zero_pad_sz = 0;
	}

	used_sz += insert_pes_header(pbuffer_out + used_sz, GET_PES_HDR_SZ_WITH_ASF(current_xfer_sz + zero_pad_sz));
	used_sz += insert_asf_header(pbuffer_out + used_sz, sz_in_asf_hdr, input_data_sz - 1);
	memcpy(pbuffer_out + used_sz, pinput_data, current_xfer_sz);
	used_sz += current_xfer_sz;
	pinput_data += current_xfer_sz;
	rem_xfer_sz -= current_xfer_sz;

	while (rem_xfer_sz) {

		if (rem_xfer_sz > MAX_CHUNK_SZ) {
			current_xfer_sz = MAX_CHUNK_SZ;
		} else {
			current_xfer_sz = rem_xfer_sz;
			zero_pad_sz = GET_ZERO_PAD_SZ(input_data_sz);
		}

		used_sz += insert_pes_header(pbuffer_out + used_sz, GET_PES_HDR_SZ(current_xfer_sz + zero_pad_sz));
		memcpy(pbuffer_out + used_sz, pinput_data, current_xfer_sz);
		used_sz += current_xfer_sz;
		pinput_data += current_xfer_sz;
		rem_xfer_sz -= current_xfer_sz;
	}

	if (zero_pad_sz) {
		memset(pbuffer_out + used_sz, 0, zero_pad_sz);
		used_sz += zero_pad_sz;
	}

	fret = bcmdec_send_buff_detect_error(bcmdec, buf, pbuffer_out, used_sz,
					     0, tCurrent, bcmdec->proc_in_flags);
	if (fret != GST_FLOW_OK) {
			GST_ERROR_OBJECT(bcmdec, "Failed to Send Data");
			return 0;
	}

#ifdef FILE_DUMP__
	fwrite(pbuffer_out, used_sz, 1, spmpfiledump);
	fclose(spmpfiledump);
#endif

	return used_sz;
}


static guint32 handle_VC1_SPMP_data(GstBcmDec *bcmdec, GstBuffer *buf,
				    guint8 *pBuffer,	/*The buffer with proc_in data*/
				    guint32 buff_sz, guint8 frm_type,
				    GstClockTime tCurrent)
{
	guint32 used_sz = 0, temp_sz = 0;
	guint8 *pcopy_loc = NULL;

	if (!bcmdec || !pBuffer || !buff_sz ||
		  !bcmdec->vc1_seq_header_sz || !bcmdec->vc1_dest_buffer) {
		GST_ERROR_OBJECT(bcmdec, "Invalid Arguments");
		return 0;
	}

	pcopy_loc = bcmdec->vc1_dest_buffer;

	if (I_FRAME == frm_type) {
		temp_sz = send_VC1_SPMP_seq_hdr(bcmdec, buf, pcopy_loc);
		if (!temp_sz) {
			GST_ERROR_OBJECT(bcmdec, "Failed to create SeHdr Payload");
			return 0;
		}
	}

	used_sz += temp_sz;
	pcopy_loc += temp_sz;
	temp_sz = send_VC1_SPMP_PES_payload_PTS(bcmdec, buf, pcopy_loc, tCurrent);
	if (!temp_sz) {
		GST_ERROR_OBJECT(bcmdec, "Failed to create PTS-PES Payload");
		return 0;
	}

	used_sz += temp_sz;
	pcopy_loc += temp_sz;
	temp_sz = send_VC1_SPMP_data(bcmdec, buf, pcopy_loc, pBuffer, buff_sz, tCurrent);
	if (!temp_sz) {
		GST_ERROR_OBJECT(bcmdec, "Failed to Send PTS-PES Payload");
		return 0;
	}

	used_sz += temp_sz;
	return used_sz;
}

static guint32 process_VC1_Input_data(GstBcmDec *bcmdec, GstBuffer *buf,
				      GstClockTime tCurrent)
{
	guint8 *pBuffer;
	gint8 frm_type = 0;
	guint8 *pcopy_loc = NULL;
	guint32 max_buff_sz_needed = 0, used_buff_sz = 0, offset = 0, buff_sz = 0;
	GstFlowReturn fret;
#ifdef FILE_DUMP__
	FILE			*modified_data_dump;
#endif

	pBuffer = GST_BUFFER_DATA(buf);
	buff_sz = GST_BUFFER_SIZE(buf);

	if (!bcmdec || !pBuffer || !buff_sz || !bcmdec->vc1_dest_buffer) {
		GST_ERROR_OBJECT(bcmdec, "Invalid Arguments");
		return 0;
	}

	if ((pBuffer[0] == 0x00) && (pBuffer[1] == 0x00) && (pBuffer[2] == 0x01) &&
		  ((pBuffer[3] == 0x0F) || (pBuffer[3] == 0x0D) || (pBuffer[3] == 0xE0))) {
		/* Just Send The Buffer TO Hardware Here */
		GST_INFO_OBJECT(bcmdec, "Found Start Codes in the Stream..!ADD CODE TO SEND BUFF!");
		return 1;
	}

	/* Come here only if start codes are not found */
	if (bcmdec->adv_profile) {
		max_buff_sz_needed = buff_sz + MAX_FRSC_SZ + bcmdec->vc1_seq_header_sz;

		if (max_buff_sz_needed > MAX_VC1_INPUT_DATA_SZ) {
			GST_ERROR_OBJECT(bcmdec, "VC1 Buffer Too Small");
			return 0;
		}

		frm_type = get_VC1_adv_prof_frame_type(bcmdec, pBuffer);

#ifdef FILE_DUMP__
		modified_data_dump = fopen("modified_data_dump", "a+");
#endif
		/*Start Code + Buffer Size*/
		used_buff_sz = buff_sz + MAX_FRSC_SZ;

		if (I_FRAME == frm_type) {
			/*Copy Sequence Header And Start Code*/
			used_buff_sz += bcmdec->vc1_seq_header_sz;
			pcopy_loc = bcmdec->vc1_dest_buffer;
			memcpy(pcopy_loc, bcmdec->vc1_advp_seq_header, bcmdec->vc1_seq_header_sz);
			pcopy_loc += bcmdec->vc1_seq_header_sz;
			memcpy(pcopy_loc, b_asf_vc1_frame_scode, MAX_FRSC_SZ);
			pcopy_loc += MAX_FRSC_SZ;
		} else {
			/*Copy Only the Start code*/
			pcopy_loc = bcmdec->vc1_dest_buffer;
			memcpy(pcopy_loc, b_asf_vc1_frame_scode, MAX_FRSC_SZ);
			pcopy_loc += MAX_FRSC_SZ;
		}

		memcpy(pcopy_loc, pBuffer, buff_sz);
#ifdef FILE_DUMP__
		fwrite(pBuffer, used_buff_sz, 1, modified_data_dump);
		fclose(modified_data_dump);
#endif

		if (bcmdec->enable_spes) {
			if (!parse_find_strt_code(&bcmdec->parse, bcmdec->input_format, bcmdec->vc1_dest_buffer, used_buff_sz, &offset)) {
				offset = 0;
				tCurrent = 0;
			}
		}

		fret = bcmdec_send_buff_detect_error(bcmdec, buf,
						     bcmdec->vc1_dest_buffer,
						     used_buff_sz, offset,
						     tCurrent,
						     bcmdec->proc_in_flags);
		if (fret != GST_FLOW_OK)
			return 0;

	} else {

		frm_type = get_VC1_SPMP_frame_type(bcmdec, pBuffer);

		used_buff_sz = handle_VC1_SPMP_data(bcmdec, buf, pBuffer,
						    buff_sz, frm_type, tCurrent);
		if (!used_buff_sz) {
			GST_ERROR_OBJECT(bcmdec, "Failed to Prepare VC-1 SPMP Data");
			return 0;
		}
	}

	return used_buff_sz;
}

#endif /* WMV_FILE_HANDLING */

/* bcmdec signals and args */
enum {
	/* FILL ME */
	LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_SILENT
};


GLB_INST_STS *g_inst_sts = NULL;

/*
 * the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
		GST_STATIC_CAPS("video/mpeg, " "mpegversion = (int) {2}, " "systemstream =(boolean) false; "
				"video/x-h264;" "video/x-vc1;" "video/x-wmv;"));

#ifdef YV12__
static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
		GST_STATIC_CAPS("video/x-raw-yuv, " "format = (fourcc) { YV12 }, " "width = (int) [ 1, MAX ], "
				"height = (int) [ 1, MAX ], " "framerate = (fraction) [ 0/1, 2147483647/1 ]"));
#define BUF_MULT (12 / 8)
#define BUF_MODE MODE420
#else
static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
		GST_STATIC_CAPS("video/x-raw-yuv, " "format = (fourcc) { YUY2 } , " "framerate = (fraction) [0,MAX], "
				"width = (int) [1,MAX], " "height = (int) [1,MAX]; " "video/x-raw-yuv, "
				"format = (fourcc) { UYVY } , " "framerate = (fraction) [0,MAX], " "width = (int) [1,MAX], "
				"height = (int) [1,MAX]; "));
#define BUF_MULT (16 / 8)
#define BUF_MODE MODE422_YUY2
#endif

GST_BOILERPLATE(GstBcmDec, gst_bcmdec, GstElement, GST_TYPE_ELEMENT);

/* GObject vmethod implementations */

static void gst_bcmdec_base_init(gpointer gclass)
{
	static GstElementDetails element_details;

	element_details.klass = (gchar *)"Codec/Decoder/Video";
	element_details.longname = (gchar *)"Generic Video Decoder";
	element_details.description = (gchar *)"Decodes various Video Formats using BCM97010";
	element_details.author = (gchar *)"BRCM";

	GstElementClass *element_class = GST_ELEMENT_CLASS(gclass);

	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get (&src_factory));
	gst_element_class_add_pad_template(element_class, gst_static_pad_template_get (&sink_factory));
	gst_element_class_set_details(element_class, &element_details);
}

/* initialize the bcmdec's class */
static void gst_bcmdec_class_init(GstBcmDecClass *klass)
{
	GObjectClass *gobject_class;
	GstElementClass *gstelement_class;

	gobject_class = (GObjectClass *)klass;
	gstelement_class = (GstElementClass *)klass;

	gstelement_class->change_state = gst_bcmdec_change_state;

	gobject_class->set_property = gst_bcmdec_set_property;
	gobject_class->get_property = gst_bcmdec_get_property;
	gobject_class->finalize     = gst_bcmdec_finalize;

	g_object_class_install_property(gobject_class, PROP_SILENT,
					g_param_spec_boolean("silent", "Silent",
							     "Produce verbose output ?",
							     FALSE, (GParamFlags)G_PARAM_READWRITE));
}

/*
 * initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void gst_bcmdec_init(GstBcmDec *bcmdec, GstBcmDecClass *gclass)
{
	/*GstElementClass *klass = GST_ELEMENT_GET_CLASS(bcmdec); */
	pid_t pid;
	BC_STATUS sts = BC_STS_SUCCESS;
	int shmid = 0;

	GST_INFO_OBJECT(bcmdec, "gst_bcmdec_init");

	bcmdec_reset(bcmdec);

	bcmdec->sinkpad = gst_pad_new_from_static_template(&sink_factory, "sink");

	gst_pad_set_event_function(bcmdec->sinkpad, GST_DEBUG_FUNCPTR(gst_bcmdec_sink_event));

	gst_pad_set_setcaps_function(bcmdec->sinkpad, GST_DEBUG_FUNCPTR(gst_bcmdec_sink_set_caps));
	/* FIXME: jarod: this is needed for newer gstreamer */
	gst_pad_set_getcaps_function(bcmdec->sinkpad, GST_DEBUG_FUNCPTR(gst_pad_proxy_getcaps));
	gst_pad_set_chain_function(bcmdec->sinkpad, GST_DEBUG_FUNCPTR(gst_bcmdec_chain));

	bcmdec->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

	/* FIXME: jarod: this is needed for newer gstreamer */
	gst_pad_set_getcaps_function(bcmdec->srcpad, GST_DEBUG_FUNCPTR(gst_pad_proxy_getcaps));

	gst_pad_set_event_function(bcmdec->srcpad, GST_DEBUG_FUNCPTR(gst_bcmdec_src_event));

	gst_pad_use_fixed_caps(bcmdec->srcpad);
	//bcmdec_negotiate_format(bcmdec);

	gst_element_add_pad(GST_ELEMENT(bcmdec), bcmdec->sinkpad);
	gst_element_add_pad(GST_ELEMENT(bcmdec), bcmdec->srcpad);
	bcmdec->silent = TRUE;
	pid = getpid();
	GST_INFO_OBJECT(bcmdec, "gst_bcmdec_init _-- PID = %x",pid);

	sts = bcmdec_create_shmem(bcmdec, &shmid);

	GST_INFO_OBJECT(bcmdec, "bcmdec_create_shmem _-- Sts = %x",sts);
}

/* plugin close function*/
static void gst_bcmdec_finalize(GObject *object)
{
	GstBcmDec *bcmdec = GST_BCMDEC(object);

	bcmdec_del_shmem(bcmdec);
	/*gst_bcmdec_cleanup(bcmdec);*/
	GST_INFO_OBJECT(bcmdec, "gst_bcmdec_finalize");
	G_OBJECT_CLASS(parent_class)->finalize(object);
}

static void gst_bcmdec_set_property(GObject *object, guint prop_id,
				    const GValue *value, GParamSpec *pspec)
{
	GstBcmDec *bcmdec = GST_BCMDEC(object);

	switch (prop_id) {
	case PROP_SILENT:
		bcmdec->silent = g_value_get_boolean (value);
		GST_INFO_OBJECT(bcmdec, "gst_bcmdec_set_property PROP_SILENT");
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
		break;
	}

	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "gst_bcmdec_set_property");
}

static void gst_bcmdec_get_property(GObject *object, guint prop_id,
				    GValue *value, GParamSpec *pspec)
{
	GstBcmDec *bcmdec = GST_BCMDEC(object);

	switch (prop_id) {
	case PROP_SILENT:
		g_value_set_boolean (value, bcmdec->silent);
		GST_INFO_OBJECT(bcmdec, "gst_bcmdec_get_property PROP_SILENT");
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}

	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "gst_bcmdec_get_property");
}

/* GstElement vmethod implementations */
static gboolean gst_bcmdec_sink_event(GstPad* pad, GstEvent* event)
{
	GstBcmDec *bcmdec;
	BC_STATUS sts = BC_STS_SUCCESS;
	bcmdec = GST_BCMDEC(gst_pad_get_parent(pad));

	gboolean result = TRUE;

	switch (GST_EVENT_TYPE(event)) {
	case GST_EVENT_NEWSEGMENT:
		GstFormat newsegment_format;
		gint64 newsegment_start;

		gst_event_parse_new_segment(event, NULL, NULL, &newsegment_format,
					    &newsegment_start, NULL, NULL);

		bcmdec->base_clock_time = newsegment_start;
		bcmdec->cur_stream_time = 0;

		if (!bcmdec->silent)
			GST_INFO_OBJECT(bcmdec, "new segment");

		bcmdec->avcc_params.inside_buffer = TRUE;
		bcmdec->avcc_params.consumed_offset = 0;
		bcmdec->avcc_params.strtcode_offset = 0;
		bcmdec->avcc_params.nal_sz = 0;
		bcmdec->insert_pps = TRUE;
		bcmdec->base_time = 0;

		bcmdec->spes_frame_cnt = 0;
		bcmdec->catchup_on = FALSE;
		bcmdec->last_output_spes_time = 0;
		bcmdec->last_spes_time = 0;

#if 0
		bcmdec->gst_clock = gst_element_get_clock((GstElement *)bcmdec);
		if (bcmdec->gst_clock)
			GST_INFO_OBJECT(bcmdec, "clock available %p",bcmdec->gst_clock);
#endif
		result = gst_pad_push_event(bcmdec->srcpad, event);
		break;

	case GST_EVENT_FLUSH_START:
		GST_INFO_OBJECT(bcmdec, "Flush Start");

#if 0
		pthread_mutex_lock(&bcmdec->fn_lock);
		if (!g_inst_sts->waiting) /*in case of playback process waiting*/
			bcmdec_process_flush_start(bcmdec);
		pthread_mutex_unlock(&bcmdec->fn_lock);
#endif
		bcmdec_process_flush_start(bcmdec);
		result = gst_pad_push_event(bcmdec->srcpad, event);
		break;

	case GST_EVENT_FLUSH_STOP:
		if (!bcmdec->silent)
			GST_INFO_OBJECT(bcmdec, "Flush Stop");
		//if (!g_inst_sts->waiting)
		//	bcmdec_process_flush_stop(bcmdec);
		bcmdec_process_flush_stop(bcmdec);
		result = gst_pad_push_event(bcmdec->srcpad, event);
		break;

	case GST_EVENT_EOS:
		if (!bcmdec->silent)
			GST_INFO_OBJECT(bcmdec, "EOS on sink pad");
		sts = decif_flush_dec(&bcmdec->decif, 0);
		GST_INFO_OBJECT(bcmdec, "dec_flush ret = %d", sts);
		bcmdec->ev_eos = event;
		gst_event_ref(bcmdec->ev_eos);
		break;

	default:
		result = gst_pad_push_event(bcmdec->srcpad, event);
		GST_INFO_OBJECT(bcmdec, "unhandled event on sink pad");
		break;
	}

	gst_object_unref(bcmdec);
	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "gst_bcmdec_sink_event");
	return result;
}

/* this function handles the link with other elements */
static gboolean gst_bcmdec_sink_set_caps(GstPad *pad, GstCaps *caps)
{
	GstBcmDec *bcmdec;
	bcmdec = GST_BCMDEC(gst_pad_get_parent(pad));
	GstStructure *structure;
	GstCaps *intersection;
	const gchar *mime;
	guint num = 0;
	guint den = 0;
	const GValue *g_value;

	intersection = gst_caps_intersect(gst_pad_get_pad_template_caps(pad), caps);
	GST_INFO_OBJECT(bcmdec, "Intersection return %", GST_PTR_FORMAT, intersection);
	if (gst_caps_is_empty(intersection)) {
		GST_ERROR_OBJECT(bcmdec, "setscaps:caps empty");
		gst_object_unref(bcmdec);
		return FALSE;
	}
	gst_caps_unref(intersection);
	structure = gst_caps_get_structure(caps, 0);
	mime = gst_structure_get_name(structure);
	if (!strcmp("video/x-h264", mime)) {
		bcmdec->input_format = BC_VID_ALGO_H264;
		GST_INFO_OBJECT(bcmdec, "InFmt H.264");
	} else if (!strcmp("video/mpeg", mime)) {
		int version = 0;
		gst_structure_get_int(structure, "mpegversion", &version);
		if (version == 2) {
			bcmdec->input_format = BC_VID_ALGO_MPEG2;
			GST_INFO_OBJECT(bcmdec, "InFmt MPEG2");
		} else {
			gst_object_unref(bcmdec);
			return FALSE;
		}
	} else if (!strcmp("video/x-vc1", mime)) {
		bcmdec->input_format = BC_VID_ALGO_VC1;
		GST_INFO_OBJECT(bcmdec, "InFmt VC1");
#ifdef WMV_FILE_HANDLING
	} else if (!strcmp("video/x-wmv", mime)) {
		GST_INFO_OBJECT(bcmdec, "Detected WMV File %s", mime);
		if (BC_STS_SUCCESS != connect_wmv_file(bcmdec,structure)) {
			GST_INFO_OBJECT(bcmdec, "WMV Connection Failure");
			gst_object_unref(bcmdec);
			return FALSE;
		}
#endif
	} else {
		GST_INFO_OBJECT(bcmdec, "unknown mime %s", mime);
		gst_object_unref(bcmdec);
		return FALSE;
	}
	if (bcmdec->play_pending) {
		bcmdec->play_pending = FALSE;
		bcmdec_process_play(bcmdec);
	}

	g_value = gst_structure_get_value(structure, "framerate");
	if (g_value != NULL) {
		num = gst_value_get_fraction_numerator(g_value);
		den = gst_value_get_fraction_denominator(g_value);

		bcmdec->input_framerate = (double)num / den;
		GST_LOG_OBJECT(bcmdec, "demux frame rate = %f ", bcmdec->input_framerate);

	} else {
		GST_INFO_OBJECT(bcmdec, "no demux framerate_value");
	}

	g_value = gst_structure_get_value(structure, "format");
	if (g_value && G_VALUE_TYPE(g_value) != GST_TYPE_FOURCC) {
		guint32 fourcc;
		//g_return_if_fail(G_VALUE_TYPE(g_value) == GST_TYPE_LIST);
		fourcc = gst_value_get_fourcc(gst_value_list_get_value (g_value, 0));
		GST_INFO_OBJECT(bcmdec, "fourcc = %d", fourcc);
	}

	g_value = gst_structure_get_value(structure, "pixel-aspect-ratio");
	if (g_value) {
		bcmdec->input_par_x = gst_value_get_fraction_numerator(g_value);
		bcmdec->input_par_y = gst_value_get_fraction_denominator(g_value);
		GST_DEBUG_OBJECT(bcmdec, "sink caps have pixel-aspect-ratio of %d:%d",
				 bcmdec->input_par_x, bcmdec->input_par_y);
		if (bcmdec->input_par_x > 5 * bcmdec->input_par_y) {
			bcmdec->input_par_x = 1;
			bcmdec->input_par_y = 1;
			GST_INFO_OBJECT(bcmdec, "demux par reset");
		}

	} else {
		GST_DEBUG_OBJECT (bcmdec, "no par from demux");
	}

	g_value = gst_structure_get_value(structure, "codec_data");
	if (g_value != NULL) {
		if (G_VALUE_TYPE(g_value) == GST_TYPE_BUFFER) {
			GstBuffer *buffer;
			guint8 *data;
			guint size;

			buffer = gst_value_get_buffer(g_value);
			data = GST_BUFFER_DATA(buffer);
			size = GST_BUFFER_SIZE(buffer);

			GST_INFO_OBJECT(bcmdec, "codec_data size = %d", size);

			if (bcmdec->avcc_params.sps_pps_buf == NULL)
				bcmdec->avcc_params.sps_pps_buf = (guint8 *)malloc(SPS_PPS_SIZE);
			if (bcmdec_insert_sps_pps(bcmdec, buffer) != BC_STS_SUCCESS) {
				bcmdec->avcc_params.pps_size = 0;
			} else if (bcmdec->dest_buf == NULL) {
				bcmdec->dest_buf = (guint8 *)malloc(ALIGN_BUF_SIZE);
				if (bcmdec->dest_buf == NULL) {
					GST_ERROR_OBJECT(bcmdec, "dest_buf malloc failed");
					return FALSE;
				}
			}
		}
	} else {
		GST_INFO_OBJECT(bcmdec, "failed to get codec_data");
	}

	gst_object_unref(bcmdec);

	return TRUE;
}

void bcmdec_msleep(gint msec)
{
	gint cnt = msec;

	while (cnt) {
		usleep(1000);
		cnt--;
	}
}

/*
 * chain function
 * this function does the actual processing
 */
static GstFlowReturn gst_bcmdec_chain(GstPad *pad, GstBuffer *buf)
{
	GstBcmDec *bcmdec;
	BC_STATUS sts = BC_STS_SUCCESS;
	guint32 buf_sz = 0;
	guint32 offset = 0;
	GstClockTime tCurrent = 0;
	guint8 *pbuffer;
	guint32 size = 0;
#ifdef WMV_FILE_HANDLING
	guint32 vc1_buff_sz = 0;
#endif


#ifdef FILE_DUMP__
	guint32 bytes_written =0;
#endif
	bcmdec = GST_BCMDEC (GST_OBJECT_PARENT (pad));

#ifdef FILE_DUMP__
	if (bcmdec->fhnd == NULL)
		bcmdec->fhnd = fopen("dump2.264", "a+");
#endif

	if (bcmdec->flushing) {
		GST_INFO_OBJECT(bcmdec, "input while flushing");
		gst_buffer_unref(buf);
		return GST_FLOW_OK;
	}

#if 0
	if (buf) {
		GST_INFO_OBJECT(bcmdec, "Chain: timeStamp = %llu size = %d data = %p",
			   GST_BUFFER_TIMESTAMP(buf), GST_BUFFER_SIZE(buf),
			   GST_BUFFER_DATA (buf));
		printf("buf sz = %d\n",GST_BUFFER_SIZE(buf));
	}
#endif

	if (GST_CLOCK_TIME_NONE != GST_BUFFER_TIMESTAMP(buf)) {
		if (bcmdec->base_time == 0) {
			bcmdec->base_time = GST_BUFFER_TIMESTAMP(buf);
			GST_INFO_OBJECT(bcmdec, "base time is set to %llu", bcmdec->base_time / 1000000);
		}
		tCurrent = GST_BUFFER_TIMESTAMP(buf);
		//printf("intime %llu\n", GST_BUFFER_TIMESTAMP(buf) / 1000000);

	}
	buf_sz = GST_BUFFER_SIZE(buf);

	if (bcmdec->play_pending) {
		bcmdec->play_pending = FALSE;
		bcmdec_process_play(bcmdec);
	} else if (!bcmdec->streaming) {
		GST_INFO_OBJECT(bcmdec, "input while streaming is false");
		gst_buffer_unref(buf);
		return GST_FLOW_OK;
	}

#ifdef WMV_FILE_HANDLING
	if (bcmdec->wmv_file) {
		vc1_buff_sz = process_VC1_Input_data(bcmdec, buf, tCurrent);
		gst_buffer_unref(buf);
		if (!vc1_buff_sz) {
			GST_ERROR_OBJECT(bcmdec, "process_VC1_Input_data failed\n");
			return GST_FLOW_ERROR;
		} else {
			return GST_FLOW_OK;
		}
	}
#endif

	if (bcmdec->avcc_params.pps_size && bcmdec->insert_pps) {
		sts = decif_send_buffer(&bcmdec->decif, bcmdec->avcc_params.sps_pps_buf,
					bcmdec->avcc_params.pps_size, 0, bcmdec->proc_in_flags);
#ifdef FILE_DUMP__
		bytes_written = fwrite(bcmdec->avcc_params.sps_pps_buf, sizeof(unsigned char),
				       bcmdec->avcc_params.pps_size, bcmdec->fhnd);
#endif
		bcmdec->insert_pps = FALSE;
		bcmdec->insert_start_code = TRUE;
	}
	if (bcmdec->insert_start_code) {
		sts = bcmdec_insert_startcode(bcmdec, buf, bcmdec->dest_buf, &size);
		if (sts != BC_STS_SUCCESS) {
			GST_ERROR_OBJECT(bcmdec, "startcode insertion failed sts = %d\n", sts);
			bcmdec->avcc_params.inside_buffer = TRUE;
			bcmdec->avcc_params.consumed_offset = 0;
			bcmdec->avcc_params.strtcode_offset = 0;
			bcmdec->avcc_params.nal_sz = 0;
		}
	}
	if (bcmdec->dest_buf && bcmdec->avcc_params.nal_size_bytes == 2) {
		pbuffer = bcmdec->dest_buf;
	} else {
		pbuffer = GST_BUFFER_DATA (buf);
		size = GST_BUFFER_SIZE(buf);
	}

	if (bcmdec->enable_spes) {
		/* FIXME: jarod: um... do nothing if true? odd... */
		if (parse_find_strt_code(&bcmdec->parse, bcmdec->input_format, pbuffer, size, &offset)) {

		} else {
			offset = 0;
			tCurrent = 0;
		}
	}
	if (GST_FLOW_OK != bcmdec_send_buff_detect_error(bcmdec, buf, pbuffer, size, offset, tCurrent, bcmdec->proc_in_flags)) {
		gst_buffer_unref(buf);
		return GST_FLOW_ERROR;
	}

#ifdef FILE_DUMP__
	bytes_written = fwrite(GST_BUFFER_DATA(buf), sizeof(unsigned char), GST_BUFFER_SIZE(buf), bcmdec->fhnd);
#endif

	gst_buffer_unref(buf);
	return GST_FLOW_OK;
}

static gboolean gst_bcmdec_src_event(GstPad *pad, GstEvent *event)
{
	gboolean result;
	GstBcmDec *bcmdec;

	bcmdec = GST_BCMDEC(GST_OBJECT_PARENT(pad));

	result = gst_pad_push_event(bcmdec->sinkpad, event);

	return result;
}

static gboolean bcmdec_negotiate_format(GstBcmDec *bcmdec)
{
	GstCaps *caps;
	guint32 fourcc;
	gboolean result;
	guint num = (guint)(bcmdec->output_params.framerate * 1000);
	guint den = 1000;
	GstStructure *s1;
	const GValue *framerate_value;

#ifdef YV12__
	fourcc = GST_STR_FOURCC("YV12");
#else
	fourcc = GST_STR_FOURCC("YUY2");
#endif
	GST_INFO_OBJECT(bcmdec, "framerate = %f", bcmdec->output_params.framerate);

	caps = gst_caps_new_simple("video/x-raw-yuv",
				   "format", GST_TYPE_FOURCC, fourcc,
				   "width", G_TYPE_INT, bcmdec->output_params.width,
				   "height", G_TYPE_INT, bcmdec->output_params.height,
				   "pixel-aspect-ratio", GST_TYPE_FRACTION, bcmdec->output_params.aspectratio_x,
				   bcmdec->output_params.aspectratio_y,
				   "framerate", GST_TYPE_FRACTION, num, den, NULL);

	result = gst_pad_set_caps(bcmdec->srcpad, caps);
	GST_INFO_OBJECT(bcmdec, "gst_bcmdec_negotiate_format %d", result);

	if (bcmdec->output_params.clr_space == MODE422_YUY2) {
		bcmdec->output_params.y_size = bcmdec->output_params.width * bcmdec->output_params.height * BUF_MULT;
		if (bcmdec->interlace)
			bcmdec->output_params.y_size /= 2;
		bcmdec->output_params.uv_size = 0;
		GST_INFO_OBJECT(bcmdec, "YUY2 set on caps");
	} else if (bcmdec->output_params.clr_space == MODE420) {
		bcmdec->output_params.y_size = bcmdec->output_params.width * bcmdec->output_params.height;
		bcmdec->output_params.uv_size = bcmdec->output_params.width * bcmdec->output_params.height / 2;
#ifdef YV12__
		if (bcmdec->interlace) {
			bcmdec->output_params.y_size = bcmdec->output_params.width * bcmdec->output_params.height / 2;
			bcmdec->output_params.uv_size = bcmdec->output_params.y_size / 2;
		}
#endif
		GST_INFO_OBJECT(bcmdec, "420 set on caps");
	}

	s1 = gst_caps_get_structure(caps, 0);

	framerate_value = gst_structure_get_value(s1, "framerate");
	if (framerate_value != NULL) {
		num = gst_value_get_fraction_numerator(framerate_value);
		den = gst_value_get_fraction_denominator(framerate_value);
		GST_INFO_OBJECT(bcmdec, "framerate = %f rate_num %d rate_den %d", bcmdec->output_params.framerate, num, den);
	} else {
		GST_INFO_OBJECT(bcmdec, "failed to get framerate_value");
	}

	framerate_value = gst_structure_get_value (s1, "pixel-aspect-ratio");
	if (framerate_value) {
		num = gst_value_get_fraction_numerator(framerate_value);
		den = gst_value_get_fraction_denominator(framerate_value);
		GST_INFO_OBJECT(bcmdec, "pixel-aspect-ratio_x = %d y %d ", num, den);
	} else {
		GST_INFO_OBJECT(bcmdec, "failed to get par");
	}

	gst_caps_unref(caps);

	return result;
}

static gboolean bcmdec_process_play(GstBcmDec *bcmdec)
{
	BC_STATUS sts = BC_STS_SUCCESS;

	sts = decif_prepare_play(&bcmdec->decif,bcmdec->input_format);
	if (sts == BC_STS_SUCCESS) {
		GST_INFO_OBJECT(bcmdec, "prepare play success");
	} else {
		GST_ERROR_OBJECT(bcmdec, "prepare play failed");
		bcmdec->streaming = FALSE;
		return FALSE;
	}

	decif_set422_mode(&bcmdec->decif, BUF_MODE);

	sts = decif_start_play(&bcmdec->decif);
	if (sts == BC_STS_SUCCESS) {
		GST_INFO_OBJECT(bcmdec, "start play success");
		bcmdec->streaming = TRUE;
	} else {
		GST_ERROR_OBJECT(bcmdec, "start play failed");
		bcmdec->streaming = FALSE;
		return FALSE;
	}

	if (sem_post(&bcmdec->play_event) == -1)
		GST_ERROR_OBJECT(bcmdec, "sem_post failed");

	if (sem_post(&bcmdec->push_start_event) == -1)
		GST_ERROR_OBJECT(bcmdec, "push_start post failed");

	return TRUE;
}

static GstStateChangeReturn gst_bcmdec_change_state(GstElement *element, GstStateChange transition)
{
	GstStateChangeReturn result = GST_STATE_CHANGE_SUCCESS;
	GstBcmDec *bcmdec = GST_BCMDEC(element);
	BC_STATUS sts = BC_STS_SUCCESS;
	GstClockTime clock_time;
	GstClockTime base_clock_time;

	switch (transition) {
	case GST_STATE_CHANGE_NULL_TO_READY:
		GST_DEBUG_OBJECT(bcmdec, "State change from NULL_TO_READY");
		if (bcmdec_mul_inst_cor(bcmdec)) {
			sts = decif_open(&bcmdec->decif);
			if (sts == BC_STS_SUCCESS) {
				GST_INFO_OBJECT(bcmdec, "dev open success");
				printf("OPEN success\n");
				parse_init(&bcmdec->parse);
			} else {
				GST_ERROR_OBJECT(bcmdec, "dev open failed %d",sts);
				GST_ERROR_OBJECT(bcmdec, "dev open failed...ret GST_STATE_CHANGE_FAILURE");
				return GST_STATE_CHANGE_FAILURE;
			}
		} else {
			GST_ERROR_OBJECT(bcmdec, "dev open failed...ret GST_STATE_CHANGE_FAILURE");
			return GST_STATE_CHANGE_FAILURE;
		}

		if (!bcmdec_start_recv_thread(bcmdec)) {
			GST_ERROR_OBJECT(bcmdec, "GST_STATE_CHANGE_NULL_TO_READY -failed");
			return GST_STATE_CHANGE_FAILURE;
		}
		if (!bcmdec_start_push_thread(bcmdec)) {
			GST_ERROR_OBJECT(bcmdec, "GST_STATE_CHANGE_READY_TO_THREAD -failed");
			return GST_STATE_CHANGE_FAILURE;
		}
		if (!bcmdec_start_get_rbuf_thread(bcmdec)) {
			GST_ERROR_OBJECT(bcmdec, "GST_STATE_CHANGE_THREAD_TO_RBUF -failed");
			return GST_STATE_CHANGE_FAILURE;
		}

		break;

	case GST_STATE_CHANGE_READY_TO_PAUSED:
		bcmdec->play_pending = TRUE;
		GST_INFO_OBJECT(bcmdec, "GST_STATE_CHANGE_READY_TO_PAUSED");
		break;
	case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		GST_INFO_OBJECT(bcmdec, "GST_STATE_CHANGE_PAUSED_TO_PLAYING");
		bcmdec->gst_clock = gst_element_get_clock(element);
		if (bcmdec->gst_clock) {
			//printf("clock available %p\n",bcmdec->gst_clock);
			base_clock_time = gst_element_get_base_time(element);
			//printf("base clock time %lld\n",base_clock_time);
			clock_time = gst_clock_get_time(bcmdec->gst_clock);
			//printf(" clock time %lld\n",clock_time);
		}
		break;

	case GST_STATE_CHANGE_PAUSED_TO_READY:
		GST_INFO_OBJECT(bcmdec, "GST_STATE_CHANGE_PAUSED_TO_READY");
		bcmdec->streaming = FALSE;
		sts = decif_flush_dec(&bcmdec->decif, 2);
		if (sts != BC_STS_SUCCESS)
			GST_ERROR_OBJECT(bcmdec, "Dec flush failed %d",sts);
		break;

	default:
		GST_INFO_OBJECT(bcmdec, "default %d", transition);
		break;
	}
	result = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
	if (result == GST_STATE_CHANGE_FAILURE) {
		GST_ERROR_OBJECT(bcmdec, "parent calss state change failed");
		return result;
	}

	switch (transition) {

	case GST_STATE_CHANGE_PAUSED_TO_READY:
		if (!bcmdec->play_pending) {
			sts = decif_stop(&bcmdec->decif);
			if (sts == BC_STS_SUCCESS) {
				if (!bcmdec->silent)
					GST_INFO_OBJECT(bcmdec, "stop play success");
				g_inst_sts->cur_decode = UNKNOWN;
				g_inst_sts->rendered_frames = 0;
				GST_INFO_OBJECT(bcmdec, "cur_dec set to UNKNOWN");

			} else {
				GST_ERROR_OBJECT(bcmdec, "stop play failed %d", sts);
			}
		}
		break;

	case GST_STATE_CHANGE_READY_TO_NULL:
		GST_INFO_OBJECT(bcmdec, "GST_STATE_CHANGE_READY_TO_NULL");
		sts = gst_bcmdec_cleanup(bcmdec);
		if (sts == BC_STS_SUCCESS)
			GST_INFO_OBJECT(bcmdec, "dev close success");
		else
			GST_ERROR_OBJECT(bcmdec, "dev close failed %d", sts);
		break;
	case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		GST_INFO_OBJECT(bcmdec, "GST_STATE_CHANGE_PLAYING_TO_PAUSED");
		break;

	default:
		GST_INFO_OBJECT(bcmdec, "default %d", transition);
		break;
	}

	return result;
}


GstClockTime gst_get_current_timex (void)
{
	GTimeVal tv;

	g_get_current_time(&tv);
	return GST_TIMEVAL_TO_TIME(tv);
}

clock_t bcm_get_tick_count()
{
	tms tm;
	return times(&tm);
}

static gboolean bcmdec_get_buffer(GstBcmDec *bcmdec, GstBuffer **obuf)
{
	GstFlowReturn ret;

	ret = gst_pad_alloc_buffer_and_set_caps(bcmdec->srcpad,
						GST_BUFFER_OFFSET_NONE,
						bcmdec->output_params.width * bcmdec->output_params.height * BUF_MULT,
						GST_PAD_CAPS (bcmdec->srcpad), obuf);
	if (ret != GST_FLOW_OK) {
		GST_ERROR_OBJECT(bcmdec, "gst_pad_alloc_buffer_and_set_caps failed %d ",ret);
		return FALSE;
	}

	if (((uintptr_t)GST_BUFFER_DATA(*obuf)) % 4)
		GST_INFO_OBJECT(bcmdec, "buf is not aligned");

	return TRUE;
}


static void bcmdec_init_procout(GstBcmDec *bcmdec,BC_DTS_PROC_OUT* pout, guint8* buf)
{
	//if (bcmdec->format_reset)
	{
		memset(pout,0,sizeof(BC_DTS_PROC_OUT));
		pout->PicInfo.width = bcmdec->output_params.width ;
		pout->PicInfo.height = bcmdec->output_params.height;
		pout->YbuffSz = bcmdec->output_params.y_size / 4;
		pout->UVbuffSz = bcmdec->output_params.uv_size / 4;

		pout->PoutFlags = BC_POUT_FLAGS_SIZE ;
#ifdef YV12__
		pout->PoutFlags |= BC_POUT_FLAGS_YV12;
#endif
		if (bcmdec->interlace)
			pout->PoutFlags |= BC_POUT_FLAGS_INTERLACED;
		if ((bcmdec->output_params.stride) || (bcmdec->interlace)) {
			pout->PoutFlags |= BC_POUT_FLAGS_STRIDE ;
			if (bcmdec->interlace)
				pout->StrideSz = bcmdec->output_params.width + bcmdec->output_params.stride;
			else
				pout->StrideSz = bcmdec->output_params.stride;
		}
//		bcmdec->format_reset = FALSE;
	}
	pout->PoutFlags = pout->PoutFlags & 0xff;
	pout->Ybuff = (uint8_t*)buf;

	if (pout->UVbuffSz) {
		if (bcmdec->interlace) {
			pout->UVbuff = buf + bcmdec->output_params.y_size * 2;
			if (bcmdec->sec_field) {
				pout->Ybuff += bcmdec->output_params.width;
				pout->UVbuff += bcmdec->output_params.width / 2;
			}
		} else {
			pout->UVbuff = buf + bcmdec->output_params.y_size;
		}
	} else {
		pout->UVbuff = NULL;
		if (bcmdec->interlace) {
			if (bcmdec->sec_field)
				pout->Ybuff +=  bcmdec->output_params.width * 2;
		}
	}

	return;
}

static void bcmdec_set_framerate(GstBcmDec * bcmdec,guint32 resolution)
{
	gdouble framerate;

	bcmdec->interlace = FALSE;

	switch (resolution) {
	case vdecRESOLUTION_480p0:
		GST_INFO_OBJECT(bcmdec, "host frame rate 480p0");
		framerate = bcmdec->input_framerate ? bcmdec->input_framerate : 60;
		break;
	case vdecRESOLUTION_576p0:
		GST_INFO_OBJECT(bcmdec, "host frame rate 576p0");
		framerate = bcmdec->input_framerate ? bcmdec->input_framerate : 25;
		break;
	case vdecRESOLUTION_720p0:
		GST_INFO_OBJECT(bcmdec, "host frame rate 720p0");
		framerate = bcmdec->input_framerate ? bcmdec->input_framerate : 60;
		break;
	case vdecRESOLUTION_1080p0:
		GST_INFO_OBJECT(bcmdec, "host frame rate 1080p0");
		framerate = bcmdec->input_framerate ? bcmdec->input_framerate : 23.976;
		break;
	case vdecRESOLUTION_480i0:
		GST_INFO_OBJECT(bcmdec, "host frame rate 480i0");
		framerate = bcmdec->input_framerate ? bcmdec->input_framerate : 59.94;
		bcmdec->interlace = TRUE;
		break;
	case vdecRESOLUTION_1080i0:
		GST_INFO_OBJECT(bcmdec, "host frame rate 1080i0");
		framerate = bcmdec->input_framerate ? bcmdec->input_framerate : 59.94;
		bcmdec->interlace = TRUE;
		break;
	case vdecRESOLUTION_1080p23_976:
		framerate = 23.976;
		break;
	case vdecRESOLUTION_1080p29_97 :
		framerate = 29.97;
		break;
	case vdecRESOLUTION_1080p30  :
		framerate = 30;
		break;
	case vdecRESOLUTION_1080p24  :
		framerate = 24;
		break;
	case vdecRESOLUTION_1080p25 :
		framerate = 25;
		break;
	case vdecRESOLUTION_1080i29_97:
		framerate = 59.94;
		bcmdec->interlace = TRUE;
		break;
	case vdecRESOLUTION_1080i25:
		framerate = 50;
		bcmdec->interlace = TRUE;
		break;
	case vdecRESOLUTION_1080i:
		framerate = 59.94;
		bcmdec->interlace = TRUE;
		break;
	case vdecRESOLUTION_720p59_94:
		framerate = 59.94;
		break;
	case vdecRESOLUTION_720p50:
		framerate = 50;
		break;
	case vdecRESOLUTION_720p:
		framerate = 60;
		break;
	case vdecRESOLUTION_720p23_976:
		framerate = 23.976;
		break;
	case vdecRESOLUTION_720p24:
		framerate = 25;
		break;
	case vdecRESOLUTION_720p29_97:
		framerate = 29.97;
		break;
	case vdecRESOLUTION_480i:
		framerate = 59.94;
		bcmdec->interlace = TRUE;
		break;
	case vdecRESOLUTION_NTSC:
		framerate = 60;
		bcmdec->interlace = TRUE;
		break;
	case vdecRESOLUTION_480p:
		framerate = 60;
		break;
	case vdecRESOLUTION_PAL1:
		framerate = 50;
		bcmdec->interlace = TRUE;
		break;
	case vdecRESOLUTION_480p23_976:
		framerate = 23.976;
		break;
	case vdecRESOLUTION_480p29_97:
		framerate = 29.97;
		break;
	case vdecRESOLUTION_576p25:
		framerate = 25;
		break;
	default:
		GST_INFO_OBJECT(bcmdec, "default frame rate %d",resolution);
		framerate = 23.976;
		break;
	}

	bcmdec->output_params.framerate = framerate;

	if (bcmdec->interlace)
		bcmdec->output_params.framerate /= 2;

	GST_INFO_OBJECT(bcmdec, "resolution = %x  interlace = %d", resolution, bcmdec->interlace);

	return;
}

static void bcmdec_set_aspect_ratio(GstBcmDec *bcmdec, BC_PIC_INFO_BLOCK *pic_info)
{
	switch (pic_info->aspect_ratio) {
	case vdecAspectRatioSquare:
		bcmdec->output_params.aspectratio_x = 1;
		bcmdec->output_params.aspectratio_y = 1;
		break;
	case vdecAspectRatio12_11:
		bcmdec->output_params.aspectratio_x = 12;
		bcmdec->output_params.aspectratio_y = 11;
		break;
	case vdecAspectRatio10_11:
		bcmdec->output_params.aspectratio_x = 10;
		bcmdec->output_params.aspectratio_y = 11;
		break;
	case vdecAspectRatio16_11:
		bcmdec->output_params.aspectratio_x = 16;
		bcmdec->output_params.aspectratio_y = 11;
		break;
	case vdecAspectRatio40_33:
		bcmdec->output_params.aspectratio_x = 40;
		bcmdec->output_params.aspectratio_y = 33;
		break;
	case vdecAspectRatio24_11:
		bcmdec->output_params.aspectratio_x = 24;
		bcmdec->output_params.aspectratio_y = 11;
		break;
	case vdecAspectRatio20_11:
		bcmdec->output_params.aspectratio_x = 20;
		bcmdec->output_params.aspectratio_y = 11;
		break;
	case vdecAspectRatio32_11:
		bcmdec->output_params.aspectratio_x = 32;
		bcmdec->output_params.aspectratio_y = 11;
		break;
	case vdecAspectRatio80_33:
		bcmdec->output_params.aspectratio_x = 80;
		bcmdec->output_params.aspectratio_y = 33;
		break;
	case vdecAspectRatio18_11:
		bcmdec->output_params.aspectratio_x = 18;
		bcmdec->output_params.aspectratio_y = 11;
		break;
	case vdecAspectRatio15_11:
		bcmdec->output_params.aspectratio_x = 15;
		bcmdec->output_params.aspectratio_y = 11;
		break;
	case vdecAspectRatio64_33:
		bcmdec->output_params.aspectratio_x = 64;
		bcmdec->output_params.aspectratio_y = 33;
		break;
	case vdecAspectRatio160_99:
		bcmdec->output_params.aspectratio_x = 160;
		bcmdec->output_params.aspectratio_y = 99;
		break;
	case vdecAspectRatio4_3:
		bcmdec->output_params.aspectratio_x = 4;
		bcmdec->output_params.aspectratio_y = 3;
		break;
	case vdecAspectRatio16_9:
		bcmdec->output_params.aspectratio_x = 16;
		bcmdec->output_params.aspectratio_y = 9;
		break;
	case vdecAspectRatio221_1:
		bcmdec->output_params.aspectratio_x = 221;
		bcmdec->output_params.aspectratio_y = 1;
		break;
	case vdecAspectRatioOther:
		bcmdec->output_params.aspectratio_x = pic_info->custom_aspect_ratio_width_height & 0x0000ffff;
		bcmdec->output_params.aspectratio_y = pic_info->custom_aspect_ratio_width_height >> 16;
		break;
	case vdecAspectRatioUnknown:
	default:
		bcmdec->output_params.aspectratio_x = 0;
		bcmdec->output_params.aspectratio_y = 0;
		break;
	}

	if (bcmdec->output_params.aspectratio_x == 0) {
		if (bcmdec->input_par_x == 0) {
			bcmdec->output_params.aspectratio_x = 1;
			bcmdec->output_params.aspectratio_y = 1;
		} else {
			bcmdec->output_params.aspectratio_x = bcmdec->input_par_x;
			bcmdec->output_params.aspectratio_y = bcmdec->input_par_y;
		}
	}

	GST_DEBUG_OBJECT(bcmdec, "dec_par x = %d", bcmdec->output_params.aspectratio_x);
	GST_DEBUG_OBJECT(bcmdec, "dec_par y = %d", bcmdec->output_params.aspectratio_y);
}

static gboolean bcmdec_format_change(GstBcmDec *bcmdec, BC_PIC_INFO_BLOCK *pic_info)
{
	gboolean result = FALSE;
	bcmdec_set_framerate(bcmdec, pic_info->frame_rate);
	if (pic_info->height == 1088)
		pic_info->height = 1080;

	bcmdec->output_params.width  = pic_info->width;
	bcmdec->output_params.height = pic_info->height;

	bcmdec_set_aspect_ratio(bcmdec,pic_info);

	result = bcmdec_negotiate_format(bcmdec);
	if (!bcmdec->silent) {
		if (result)
			GST_INFO_OBJECT(bcmdec, "negotiate_format success");
		else
			GST_ERROR_OBJECT(bcmdec, "negotiate_format failed");
	}
	//bcmdec->format_reset = TRUE;
	return result;
}

static int bcmdec_wait_for_event(GstBcmDec *bcmdec)
{
	int ret = 0, i = 0;
	sem_t *event_list[] = { &bcmdec->play_event, &bcmdec->quit_event };

	while (1) {
		for (i = 0; i < 2; i++) {

			ret = sem_trywait(event_list[i]);
			if (ret == 0) {
				GST_INFO_OBJECT(bcmdec, "event wait over in Rx thread ret = %d",i);
				return i;
			} else if (errno == EINTR) {
				break;
			}
			if (bcmdec->streaming)
				break;
		}
		usleep(10);
	}
}

static void bcmdec_flush_gstbuf_queue(GstBcmDec *bcmdec)
{
	GSTBUF_LIST *gst_queue_element;

	do {
		gst_queue_element = bcmdec_rem_buf(bcmdec);
		if (gst_queue_element) {
			if (gst_queue_element->gstbuf) {
				gst_buffer_unref (gst_queue_element->gstbuf);
				bcmdec_put_que_mem_buf(bcmdec,gst_queue_element);
			}
		} else {
			GST_INFO_OBJECT(bcmdec, "no gst_queue_element");
		}
	} while (gst_queue_element && gst_queue_element->gstbuf);
}

static void * bcmdec_process_push(void *ctx)
{
	GstBcmDec *bcmdec = (GstBcmDec *)ctx;
	GSTBUF_LIST *gst_queue_element = NULL;
	gboolean result = FALSE, done = FALSE;
	struct timespec ts;
	gint ret;

	ts.tv_sec = time(NULL);
	ts.tv_nsec = 30 * 1000000;

	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "process push starting ");

	while (1) {

		if (!bcmdec->recv_thread && !bcmdec->streaming) {
			if (!bcmdec->silent)
				GST_INFO_OBJECT(bcmdec, "process push exiting..");
			break;
		}

		ret = sem_timedwait(&bcmdec->push_start_event, &ts);
		if (ret < 0) {
			if (errno == ETIMEDOUT)
				continue;
			else if (errno == EINTR)
				break;
		} else {
			GST_INFO_OBJECT(bcmdec, "push_start wait over");
			done = FALSE;
		}

		ts.tv_sec = time(NULL) + 1;
		ts.tv_nsec = 0;

		while (bcmdec->streaming && !done) {

			ret = sem_timedwait(&bcmdec->buf_event, &ts);
			if (ret < 0) {
				switch (errno) {
				case ETIMEDOUT:
					if (bcmdec->streaming) {
						continue;
					} else {
						done = TRUE;
						GST_INFO_OBJECT(bcmdec, "TOB ");
						break;
					}
				case EINTR:
					GST_INFO_OBJECT(bcmdec, "Sig interrupt ");
					done = TRUE;
					break;

				default:
					GST_ERROR_OBJECT(bcmdec, "sem wait failed %d ", errno);
					done = TRUE;
					break;
				}
			}
			if (ret == 0) {
				gst_queue_element = bcmdec_rem_buf(bcmdec);
				if (gst_queue_element) {
					if (gst_queue_element->gstbuf) {
						result = gst_pad_push(bcmdec->srcpad, gst_queue_element->gstbuf);
						if (result != GST_FLOW_OK) {
							GST_INFO_OBJECT(bcmdec, "exiting, failed to push sts = %d", result);
							gst_buffer_unref(gst_queue_element->gstbuf);
							done = TRUE;
						} else {
							if (g_inst_sts->rendered_frames < 5) {// || bcmdec->paused == TRUE
								GST_INFO_OBJECT(bcmdec, "PUSHED, Qcnt:%d, Rcnt:%d", bcmdec->gst_que_cnt, bcmdec->gst_rbuf_que_cnt);
							}
							if ((g_inst_sts->rendered_frames++ > THUMBNAIL_FRAMES) && (g_inst_sts->cur_decode != PLAYBACK)) {
								g_inst_sts->cur_decode = PLAYBACK;
								GST_INFO_OBJECT(bcmdec, "cur_dec set to PLAYBACK");
							}
						}
					} else {
						/*exit */ /* NULL value of gstbuf indicates EOS */
						gst_pad_push_event(bcmdec->srcpad, bcmdec->ev_eos);
						gst_event_unref(bcmdec->ev_eos);
						done = TRUE;
						bcmdec->streaming = FALSE;
						GST_INFO_OBJECT(bcmdec, "eos sent, cnt:%d", bcmdec->gst_que_cnt);
					}
					bcmdec_put_que_mem_buf(bcmdec, gst_queue_element);
					//if ((bcmdec->gst_que_cnt < RESUME_THRESHOLD) && (bcmdec->paused )) {
					if ((bcmdec->paused ) && (bcmdec->gst_que_cnt < RESUME_THRESHOLD) && (bcmdec->gst_rbuf_que_cnt > GST_RENDERER_BUF_NORMAL_THRESHOLD)) {
						decif_pause(&bcmdec->decif, FALSE);
						//if (!bcmdec->silent) {
							//GST_INFO_OBJECT(bcmdec, "resumed %d", bcmdec->gst_que_cnt);
							GST_INFO_OBJECT(bcmdec, "resumed by push thread que_cnt:%d, rbuf_que_cnt:%d", bcmdec->gst_que_cnt, bcmdec->gst_rbuf_que_cnt);
						//}
						bcmdec->paused = FALSE;
					}
				}
			}
			if (bcmdec->flushing && bcmdec->push_exit) {
				GST_INFO_OBJECT(bcmdec, "push -flush exit");
				break;
			}
		}
		if (bcmdec->flushing) {
			bcmdec_flush_gstbuf_queue(bcmdec);
			if (sem_post(&bcmdec->push_stop_event) == -1)
				GST_ERROR_OBJECT(bcmdec, "push_stop sem_post failed");
			g_inst_sts->rendered_frames = 0;
		}
	}
	bcmdec_flush_gstbuf_queue(bcmdec);
	GST_DEBUG_OBJECT(bcmdec, "process push exiting.. ");
	pthread_exit((void*)&result);
}

static void * bcmdec_process_output(void *ctx)
{
	BC_DTS_PROC_OUT pout;
	BC_STATUS sts = BC_STS_SUCCESS;
	GstBcmDec *bcmdec = (GstBcmDec *)ctx;
	GstBuffer *gstbuf = NULL;
	gboolean rx_flush = FALSE;
	const guint first_picture = 3;
	guint32 pic_number = 0;
	GstClockTime clock_time = 0;
	gboolean first_frame_after_seek = FALSE;
	GstClockTime cur_stream_time_diff = 0;
	int wait_cnt = 0;

	GSTBUF_LIST *gst_rqueue_element = NULL;

	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "Rx thread started");

	while (1) {
		if (1 == bcmdec_wait_for_event(bcmdec)) {
			if (!bcmdec->silent)
				GST_INFO_OBJECT(bcmdec, "quit event set, exit");
			break;
		}

		GST_INFO_OBJECT(bcmdec, "wait over streaming = %d", bcmdec->streaming);
		while (bcmdec->streaming && !bcmdec->last_picture_set) {
			guint8* data_ptr;

			if (gstbuf == NULL) {
				if (!bcmdec->rbuf_thread_running) {
					if (!bcmdec_get_buffer(bcmdec, &gstbuf)) {
						usleep(30 * 1000);
						continue;
					}
				} else {
					if (gst_rqueue_element) {
						if (gst_rqueue_element->gstbuf)
							gst_buffer_unref(gst_rqueue_element->gstbuf);
						bcmdec_put_que_mem_rbuf(bcmdec, gst_rqueue_element);
						gst_rqueue_element = NULL;
					}

					gst_rqueue_element = bcmdec_rem_rbuf(bcmdec);
					if (!gst_rqueue_element) {
						GST_INFO_OBJECT(bcmdec, "rbuf queue empty");
						usleep(30 * 1000);
						continue;
					}

					gstbuf = gst_rqueue_element->gstbuf;
					if (!gstbuf) {
						bcmdec_put_que_mem_rbuf(bcmdec, gst_rqueue_element);
						gst_rqueue_element = NULL;
						usleep(30 * 1000);
						continue;
					}
				}
			}

			data_ptr = GST_BUFFER_DATA(gstbuf);
			//GST_INFO_OBJECT(bcmdec, "process_output data_ptr:0x%0x", data_ptr);

			bcmdec_init_procout(bcmdec, &pout, data_ptr);
			rx_flush = TRUE;
			pout.PicInfo.picture_number = 0;
			sts = DtsProcOutput(bcmdec->decif.hdev, PROC_TIMEOUT,&pout);

			switch (sts) {
			case BC_STS_FMT_CHANGE:
				if (!bcmdec->silent)
					GST_INFO_OBJECT(bcmdec, "procout ret FMT");
				if ((pout.PoutFlags & BC_POUT_FLAGS_PIB_VALID) &&
				    (pout.PoutFlags & BC_POUT_FLAGS_FMT_CHANGE)) {
					if (bcmdec_format_change(bcmdec, &pout.PicInfo)) {
						GST_INFO_OBJECT(bcmdec, "format chnage success");
						bcmdec->frame_time = (GstClockTime)(UNITS / bcmdec->output_params.framerate);
						bcmdec->last_spes_time  = 0;
						bcmdec->prev_clock_time = 0;
						cur_stream_time_diff    = 0;
						first_frame_after_seek  = TRUE;
					} else {
						GST_INFO_OBJECT(bcmdec, "format chnage failed");
					}
				}
				gst_buffer_unref(gstbuf);
				gstbuf = NULL;
				bcmdec->sec_field = FALSE;

				decif_pause(&bcmdec->decif, TRUE);
				bcmdec->paused = TRUE;

				if (sem_post(&bcmdec->rbuf_start_event) == -1)
					GST_ERROR_OBJECT(bcmdec, "rbuf sem_post failed");

				//should modify to wait event
				wait_cnt = 0;
				while (!bcmdec->rbuf_thread_running && (wait_cnt < 5000)) {
					usleep(1000);
					wait_cnt++;
				}
				GST_INFO_OBJECT(bcmdec, "format chnage start wait_cnt:%d", wait_cnt);

				break;
			case BC_STS_SUCCESS:
				if (!(pout.PoutFlags & BC_POUT_FLAGS_PIB_VALID)) {
					if (!bcmdec->silent)
						GST_INFO_OBJECT(bcmdec, "procout ret PIB miss %d", pout.PicInfo.picture_number - 3);
					continue;
				}

				pic_number = pout.PicInfo.picture_number - first_picture;

				if (bcmdec->flushing) {
					GST_INFO_OBJECT(bcmdec, "flushing discard, pic = %d",pout.PicInfo.picture_number - 3);
					continue;
				}
				if (bcmdec->prev_pic + 1 < pic_number) {
					if (!bcmdec->silent)
						GST_INFO_OBJECT(bcmdec, "pic_no = %d, prev = %d", pic_number, bcmdec->prev_pic);
				}
				//printf("pic_no = %d\n", pout.PicInfo.picture_number - 3);
				if ((bcmdec->prev_pic == pic_number) && (bcmdec->ses_nbr  == pout.PicInfo.sess_num) && !bcmdec->interlace) {
					if (!bcmdec->silent)
						GST_INFO_OBJECT(bcmdec, "rp");

					//printf("rp ses_nbr = %d, pic = %d\n",pout.PicInfo.sess_num,pout.PicInfo.picture_number - 3);
					if (!(pout.PicInfo.flags &  VDEC_FLAG_LAST_PICTURE))
						continue;
				}
				if (bcmdec->ses_nbr  == pout.PicInfo.sess_num) {
					if (bcmdec->ses_change) {
						GST_INFO_OBJECT(bcmdec, "discard old ses nbr picture , old = %d new = %d pic = %d",
								bcmdec->ses_nbr, pout.PicInfo.sess_num, pic_number);
						continue;
					}
				} else {
					GST_INFO_OBJECT(bcmdec, "session changed old ses nbr picture , old = %d new = %d pic = %d",
							bcmdec->ses_nbr, pout.PicInfo.sess_num, pic_number);
					bcmdec->ses_nbr = pout.PicInfo.sess_num;
					bcmdec->ses_change = FALSE;
					first_frame_after_seek = TRUE;
				}

				if (!bcmdec->interlace || bcmdec->sec_field) {
					GST_BUFFER_OFFSET(gstbuf) = 0;

					GST_BUFFER_TIMESTAMP(gstbuf) = bcmdec_get_time_stamp(bcmdec, pic_number, pout.PicInfo.timeStamp);
					GST_BUFFER_DURATION(gstbuf)  = bcmdec->frame_time;
					if (bcmdec->gst_clock) {
						clock_time = gst_clock_get_time(bcmdec->gst_clock);
						if (first_frame_after_seek) {
							bcmdec->prev_clock_time = clock_time;
							first_frame_after_seek = FALSE;
							cur_stream_time_diff = 0;
						}
						if (bcmdec->prev_clock_time > clock_time)
							bcmdec->prev_clock_time = 0;
						cur_stream_time_diff += clock_time - bcmdec->prev_clock_time;
						bcmdec->cur_stream_time = cur_stream_time_diff + bcmdec->base_clock_time;
						bcmdec->prev_clock_time = clock_time;
						//printf("spes ts = %lld  clo = %lld  sttime = %lld picno = %d\n",pout.PicInfo.timeStamp/1000000,clock_time/1000000,
							//  bcmdec->cur_stream_time/1000000,pic_number );
						if ((bcmdec->last_spes_time < bcmdec->cur_stream_time) &&
						    (!bcmdec->catchup_on) && (pout.PicInfo.timeStamp)) {
							bcmdec->catchup_on = TRUE;
							decif_decode_catchup(&bcmdec->decif, TRUE);
						} else if (bcmdec->catchup_on) {
							decif_decode_catchup(&bcmdec->decif, FALSE);
							bcmdec->catchup_on = FALSE;
						}
					}
				}

				GST_BUFFER_SIZE(gstbuf) = bcmdec->output_params.width * bcmdec->output_params.height * BUF_MULT;

				if (!bcmdec->interlace || bcmdec->sec_field) {
					GSTBUF_LIST *gst_queue_element = bcmdec_get_que_mem_buf(bcmdec);
					if (gst_queue_element) {
						gst_queue_element->gstbuf = gstbuf;
						bcmdec_ins_buf(bcmdec, gst_queue_element);
						bcmdec->prev_pic = pic_number;

						if (gst_rqueue_element) {
							gst_rqueue_element->gstbuf = NULL;
							bcmdec_put_que_mem_rbuf(bcmdec, gst_rqueue_element);
							gst_rqueue_element = NULL;
						}

					} else {
						GST_ERROR_OBJECT(bcmdec, "mem pool is full");//pending error recovery
					}
					if ((bcmdec->gst_que_cnt > PAUSE_THRESHOLD || bcmdec->gst_rbuf_que_cnt < GST_RENDERER_BUF_LOW_THRESHOLD ) &&
					    !(bcmdec->paused) && !bcmdec->flushing) {
						decif_pause(&bcmdec->decif, TRUE);
						bcmdec->paused = TRUE;
						//if (!bcmdec->silent)
							GST_DEBUG_OBJECT(bcmdec, "paused frame_que:%d, rbuf_que:%d",
									 bcmdec->gst_que_cnt, bcmdec->gst_rbuf_que_cnt);
					}
					gstbuf = NULL;
					bcmdec->sec_field = FALSE;
		    			if (!(bcmdec->prev_pic % 100))
						GST_DEBUG_OBJECT(bcmdec, ".");
				} else {
					bcmdec->sec_field = TRUE;
				}
				if (pout.PicInfo.flags & VDEC_FLAG_LAST_PICTURE) {
					GSTBUF_LIST *gst_queue_element = bcmdec_get_que_mem_buf(bcmdec);
					if (gst_queue_element) {
						gst_queue_element->gstbuf = NULL;
						bcmdec_ins_buf(bcmdec, gst_queue_element);
					} else {
						GST_INFO_OBJECT(bcmdec, "queue elemnt get failed");
					}
					GST_INFO_OBJECT(bcmdec, "last picture set ");
					bcmdec->last_picture_set = TRUE;
				}
				break;

			case BC_STS_TIMEOUT:
				GST_INFO_OBJECT(bcmdec, "procout timeout QCnt:%d, RCnt:%d, Paused:%d",
						bcmdec->gst_que_cnt, bcmdec->gst_rbuf_que_cnt, bcmdec->paused);
				continue;
				break;
			case BC_STS_IO_XFR_ERROR:
				GST_INFO_OBJECT(bcmdec, "procout xfer error");
				continue;
				break;
			case BC_STS_IO_USER_ABORT:
			case BC_STS_IO_ERROR:
				bcmdec->streaming = FALSE;
				GST_INFO_OBJECT(bcmdec, "ABORT sts = %d", sts);
				if (gstbuf) {
					gst_buffer_unref(gstbuf);
					gstbuf = NULL;
				}
				break;
			default:
				GST_INFO_OBJECT(bcmdec, "unhandled status from Procout sts %d",sts);
				if (gstbuf) {
					gst_buffer_unref(gstbuf);
					gstbuf = NULL;
				}
				break;
			}
		}
		if (gstbuf) {
			gst_buffer_unref(gstbuf);
			gstbuf = NULL;
		}
		if (gst_rqueue_element) {
			bcmdec_put_que_mem_rbuf(bcmdec, gst_rqueue_element);
			gst_rqueue_element = NULL;
		}
		if (rx_flush) {
			if (!bcmdec->flushing) {
				GST_INFO_OBJECT(bcmdec, "DtsFlushRxCapture called");
				sts = decif_flush_rxbuf(&bcmdec->decif, FALSE);
				if (sts != BC_STS_SUCCESS)
					GST_INFO_OBJECT(bcmdec, "DtsFlushRxCapture failed");
			}
			rx_flush = FALSE;
			if (bcmdec->flushing) {
				if (sem_post(&bcmdec->recv_stop_event) == -1)
					GST_ERROR_OBJECT(bcmdec, "recv_stop sem_post failed");
			}
		}
	}
	GST_INFO_OBJECT(bcmdec, "Rx thread exiting ..");
	pthread_exit((void*)&sts);
}

static gboolean bcmdec_start_push_thread(GstBcmDec *bcmdec)
{
	gboolean result = TRUE;
	pthread_attr_t thread_attr;
	gint ret = 0;

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&bcmdec->push_thread, &thread_attr, bcmdec_process_push, bcmdec);
	pthread_attr_destroy(&thread_attr);

	if (!bcmdec->push_thread) {
		GST_ERROR_OBJECT(bcmdec, "Failed to create PushThread");
		result = FALSE;
	} else {
		GST_INFO_OBJECT(bcmdec, "Success to create PushThread");
	}

	ret = sem_init(&bcmdec->buf_event, 0, 0);
	if (ret != 0) {
		GST_ERROR_OBJECT(bcmdec, "play event init failed");
		result = FALSE;
	}

	ret = sem_init(&bcmdec->push_start_event, 0, 0);
	if (ret != 0) {
		GST_ERROR_OBJECT(bcmdec, "play event init failed");
		result = FALSE;
	}

	ret = sem_init(&bcmdec->push_stop_event, 0, 0);
	if (ret != 0) {
		GST_ERROR_OBJECT(bcmdec, "push_stop event init failed");
		result = FALSE;
	}

	return result;
}

static gboolean bcmdec_start_recv_thread(GstBcmDec *bcmdec)
{
	gboolean result = TRUE;
	gint ret = 0;
	pthread_attr_t thread_attr;

	if (!bcmdec_alloc_mem_buf_que_pool(bcmdec))
		GST_ERROR_OBJECT(bcmdec, "pool alloc failed/n");

	ret = sem_init(&bcmdec->play_event, 0, 0);
	if (ret != 0) {
		GST_ERROR_OBJECT(bcmdec, "play event init failed");
		result = FALSE;
	}

	ret = sem_init(&bcmdec->quit_event, 0, 0);
	if (ret != 0) {
		GST_ERROR_OBJECT(bcmdec, "play event init failed");
		result = FALSE;
	}

	ret = sem_init(&bcmdec->recv_stop_event, 0, 0);
	if (ret != 0) {
		GST_ERROR_OBJECT(bcmdec, "recv_stop event init failed");
		result = FALSE;
	}

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&bcmdec->recv_thread, &thread_attr, bcmdec_process_output, bcmdec);
	pthread_attr_destroy(&thread_attr);

	if (!bcmdec->recv_thread) {
		GST_ERROR_OBJECT(bcmdec, "Failed to create RxThread");
		result = FALSE;
	} else {
		GST_INFO_OBJECT(bcmdec, "Success to create RxThread");
	}

	return result;
}

static GstClockTime bcmdec_get_time_stamp(GstBcmDec *bcmdec, guint32 pic_no, GstClockTime spes_time)
{
	GstClockTime time_stamp = 0;
	GstClockTime frame_time = (GstClockTime)(UNITS / bcmdec->output_params.framerate);

	if (bcmdec->enable_spes) {
		if (spes_time) {
			time_stamp = spes_time ;
			if (bcmdec->spes_frame_cnt && bcmdec->last_output_spes_time) {
				bcmdec->frame_time = (time_stamp - bcmdec->last_output_spes_time) / bcmdec->spes_frame_cnt;
				bcmdec->spes_frame_cnt = 0;
			}
			if (bcmdec->frame_time > 0)
				frame_time = bcmdec->frame_time;
			bcmdec->spes_frame_cnt++;
			bcmdec->last_output_spes_time = bcmdec->last_spes_time = time_stamp;
		} else {
			bcmdec->last_spes_time += frame_time;
			time_stamp = bcmdec->last_spes_time;
			bcmdec->spes_frame_cnt++;
		}
	} else {
		time_stamp = (GstClockTime)(bcmdec->base_time + frame_time * pic_no);
	}

	if (!bcmdec->enable_spes) {
		if (bcmdec->interlace) {
			if (bcmdec->prev_pic == pic_no)
				bcmdec->rpt_pic_cnt++;
			time_stamp += bcmdec->rpt_pic_cnt * frame_time;
		}
	}
	//printf("output ts %llu  picn = %d  rp_cnt = %d\n",time_stamp/1000000,pic_no,bcmdec->rpt_pic_cnt);

	return time_stamp;
}

static void bcmdec_process_flush_stop(GstBcmDec *bcmdec)
{
	BC_STATUS sts = BC_STS_SUCCESS;

	bcmdec->ses_change  = TRUE;
	bcmdec->base_time   = 0;
	bcmdec->flushing    = FALSE;
	bcmdec->streaming   = TRUE;
	bcmdec->rpt_pic_cnt = 0;

	if (sem_post(&bcmdec->play_event) == -1)
		GST_ERROR_OBJECT(bcmdec, "sem_post failed");
	sts = decif_pause(&bcmdec->decif, FALSE);
#if 0
	/* FIXME: jarod: why check if not actually doing anything? */
	if (sts != BC_STS_SUCCESS) {
	}
#endif
	bcmdec->push_exit = FALSE;

	if (sem_post(&bcmdec->push_start_event) == -1)
		GST_ERROR_OBJECT(bcmdec, "push_start post failed");

}

static void bcmdec_process_flush_start(GstBcmDec *bcmdec)
{
	gint ret = 1;
	BC_STATUS sts = BC_STS_SUCCESS;
	struct timespec ts;

	ts.tv_sec=time(NULL) + 5;
	ts.tv_nsec = 0;

	bcmdec->flushing  = TRUE;
	bcmdec->streaming = FALSE;

	ret = sem_timedwait(&bcmdec->recv_stop_event, &ts);
	if (ret < 0) {
		switch (errno) {
		case ETIMEDOUT:
			GST_INFO_OBJECT(bcmdec, "recv_stop_event sig timed out ");
			break;
		case EINTR:
			GST_INFO_OBJECT(bcmdec, "Sig interrupt ");
			break;
		default:
			GST_ERROR_OBJECT(bcmdec, "sem wait failed %d ",errno);
			break;
		}
	}

	bcmdec->push_exit = TRUE;

	sts = decif_pause(&bcmdec->decif, TRUE);
#if 0
	/* FIXME: jarod: why check if not actually doing anything? */
	if (sts != BC_STS_SUCCESS) {
	}
#endif

	ret = sem_timedwait(&bcmdec->push_stop_event, &ts);
	if (ret < 0) {
		switch (errno) {
		case ETIMEDOUT:
			GST_INFO_OBJECT(bcmdec, "push_stop_event sig timed out ");
			break;
		case EINTR:
			GST_INFO_OBJECT(bcmdec, "Sig interrupt ");
			break;
		default:
			GST_ERROR_OBJECT(bcmdec, "sem wait failed %d ",errno);
			break;
		}
	}
	sts = decif_flush_dec(&bcmdec->decif, 2);
	if (sts != BC_STS_SUCCESS)
		GST_ERROR_OBJECT(bcmdec, "flush_dec failed");
}

static BC_STATUS gst_bcmdec_cleanup(GstBcmDec *bcmdec)
{
	BC_STATUS sts = BC_STS_SUCCESS;
	int ret = 1;

	GST_INFO_OBJECT(bcmdec, "gst_bcmdec_cleanup - enter");
	bcmdec->streaming = FALSE;

	if (bcmdec->get_rbuf_thread) {
		GST_INFO_OBJECT(bcmdec, "gst_bcmdec_cleanup - post quit_event");
		if (sem_post(&bcmdec->rbuf_stop_event) == -1)
			GST_ERROR_OBJECT(bcmdec, "sem_post failed");
		GST_INFO_OBJECT(bcmdec, "waiting for get_rbuf_thread exit");
		ret = pthread_join(bcmdec->get_rbuf_thread, NULL);
		GST_INFO_OBJECT(bcmdec, "get_rbuf_thread exit - %d errno = %d", ret, errno);
		bcmdec->get_rbuf_thread = 0;
	}

	if (bcmdec->recv_thread) {
		GST_INFO_OBJECT(bcmdec, "gst_bcmdec_cleanup - post quit_event");
		if (sem_post(&bcmdec->quit_event) == -1)
			GST_ERROR_OBJECT(bcmdec, "sem_post failed");
		GST_INFO_OBJECT(bcmdec, "waiting for rec_thread exit");
		ret = pthread_join(bcmdec->recv_thread, NULL);
		GST_INFO_OBJECT(bcmdec, "thread exit - %d errno = %d", ret, errno);
		bcmdec->recv_thread = 0;
	}

	if (bcmdec->push_thread) {
		GST_INFO_OBJECT(bcmdec, "waiting for push_thread exit");
		ret = pthread_join(bcmdec->push_thread, NULL);
		GST_INFO_OBJECT(bcmdec, "push_thread exit - %d errno = %d", ret, errno);
		bcmdec->push_thread = 0;
	}

	bcmdec_release_mem_buf_que_pool(bcmdec);
	bcmdec_release_mem_rbuf_que_pool(bcmdec);

	if (bcmdec->decif.hdev)
		sts = decif_close(&bcmdec->decif);

	sem_destroy(&bcmdec->quit_event);
	sem_destroy(&bcmdec->play_event);
	sem_destroy(&bcmdec->push_start_event);
	sem_destroy(&bcmdec->buf_event);
	sem_destroy(&bcmdec->rbuf_start_event);
	sem_destroy(&bcmdec->rbuf_stop_event);
	sem_destroy(&bcmdec->rbuf_ins_event);
	sem_destroy(&bcmdec->push_stop_event);
	sem_destroy(&bcmdec->recv_stop_event);

	pthread_mutex_destroy(&bcmdec->gst_buf_que_lock);
	pthread_mutex_destroy(&bcmdec->gst_rbuf_que_lock);
	//pthread_mutex_destroy(&bcmdec->fn_lock);
	if (bcmdec->avcc_params.sps_pps_buf) {
		free(bcmdec->avcc_params.sps_pps_buf);
		bcmdec->avcc_params.sps_pps_buf = NULL;
	}

	if (bcmdec->dest_buf) {
		free(bcmdec->dest_buf);
		bcmdec->dest_buf = NULL;
	}

#ifdef _WMV_FILE_HANDLING
	if (bcmdec->vc1_dest_buffer) {
		free(bcmdec->vc1_dest_buffer);
		bcmdec->vc1_dest_buffer = NULL;
	}
#endif
	if (bcmdec->gst_clock) {
		gst_object_unref(bcmdec->gst_clock);
		bcmdec->gst_clock = NULL;
	}

	if (sem_post(&g_inst_sts->inst_ctrl_event) == -1)
		GST_ERROR_OBJECT(bcmdec, "inst_ctrl_event post failed");
	else
		GST_INFO_OBJECT(bcmdec, "inst_ctrl_event posted");

	return sts;
}

static void bcmdec_reset(GstBcmDec * bcmdec)
{
	bcmdec->dec_ready = FALSE;
	bcmdec->streaming = FALSE;
	bcmdec->format_reset = TRUE;
	bcmdec->interlace = FALSE;

	bcmdec->output_params.width = 720;
	bcmdec->output_params.height = 480;
	bcmdec->output_params.framerate = 29;
	bcmdec->output_params.aspectratio_x = 16;
	bcmdec->output_params.aspectratio_y = 9;
	bcmdec->output_params.clr_space = BUF_MODE;
	if (bcmdec->output_params.clr_space == MODE420) { /* MODE420 */
		bcmdec->output_params.y_size = 720 * 480;
		bcmdec->output_params.uv_size = 720 * 480 / 2;
	} else { /* MODE422_YUV */
		bcmdec->output_params.y_size = 720 * 480 * 2;
		bcmdec->output_params.uv_size = 0;
	}

	bcmdec->output_params.stride = 0;

	bcmdec->base_time = 0;
	bcmdec->fhnd = NULL;

	bcmdec->play_pending = FALSE;

	bcmdec->gst_buf_que_hd = NULL;
	bcmdec->gst_buf_que_tl = NULL;
	bcmdec->gst_que_cnt = 0;
	bcmdec->last_picture_set = FALSE;
	bcmdec->gst_buf_que_sz = GST_BUF_LIST_POOL_SZ;
	bcmdec->gst_rbuf_que_sz = GST_RENDERER_BUF_POOL_SZ;
	bcmdec->rbuf_thread_running = FALSE;

	bcmdec->insert_start_code = FALSE;
	bcmdec->avcc_params.sps_pps_buf = NULL;

	bcmdec->input_framerate = 0;
	bcmdec->input_par_x = 0;
	bcmdec->input_par_y = 0;
	bcmdec->prev_pic = -1;

	bcmdec->avcc_params.inside_buffer = TRUE;
	bcmdec->avcc_params.consumed_offset = 0;
	bcmdec->avcc_params.strtcode_offset = 0;
	bcmdec->avcc_params.nal_sz = 0;
	bcmdec->avcc_params.pps_size = 0;
	bcmdec->avcc_params.nal_size_bytes = 4;

	bcmdec->paused = FALSE;

	bcmdec->flushing = FALSE;
	bcmdec->ses_nbr = 0;
	bcmdec->insert_pps = TRUE;
	bcmdec->ses_change = FALSE;

	bcmdec->push_exit = FALSE;

	bcmdec->suspend_mode = FALSE;
	bcmdec->gst_clock = NULL;
	bcmdec->rpt_pic_cnt = 0;

	//bcmdec->enable_spes = FALSE;
	bcmdec->enable_spes = TRUE;
	bcmdec->dest_buf = NULL;
	bcmdec->catchup_on = FALSE;
	bcmdec->last_output_spes_time = 0;

	pthread_mutex_init(&bcmdec->gst_buf_que_lock, NULL);
	pthread_mutex_init(&bcmdec->gst_rbuf_que_lock, NULL);
	//pthread_mutex_init(&bcmdec->fn_lock,NULL);
}

static void bcmdec_put_que_mem_buf(GstBcmDec *bcmdec, GSTBUF_LIST *gst_queue_element)
{
	pthread_mutex_lock(&bcmdec->gst_buf_que_lock);

	gst_queue_element->next = bcmdec->gst_mem_buf_que_hd;
	bcmdec->gst_mem_buf_que_hd = gst_queue_element;

	pthread_mutex_unlock(&bcmdec->gst_buf_que_lock);
}

static GSTBUF_LIST * bcmdec_get_que_mem_buf(GstBcmDec *bcmdec)
{
	GSTBUF_LIST *gst_queue_element = NULL;

	pthread_mutex_lock(&bcmdec->gst_buf_que_lock);

	gst_queue_element = bcmdec->gst_mem_buf_que_hd;
	if (gst_queue_element)
		bcmdec->gst_mem_buf_que_hd = bcmdec->gst_mem_buf_que_hd->next;

	pthread_mutex_unlock(&bcmdec->gst_buf_que_lock);

	return gst_queue_element;
}

static gboolean bcmdec_alloc_mem_buf_que_pool(GstBcmDec *bcmdec)
{
	GSTBUF_LIST *gst_queue_element = NULL;
	guint i = 0;

	bcmdec->gst_mem_buf_que_hd = NULL;
	while (i++<bcmdec->gst_buf_que_sz) {
		if (!(gst_queue_element = (GSTBUF_LIST *)malloc(sizeof(GSTBUF_LIST)))) {
			GST_ERROR_OBJECT(bcmdec, "mempool malloc failed ");
			return FALSE;
		}
		memset(gst_queue_element, 0, sizeof(GSTBUF_LIST));
		bcmdec_put_que_mem_buf(bcmdec, gst_queue_element);
	}

	return TRUE;
}

static gboolean bcmdec_release_mem_buf_que_pool(GstBcmDec *bcmdec)
{
	GSTBUF_LIST *gst_queue_element;
	guint i = 0;

	do {
		gst_queue_element = bcmdec_get_que_mem_buf(bcmdec);
		if (gst_queue_element) {
			free(gst_queue_element);
			i++;
		}
	} while (gst_queue_element);

	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "mem_buf_que_pool released...  %d", i);

	return TRUE;
}

static void bcmdec_ins_buf(GstBcmDec *bcmdec,GSTBUF_LIST	*gst_queue_element)
{
	pthread_mutex_lock(&bcmdec->gst_buf_que_lock);

	if (!bcmdec->gst_buf_que_hd) {
		bcmdec->gst_buf_que_hd = bcmdec->gst_buf_que_tl = gst_queue_element;
	} else {
		bcmdec->gst_buf_que_tl->next = gst_queue_element;
		bcmdec->gst_buf_que_tl = gst_queue_element;
		gst_queue_element->next = NULL;
	}

	bcmdec->gst_que_cnt++;
	if (sem_post(&bcmdec->buf_event) == -1)
		GST_ERROR_OBJECT(bcmdec, "buf sem_post failed");

	pthread_mutex_unlock(&bcmdec->gst_buf_que_lock);
}

static GSTBUF_LIST * bcmdec_rem_buf(GstBcmDec *bcmdec)
{
	GSTBUF_LIST *temp;

	pthread_mutex_lock(&bcmdec->gst_buf_que_lock);

	if (bcmdec->gst_buf_que_hd == bcmdec->gst_buf_que_tl) {
		temp = bcmdec->gst_buf_que_hd;
		bcmdec->gst_buf_que_hd = bcmdec->gst_buf_que_tl = NULL;
	} else {
		temp = bcmdec->gst_buf_que_hd;
		bcmdec->gst_buf_que_hd = temp->next;
	}

	if (temp)
		bcmdec->gst_que_cnt--;

	pthread_mutex_unlock(&bcmdec->gst_buf_que_lock);

	return temp;
}

static BC_STATUS bcmdec_insert_sps_pps(GstBcmDec *bcmdec, GstBuffer* gstbuf)
{
	BC_STATUS sts = BC_STS_SUCCESS;
	guint8 *data = GST_BUFFER_DATA(gstbuf);
	guint32 data_size = GST_BUFFER_SIZE(gstbuf);
	gint profile;
	guint nal_size;
	guint num_sps, num_pps, i;

	bcmdec->avcc_params.pps_size = 0;

	if (data_size < 7) {
		if (!bcmdec->silent)
			GST_INFO_OBJECT(bcmdec, "too small");
		return BC_STS_ERROR;
	}

	if (data[0] != 1) {
		if (!bcmdec->silent)
			GST_INFO_OBJECT(bcmdec, "wrong version");
		return BC_STS_ERROR;
	}

	profile = (data[1] << 16) | (data[2] << 8) | data[3];
	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "profile %06x",profile);

	bcmdec->avcc_params.nal_size_bytes = (data[4] & 0x03) + 1;

	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "nal size %d",bcmdec->avcc_params.nal_size_bytes);

	num_sps = data[5] & 0x1f;
	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "num sps %d",num_sps);

	data += 6;
	data_size -= 6;

	for (i = 0; i < num_sps; i++) {

		if (data_size < 2) {
			if (!bcmdec->silent)
				GST_INFO_OBJECT(bcmdec, "too small 2");
			return BC_STS_ERROR;
		}

		nal_size = (data[0] << 8) | data[1];
		data += 2;
		data_size -= 2;

		if (data_size < nal_size) {
			if (!bcmdec->silent)
				GST_INFO_OBJECT(bcmdec, "too small 3");
			return BC_STS_ERROR;
		}

		bcmdec->avcc_params.sps_pps_buf[0] = 0;
		bcmdec->avcc_params.sps_pps_buf[1] = 0;
		bcmdec->avcc_params.sps_pps_buf[2] = 0;
		bcmdec->avcc_params.sps_pps_buf[3] = 1;

		bcmdec->avcc_params.pps_size += 4;

		memcpy(bcmdec->avcc_params.sps_pps_buf + bcmdec->avcc_params.pps_size, data, nal_size);
		bcmdec->avcc_params.pps_size += nal_size;

		data += nal_size;
		data_size -= nal_size;
	}

	if (data_size < 1) {
		if (!bcmdec->silent)
			GST_INFO_OBJECT(bcmdec, "too small 4");
		return BC_STS_ERROR;
	}

	num_pps = data[0];
	data += 1;
	data_size -= 1;

	for (i = 0; i < num_pps; i++) {

		if (data_size < 2) {
			if (!bcmdec->silent)
				GST_INFO_OBJECT(bcmdec, "too small 5");
			return BC_STS_ERROR;
		}

		nal_size = (data[0] << 8) | data[1];
		data += 2;
		data_size -= 2;

		if (data_size < nal_size) {
			if (!bcmdec->silent)
				GST_INFO_OBJECT(bcmdec, "too small 6");
			return BC_STS_ERROR;
		}

		bcmdec->avcc_params.sps_pps_buf[bcmdec->avcc_params.pps_size+0] = 0;
		bcmdec->avcc_params.sps_pps_buf[bcmdec->avcc_params.pps_size+1] = 0;
		bcmdec->avcc_params.sps_pps_buf[bcmdec->avcc_params.pps_size+2] = 0;
		bcmdec->avcc_params.sps_pps_buf[bcmdec->avcc_params.pps_size+3] = 1;

		bcmdec->avcc_params.pps_size += 4;

		memcpy(bcmdec->avcc_params.sps_pps_buf + bcmdec->avcc_params.pps_size, data, nal_size);
		bcmdec->avcc_params.pps_size += nal_size;

		data += nal_size;
		data_size -= nal_size;
	}

	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "data size at end = %d ",data_size);

	return sts;
}

static guint32 insert_strtcode(GstBcmDec *bcmdec, guint8 *data_buf, guint32 offset)
{
	guint32 nal_sz = 0;
	guint i;

	for (i = 0; i < bcmdec->avcc_params.nal_size_bytes; i++) {
		nal_sz <<= 8;
		nal_sz += data_buf[offset + i];
	}

	if (nal_sz == 0)
		return nal_sz;

	if (bcmdec->avcc_params.nal_size_bytes == 4) {
		for (i = 0; i < 3; i++)
			data_buf[offset + i] = 0;
		data_buf[offset + 3] = 1;
	}

	return nal_sz;
}

static BC_STATUS bcmdec_insert_startcode(GstBcmDec *bcmdec, GstBuffer *gst_buf,
					 guint8 *dest_buf, guint32 *dest_buf_sz)
{
	guint8 *gst_data_buf = GST_BUFFER_DATA(gst_buf);
	guint32 gst_buf_sz = GST_BUFFER_SIZE(gst_buf);
	guint32 data_remaining = gst_buf_sz;
	guint8 start_code_buf[4] = { 0, 0, 0, 1 };
	BC_STATUS sts = BC_STS_SUCCESS;
	guint32 data_buf_pos = 0;
	guint32 dest_buf_pos = 0;
	guint8* temp_data_buf = NULL;

	/* FIXME: jarod: Why all the sts != BC_STS_SUCCESS checks when sts is NEVER set anywhere? */
	while (data_remaining > 0) {
		if (bcmdec->avcc_params.inside_buffer) {
			bcmdec->avcc_params.nal_sz = insert_strtcode(bcmdec, gst_data_buf, bcmdec->avcc_params.strtcode_offset);
			if (bcmdec->avcc_params.nal_sz == 0)
				return BC_STS_IO_USER_ABORT;

			if (bcmdec->avcc_params.nal_size_bytes == 2) {
				memcpy(bcmdec->dest_buf + dest_buf_pos, start_code_buf, 4);
				if (sts != BC_STS_SUCCESS)
					return sts;

				data_buf_pos += bcmdec->avcc_params.nal_size_bytes;
				dest_buf_pos += 4;
			}
			data_remaining -= bcmdec->avcc_params.nal_size_bytes;
			bcmdec->avcc_params.strtcode_offset += bcmdec->avcc_params.nal_size_bytes;
#if 0
			printf(" 1 .......dataR = %d, stoff = %d, nalS = %d datapo = %d, destpo = %d\n",
			       data_remaining, bcmdec->avcc_params.strtcode_offset,
			       bcmdec->avcc_params.nal_sz,data_buf_pos,dest_buf_pos);
#endif
		} else {
			if (data_remaining > (bcmdec->avcc_params.nal_sz - bcmdec->avcc_params.consumed_offset)) {
				bcmdec->avcc_params.strtcode_offset = bcmdec->avcc_params.nal_sz - bcmdec->avcc_params.consumed_offset;
				data_remaining -= bcmdec->avcc_params.nal_sz - bcmdec->avcc_params.consumed_offset;
				if (bcmdec->avcc_params.nal_size_bytes == 2) {
					memcpy(bcmdec->dest_buf + dest_buf_pos, gst_data_buf + data_buf_pos, bcmdec->avcc_params.strtcode_offset);
					if (sts != BC_STS_SUCCESS)
						return sts;
					data_buf_pos += bcmdec->avcc_params.strtcode_offset;
					dest_buf_pos += bcmdec->avcc_params.strtcode_offset;
				}
				bcmdec->avcc_params.nal_sz = insert_strtcode(bcmdec, gst_data_buf, bcmdec->avcc_params.strtcode_offset);
				if (bcmdec->avcc_params.nal_sz == 0)
					return BC_STS_IO_USER_ABORT;

				if (bcmdec->avcc_params.nal_size_bytes == 2) {
					memcpy(bcmdec->dest_buf + dest_buf_pos, start_code_buf, 4);
					if (sts != BC_STS_SUCCESS)
						return sts;
					data_buf_pos += bcmdec->avcc_params.nal_size_bytes;
					dest_buf_pos += 4;
				}
				data_remaining -= bcmdec->avcc_params.nal_size_bytes;
				bcmdec->avcc_params.strtcode_offset += bcmdec->avcc_params.nal_size_bytes;

			} else {
				bcmdec->avcc_params.consumed_offset += data_remaining;
				data_remaining = 0;
				break;
			}
		}
		if (data_remaining > bcmdec->avcc_params.nal_sz) {
			if (bcmdec->avcc_params.nal_size_bytes == 2) {
				temp_data_buf = gst_data_buf + data_buf_pos;
				memcpy(bcmdec->dest_buf + dest_buf_pos, gst_data_buf + data_buf_pos, bcmdec->avcc_params.nal_sz);
				if (sts != BC_STS_SUCCESS)
					return sts;
				data_buf_pos += bcmdec->avcc_params.nal_sz;
				dest_buf_pos += bcmdec->avcc_params.nal_sz;
			}

			bcmdec->avcc_params.strtcode_offset += bcmdec->avcc_params.nal_sz;
			data_remaining -= bcmdec->avcc_params.nal_sz;
			bcmdec->avcc_params.inside_buffer = TRUE;
			continue;
		} else if (data_remaining < bcmdec->avcc_params.nal_sz) {
			if (bcmdec->avcc_params.nal_size_bytes == 2) {
				temp_data_buf = gst_data_buf + data_buf_pos;
				memcpy(bcmdec->dest_buf + dest_buf_pos, gst_data_buf + data_buf_pos, data_remaining);
				if (sts != BC_STS_SUCCESS)
					return sts;
				data_buf_pos += data_remaining;
				dest_buf_pos += data_remaining;
			}
			bcmdec->avcc_params.inside_buffer = FALSE;
			bcmdec->avcc_params.consumed_offset = data_remaining;
			data_remaining = 0;
			break;
		} else {
			if (bcmdec->avcc_params.nal_size_bytes == 2) {
				temp_data_buf = gst_data_buf + data_buf_pos;
				memcpy(bcmdec->dest_buf + dest_buf_pos, gst_data_buf + data_buf_pos, bcmdec->avcc_params.nal_sz);
				if (sts != BC_STS_SUCCESS)
					return sts;
				data_buf_pos += bcmdec->avcc_params.nal_sz;
				dest_buf_pos += bcmdec->avcc_params.nal_sz;
			}
			bcmdec->avcc_params.inside_buffer = FALSE;
			bcmdec->avcc_params.consumed_offset = data_remaining;
			bcmdec->avcc_params.strtcode_offset = 0;
			data_remaining = 0;
			break;
		}
	}

	*dest_buf_sz = dest_buf_pos;

	return BC_STS_SUCCESS;
}

static BC_STATUS bcmdec_suspend_callback(GstBcmDec *bcmdec)
{
	BC_STATUS sts = BC_STS_SUCCESS;
	bcmdec_flush_gstbuf_queue(bcmdec);

	bcmdec->base_time = 0;
	if (bcmdec->decif.hdev)
		sts = decif_close(&bcmdec->decif);
	bcmdec->avcc_params.inside_buffer = TRUE;
	bcmdec->avcc_params.consumed_offset = 0;
	bcmdec->avcc_params.strtcode_offset = 0;
	bcmdec->avcc_params.nal_sz = 0;
	bcmdec->insert_pps = TRUE;

	return sts;
}

static BC_STATUS bcmdec_resume_callback(GstBcmDec *bcmdec)
{
	BC_STATUS sts = BC_STS_SUCCESS;

	sts = decif_open(&bcmdec->decif);
	if (sts == BC_STS_SUCCESS) {
		GST_INFO_OBJECT(bcmdec, "dev open success");
	} else {
		GST_ERROR_OBJECT(bcmdec, "dev open failed %d", sts);
		return sts;
	}

	sts = decif_prepare_play(&bcmdec->decif, bcmdec->input_format);
	if (sts == BC_STS_SUCCESS) {
		GST_INFO_OBJECT(bcmdec, "prepare play success");
	} else {
		GST_ERROR_OBJECT(bcmdec, "prepare play failed %d", sts);
		bcmdec->streaming = FALSE;
		return sts;
	}

	decif_set422_mode(&bcmdec->decif, BUF_MODE);

	sts = decif_start_play(&bcmdec->decif);
	if (sts == BC_STS_SUCCESS) {
		GST_INFO_OBJECT(bcmdec, "start play success");
		bcmdec->streaming = TRUE;
	} else {
		GST_ERROR_OBJECT(bcmdec, "start play failed %d", sts);
		bcmdec->streaming = FALSE;
		return sts;
	}

	if (sem_post(&bcmdec->play_event) == -1)
		GST_ERROR_OBJECT(bcmdec, "sem_post failed");

	if (sem_post(&bcmdec->push_start_event) == -1)
		GST_ERROR_OBJECT(bcmdec, "push_start post failed");

	return sts;
}

static gboolean bcmdec_mul_inst_cor(GstBcmDec *bcmdec)
{
	struct timespec ts;
	gint ret = 0;
	int i = 0;

	if ((intptr_t)g_inst_sts == -1) {
		GST_ERROR_OBJECT(bcmdec, "mul_inst_cor :shmem ptr invalid");
		return FALSE;
	}

	if (g_inst_sts->cur_decode == PLAYBACK) {
		GST_INFO_OBJECT(bcmdec, "mul_inst_cor : ret false %d", g_inst_sts->cur_decode);
		return FALSE;
	}

	for (i = 0; i < 15; i++) {
		ts.tv_sec = time(NULL) + 3;
		ts.tv_nsec = 0;
		ret = sem_timedwait(&g_inst_sts->inst_ctrl_event, &ts);
		if (ret < 0) {
			if (errno == ETIMEDOUT) {
				if (g_inst_sts->cur_decode == PLAYBACK) {
					GST_INFO_OBJECT(bcmdec, "mul_inst_cor :playback is set , exit");
					return FALSE;
				} else {
					GST_INFO_OBJECT(bcmdec, "mul_inst_cor :wait for thumb nail decode finish");
					continue;
				}
			} else if (errno == EINTR) {
				return FALSE;
			}
		} else {
			GST_INFO_OBJECT(bcmdec, "mul_inst_cor :ctrl_event is given");
			return TRUE;
		}
	}
	GST_INFO_OBJECT(bcmdec, "mul_inst_cor : ret false cur_dec = %d wait = %d", g_inst_sts->cur_decode, g_inst_sts->waiting);

	return FALSE;
}

static BC_STATUS bcmdec_create_shmem(GstBcmDec *bcmdec, int *shmem_id)
{
	int shmid = -1;
	key_t shmkey = BCM_GST_SHMEM_KEY;
	shmid_ds buf;

	if (shmem_id == NULL) {
		GST_ERROR_OBJECT(bcmdec, "Invalid argument ...");
		return BC_STS_INSUFF_RES;
	}

	*shmem_id = shmid;

	//First Try to create it.
	shmid = shmget(shmkey, 1024, 0644 | IPC_CREAT | IPC_EXCL);
	if (shmid == -1) {
		if (errno == EEXIST) {
			GST_INFO_OBJECT(bcmdec, "bcmdec_create_shmem:shmem already exists :%d", errno);
			shmid = shmget(shmkey, 1024, 0644);
			if (shmid == -1) {
				GST_ERROR_OBJECT(bcmdec, "bcmdec_create_shmem:unable to get shmid :%d", errno);
				return BC_STS_INSUFF_RES;
			}

			//we got the shmid, see if any process is alreday attached to it
			if (shmctl(shmid,IPC_STAT,&buf) == -1) {
				GST_ERROR_OBJECT(bcmdec, "bcmdec_create_shmem:shmctl failed ...");
				return BC_STS_ERROR;
			}

			if (buf.shm_nattch == 0) {
				sem_destroy(&g_inst_sts->inst_ctrl_event);
				//No process is currently attached to the shmem seg. go ahead and delete it as its contents are stale.
				if (shmctl(shmid, IPC_RMID, NULL) != -1)
						GST_INFO_OBJECT(bcmdec, "bcmdec_create_shmem:deleted shmem segment and creating a new one ...");
				//create a new shmem
				shmid = shmget(shmkey, 1024, 0644 | IPC_CREAT | IPC_EXCL);
				if (shmid == -1) {
					GST_ERROR_OBJECT(bcmdec, "bcmdec_create_shmem:unable to get shmid :%d", errno);
					return BC_STS_INSUFF_RES;
				}
				//attach to it
				bcmdec_get_shmem(bcmdec, shmid, TRUE);

			} else {
				//attach to it
				bcmdec_get_shmem(bcmdec, shmid, FALSE);
			}

		} else {
			GST_ERROR_OBJECT(bcmdec, "shmcreate failed with err %d",errno);
			return BC_STS_ERROR;
		}
	} else {
		//we created just attach to it
		bcmdec_get_shmem(bcmdec, shmid, TRUE);
	}

	*shmem_id = shmid;

	return BC_STS_SUCCESS;
}

static BC_STATUS bcmdec_get_shmem(GstBcmDec *bcmdec, int shmid, gboolean newmem)
{
	gint ret = 0;
	g_inst_sts = (GLB_INST_STS *)shmat(shmid, (void *)0, 0);
	if ((intptr_t)g_inst_sts == -1) {
		GST_ERROR_OBJECT(bcmdec, "Unable to open shared memory ...errno = %d", errno);
		return BC_STS_ERROR;
	}

	if (newmem) {
		ret = sem_init(&g_inst_sts->inst_ctrl_event, 5, 1);
		if (ret != 0) {
			GST_ERROR_OBJECT(bcmdec, "inst_ctrl_event failed");
			return BC_STS_ERROR;
		}
	}

	return BC_STS_SUCCESS;
}

static BC_STATUS bcmdec_del_shmem(GstBcmDec *bcmdec)
{
	int shmid = 0;
	shmid_ds buf;

	//First dettach the shared mem segment
	if (shmdt(g_inst_sts) == -1)
		GST_ERROR_OBJECT(bcmdec, "Unable to detach shared memory ...");

	//delete the shared mem segment if there are no other attachments
	shmid = shmget((key_t)BCM_GST_SHMEM_KEY, 0, 0);
	if (shmid == -1) {
			GST_ERROR_OBJECT(bcmdec, "bcmdec_del_shmem:Unable get shmid ...");
			return BC_STS_ERROR;
	}

	if (shmctl(shmid, IPC_STAT, &buf) == -1) {
		GST_ERROR_OBJECT(bcmdec, "bcmdec_del_shmem:shmctl failed ...");
		return BC_STS_ERROR;
	}

	if (buf.shm_nattch == 0) {
		sem_destroy(&g_inst_sts->inst_ctrl_event);
		//No process is currently attached to the shmem seg. go ahead and delete it
		if (shmctl(shmid, IPC_RMID, NULL) != -1) {
				GST_ERROR_OBJECT(bcmdec, "bcmdec_del_shmem:deleted shmem segment ...");
				return BC_STS_ERROR;
		} else {
			GST_ERROR_OBJECT(bcmdec, "bcmdec_del_shmem:unable to delete shmem segment ...");
		}
	}

	return BC_STS_SUCCESS;
}

// For renderer buffer
static void bcmdec_flush_gstrbuf_queue(GstBcmDec *bcmdec)
{
	GSTBUF_LIST *gst_queue_element = NULL;

	while (1) {
		gst_queue_element = bcmdec_rem_rbuf(bcmdec);
		if (gst_queue_element) {
			if (gst_queue_element->gstbuf) {
				gst_buffer_unref (gst_queue_element->gstbuf);
				bcmdec_put_que_mem_rbuf(bcmdec, gst_queue_element);
			} else {
				break;
			}
		}
		else {
			GST_INFO_OBJECT(bcmdec, "no gst_queue_element");
			break;
		}
	}
}

static void * bcmdec_process_get_rbuf(void *ctx)
{
	GstBcmDec *bcmdec = (GstBcmDec *)ctx;
	GstFlowReturn ret = GST_FLOW_ERROR;
	GSTBUF_LIST *gst_queue_element = NULL;
	gboolean result = FALSE, done = FALSE;
	GstBuffer *gstbuf = NULL;
	guint bufSz = 0;
	guint8 *data_ptr;
	gboolean get_buf_start = FALSE;
	int revent = -1;

	while (1) {
		revent = sem_trywait(&bcmdec->rbuf_start_event);
		if (revent == 0) {
			if (!bcmdec->silent)
				GST_INFO_OBJECT(bcmdec, "got start get buf event ");
			get_buf_start = TRUE;
			bcmdec->rbuf_thread_running = TRUE;
		}

		revent = sem_trywait(&bcmdec->rbuf_stop_event);
		if (revent == 0) {
			if (!bcmdec->silent)
				GST_INFO_OBJECT(bcmdec, "quit event set, exit");
			break;
		}

		if (!bcmdec->streaming || !get_buf_start)
			usleep(100 * 1000);

		while (bcmdec->streaming && get_buf_start)
		{
			//GST_INFO_OBJECT(bcmdec, "process get rbuf start....");
			gstbuf = NULL;

			if (!bcmdec->recv_thread && !bcmdec->streaming) {
				if (!bcmdec->silent)
					GST_INFO_OBJECT(bcmdec, "process get rbuf prepare exiting..");
				done = TRUE;
				break;
			}

			if (gst_queue_element == NULL)
				gst_queue_element = bcmdec_get_que_mem_rbuf(bcmdec);
			if (!gst_queue_element) {
				if (!bcmdec->silent)
					GST_INFO_OBJECT(bcmdec, "rbuf_full == TRUE RCnt:%d", bcmdec->gst_rbuf_que_cnt);

				usleep(100 * 1000);
				continue;
			}

			bufSz = bcmdec->output_params.width * bcmdec->output_params.height * BUF_MULT;

			//GST_INFO_OBJECT(bcmdec, "process get rbuf gst_pad_alloc_buffer_and_set_caps ....");
			ret = gst_pad_alloc_buffer_and_set_caps(bcmdec->srcpad, GST_BUFFER_OFFSET_NONE,
								bufSz, GST_PAD_CAPS(bcmdec->srcpad), &gstbuf);
			if (ret != GST_FLOW_OK) {
				if (!bcmdec->silent)
					GST_ERROR_OBJECT(bcmdec, "gst_pad_alloc_buffer_and_set_caps failed %d ",ret);
				usleep(30 * 1000);
				continue;
			}

			data_ptr = GST_BUFFER_DATA(gstbuf);
#if 0
			if ((uintptr_t)data_ptr % 4 == 0) {
				GST_INFO_OBJECT(bcmdec, "process get rbuf:0x%0x, bufSz:%d, data_ptr:0x%0x", gstbuf, bufSz, data_ptr);
			} else {
				GST_INFO_OBJECT(bcmdec, "process get rbuf is not aligned rbuf:0x%0x, bufSz:%d, data_ptr:0x%0x", gstbuf, bufSz, data_ptr);
			}
#endif

			gst_queue_element->gstbuf = gstbuf;
			bcmdec_ins_rbuf(bcmdec, gst_queue_element);
			gst_queue_element = NULL;
			//GST_INFO_OBJECT(bcmdec, "rbuf inserted cn:%d rbuf:0x%0x, bufSz:%d, data_ptr:0x%0x", bcmdec->gst_rbuf_que_cnt, gstbuf, bufSz, data_ptr);

			if (bcmdec->paused && (bcmdec->gst_que_cnt < RESUME_THRESHOLD) &&
			    (bcmdec->gst_rbuf_que_cnt > GST_RENDERER_BUF_NORMAL_THRESHOLD)) {
				decif_pause(&bcmdec->decif, FALSE);
				//if (!bcmdec->silent)
				GST_INFO_OBJECT(bcmdec, "resumed by get rbuf thread que_cnt:%d, rbuf_que_cnt:%d", bcmdec->gst_que_cnt, bcmdec->gst_rbuf_que_cnt);
				bcmdec->paused = FALSE;
			}

			usleep(15 * 1000);
		}

		if (done) {
			GST_INFO_OBJECT(bcmdec, "process get rbuf done ");
			break;
		}
	}
	bcmdec_flush_gstrbuf_queue(bcmdec);
	GST_DEBUG_OBJECT(bcmdec, "process get rbuf exiting.. ");
	pthread_exit((void *)&result);
}

static gboolean bcmdec_start_get_rbuf_thread(GstBcmDec *bcmdec)
{
	gboolean result = TRUE;
	gint ret = 0;
	pthread_attr_t thread_attr;

	if (!bcmdec_alloc_mem_rbuf_que_pool(bcmdec))
		GST_ERROR_OBJECT(bcmdec, "rend pool alloc failed/n");

	ret = sem_init(&bcmdec->rbuf_ins_event, 0, 0);
	if (ret != 0) {
		GST_ERROR_OBJECT(bcmdec, "get rbuf ins event init failed");
		result = FALSE;
	}

	ret = sem_init(&bcmdec->rbuf_start_event, 0, 0);
	if (ret != 0) {
		GST_ERROR_OBJECT(bcmdec, "get rbuf start event init failed");
		result = FALSE;
	}

	ret = sem_init(&bcmdec->rbuf_stop_event, 0, 0);
	if (ret != 0) {
		GST_ERROR_OBJECT(bcmdec, "get rbuf stop event init failed");
		result = FALSE;
	}

	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_JOINABLE);
	pthread_create(&bcmdec->get_rbuf_thread, &thread_attr,
		       bcmdec_process_get_rbuf, bcmdec);
	pthread_attr_destroy(&thread_attr);

	if (!bcmdec->get_rbuf_thread) {
		GST_ERROR_OBJECT(bcmdec, "Failed to create Renderer buffer Thread");
		result = FALSE;
	} else {
		GST_INFO_OBJECT(bcmdec, "Success to create Renderer buffer Thread");
	}

	return result;
}

static void bcmdec_put_que_mem_rbuf(GstBcmDec *bcmdec, GSTBUF_LIST *gst_queue_element)
{
	pthread_mutex_lock(&bcmdec->gst_rbuf_que_lock);

	gst_queue_element->next = bcmdec->gst_mem_rbuf_que_hd;
	bcmdec->gst_mem_rbuf_que_hd = gst_queue_element;

	pthread_mutex_unlock(&bcmdec->gst_rbuf_que_lock);
}

static GSTBUF_LIST * bcmdec_get_que_mem_rbuf(GstBcmDec *bcmdec)
{
	GSTBUF_LIST *gst_queue_element;

	pthread_mutex_lock(&bcmdec->gst_rbuf_que_lock);

	gst_queue_element = bcmdec->gst_mem_rbuf_que_hd;
	if (gst_queue_element !=NULL)
		bcmdec->gst_mem_rbuf_que_hd = bcmdec->gst_mem_rbuf_que_hd->next;

	pthread_mutex_unlock(&bcmdec->gst_rbuf_que_lock);

	return gst_queue_element;
}

static gboolean bcmdec_alloc_mem_rbuf_que_pool(GstBcmDec *bcmdec)
{
	GSTBUF_LIST *gst_queue_element;
	guint i;

	bcmdec->gst_mem_rbuf_que_hd = NULL;
	for (i = 1; i < bcmdec->gst_rbuf_que_sz; i++) {
		gst_queue_element = (GSTBUF_LIST *)malloc(sizeof(GSTBUF_LIST));
		if (!gst_queue_element) {
			GST_ERROR_OBJECT(bcmdec, "mem_rbuf_que_pool malloc failed ");
			return FALSE;
		}
		memset(gst_queue_element, 0, sizeof(GSTBUF_LIST));
		bcmdec_put_que_mem_rbuf(bcmdec, gst_queue_element);
	}

	return TRUE;
}

static gboolean bcmdec_release_mem_rbuf_que_pool(GstBcmDec *bcmdec)
{
	GSTBUF_LIST *gst_queue_element;
	guint i = 0;

	do {
		gst_queue_element = bcmdec_get_que_mem_rbuf(bcmdec);
		if (gst_queue_element) {
			free(gst_queue_element);
			i++;
		}
	} while (gst_queue_element);

	if (!bcmdec->silent)
		GST_INFO_OBJECT(bcmdec, "rend_rbuf_que_pool released... %d", i);

	return TRUE;
}

static void bcmdec_ins_rbuf(GstBcmDec *bcmdec, GSTBUF_LIST *gst_queue_element)
{
	pthread_mutex_lock(&bcmdec->gst_rbuf_que_lock);

	if (!bcmdec->gst_rbuf_que_hd) {
		bcmdec->gst_rbuf_que_hd = bcmdec->gst_rbuf_que_tl = gst_queue_element;
	} else {
		bcmdec->gst_rbuf_que_tl->next = gst_queue_element;
		bcmdec->gst_rbuf_que_tl = gst_queue_element;
		gst_queue_element->next = NULL;
	}

	bcmdec->gst_rbuf_que_cnt++;

	if (sem_post(&bcmdec->rbuf_ins_event) == -1)
		GST_ERROR_OBJECT(bcmdec, "rbuf sem_post failed");

	pthread_mutex_unlock(&bcmdec->gst_rbuf_que_lock);
}

static GSTBUF_LIST *bcmdec_rem_rbuf(GstBcmDec *bcmdec)
{
	GSTBUF_LIST *temp;

	pthread_mutex_lock(&bcmdec->gst_rbuf_que_lock);

	if (bcmdec->gst_rbuf_que_hd == bcmdec->gst_rbuf_que_tl) {
		temp = bcmdec->gst_rbuf_que_hd;
		bcmdec->gst_rbuf_que_hd = bcmdec->gst_rbuf_que_tl = NULL;
	} else {
		temp = bcmdec->gst_rbuf_que_hd;
		bcmdec->gst_rbuf_que_hd = temp->next;
	}

	if (temp)
		bcmdec->gst_rbuf_que_cnt--;

	pthread_mutex_unlock(&bcmdec->gst_rbuf_que_lock);

	return temp;
}

// End of renderer buffer

/*
 * entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean plugin_init(GstPlugin *bcmdec)
{
	//printf("BcmDec_init");

	/*
	 * debug category for fltering log messages
	 *
	 * exchange the string 'Template bcmdec' with your description
	 */
	GST_DEBUG_CATEGORY_INIT(gst_bcmdec_debug, "bcmdec", 0, "Broadcom video decoder");

	return gst_element_register(bcmdec, "bcmdec", GST_BCMDEC_RANK, GST_TYPE_BCMDEC);
}

/* gstreamer looks for this structure to register bcmdec */
GST_PLUGIN_DEFINE(GST_VERSION_MAJOR, GST_VERSION_MINOR,
		  "bcmdec", "Video decoder", plugin_init, VERSION,
		  "LGPL", "bcmdec", "http://broadcom.com/")

