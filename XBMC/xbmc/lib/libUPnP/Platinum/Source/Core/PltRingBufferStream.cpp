/*****************************************************************
|
|   Platinum - Ring Buffer Stream
|
| Copyright (c) 2004-2008, Plutinosoft, LLC.
| All rights reserved.
| http://www.plutinosoft.com
|
| This program is free software; you can redistribute it and/or
| modify it under the terms of the GNU General Public License
| as published by the Free Software Foundation; either version 2
| of the License, or (at your option) any later version.
|
| OEMs, ISVs, VARs and other distributors that combine and 
| distribute commercially licensed software with Platinum software
| and do not wish to distribute the source code for the commercially
| licensed software under version 2, or (at your option) any later
| version, of the GNU General Public License (the "GPL") must enter
| into a commercial license agreement with Plutinosoft, LLC.
| 
| This program is distributed in the hope that it will be useful,
| but WITHOUT ANY WARRANTY; without even the implied warranty of
| MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
| GNU General Public License for more details.
|
| You should have received a copy of the GNU General Public License
| along with this program; see the file LICENSE.txt. If not, write to
| the Free Software Foundation, Inc., 
| 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
| http://www.gnu.org/licenses/gpl-2.0.html
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltRingBufferStream.h"
#include "Neptune.h"

/*----------------------------------------------------------------------
|   defines
+---------------------------------------------------------------------*/
#ifdef max
#undef max
#endif
#define max(a,b)    (((a) > (b)) ? (a) : (b))

#ifdef min
#undef min
#endif
#define min(a,b)    (((a) < (b)) ? (a) : (b))

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::PLT_RingBufferStream
+---------------------------------------------------------------------*/
PLT_RingBufferStream::PLT_RingBufferStream(NPT_Size buffer_size,
                                           bool     blocking /* = true */) : 
    m_TotalBytesRead(0),
    m_TotalBytesWritten(0),
    m_Eos(false),
    m_Blocking(blocking)
{
    m_RingBuffer = new NPT_RingBuffer(buffer_size);
}

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::PLT_RingBufferStream
+---------------------------------------------------------------------*/
PLT_RingBufferStream::PLT_RingBufferStream(NPT_RingBufferReference& buffer,
                                           bool blocking /* = true */) : 
    m_RingBuffer(buffer),
    m_TotalBytesRead(0),
    m_TotalBytesWritten(0),
    m_Eos(false),
    m_Blocking(blocking)
{
}

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::~PLT_RingBufferStream
+---------------------------------------------------------------------*/
PLT_RingBufferStream::~PLT_RingBufferStream()
{
}

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::Read
+---------------------------------------------------------------------*/
NPT_Result 
PLT_RingBufferStream::Read(void*     buffer, 
                           NPT_Size  max_bytes_to_read, 
                           NPT_Size* bytes_read /*= NULL*/)
{
    NPT_Size bytes_avail, to_read;
    NPT_Size last_read = 0;

    // reset output param first
    if (bytes_read) *bytes_read = 0;

    // wait for data
    do {
        {
            NPT_AutoLock autoLock(m_Lock);
            if (m_RingBuffer->GetContiguousAvailable()) break;
        }

        if (m_Eos || m_RingBuffer->IsClosed()) {
            return NPT_ERROR_EOS;
        } else if (!m_Blocking) {
            return NPT_ERROR_WOULD_BLOCK;
        }

        // sleep and try again
        NPT_System::Sleep(NPT_TimeInterval(0, 10000));
    } while (1);

    {
        NPT_AutoLock autoLock(m_Lock);

        // try twice in case we looped
        for (int i=0; i<2; i++) {
            bytes_avail = m_RingBuffer->GetContiguousAvailable();
            to_read = min(max_bytes_to_read - last_read, bytes_avail);

            // break if nothing to read
            if (to_read == 0) break;

            // read into buffer and advance
            NPT_CHECK(m_RingBuffer->Read((unsigned char*)buffer+last_read, to_read));

            // keep track of the total bytes we have read so far
            m_TotalBytesRead += to_read;
            last_read += to_read;

            if (bytes_read) *bytes_read += to_read;
        }
    }

    // we have read some chars, so return success
    // even if we have read less than asked
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::Write
+---------------------------------------------------------------------*/
NPT_Result 
PLT_RingBufferStream::Write(const void* buffer, 
                            NPT_Size    bytes_to_write, 
                            NPT_Size*   bytes_written /*= NULL*/)
{
    NPT_Size     space_avail, to_write;
    NPT_Size     last_written = 0;

    // reset output param first
    if (bytes_written) *bytes_written = 0;

    // wait for space
    do {
        {
            NPT_AutoLock autoLock(m_Lock);
            if (m_RingBuffer->GetContiguousSpace()) break;
        }

        if (m_Eos || m_RingBuffer->IsClosed()) {
            return NPT_ERROR_EOS;
        } else if (!m_Blocking) {
            return NPT_ERROR_WOULD_BLOCK;
        }

        // sleep and try again
        NPT_System::Sleep(NPT_TimeInterval(0, 10000));
    } while (1);

    {
        NPT_AutoLock autoLock(m_Lock);

        // try twice in case we looped
        for (int i=0; i<2; i++) {
            space_avail = m_RingBuffer->GetContiguousSpace();
            to_write = min(bytes_to_write - last_written, space_avail);

            // break if no space to write
            if (to_write == 0) break;

            // write into buffer
            NPT_CHECK(m_RingBuffer->Write((unsigned char*)buffer+last_written, to_write));

            m_TotalBytesWritten += to_write; 
            last_written += to_write;

            if (bytes_written) *bytes_written += to_write;
        }
    }

    // we have written some chars, so return success
    // even if we have written less than provided
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::Flush
+---------------------------------------------------------------------*/
NPT_Result 
PLT_RingBufferStream::Flush()
{
    NPT_AutoLock autoLock(m_Lock);

    m_RingBuffer->Flush();
    m_TotalBytesRead = 0;
    m_TotalBytesWritten = 0;
    return NPT_SUCCESS;
}
