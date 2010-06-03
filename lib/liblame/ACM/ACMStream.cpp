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
	\version \$Id: ACMStream.cpp,v 1.12.2.1 2008/11/01 20:41:47 robert Exp $
*/

#if !defined(STRICT)
#define STRICT
#endif // STRICT

#include <assert.h>
#include <windows.h>

#include "adebug.h"

#include "ACMStream.h"

#include <lame.h>

// static methods

ACMStream * ACMStream::Create()
{
	ACMStream * Result;

	Result = new ACMStream;

	return Result;
}

const bool ACMStream::Erase(const ACMStream * a_ACMStream)
{
	delete a_ACMStream;
	return true;
}

// class methods

ACMStream::ACMStream() :
 m_WorkingBufferUseSize(0),
 gfp(NULL)
{
	 /// \todo get the debug level from the registry
my_debug = new ADbg(DEBUG_LEVEL_CREATION);
	if (my_debug != NULL) {
		unsigned char DebugFileName[512];

		my_debug->setPrefix("LAMEstream"); /// \todo get it from the registry
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
		my_debug->OutPut(DEBUG_LEVEL_FUNC_START, "ACMStream Creation (0X%08X)",this);
	}
	else {
		ADbg debug;
		debug.OutPut("ACMStream::ACMACMStream : Impossible to create my_debug");
	}

}

ACMStream::~ACMStream()
{
        // release memory - encoding is finished
	if (gfp) lame_close( gfp );

	if (my_debug != NULL)
	{
		my_debug->OutPut(DEBUG_LEVEL_FUNC_START, "ACMStream Deletion (0X%08X)",this);
		delete my_debug;
	}
}

bool ACMStream::init(const int nSamplesPerSec, const int nOutputSamplesPerSec, const int nChannels, const int nAvgBytesPerSec, const vbr_mode mode)
{
	bool bResult = false;

	my_SamplesPerSec  = nSamplesPerSec;
	my_OutBytesPerSec = nOutputSamplesPerSec;
	my_Channels       = nChannels;
	my_AvgBytesPerSec = nAvgBytesPerSec;
	my_VBRMode = mode;

	bResult = true;

	return bResult;

}

bool ACMStream::open(const AEncodeProperties & the_Properties)
{
	bool bResult = false;

	// Init the MP3 Stream
	// Init the global flags structure
	gfp = lame_init();

	// Set input sample frequency
	lame_set_in_samplerate( gfp, my_SamplesPerSec );

	// Set output sample frequency
	lame_set_out_samplerate( gfp, my_OutBytesPerSec );

	lame_set_num_channels( gfp, my_Channels );
	if (my_Channels == 1)
		lame_set_mode( gfp, MONO );
	else
		lame_set_mode( gfp, (MPEG_mode_e)the_Properties.GetChannelModeValue()) ; /// \todo Get the mode from the default configuration

//	lame_set_VBR( gfp, vbr_off ); /// \note VBR not supported for the moment
	lame_set_VBR( gfp, my_VBRMode ); /// \note VBR not supported for the moment
	
	if (my_VBRMode == vbr_abr)
	{
		lame_set_VBR_q( gfp, 1 );

		lame_set_VBR_mean_bitrate_kbps( gfp, (my_AvgBytesPerSec * 8 + 500) / 1000 );

		if (24000 > lame_get_in_samplerate( gfp ))
		{
			// For MPEG-II
			lame_set_VBR_min_bitrate_kbps( gfp, 8);

			lame_set_VBR_max_bitrate_kbps( gfp, 160);
		}
		else
		{
			// For MPEG-I
			lame_set_VBR_min_bitrate_kbps( gfp, 32);

			lame_set_VBR_max_bitrate_kbps( gfp, 320);
		}
	}

	// Set bitrate
	lame_set_brate( gfp, my_AvgBytesPerSec * 8 / 1000 );

	/// \todo Get the mode from the default configuration
	// Set copyright flag?
	lame_set_copyright( gfp, the_Properties.GetCopyrightMode()?1:0 );
	// Do we have to tag  it as non original 
	lame_set_original( gfp, the_Properties.GetOriginalMode()?1:0 );
	// Add CRC?
	lame_set_error_protection( gfp, the_Properties.GetCRCMode()?1:0 );
	// Set private bit?
	lame_set_extension( gfp, the_Properties.GetPrivateMode()?1:0 );
	// INFO tag support not possible in ACM - it requires rewinding 
        // output stream to the beginning after encoding is finished.   
	lame_set_bWriteVbrTag( gfp, 0 );

	if (0 == lame_init_params( gfp ))
	{
		//LAME encoding call will accept any number of samples.  
		if ( 0 == lame_get_version( gfp ) )
		{
			// For MPEG-II, only 576 samples per frame per channel
			my_SamplesPerBlock = 576 * lame_get_num_channels( gfp );
		}
		else
		{
			// For MPEG-I, 1152 samples per frame per channel
			my_SamplesPerBlock = 1152 * lame_get_num_channels( gfp );
		}
	}

	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "version                =%d",lame_get_version( gfp ) );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "Layer                  =3");
	switch ( lame_get_mode( gfp ) )
	{
		case STEREO:       my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG,  "mode                   =Stereo" ); break;
		case JOINT_STEREO: my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG,  "mode                   =Joint-Stereo" ); break;
		case DUAL_CHANNEL: my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG,  "mode                   =Forced Stereo" ); break;
		case MONO:         my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG,  "mode                   =Mono" ); break;
		case NOT_SET:      /* FALLTROUGH */
		default:           my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG,  "mode                   =Error (unknown)" ); break;
	}

	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "sampling frequency     =%.1f kHz", lame_get_in_samplerate( gfp ) /1000.0 );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "bitrate                =%d kbps", lame_get_brate( gfp ) );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "Vbr Min bitrate        =%d kbps", lame_get_VBR_min_bitrate_kbps( gfp ) );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "Vbr Max bitrate        =%d kbps", lame_get_VBR_max_bitrate_kbps( gfp ) );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "Quality Setting        =%d", lame_get_quality( gfp ) );

	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "Low pass frequency     =%d", lame_get_lowpassfreq( gfp ) );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "Low pass width         =%d", lame_get_lowpasswidth( gfp ) );

	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "High pass frequency    =%d", lame_get_highpassfreq( gfp ) );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "High pass width        =%d", lame_get_highpasswidth( gfp ) );

	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "No Short Blocks        =%d", lame_get_no_short_blocks( gfp ) );

	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "de-emphasis            =%d", lame_get_emphasis( gfp ) );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "private flag           =%d", lame_get_extension( gfp ) );

	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "copyright flag         =%d", lame_get_copyright( gfp ) );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "original flag          =%d",	lame_get_original( gfp ) );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "CRC                    =%s", lame_get_error_protection( gfp ) ? "on" : "off" );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "Fast mode              =%s", ( lame_get_quality( gfp ) )? "enabled" : "disabled" );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "Force mid/side stereo  =%s", ( lame_get_force_ms( gfp ) )?"enabled":"disabled" );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "Disable Resorvoir      =%d", lame_get_disable_reservoir( gfp ) );
	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "VBR                    =%s, VBR_q =%d, VBR method =",
					( lame_get_VBR( gfp ) !=vbr_off ) ? "enabled": "disabled",
		            lame_get_VBR_q( gfp ) );

	switch ( lame_get_VBR( gfp ) )
	{
		case vbr_off:	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG,  "vbr_off" );	break;
		case vbr_mt :	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG,  "vbr_mt" );	break;
		case vbr_rh :	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG,  "vbr_rh" );	break;
		case vbr_mtrh:	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG,  "vbr_mtrh" );	break;
		case vbr_abr: 
			my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG,  "vbr_abr (average bitrate %d kbps)", lame_get_VBR_mean_bitrate_kbps( gfp ) );
		break;
		default:
			my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "error, unknown VBR setting");
		break;
	}

	my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "Write VBR Header       =%s\n", ( lame_get_bWriteVbrTag( gfp ) ) ?"Yes":"No");

