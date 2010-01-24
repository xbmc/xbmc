/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
*/

#include "stdafx.h"
#include "sndfile.h"

BOOL PP20_Unpack(LPCBYTE *ppMemFile, LPDWORD pdwMemLength);

typedef struct MMCMPFILEHEADER
{
	DWORD id_ziRC;	// "ziRC"
	DWORD id_ONia;	// "ONia"
	WORD hdrsize;
} MMCMPFILEHEADER, *LPMMCMPFILEHEADER;

typedef struct MMCMPHEADER
{
	WORD version;
	WORD nblocks;
	DWORD filesize;
	DWORD blktable;
	BYTE glb_comp;
	BYTE fmt_comp;
} MMCMPHEADER, *LPMMCMPHEADER;

typedef struct MMCMPBLOCK
{
	DWORD unpk_size;
	DWORD pk_size;
	DWORD xor_chk;
	WORD sub_blk;
	WORD flags;
	WORD tt_entries;
	WORD num_bits;
} MMCMPBLOCK, *LPMMCMPBLOCK;

typedef struct MMCMPSUBBLOCK
{
	DWORD unpk_pos;
	DWORD unpk_size;
} MMCMPSUBBLOCK, *LPMMCMPSUBBLOCK;

#define MMCMP_COMP		0x0001
#define MMCMP_DELTA		0x0002
#define MMCMP_16BIT		0x0004
#define MMCMP_STEREO	0x0100
#define MMCMP_ABS16		0x0200
#define MMCMP_ENDIAN	0x0400

typedef struct MMCMPBITBUFFER
{
	UINT bitcount;
	DWORD bitbuffer;
	LPCBYTE pSrc;
	LPCBYTE pEnd;

	DWORD GetBits(UINT nBits);
} MMCMPBITBUFFER;


DWORD MMCMPBITBUFFER::GetBits(UINT nBits)
//---------------------------------------
{
	DWORD d;
	if (!nBits) return 0;
	while (bitcount < 24)
	{
		bitbuffer |= ((pSrc < pEnd) ? *pSrc++ : 0) << bitcount;
		bitcount += 8;
	}
	d = bitbuffer & ((1 << nBits) - 1);
	bitbuffer >>= nBits;
	bitcount -= nBits;
	return d;
}

//#define MMCMP_LOG

#ifdef MMCMP_LOG
extern void Log(LPCSTR s, ...);
#endif

const DWORD MMCMP8BitCommands[8] =
{
	0x01, 0x03,	0x07, 0x0F,	0x1E, 0x3C,	0x78, 0xF8
};

const UINT MMCMP8BitFetch[8] =
{
	3, 3, 3, 3, 2, 1, 0, 0
};

const DWORD MMCMP16BitCommands[16] =
{
	0x01, 0x03,	0x07, 0x0F,	0x1E, 0x3C,	0x78, 0xF0,
	0x1F0, 0x3F0, 0x7F0, 0xFF0, 0x1FF0, 0x3FF0, 0x7FF0, 0xFFF0
};

const UINT MMCMP16BitFetch[16] =
{
	4, 4, 4, 4, 3, 2, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0
};


