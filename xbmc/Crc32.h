/*
 * XBoxMediaPlayer
 * Copyright (c) 2002 d7o3g4q and RUNTiME
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#pragma once

class Crc32 
{
public:
		Crc32()
		{
			Reset();
		}
	    
		Crc32(const void* buffer, unsigned int count)
		{
			Reset();
			Compute(buffer, count);
		}

		Crc32(const CStdString& strValue)
		{
			Reset();
			Compute(strValue.c_str(), strValue.size());
		}

		void Reset()
		{
			m_crc = 0xFFFFFFFF;
		}

		void Compute(const void* buffer, unsigned int count);
		void Compute(unsigned char value);
		void Compute(const CStdString& strValue);
		void ComputeFromLowerCase(const CStdString& strValue);

		operator unsigned __int32 () const 
		{
			return m_crc;
		}

private:
		unsigned __int32 m_crc;
};

