/*
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is MPEG4IP.
 * 
 * The Initial Developer of the Original Code is Cisco Systems Inc.
 * Portions created by Cisco Systems Inc. are
 * Copyright (C) Cisco Systems Inc. 2001-2002.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4av_common.h"

void CMemoryBitstream::AllocBytes(u_int32_t numBytes) 
{
	m_pBuf = (u_int8_t*)calloc(numBytes, 1);
	if (!m_pBuf) {
		throw ENOMEM;
	}
	m_bitPos = 0;
	m_numBits = numBytes << 3;
}

void CMemoryBitstream::SetBytes(u_int8_t* pBytes, u_int32_t numBytes) 
{
	m_pBuf = pBytes;
	m_bitPos = 0;
	m_numBits = numBytes << 3;
}

void CMemoryBitstream::PutBytes(u_int8_t* pBytes, u_int32_t numBytes)
{
	u_int32_t numBits = numBytes << 3;

	if (numBits + m_bitPos > m_numBits) {
		throw EIO;
	}

	if ((m_bitPos & 7) == 0) {
		memcpy(&m_pBuf[m_bitPos >> 3], pBytes, numBytes);
		m_bitPos += numBits;
	} else {
		for (u_int32_t i = 0; i < numBytes; i++) {
			PutBits(pBytes[i], 8);
		}
	}
}

void CMemoryBitstream::PutBits(u_int32_t bits, u_int32_t numBits)
{
	if (numBits + m_bitPos > m_numBits) {
		throw EIO;
	}
	if (numBits > 32) {
		throw EIO;
	}

	for (int8_t i = numBits - 1; i >= 0; i--) {
		m_pBuf[m_bitPos >> 3] |= ((bits >> i) & 1) << (7 - (m_bitPos & 7));
		m_bitPos++;
	}
}

u_int32_t CMemoryBitstream::GetBits(u_int32_t numBits)
{
	if (numBits + m_bitPos > m_numBits) {
		throw EIO;
	}
	if (numBits > 32) {
		throw EIO;
	}

	u_int32_t bits = 0;

	for (u_int8_t i = 0; i < numBits; i++) {
		bits <<= 1;
		bits |= (m_pBuf[m_bitPos >> 3] >> (7 - (m_bitPos & 7))) & 1;
		m_bitPos++;
	}

	return bits;
}

