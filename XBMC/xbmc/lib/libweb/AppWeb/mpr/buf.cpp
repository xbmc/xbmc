///
///	@file 	buf.cpp
/// @brief 	Fast, growable buffer mechanism using a ring queue metaphore
/// @overview This buffer implementation provides a growable buffer 
///		mechanism that strives for maximal usage of space. It is also 
///		designed to be as fast as possible. It is the callers responsibility 
///		to do thread synchronization. 
///		\n\n
///		The internal interface is somewhat exposed to the caller via methods, 
///		but the design paradigm of ring buffers is very stable and not 
///		expected or even desired to change.
///		\n\n
///	Operation of a buffer:
///		buflen is the size of the buffer.
///		buf points to the start of the buffer.
///		start points to the next byte of data to be read.
///		end points to the next available space to store new data
///		endbuf points just past the end of the buffer.
///		\n\n
///	The buffer is empty if start == end. The buffer will thus hold at most 
///	buflen -1 bytes. If a pointer is incremented past the end of the buffer,
///	we always wrap before returning.
///
///	@remarks This module is not thread-safe. It is the callers responsibility
///	to perform all thread synchronization.
//
////////////////////////////////// Copyright ///////////////////////////////////
//
//	@copy	default
//	
//	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
//	
//	This software is distributed under commercial and open source licenses.
//	You may use the GPL open source license described below or you may acquire 
//	a commercial license from Mbedthis Software. You agree to be fully bound 
//	by the terms of either license. Consult the LICENSE.TXT distributed with 
//	this software for full details.
//	
//	This software is open source; you can redistribute it and/or modify it 
//	under the terms of the GNU General Public License as published by the 
//	Free Software Foundation; either version 2 of the License, or (at your 
//	option) any later version. See the GNU General Public License for more 
//	details at: http://www.mbedthis.com/downloads/gplLicense.html
//	
//	This program is distributed WITHOUT ANY WARRANTY; without even the 
//	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
//	
//	This GPL license does NOT permit incorporating this software into 
//	proprietary programs. If you are unable to comply with the GPL, you must
//	acquire a commercial license to use this software. Commercial licenses 
//	for this software and support services are available from Mbedthis 
//	Software at http://www.mbedthis.com 
//	
//	@end
//
////////////////////////////////// Includes ////////////////////////////////////

#include	"mpr.h"

//////////////////////////////////// Code //////////////////////////////////////
//
//	Allow creation of the buffer via a constructor below or via setBuffer()
//

