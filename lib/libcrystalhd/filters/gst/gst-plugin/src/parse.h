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
#ifndef CPARSE
#define CPARSE

//VC1 prefix 000001
#define VC1_FRM_SUFFIX	0x0D
#define VC1_SEQ_SUFFIX	0x0F

//VC1 SM Profile prefix 000001
#define VC1_SM_FRM_SUFFIX	0xE0

//Check WMV SP/MP PES Payload for PTS Info
#define VC1_SM_MAGIC_WORD	0x5A5A5A5A
#define VC1_SM_PTS_INFO_START_CODE	0xBD

//MPEG2 prefix 000001
#define	MPEG2_FRM_SUFFIX 0x00
#define	MPEG2_SEQ_SUFFIX 0xB3

typedef enum 
{
  P_SLICE = 0,
  B_SLICE,
  I_SLICE,
  SP_SLICE,
  SI_SLICE
} SliceType;


typedef enum
{
	NALU_TYPE_SLICE =  1,
	NALU_TYPE_DPA,
	NALU_TYPE_DPB,
	NALU_TYPE_DPC,
	NALU_TYPE_IDR,
	NALU_TYPE_SEI,
	NALU_TYPE_SPS,
	NALU_TYPE_PPS,
	NALU_TYPE_AUD,
	NALU_TYPE_EOSEQ,
	NALU_TYPE_EOSTREAM,
	NALU_TYPE_FILL
}NALuType;

typedef struct 
{
  gint StartcodePrefixLen;		//! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
  guint Len;				//! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
  guint MaxSize;			//! Nal Unit Buffer size
  gint NalUnitType;				//! NALU_TYPE_xxxx  
  gint ForbiddenBit;				//! should be always FALSE  
  guint8* pNalBuf;
} NALU_t;

typedef struct
{
	guint8*	m_pInputBuffer;
	guint8*	m_pInputBufferEnd;	
	guint8*	m_pCurrent;
	guint32	m_ulMask;
	guint32	m_ulOffset;
	gint     m_nSize;
	gint     m_nUsed;
	guint32	m_ulZero;
}SymbInt;

typedef struct {
	gboolean bIsFirstByteStreamNALU;
	SymbInt symb_int;

}Parse;


void
parse_init(Parse* parse);

gint 
parseAVC(Parse* parse,guint8* pInputBuf,guint32 ulSize,guint32* Offset);

gboolean
parse_find_strt_code(Parse* parse,guint8 input_format,guint8* in_buffer,guint32 size,guint32* poffset);

gint 
GetNaluType(Parse* parse,guint8* pInputBuf, guint32 ulSize, NALU_t* pNalu);

gboolean 
SiBuffer(SymbInt* simb_int,guint8 * pInputBuffer, guint32 ulSize);	

gboolean 
SiUe(SymbInt* simb_int,guint32* pCode);

guint32	
SiOffset(SymbInt*);	

inline gint 
NextBit ( SymbInt*);


#endif