BOOL MMCMP_Unpack(LPCBYTE *ppMemFile, LPDWORD pdwMemLength)
//---------------------------------------------------------
{
	DWORD dwMemLength = *pdwMemLength;
	LPCBYTE lpMemFile = *ppMemFile;
	LPBYTE pBuffer;
	LPMMCMPFILEHEADER pmfh = (LPMMCMPFILEHEADER)(lpMemFile);
	LPMMCMPHEADER pmmh = (LPMMCMPHEADER)(lpMemFile+10);
	LPDWORD pblk_table;
	DWORD dwFileSize;

	if (PP20_Unpack(ppMemFile, pdwMemLength))
	{
		return TRUE;
	}
	if ((dwMemLength < 256) || (!pmfh) || (pmfh->id_ziRC != 0x4352697A) || (pmfh->id_ONia != 0x61694e4f) || (pmfh->hdrsize < 14)
	 || (!pmmh->nblocks) || (pmmh->filesize < 16) || (pmmh->filesize > 0x8000000)
	 || (pmmh->blktable >= dwMemLength) || (pmmh->blktable + 4*pmmh->nblocks > dwMemLength)) return FALSE;
	dwFileSize = pmmh->filesize;
	if ((pBuffer = (LPBYTE)GlobalAllocPtr(GHND, (dwFileSize + 31) & ~15)) == NULL) return FALSE;
	pblk_table = (LPDWORD)(lpMemFile+pmmh->blktable);
	for (UINT nBlock=0; nBlock<pmmh->nblocks; nBlock++)
	{
		DWORD dwMemPos = pblk_table[nBlock];
		LPMMCMPBLOCK pblk = (LPMMCMPBLOCK)(lpMemFile+dwMemPos);
		LPMMCMPSUBBLOCK psubblk = (LPMMCMPSUBBLOCK)(lpMemFile+dwMemPos+20);

		if ((dwMemPos + 20 >= dwMemLength) || (dwMemPos + 20 + pblk->sub_blk*8 >= dwMemLength)) break;
		dwMemPos += 20 + pblk->sub_blk*8;
#ifdef MMCMP_LOG
		Log("block %d: flags=%04X sub_blocks=%d", nBlock, (UINT)pblk->flags, (UINT)pblk->sub_blk);
		Log(" pksize=%d unpksize=%d", pblk->pk_size, pblk->unpk_size);
		Log(" tt_entries=%d num_bits=%d\n", pblk->tt_entries, pblk->num_bits);
#endif
		// Data is not packed
		if (!(pblk->flags & MMCMP_COMP))
		{
			for (UINT i=0; i<pblk->sub_blk; i++)
			{
				if ((psubblk->unpk_pos > dwFileSize) || (psubblk->unpk_pos + psubblk->unpk_size > dwFileSize)) break;
#ifdef MMCMP_LOG
				Log("  Unpacked sub-block %d: offset %d, size=%d\n", i, psubblk->unpk_pos, psubblk->unpk_size);
#endif
				memcpy(pBuffer+psubblk->unpk_pos, lpMemFile+dwMemPos, psubblk->unpk_size);
				dwMemPos += psubblk->unpk_size;
				psubblk++;
			}
		} else
		// Data is 16-bit packed
		if (pblk->flags & MMCMP_16BIT)
		{
			MMCMPBITBUFFER bb;
			LPWORD pDest = (LPWORD)(pBuffer + psubblk->unpk_pos);
			DWORD dwSize = psubblk->unpk_size >> 1;
			DWORD dwPos = 0;
			UINT numbits = pblk->num_bits;
			UINT subblk = 0, oldval = 0;

#ifdef MMCMP_LOG
			Log("  16-bit block: pos=%d size=%d ", psubblk->unpk_pos, psubblk->unpk_size);
			if (pblk->flags & MMCMP_DELTA) Log("DELTA ");
			if (pblk->flags & MMCMP_ABS16) Log("ABS16 ");
			Log("\n");
#endif
			bb.bitcount = 0;
			bb.bitbuffer = 0;
			bb.pSrc = lpMemFile+dwMemPos+pblk->tt_entries;
			bb.pEnd = lpMemFile+dwMemPos+pblk->pk_size;
			while (subblk < pblk->sub_blk)
			{
				UINT newval = 0x10000;
				DWORD d = bb.GetBits(numbits+1);

				if (d >= MMCMP16BitCommands[numbits])
				{
					UINT nFetch = MMCMP16BitFetch[numbits];
					UINT newbits = bb.GetBits(nFetch) + ((d - MMCMP16BitCommands[numbits]) << nFetch);
					if (newbits != numbits)
					{
						numbits = newbits & 0x0F;
					} else
					{
						if ((d = bb.GetBits(4)) == 0x0F)
						{
							if (bb.GetBits(1)) break;
							newval = 0xFFFF;
						} else
						{
							newval = 0xFFF0 + d;
						}
					}
				} else
				{
					newval = d;
				}
				if (newval < 0x10000)
				{
					newval = (newval & 1) ? (UINT)(-(LONG)((newval+1) >> 1)) : (UINT)(newval >> 1);
					if (pblk->flags & MMCMP_DELTA)
					{
						newval += oldval;
						oldval = newval;
					} else
					if (!(pblk->flags & MMCMP_ABS16))
					{
						newval ^= 0x8000;
					}
					pDest[dwPos++] = (WORD)newval;
				}
				if (dwPos >= dwSize)
				{
					subblk++;
					dwPos = 0;
					dwSize = psubblk[subblk].unpk_size >> 1;
					pDest = (LPWORD)(pBuffer + psubblk[subblk].unpk_pos);
				}
			}
		} else
		// Data is 8-bit packed
		{
			MMCMPBITBUFFER bb;
			LPBYTE pDest = pBuffer + psubblk->unpk_pos;
			DWORD dwSize = psubblk->unpk_size;
			DWORD dwPos = 0;
			UINT numbits = pblk->num_bits;
			UINT subblk = 0, oldval = 0;
			LPCBYTE ptable = lpMemFile+dwMemPos;

			bb.bitcount = 0;
			bb.bitbuffer = 0;
			bb.pSrc = lpMemFile+dwMemPos+pblk->tt_entries;
			bb.pEnd = lpMemFile+dwMemPos+pblk->pk_size;
			while (subblk < pblk->sub_blk)
			{
				UINT newval = 0x100;
				DWORD d = bb.GetBits(numbits+1);

				if (d >= MMCMP8BitCommands[numbits])
				{
					UINT nFetch = MMCMP8BitFetch[numbits];
					UINT newbits = bb.GetBits(nFetch) + ((d - MMCMP8BitCommands[numbits]) << nFetch);
					if (newbits != numbits)
					{
						numbits = newbits & 0x07;
					} else
					{
						if ((d = bb.GetBits(3)) == 7)
						{
							if (bb.GetBits(1)) break;
							newval = 0xFF;
						} else
						{
							newval = 0xF8 + d;
						}
					}
				} else
				{
					newval = d;
				}
				if (newval < 0x100)
				{
					int n = ptable[newval];
					if (pblk->flags & MMCMP_DELTA)
					{
						n += oldval;
						oldval = n;
					}
					pDest[dwPos++] = (BYTE)n;
				}
				if (dwPos >= dwSize)
				{
					subblk++;
					dwPos = 0;
					dwSize = psubblk[subblk].unpk_size;
					pDest = pBuffer + psubblk[subblk].unpk_pos;
				}
			}
		}
	}
	*ppMemFile = pBuffer;
	*pdwMemLength = dwFileSize;
	return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
//
// PowerPack PP20 Unpacker
//

typedef struct _PPBITBUFFER
{
	UINT bitcount;
	ULONG bitbuffer;
	LPCBYTE pStart;
	LPCBYTE pSrc;

	ULONG GetBits(UINT n);
} PPBITBUFFER;


ULONG PPBITBUFFER::GetBits(UINT n)
{
	ULONG result = 0;

	for (UINT i=0; i<n; i++)
	{
		if (!bitcount)
		{
			bitcount = 8;
			if (pSrc != pStart) pSrc--;
			bitbuffer = *pSrc;
		}
		result = (result<<1) | (bitbuffer&1);
		bitbuffer >>= 1;
		bitcount--;
    }
    return result;
}


VOID PP20_DoUnpack(const BYTE *pSrc, UINT nSrcLen, BYTE *pDst, UINT nDstLen)
{
	PPBITBUFFER BitBuffer;
	ULONG nBytesLeft;

	BitBuffer.pStart = pSrc;
	BitBuffer.pSrc = pSrc + nSrcLen - 4;
	BitBuffer.bitbuffer = 0;
	BitBuffer.bitcount = 0;
	BitBuffer.GetBits(pSrc[nSrcLen-1]);
	nBytesLeft = nDstLen;
	while (nBytesLeft > 0)
	{
		if (!BitBuffer.GetBits(1))
		{
			UINT n = 1;
			while (n < nBytesLeft)
			{
				UINT code = BitBuffer.GetBits(2);
				n += code;
				if (code != 3) break;
			}
			for (UINT i=0; i<n; i++)
			{
				pDst[--nBytesLeft] = (BYTE)BitBuffer.GetBits(8);
			}
			if (!nBytesLeft) break;
		}
		{
			UINT n = BitBuffer.GetBits(2)+1;
			UINT nbits = pSrc[n-1];
			UINT nofs;
			if (n==4)
			{
				nofs = BitBuffer.GetBits( (BitBuffer.GetBits(1)) ? nbits : 7 );
				while (n < nBytesLeft)
				{
					UINT code = BitBuffer.GetBits(3);
					n += code;
					if (code != 7) break;
				}
			} else
			{
				nofs = BitBuffer.GetBits(nbits);
			}
			for (UINT i=0; i<=n; i++)
			{
				pDst[nBytesLeft-1] = (nBytesLeft+nofs < nDstLen) ? pDst[nBytesLeft+nofs] : 0;
				if (!--nBytesLeft) break;
			}
		}
	}
}


BOOL PP20_Unpack(LPCBYTE *ppMemFile, LPDWORD pdwMemLength)
{
	DWORD dwMemLength = *pdwMemLength;
	LPCBYTE lpMemFile = *ppMemFile;
	DWORD dwDstLen;
	LPBYTE pBuffer;

	if ((!lpMemFile) || (dwMemLength < 256) || (*(DWORD *)lpMemFile != 0x30325050)) return FALSE;
	dwDstLen = (lpMemFile[dwMemLength-4]<<16) | (lpMemFile[dwMemLength-3]<<8) | (lpMemFile[dwMemLength-2]);
	//Log("PP20 detected: Packed length=%d, Unpacked length=%d\n", dwMemLength, dwDstLen);
	if ((dwDstLen < 512) || (dwDstLen > 0x400000) || (dwDstLen > 16*dwMemLength)) return FALSE;
	if ((pBuffer = (LPBYTE)GlobalAllocPtr(GHND, (dwDstLen + 31) & ~15)) == NULL) return FALSE;
	PP20_DoUnpack(lpMemFile+4, dwMemLength-4, pBuffer, dwDstLen);
	*ppMemFile = pBuffer;
	*pdwMemLength = dwDstLen;
	return TRUE;
}





