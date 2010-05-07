/**
 *
 * Lame ACM wrapper, encode/decode MP3 based RIFF/AVI files in MS Windows
 *
 *  Copyright (c) 2002 Steve Lhomme <steve.lhomme at free.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
 
/*!
	\author Steve Lhomme
	\version \$Id: DecodeStream.cpp,v 1.3 2002/01/25 17:51:42 robux4 Exp $
*/

#if !defined(STRICT)
#define STRICT
#endif // STRICT

#include <assert.h>
#include <windows.h>

#ifdef ENABLE_DECODING

#include "adebug.h"

#include "DecodeStream.h"

// static methods

DecodeStream * DecodeStream::Create()
{
	DecodeStream * Result;

	Result = new DecodeStream;

	return Result;
}

const bool DecodeStream::Erase(const DecodeStream * a_ACMStream)
{
	delete a_ACMStream;
	return true;
}

// class methods

DecodeStream::DecodeStream() :
 m_WorkingBufferUseSize(0),
 gfp(NULL)
{
	 /// \todo get the debug level from the registry
my_debug = new ADbg(DEBUG_LEVEL_CREATION);
	if (my_debug != NULL) {
		unsigned char DebugFileName[512];

		my_debug->setPrefix("MPG123stream"); /// \todo get it from the registry
my_debug->setIncludeTime(true);  /// \todo get it from the registry

// Check in the registry if we have to Output Debug information
DebugFileName[0] = '\0';

		HKEY OssKey;
		if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SOFTWARE\\MUKOLI", 0, KEY_READ , &OssKey ) == ERROR_SUCCESS) {
			DWORD DataType;
			DWORD DebugFileNameSize = 512;
			if (RegQueryValueEx( OssKey, "DebugFile", NULL, &DataType, DebugFileName, &DebugFileNameSize ) == ERROR_SUCCESS) {
				if (DataType == REG_SZ) {
					my_debug->setUseFile(true);
					my_debug->setDebugFile((char *)DebugFileName);
					my_debug->OutPut("Debug file is %s",(char *)DebugFileName);
				}
			}
		}
		my_debug->OutPut(DEBUG_LEVEL_FUNC_START, "DecodeStream Creation (0X%08X)",this);
	}
	else {
		ADbg debug;
		debug.OutPut("DecodeStream::ACMACMStream : Impossible to create my_debug");
	}

}

DecodeStream::~DecodeStream()
{
//	lame_close( gfp );

	if (my_debug != NULL)
	{
		my_debug->OutPut(DEBUG_LEVEL_FUNC_START, "DecodeStream Deletion (0X%08X)",this);
		delete my_debug;
	}
}

bool DecodeStream::init(const int nSamplesPerSec, const int nChannels, const int nAvgBytesPerSec, const int nSourceBitrate)
{
	bool bResult = false;

	my_SamplesPerSec  = nSamplesPerSec;
	my_Channels       = nChannels;
	my_AvgBytesPerSec = nAvgBytesPerSec;
	my_SourceBitrate  = nSourceBitrate;

	bResult = true;

	return bResult;
}

bool DecodeStream::open()
{
	bool bResult = false;

	bResult = bool(InitMP3(&my_DecodeData) != 0);

	return bResult;
}

bool DecodeStream::close(LPBYTE pOutputBuffer, DWORD *pOutputSize)
{

	bool bResult = false;
/*
	int nOutputSamples = 0;

    nOutputSamples = lame_encode_flush( gfp, pOutputBuffer, 0 );

	if ( nOutputSamples < 0 )
	{
		// BUFFER_TOO_SMALL
		*pOutputSize = 0;
	}
	else
	{
		*pOutputSize = nOutputSamples;

		bResult = true;
	}
/*
	// lame will be close in VbrWriteTag function
	if ( !lame_get_bWriteVbrTag( gfp ) )
	{
		// clean up of allocated memory
		lame_close( gfp );
	}
*/
    
	ExitMP3(&my_DecodeData);

	bResult = true;
    
	return bResult;
}

DWORD DecodeStream::GetOutputSizeForInput(const DWORD the_SrcLength) const
{
	DWORD Result;

	double OutputInputRatio = double(my_SamplesPerSec * 2 * my_Channels) / double(my_SourceBitrate);

	OutputInputRatio *= 1.15; // allow 15% more

	Result = DWORD(double(the_SrcLength) * OutputInputRatio);

	my_debug->OutPut(DEBUG_LEVEL_FUNC_CODE, "Result = %d (OutputInputRatio = %f)",Result,OutputInputRatio);

	return Result;
}

bool DecodeStream::ConvertBuffer(LPACMDRVSTREAMHEADER a_StreamHeader)
{
	bool result = false;

if (my_debug != NULL)
{
my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "enter DecodeStream::ConvertBuffer");
}

	int ProcessedBytes;

	int ret = decodeMP3(&my_DecodeData, a_StreamHeader->pbSrc, a_StreamHeader->cbSrcLength, (char *)a_StreamHeader->pbDst, a_StreamHeader->cbDstLength, &ProcessedBytes);

	switch (ret)
	{
	    case MP3_OK:
			a_StreamHeader->cbSrcLengthUsed = a_StreamHeader->cbSrcLength;
			a_StreamHeader->cbDstLengthUsed = ProcessedBytes;
			result = true;
			break;
	    case MP3_NEED_MORE:
			a_StreamHeader->cbSrcLengthUsed = 0;
	a_StreamHeader->cbDstLengthUsed = 0;
			break;
	    case MP3_ERR:
			break;
	}

/*
	DWORD InSize = a_StreamHeader->cbSrcLength / 2, OutSize = a_StreamHeader->cbDstLength; // 2 for 8<->16 bits

// Encode it
int dwSamples;
	int nOutputSamples = 0;

	dwSamples = InSize / lame_get_num_channels( gfp );

	if ( 1 == lame_get_num_channels( gfp ) )
	{
		nOutputSamples = lame_encode_buffer(gfp,(PSHORT)a_StreamHeader->pbSrc,(PSHORT)a_StreamHeader->pbSrc,dwSamples,a_StreamHeader->pbDst,a_StreamHeader->cbDstLength);
	}
	else
	{
		nOutputSamples = lame_encode_buffer_interleaved(gfp,(PSHORT)a_StreamHeader->pbSrc,dwSamples,a_StreamHeader->pbDst,a_StreamHeader->cbDstLength);
	}

	a_StreamHeader->cbSrcLengthUsed = a_StreamHeader->cbSrcLength;
	a_StreamHeader->cbDstLengthUsed = nOutputSamples;

	result = a_StreamHeader->cbDstLengthUsed <= a_StreamHeader->cbDstLength;
*/
	my_debug->OutPut(DEBUG_LEVEL_FUNC_CODE, "UsedSize = %d / EncodedSize = %d, result = %d, ret = %s", a_StreamHeader->cbSrcLengthUsed, a_StreamHeader->cbDstLengthUsed, result,
		(ret == MP3_OK)?"MP3_OK":(ret == MP3_NEED_MORE)?"MP3_NEED_MORE":"error");

if (my_debug != NULL)
{
my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "DecodeStream::ConvertBuffer result = %d",result);
}

	return result;
}
#endif // ENABLE_DECODING
