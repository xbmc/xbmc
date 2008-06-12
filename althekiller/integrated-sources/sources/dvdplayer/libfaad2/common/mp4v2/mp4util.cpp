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
 * Copyright (C) Cisco Systems Inc. 2001.  All Rights Reserved.
 * 
 * Contributor(s): 
 *		Dave Mackie		dmackie@cisco.com
 */

#include "mp4common.h"

void MP4Error::Print(FILE* pFile)
{
	fprintf(pFile, "MP4ERROR: ");
	if (m_where) {
		fprintf(pFile, "%s", m_where);
	}
	if (m_errstring) {
		if (m_where) {
			fprintf(pFile, ": ");
		}
		fprintf(pFile, "%s", m_errstring);
	}
	if (m_errno) {
		if (m_where || m_errstring) {
			fprintf(pFile, ": ");
		}
		fprintf(pFile, "%s", strerror(m_errno));
	}
	fprintf(pFile, "\n");
}

void MP4HexDump(
	u_int8_t* pBytes, u_int32_t numBytes,
	FILE* pFile, u_int8_t indent)
{
	if (pFile == NULL) {
		pFile = stdout;
	}
	Indent(pFile, indent);
	fprintf(pFile, "<%u bytes> ", numBytes);
	for (u_int32_t i = 0; i < numBytes; i++) {
		if ((i % 16) == 0 && numBytes > 16) {
			fprintf(pFile, "\n");
			Indent(pFile, indent);
		}
		fprintf(pFile, "%02x ", pBytes[i]);
	}
	fprintf(pFile, "\n");
}

bool MP4NameFirstMatches(const char* s1, const char* s2) 
{
	if (s1 == NULL || *s1 == '\0' || s2 == NULL || *s2 == '\0') {
		return false;
	}

	if (*s2 == '*') {
		return true;
	}

	while (*s1 != '\0') {
		if (*s2 == '\0' || strchr("[.", *s2)) {
			break;
		}
		if (tolower(*s1) != tolower(*s2)) {
			return false;
		}
		s1++;
		s2++;
	}
	return true;
}

bool MP4NameFirstIndex(const char* s, u_int32_t* pIndex)
{
	if (s == NULL) {
		return false;
	}

	while (*s != '\0' && *s != '.') {
		if (*s == '[') {
			s++;
			ASSERT(pIndex);
			if (sscanf(s, "%u", pIndex) != 1) {
				return false;
			}
			return true;
		}
		s++;
	}
	return false;
}

char* MP4NameFirst(const char *s)
{
	if (s == NULL) {
		return NULL;
	}

	const char* end = s;

	while (*end != '\0' && *end != '.') {
		end++;
	}

	char* first = (char*)MP4Calloc((end - s) + 1);

	if (first) {
		strncpy(first, s, end - s);
	}

	return first;
}

const char* MP4NameAfterFirst(const char *s)
{
	if (s == NULL) {
		return NULL;
	}

	while (*s != '\0') {
		if (*s == '.') {
			s++;
			if (*s == '\0') {
				return NULL;
			}
			return s;
		}
		s++;
	}
	return NULL;
}

char* MP4ToBase16(const u_int8_t* pData, u_int32_t dataSize)
{
	if (dataSize) {
		ASSERT(pData);
	}

	char* s = (char*)MP4Calloc((2 * dataSize) + 1);

	u_int32_t i, j;
	for (i = 0, j = 0; i < dataSize; i++) {
		sprintf(&s[j], "%02x", pData[i]);
		j += 2;
	}

	return s;	/* N.B. caller is responsible for free'ing s */
}

char* MP4ToBase64(const u_int8_t* pData, u_int32_t dataSize)
{
	if (dataSize) {
		ASSERT(pData);
	}

	static char encoding[64] = {
		'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
		'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
		'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
		'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
	};

	char* s = (char*)MP4Calloc((((dataSize + 2) * 4) / 3) + 1);

	const u_int8_t* src = pData;
	char* dest = s;
	u_int32_t numGroups = dataSize / 3;

	for (u_int32_t i = 0; i < numGroups; i++) {
		*dest++ = encoding[src[0] >> 2];
		*dest++ = encoding[((src[0] & 0x03) << 4) | (src[1] >> 4)];
		*dest++ = encoding[((src[1] & 0x0F) << 2) | (src[2] >> 6)];
		*dest++ = encoding[src[2] & 0x3F];
		src += 3;
	}

	if (dataSize % 3 == 1) {
		*dest++ = encoding[src[0] >> 2];
		*dest++ = encoding[((src[0] & 0x03) << 4)];
		*dest++ = '=';
		*dest++ = '=';
	} else if (dataSize % 3 == 2) {
		*dest++ = encoding[src[0] >> 2];
		*dest++ = encoding[((src[0] & 0x03) << 4) | (src[1] >> 4)];
		*dest++ = encoding[((src[1] & 0x0F) << 2)];
		*dest++ = '=';
	}

	return s;	/* N.B. caller is responsible for free'ing s */
}

// log2 of value, rounded up
static u_int8_t ilog2(u_int64_t value)
{
	u_int64_t powerOf2 = 1;
	for (u_int8_t i = 0; i < 64; i++) {
		if (value <= powerOf2) {
			return i;
		}
		powerOf2 <<= 1;
	} 
	return 64;
}

u_int64_t MP4ConvertTime(u_int64_t t, 
	u_int32_t oldTimeScale, u_int32_t newTimeScale)
{
	// avoid float point exception
	if (oldTimeScale == 0) {
		throw new MP4Error("division by zero", "MP4ConvertTime");
	}

	// check if we can safely use integer operations
	if (ilog2(t) + ilog2(newTimeScale) <= 64) {
		return (t * newTimeScale) / oldTimeScale;
	}

	// final resort is to use floating point
	double d = ((double)newTimeScale / (double)oldTimeScale) + 0.5;
	d *= UINT64_TO_DOUBLE(t);

	return (u_int64_t)d;
}

