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
	\version \$Id: ACM.h,v 1.8 2006/12/25 21:37:34 robert Exp $
*/

#if !defined(_ACM_H__INCLUDED_)
#define _ACM_H__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <vector>

#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <msacm.h>
#include <msacmdrv.h>


#include "ADbg/ADbg.h"

class AEncodeProperties;

typedef enum vbr_mode_e vbr_mode;

class bitrate_item {
	public:
		unsigned int frequency;
		unsigned int bitrate;
		unsigned int channels;
		vbr_mode     mode;
	
		bool operator<(const bitrate_item & bitrate) const;
};

class ACM  
{
public:
	ACM( HMODULE hModule );
	virtual ~ACM();

	LONG DriverProcedure(const HDRVR hdrvr, const UINT msg, LONG lParam1, LONG lParam2);

	static const char * GetVersionString(void) {return VersionString;}

protected:
//	inline DWORD Configure( HWND hParentWindow, LPDRVCONFIGINFO pConfig );
	inline DWORD About( HWND hParentWindow );

	inline DWORD OnDriverDetails(const HDRVR hdrvr, LPACMDRIVERDETAILS a_DriverDetail);
	inline DWORD OnFormatTagDetails(LPACMFORMATTAGDETAILS a_FormatTagDetails, const LPARAM a_Query);
	inline DWORD OnFormatDetails(LPACMFORMATDETAILS a_FormatDetails, const LPARAM a_Query);
	inline DWORD OnFormatSuggest(LPACMDRVFORMATSUGGEST a_FormatSuggest);
	inline DWORD OnStreamOpen(LPACMDRVSTREAMINSTANCE a_StreamInstance);
	inline DWORD OnStreamClose(LPACMDRVSTREAMINSTANCE a_StreamInstance);
	inline DWORD OnStreamSize(LPACMDRVSTREAMINSTANCE a_StreamInstance, LPACMDRVSTREAMSIZE the_StreamSize);
	inline DWORD OnStreamPrepareHeader(LPACMDRVSTREAMINSTANCE a_StreamInstance, LPACMSTREAMHEADER a_StreamHeader);
	inline DWORD OnStreamUnPrepareHeader(LPACMDRVSTREAMINSTANCE a_StreamInstance, LPACMSTREAMHEADER a_StreamHeader);
	inline DWORD OnStreamConvert(LPACMDRVSTREAMINSTANCE a_StreamInstance, LPACMDRVSTREAMHEADER a_StreamHeader);

	void GetMP3FormatForIndex(const DWORD the_Index, WAVEFORMATEX & the_Format, unsigned short the_String[ACMFORMATDETAILS_FORMAT_CHARS]) const;
	void GetPCMFormatForIndex(const DWORD the_Index, WAVEFORMATEX & the_Format, unsigned short the_String[ACMFORMATDETAILS_FORMAT_CHARS]) const;
	DWORD GetNumberEncodingFormats() const;
	bool IsSmartOutput(const int frequency, const int bitrate, const int channels) const;
	void BuildBitrateTable();

	HMODULE my_hModule;
	HICON   my_hIcon;
	ADbg    my_debug;
	AEncodeProperties my_EncodingProperties;
	std::vector<bitrate_item> bitrate_table;

	static char VersionString[120];
};

#endif // !defined(_ACM_H__INCLUDED_)