MprBuf::MprBuf()
{
	growBy = MPR_BUFSIZE;
	buf = 0;
	maxsize = -1;
	buflen = 0;
	endbuf = 0;
	start = 0;
	end = 0;
	refillProc = 0;
	refillArg = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Create a new buffer. "maxsize" is the limit to which the buffer can 
//	ever grow. -1 means no limit. The buffer can ever only fix maxsize-1 bytes.
//	"initialSize" is used to define the amount to increase the size of the 
//	buffer each time if it becomes full. (Note: grow() will exponentially 
//	increase this number for performance.)
//

MprBuf::MprBuf(int initialSize, int max)
{
	buf = 0;
	refillProc = 0;
	refillArg = 0;
	setBuf(initialSize, max);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Delete the buffer
//

MprBuf::~MprBuf()
{
	if (buf && growBy >= 0) {
		mprFree(buf);
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Define a user buffer
//

void MprBuf::setBuf(uchar *userBuf, int initialSize)
{
	mprAssert(initialSize > 0);

	//
	//	Add one because ring queues are always one byte short
	//
	initialSize++;

	if (buf && growBy > 0) {
		mprFree(buf);
	}
	buf = userBuf;
	growBy = -1;							// Signifies a user buffer
	maxsize = -1;
	buflen = initialSize;
	endbuf = &buf[buflen];
	start = buf;
	end = buf;
	*start = '\0';
}

////////////////////////////////////////////////////////////////////////////////
//
//	Set the initial buffer parameters and create the first buffer
//

void MprBuf::setBuf(int initialSize, int max)
{
	mprAssert(initialSize > 0);


	if (max > 0 && initialSize > max) {
		initialSize = max;
	}

	//
	//	Add one because ring queues are always one byte short
	//
	initialSize++;

	if (buf && growBy > 0) {
		mprFree(buf);
	}

	buf = (uchar*) mprMalloc(initialSize);
	growBy = initialSize - 1;
	maxsize = max;
	buflen = initialSize;
	endbuf = &buf[buflen];
	start = buf;
	end = buf;
	*start = '\0';
}

////////////////////////////////////////////////////////////////////////////////
//
//	Preserve (and remove) the storage buffer from the buf object
//

uchar *MprBuf::takeBuffer()
{
	uchar	*bp;

	bp = buf;
	buf = 0;
	maxsize = 0;
	buflen = 0;
	endbuf = 0;
	start = 0;
	end = 0;
	return bp;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Look at the last character put into the buffer without reading it. 
//	Return -1 if empty.
//

int MprBuf::lookLast()
{
	if (start == end) {
		return -1;
	}
	return (end == buf) ? endbuf[-1] : end[-1];
}

////////////////////////////////////////////////////////////////////////////////
//
//	Insert a character at the front of the buffer
//

int MprBuf::insert(char c)
{
	char		*cp;
	int			space;

	mprAssert(buflen == (endbuf - buf));

	space = buflen - getLength() - 1;
	if (space < (int) sizeof(char)) {
		if (!grow()) {
			return -1;
		}
	}
	if (start <= buf) {
		start = endbuf;
	}
	cp = (char*) start;
	*--cp = (char) c;
	start = (uchar *) cp;
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a character to the buffer
//

int MprBuf::put(char c)
{
	char	*cp;
	int		space;

	mprAssert(buflen == (endbuf - buf));

	space = buflen - getLength() - 1;
	if (space < (int) sizeof(char)) {
		if (! grow()) {
			return -1;
		}
	}

	cp = (char*) end;
	*cp++ = (char) c;
	end = (uchar *) cp;
	if (end >= endbuf) {
		end = buf;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a string to the buffer (always null terminated).
//

int MprBuf::put(char *str)
{
	int		rc;

	mprAssert(str);
	mprAssert(buflen == (endbuf - buf));

	rc = put((uchar*) str, strlen(str) * sizeof(char));
	*((char*) end) = (char) '\0';

	return rc;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a number to the buffer (always null terminated).
//

int MprBuf::putInt(int i)
{
	char	numBuf[16];
	int		rc;

	mprItoa(i, numBuf, sizeof(numBuf));
	rc = put(numBuf);
	*((char*) end) = (char) '\0';

	return rc;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Add a block of data to the buf. Return the number of bytes added.
//

int MprBuf::put(uchar *ibuf, int size)
{
	int		thisLen, bytes;

	mprAssert(ibuf);
	mprAssert(size >= 0);
	mprAssert(buflen == (endbuf - buf));

	//
	//	Add the max we can in one copy
	//
	bytes = 0;
	while (size > 0) {
		thisLen = min(getLinearSpace(), size);
		if (thisLen <= 0) {
			if (! grow()) {
				break;
			}
			thisLen = min(getLinearSpace(), size);
		}

		memcpy(end, ibuf, thisLen);
		ibuf += thisLen;
		end += thisLen;
		size -= thisLen;
		bytes += thisLen;

		if (end >= endbuf) {
			end = buf;
		}
	}
	return bytes;
}

////////////////////////////////////////////////////////////////////////////////

int MprBuf::putFmt(char *fmt, ...)
{
	va_list		vargs;
	char		buf[MPR_MAX_STRING];
	int			len;
	
	va_start(vargs, fmt);

	len = mprVsprintf(buf, MPR_MAX_STRING, fmt, vargs);
	if (len >= MPR_MAX_STRING) {
		mprLog(MPR_VERBOSE, " putFmt buffer overflow\n");
		va_end(vargs);
		return 0;
	}
	va_end(vargs);
	return put((uchar*) buf, len);
}

////////////////////////////////////////////////////////////////////////////////
//
//	Get a block of data from the buffer. Return the number of bytes in the block
//

int MprBuf::get(uchar *obuf, int size)
{
	int		thisLen, bytesRead;

	mprAssert(obuf);
	mprAssert(size > 0);
	mprAssert(buflen == (endbuf - buf));

	//
	//	Get the max bytes in a straight copy
	//
	bytesRead = 0;
	while (size > 0) {
		thisLen = getLinearData();
		thisLen = min(thisLen, size);
		if (thisLen <= 0) {
			break;
		}

		memcpy(obuf, start, thisLen);
		obuf += thisLen;
		start += thisLen;
		size -= thisLen;
		bytesRead += thisLen;

		if (start >= endbuf) {
			start = buf;
		}
	}
	return bytesRead;
}

////////////////////////////////////////////////////////////////////////////////
//
//	Adjust the end pointer after a user copy.
//

void MprBuf::adjustEnd(int size)
{
	mprAssert(buflen == (endbuf - buf));
	mprAssert(size < buflen);

	end += size;
	if (end >= endbuf) {
		end -= buflen;
	}
	if (end < buf) {
		end += buflen;
	}

	if (end >= endbuf) {
		mprAssert(end < endbuf);
		flush();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Adjust the start pointer after a user copy
//

void MprBuf::adjustStart(int size)
{
	mprAssert(buflen == (endbuf - buf));
	mprAssert(size < buflen);

	start += size;
	while (start >= endbuf) {
		start -= buflen;
	}
	while (start < buf) {
		start += buflen;
	}

	//
	//	Flush the buffer if the start pointer is corrupted via a bad size
	//
	if (start >= endbuf) {
		mprAssert(start < endbuf);
		flush();
	}
}

////////////////////////////////////////////////////////////////////////////////
//
//	Grow the buffer to fit new data. Return 1 if the buffer can grow. 
//	Grow using the growBy size specified when creating the buffer. 
//

int MprBuf::grow()
{
	uchar	*newbuf;

	if (maxsize >= 0 && buflen >= maxsize) {
		return 0;
	}
	if (growBy < 0) {
		//
		//	User supplied buffer
		//
		return 0;
	}

	if (start > end) {
		copyDown();
	}

	newbuf = (uchar*) mprMalloc(buflen + growBy);
	if (buf) {
		memcpy(newbuf, buf, buflen);
		mprFree(buf);
	}

	buflen += growBy;
	end = newbuf + (end - buf);
	start = newbuf + (start - buf);
	buf = newbuf;
	endbuf = &buf[buflen];

	//
	//	Increase growBy to reduce overhead
	//
	if (((buflen - 1) + (growBy * 2)) < maxsize) {
		growBy *= 2;
	}
mprAssert(buflen < 800000);
	return 1;
}

////////////////////////////////////////////////////////////////////////////////

void MprBuf::copyDown()
{
	if (getLength() == 0) {
		flush();
		return;
	}
	memmove(buf, start, (end - start));
	end -= (start - buf);
	start = buf;
mprAssert(buflen < 800000);
}

////////////////////////////////////////////////////////////////////////////////

//
// Local variables:
// tab-width: 4
// c-basic-offset: 4
// End:
// vim: sw=4 ts=4 
//
