/* 
 * $Id: GolombBuffer.h 956 2009-01-07 17:55:16Z casimir666 $
 *
 * (C) 2006-2007 see AUTHORS
 *
 * This file is part of mplayerc.
 *
 * Mplayerc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mplayerc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#pragma once


class CGolombBuffer
{
public:
	CGolombBuffer(BYTE* pBuffer, int nSize);

	UINT64			BitRead(int nBits, bool fPeek = false);
	UINT64			UExpGolombRead();
	INT64			SExpGolombRead();
	void			BitByteAlign();

	inline BYTE		ReadByte()  { return (BYTE) BitRead ( 8); };
	inline SHORT	ReadShort() { return (SHORT)BitRead (16); };
	inline DWORD	ReadDword() { return (DWORD)BitRead (32); };
	void			ReadBuffer(BYTE* pDest, int nSize);
	
	void			Reset();
	void			Reset(BYTE* pNewBuffer, int nNewSize);

	void			SetSize(int nValue) { m_nSize = nValue; };
	int				GetSize()			{ return m_nSize; };
	bool			IsEOF()				{ return m_nBitPos >= m_nSize; };
	INT64			GetPos();
	BYTE*			GetBufferPos()		{ return m_pBuffer + m_nBitPos; };

	void			SkipBytes(int nCount);

private :
	BYTE*		m_pBuffer;
	int			m_nSize;
	int			m_nBitPos;
	int			m_bitlen;
	INT64		m_bitbuff;
};
