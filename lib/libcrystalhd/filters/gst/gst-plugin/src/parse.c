 /*******************************************************************
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
#include <pthread.h>
#include <getopt.h>
#include <errno.h>
#include <glib.h>
#include <gst/gst.h>
#include "bc_dts_defs.h"
#include "parse.h"

void parse_init(Parse *parse)
{
	parse->bIsFirstByteStreamNALU = TRUE;
}

gboolean parse_find_strt_code(Parse *parse, guint8 input_format, guint8 *in_buffer,
			      guint32 size, guint32 *poffset)
{
	guint32 i = 0;
	guint8 Suffix1 = 0;
	guint8 Suffix2 = 0;

	if (input_format == BC_VID_ALGO_VC1) {
		Suffix1 = VC1_FRM_SUFFIX;
		Suffix2 = VC1_SEQ_SUFFIX;
	}
	else if (input_format == BC_VID_ALGO_MPEG2) {
		Suffix1 = MPEG2_FRM_SUFFIX;
		Suffix2 = MPEG2_SEQ_SUFFIX;
	}
	/* For VC-1 SP/MP */
	else if (input_format == BC_VID_ALGO_VC1MP) {
		Suffix1 = VC1_SM_FRM_SUFFIX;
	}

	if (input_format == BC_VID_ALGO_H264) {
		int nNalType = 0;
		uint32_t ulPos = 0;
		nNalType = parseAVC(parse, in_buffer, size, &ulPos);
		if ((nNalType == NALU_TYPE_SEI) || (nNalType == NALU_TYPE_PPS) ||
		    (nNalType == NALU_TYPE_SPS)) {
			*poffset = ulPos;
			return TRUE;
		} else if ((nNalType == NALU_TYPE_SLICE) | (nNalType == NALU_TYPE_IDR)) {
			*poffset = 0;
			return TRUE;
		}
	} else { /* VC1, MPEG2 */
		while (i < size) {
			if ((*(in_buffer + i) == Suffix1) || (*(in_buffer + i) == Suffix2)) {
				if (i >= 3) {
					if ((*(in_buffer + (i - 3)) == 0x00) &&
					    (*(in_buffer + (i - 2)) == 0x00) &&
					    (*(in_buffer + (i - 1)) == 0x01)) {
						*poffset = i-3;
						return TRUE;
					}
				}
			}
			i++;
		}
	}

	return FALSE;
}

gint FindBSStartCode(guint8 *Buf, gint ZerosInStartcode)
{
	BOOL bStartCode = TRUE;
	gint i;

	for (i = 0; i < ZerosInStartcode; i++)
		if (Buf[i] != 0)
			bStartCode = FALSE;

	if (Buf[i] != 1)
		bStartCode = FALSE;

	return bStartCode;
}

gint GetNaluType(Parse *parse, guint8 *pInputBuf, guint32 ulSize, NALU_t *pNalu)
{
	gint b20sInSC, b30sInSC;
	gint bStartCodeFound, Rewind;
	gint nLeadingZero8BitsCount = 0, TrailingZero8Bits = 0;
	guint32 Pos = 0;

	while (Pos <= ulSize) {
		if ((pInputBuf[Pos++]) == 0)
			continue;
		else
			break;
	}

	if (pInputBuf[Pos - 1] != 1)
		return -1;

	if (Pos < 3) {
		return -1;
	} else if (Pos == 3) {
		pNalu->StartcodePrefixLen = 3;
		nLeadingZero8BitsCount = 0;
	} else {
		nLeadingZero8BitsCount = Pos - 4;
		pNalu->StartcodePrefixLen = 4;
	}

	/*
	 * the 1st byte stream NAL unit can has nLeadingZero8BitsCount, but subsequent
	 * ones are not allowed to contain it since these zeros (if any) are considered
	 * trailing_zero_8bits of the previous byte stream NAL unit.
	 */
	if (!parse->bIsFirstByteStreamNALU && nLeadingZero8BitsCount > 0)
		return -1;

	parse->bIsFirstByteStreamNALU = false;

	bStartCodeFound = 0;
	b20sInSC = 0;
	b30sInSC = 0;

	while ((!bStartCodeFound) && (Pos < ulSize)) {
		Pos++;
		if (Pos > ulSize)
			printf("GetNaluType : Pos > size = %d\n", ulSize);

		b30sInSC = FindBSStartCode((pInputBuf + Pos - 4), 3);
		if (b30sInSC != 1)
			b20sInSC = FindBSStartCode((pInputBuf + Pos - 3), 2);
		bStartCodeFound = (b20sInSC  || b30sInSC);
	}

	Rewind = 0;
#if 0
	if (!bStartCodeFound) {
		//even if next start code is not found pprocess this NAL.
		return -1;
	}
#endif

	if (bStartCodeFound) {
		//Count the trailing_zero_8bits
		//TrailingZero8Bits is present only for start code 00 00 00 01
		if (b30sInSC) {
			while(pInputBuf[Pos - 5 - TrailingZero8Bits] == 0)
				TrailingZero8Bits++;
		}
		// Here, we have found another start code (and read length of startcode bytes more than we should
		// have.  Hence, go back in the file

		if (b30sInSC)
			Rewind = -4;
		else if (b20sInSC)
			Rewind = -3;
	}

	// Here the leading zeros(if any), Start code, the complete NALU, trailing zeros(if any)
	// until the  next start code .
	// Total size traversed is Pos, Pos+rewind are the number of bytes excluding the next
	// start code, and (Pos+rewind)-StartcodePrefixLen-LeadingZero8BitsCount-TrailingZero8Bits
	// is the size of the NALU.

	pNalu->Len = (Pos + Rewind) - pNalu->StartcodePrefixLen - nLeadingZero8BitsCount - TrailingZero8Bits;

	pNalu->NalUnitType = (pInputBuf[nLeadingZero8BitsCount + pNalu->StartcodePrefixLen]) & 0x1f;

	/* FIXME: jarod: is this broken? */
	if (0) {
		gint NalTypePos = nLeadingZero8BitsCount + pNalu->StartcodePrefixLen;
		guint32 tmp = 0, ulFirstMB;
		SliceType eSliceType = P_SLICE;

		if (pNalu->NalUnitType == NALU_TYPE_IDR) {
			//m_bFindIDR = false;
		} else if (pNalu->NalUnitType == NALU_TYPE_SLICE) {
			pNalu->pNalBuf = (guint8*) calloc(524 *1024, sizeof(guint8));
			memcpy (pNalu->pNalBuf, &pInputBuf[NalTypePos + 1], pNalu->Len - 1);

			SiBuffer(&parse->symb_int,pNalu->pNalBuf,pNalu->Len-1);

			if (SiUe(&parse->symb_int, &ulFirstMB)) { ////first mb in slice
				if (SiUe(&parse->symb_int, &tmp)) { //slice type
					printf("SliceType1 = %d\n", tmp);

					if (tmp > 4)
						tmp -= 5;

					eSliceType = (SliceType)tmp;
				} else {
					//printf("failed to get Slice type\n");
				}
			} else {
				printf("failed to first slice nr.\n");
			}

			if ((eSliceType == I_SLICE) || (eSliceType == SI_SLICE)) {
				pInputBuf[NalTypePos] = (pInputBuf[NalTypePos] & ~0x1f) | NALU_TYPE_IDR;
				//m_bFindIDR = false;
				printf("IDR not found REPLACED SliceType = %d\n", eSliceType);
			}

			free(pNalu->pNalBuf);
		}
	}
	return (Pos+Rewind);
}