#ifdef FROM_DLL
beConfig.format.LHV1.dwReSampleRate		= my_OutBytesPerSec;	  // force the user resampling
#endif // FROM_DLL

	bResult = true;

	return bResult;
}

bool ACMStream::close(LPBYTE pOutputBuffer, DWORD *pOutputSize)
{

bool bResult = false;

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

	// lame will be closed in destructor
        //lame_close( gfp );

	return bResult;
}

DWORD ACMStream::GetOutputSizeForInput(const DWORD the_SrcLength) const
{
/*	double OutputInputRatio;

	if (my_VBRMode == vbr_off)
		OutputInputRatio = double(my_AvgBytesPerSec) / double(my_OutBytesPerSec * 2);
	else // reserve the space for 320 kbps
		OutputInputRatio = 40000.0 / double(my_OutBytesPerSec * 2);

	OutputInputRatio *= 1.15; // allow 15% more*/

    DWORD Result;

//	Result = DWORD(double(the_SrcLength) * OutputInputRatio);
    Result = DWORD(1.25*the_SrcLength + 7200);

my_debug->OutPut(DEBUG_LEVEL_FUNC_CODE, "Result = %d",Result);

	return Result;
}

bool ACMStream::ConvertBuffer(LPACMDRVSTREAMHEADER a_StreamHeader)
{
	bool result;

if (my_debug != NULL)
{
my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "enter ACMStream::ConvertBuffer");
}

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

	my_debug->OutPut(DEBUG_LEVEL_FUNC_CODE, "UsedSize = %d / EncodedSize = %d, result = %d (%d <= %d)", InSize, OutSize, result, a_StreamHeader->cbDstLengthUsed, a_StreamHeader->cbDstLength);

if (my_debug != NULL)
{
my_debug->OutPut(DEBUG_LEVEL_FUNC_DEBUG, "ACMStream::ConvertBuffer result = %d (0x%02X 0x%02X)",result,a_StreamHeader->pbDst[0],a_StreamHeader->pbDst[1]);
}

	return result;
}

/* map frequency to a valid MP3 sample frequency
 *
 * Robert Hegemann 2000-07-01
 */
static int
map2MP3Frequency(int freq)
{
    if (freq <= 8000)
        return 8000;
    if (freq <= 11025)
        return 11025;
    if (freq <= 12000)
        return 12000;
    if (freq <= 16000)
        return 16000;
    if (freq <= 22050)
        return 22050;
    if (freq <= 24000)
        return 24000;
    if (freq <= 32000)
        return 32000;
    if (freq <= 44100)
        return 44100;

    return 48000;
}


unsigned int ACMStream::GetOutputSampleRate(int samples_per_sec, int bitrate, int channels)
{
    if (bitrate==0)
        bitrate = (64000*channels)/8;
  
        /// \todo pass through the same LAME routine
	unsigned int OutputFrequency;
	double compression_ratio = double(samples_per_sec * 16 * channels / (bitrate * 8));
	if (compression_ratio > 13.)
		OutputFrequency = map2MP3Frequency( int((10. * bitrate * 8) / (16 * channels)));
	else
		OutputFrequency = map2MP3Frequency( int(0.97 * samples_per_sec) );

	return OutputFrequency;

}

