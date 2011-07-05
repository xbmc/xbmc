/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "pch.h"
#include "PCMCodec.h"
#include "utils/log.h"

PCMCodec::PCMCodec()
{
	m_CodecName = "PCM";
	m_TotalTime = 0;
	m_SampleRate = 44100;
	m_Channels = 2;
	m_BitsPerSample = 16;
	m_Bitrate = m_SampleRate * m_Channels * m_BitsPerSample;
	iBytesPerSecond = m_Bitrate / 8;
}

PCMCodec::~PCMCodec()
{
	DeInit();
}

bool PCMCodec::Init(const CStdString &strFile, unsigned int filecache)
{
	m_file.Close();
	if (!m_file.Open(strFile, READ_CACHED))
	{
		CLog::Log(LOGERROR, "PCMCodec::Init - Failed to open file");
		return false;
	}
	int64_t length = m_file.GetLength();
	m_TotalTime = (int)(((float)length / iBytesPerSecond) * 1000);
	m_file.Seek(0, SEEK_SET);
	return true;
}

void PCMCodec::DeInit()
{
	m_file.Close();
}

__int64 PCMCodec::Seek(__int64 iSeekTime)
{
	m_file.Seek((iSeekTime / 1000) * iBytesPerSecond);
	return iSeekTime;
}

int PCMCodec::ReadPCM(BYTE *pBuffer, int size, int *actualsize)
{
	*actualsize = 0;
	int iAmountRead = m_file.Read(pBuffer, size);
	if (iAmountRead > 0)
	{
		*actualsize = iAmountRead;
		return READ_SUCCESS;
	}
	return READ_ERROR;
}

bool PCMCodec::CanInit()
{
	return true;
}

void PCMCodec::SetMimeParams(const CStdString& strMimeParams)
{
	if (strMimeParams.Find("rate=48000") > 0)
		m_SampleRate = 48000;
	if (strMimeParams.Find("channels=1") > 0)
		m_Channels = 1;

	m_Bitrate = m_SampleRate * m_Channels * m_BitsPerSample;
	iBytesPerSecond = m_Bitrate / 8;
}