gint parseAVC(Parse *parse, guint8 *pInputBuf, guint32 ulSize, guint32 *Offset)
{
	NALU_t Nalu;
	gint ret = 0;
	guint32 Pos = 0;
	gboolean bResult = false;

	while (1)
	{
		ret = GetNaluType(parse, pInputBuf + Pos, ulSize - Pos, &Nalu);
		if (ret <= 0)
			return -1;

		Pos += ret;

		switch (Nalu.NalUnitType) {
		case NALU_TYPE_SLICE:
		case NALU_TYPE_IDR:
			bResult = true;
			break;
		case NALU_TYPE_SEI:
		case NALU_TYPE_PPS:
		case NALU_TYPE_SPS:
			bResult = true;
			break;
		case NALU_TYPE_DPA:
		case NALU_TYPE_DPC:
		case NALU_TYPE_AUD:
		case NALU_TYPE_EOSEQ:
		case NALU_TYPE_EOSTREAM:
		case NALU_TYPE_FILL:
		default:
			break;
		}

		if (bResult) {
			*Offset = Pos;
			break;
		}
	}

	return Nalu.NalUnitType;
}

gboolean SiBuffer(SymbInt *simb_int, guint8 *pInputBuffer, guint32 ulSize)
{
	simb_int->m_pCurrent = simb_int->m_pInputBuffer = (guint8 *)pInputBuffer;
	simb_int->m_nSize = ulSize;
	simb_int->m_pInputBufferEnd = simb_int->m_pInputBuffer + ulSize;
	simb_int->m_nUsed = 1;
	simb_int->m_ulOffset = 0;
	simb_int->m_ulMask = 0x80;

	return TRUE;
}

gboolean SiUe(SymbInt *simb_int, guint32 *pCode)
{
	guint32	ulSuffix;
	int nLeadingZeros;
	int nBit;

	nLeadingZeros = -1;
	for (nBit = 0; nBit == 0; nLeadingZeros++) {
		nBit = NextBit(simb_int);
		if (simb_int->m_nUsed >= simb_int->m_nSize)
			return FALSE;
	}

	*pCode = (1 << nLeadingZeros) - 1;

	ulSuffix = 0;
	while (nLeadingZeros-- > 0) {
		ulSuffix = (ulSuffix << 1) | NextBit(simb_int);
		if (simb_int->m_nUsed >= simb_int->m_nSize)
			return FALSE;
	}
	*pCode += ulSuffix;

	return TRUE;
}

inline gint NextBit(SymbInt *simb_int)
{
	int nBit;

	nBit = (simb_int->m_pCurrent[0] & simb_int->m_ulMask) ? 1 : 0;

	if ((simb_int->m_ulMask >>= 1) == 0) {
		simb_int->m_ulMask = 0x80;

		if (simb_int->m_nUsed == simb_int->m_nSize)
			simb_int->m_pCurrent = simb_int->m_pInputBuffer; //reset look again
		else {
			if (++simb_int->m_pCurrent == simb_int->m_pInputBufferEnd)
				simb_int->m_pCurrent = simb_int->m_pInputBuffer;
			simb_int->m_nUsed++;
		}
	}
	simb_int->m_ulOffset++;

	return nBit;
}

guint32 SiOffset(SymbInt *simb_int)
{
	return simb_int->m_ulOffset;
}
