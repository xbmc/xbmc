/***************************************************************************
                          smm.cpp  -  sidusage memory map file support
                             -------------------
    begin                : Wed Dec 03 2003
    copyright            : (C) 2003-2004 by Simon White
    email                : sidplay2@yahoo.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <sidplay/sidendian.h>
#include "sidplay/utils/SidUsage.h"
#include "smm0.h"


// Read out all sub chunks that make up the rest of this chunks payload
// usage  - where we read data to
// length - length remaining of this chunk
bool Chunk::read (FILE *file, sid2_usage_t &usage, uint_least32_t length)
{
    m_used = true;

    {   // Initialise data for sub chunks
        Chunk *sub = m_sub;
        while (sub)
        {
            sub->init (usage);
            sub->m_used = false;
            sub = sub->m_next;
        }
    }

    // Read out all sub chunks
    if (length && m_sub)
    {
        uint8_t tmp[4];
        uint_least32_t id, chunk_len;

        // Process all sub chunks
        do
        {   // Align to even byte boundary as per IFF standard
            if (ftell (file) & 1)
            {
                if (!_read (file, tmp, sizeof(uint8_t), length))
                    return false;
            }

            // Read ID field
            if (!_read (file, tmp, sizeof(tmp), length))
                return false;
            id = endian_big32(tmp);

            // Read length
            if (!_read (file, tmp, sizeof(tmp), length))
                return false;
            chunk_len = endian_big32(tmp);
            // Chunk size is bigger than rest of file
            if (chunk_len > length)
                return false;

            // Look up the chunk to process
            Chunk *chunk = match (id);
            if (!chunk)
                fseek (file, (long) length, SEEK_CUR);
            else
            {   // Read sub chunks contents
                if (!chunk->read (file, usage, chunk_len))
                    return false;
            }
            length -= chunk_len;
        } while (length);
    }

    // Align to even byte boundary as per IFF standard
    if (ftell (file) & 1)
    {
        uint8_t tmp;
        if (!_read (file, &tmp, sizeof(uint8_t), length))
        {   // For compatibility with older files that
            // wrongly didn't pad the last chunk
            if (!feof (file))
                return false;
        }
    }

    {   // All compulsory chunks read
        Chunk *sub = m_sub;
        while (sub)
        {
            if (sub->m_compulsory && !sub->m_used)
                return false;
            sub = sub->m_next;
        }
    }
    return length == 0;
}


// Write out all sub chunks that make up the rest of this chunks payload
// usage  - where we obtain data from
// length - final length of this chunk
bool Chunk::write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length)
{
    Chunk *sub = m_sub;
    // Write out all sub chunks
    while (sub)
    {   // Only write out used chunks
        if (!sub->used (usage))
        {
            sub = sub->m_next;
            continue;
        }

        uint8_t tmp[4];

        // Align to even byte boundary as per IFF standard
        if (ftell (file) & 1)
        {
            tmp[0] = 0;
            if (!_write (file, tmp, sizeof(uint8_t), length))
                return false;
        }

        // Write ID
        endian_big32(tmp, sub->m_id);
        if (!_write (file, tmp, sizeof(tmp), length))
            return false;

        // Write length holder
        memset (tmp, 0, sizeof (tmp));
        if (!_write (file, tmp, sizeof(tmp), length))
            return false;

        // Write sub chunks contents
        uint_least32_t chunk_len = 0;
        if (!sub->write (file, usage, chunk_len))
            return false;
        length += chunk_len;

        {   // Write real length
            uint_least32_t dummy = 0;
            // Move back to start chunks length field
            fseek (file, -(long)chunk_len - sizeof(tmp), SEEK_CUR);
            endian_big32(tmp, chunk_len);
            if (!_write (file, tmp, sizeof(tmp), dummy))
                return false;
            // Move to end of chunk
            fseek (file, (long)chunk_len, SEEK_CUR);
        }
        sub = sub->m_next;
    }

    // Align to even byte boundary as per IFF standard
    if (ftell (file) & 1)
    {
        uint8_t tmp = 0;
        if (!_write (file, &tmp, sizeof(uint8_t), length))
            return false;
    }

    return true;
}


// Read some bytes from the remaining data available in this chunk payload
// data      - output buffer
// length    - length of output buffer
// remaining - length remaining of chunk
bool Chunk::_read (FILE *file, uint8_t *data, uint_least32_t length, uint_least32_t &remaining)
{
    if (length)
    {
        size_t ret = fread ((char *)data, length, 1, file);
        if ( ret != 1 )
            return false;
        if (length > remaining)
            return false;
    }
    remaining -= length;
    return true;
}


// Write bytes into the chunks payload
// data   - input buffer
// length - length of input buffer
// count  - total count of bytes in chunks payload
bool Chunk::_write (FILE *file, const uint8_t *data, uint_least32_t length, uint_least32_t &count)
{
    if (length)
    {
        size_t ret = fwrite ((char *)data, length, 1, file);
        if ( ret != 1 )
            return false;
    }
    count += length;
    return true;
}

Chunk *Chunk::match (uint_least32_t id)
{
    Chunk *sub = m_sub;
    while (sub)
    {
        if (sub->m_id == id)
            break;
        sub = sub->m_next;
    }
    return sub;
}

//-----------------------------------------------------------------------------
// INF support functions
void Inf_v0::init (sid2_usage_t &usage)
{
    usage.start = 0;
    usage.end   = 0;
}

bool Inf_v0::read (FILE *file, sid2_usage_t &usage, uint_least32_t length)
{
    uint8_t tmp[2];
    if (!_read (file, tmp, sizeof(tmp), length))
        return false;
    usage.start = endian_big16 (tmp);
    if (!_read (file, tmp, sizeof(tmp), length))
        return false;
    usage.end   = endian_big16 (tmp);

    // Check read values are valid
    if (usage.end < usage.start)
        return false;

    return Chunk::read (file, usage, length);
}

bool Inf_v0::write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length)
{
    uint8_t tmp[2];
    endian_big16 (tmp, usage.start);
    if (!_write (file, tmp, sizeof(tmp), length))
        return false;
    endian_big16 (tmp, usage.end);
    if (!_write (file, tmp, sizeof(tmp), length))
        return false;
    return Chunk::write (file, usage, length);
}


//-----------------------------------------------------------------------------
// ERR support functions
void Err_v0::init (sid2_usage_t &usage)
{
    usage.flags = 0;
}

bool Err_v0::read (FILE *file, sid2_usage_t &usage, uint_least32_t length)
{
    uint8_t tmp[2];
    if (!_read (file, tmp, sizeof(tmp), length))
        return false;
    usage.flags = endian_big16 (tmp);

    // If the block hasn't completed then we have 32 bit flags
    if (length)
    {
        usage.flags <<= 16;
        if (!_read (file, tmp, sizeof(tmp), length))
            return false;
        usage.flags |= endian_big16 (tmp);
    }
    return Chunk::read (file, usage, length);
}

bool Err_v0::write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length)
{
    uint8_t tmp[2];
    uint_least16_t flags = (uint_least16_t) (usage.flags >> 16);

    // For compatibility only write these flags if they are present
    if (flags)
    {
        if (!_write (file, tmp, sizeof(tmp), length))
            return false;
    }

    flags = (uint_least16_t) usage.flags;
    endian_big16 (tmp, flags);
    if (!_write (file, tmp, sizeof(tmp), length))
        return false;
    return Chunk::write (file, usage, length);    
}

bool Err_v0::used (const sid2_usage_t &usage)
{
    if (!usage.flags)
        return false;
    return true;
}


//-----------------------------------------------------------------------------
// MD5 support functions
void Md5::init (sid2_usage_t &usage)
{
    usage.md5[0] = '\0';
    usage.md5[SIDTUNE_MD5_LENGTH] = '\0';
}

bool Md5::read (FILE *file, sid2_usage_t &usage, uint_least32_t length)
{
    if (!_read (file, (uint8_t *) usage.md5, SIDTUNE_MD5_LENGTH, length))
        return false;
    // Validate MD5 is legal values
    // @FIXME@ check hex and convert case to lower??
    if (strlen(usage.md5) != SIDTUNE_MD5_LENGTH)
        return false;
    return Chunk::read (file, usage, length);
}

bool Md5::write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length)
{
    if (!_write (file, (const uint8_t *) usage.md5, SIDTUNE_MD5_LENGTH, length))
        return false;
    return Chunk::write (file, usage, length);    
}

bool Md5::used (const sid2_usage_t &usage)
{
    if (strlen (usage.md5) != SIDTUNE_MD5_LENGTH)
        return false;
    return true;
}


//-----------------------------------------------------------------------------
// Time support functions
void Time::init (sid2_usage_t &usage)
{
    usage.length = 0;
}

bool Time::read (FILE *file, sid2_usage_t &usage, uint_least32_t length)
{
    uint8_t tmp[2];
    if (!_read (file, tmp, sizeof(tmp), length))
        return false;
    usage.length = endian_big16 (tmp);
    return Chunk::read (file, usage, length);
}

bool Time::write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length)
{
    uint8_t tmp[2];
    endian_big16 (tmp, usage.length);
    if (!_write (file, tmp, sizeof(tmp), length))
        return false;
    return Chunk::write (file, usage, length);    
}

bool Time::used (const sid2_usage_t &usage)
{
    if (!usage.length)
        return false;
    return true;
}


//-----------------------------------------------------------------------------
// Body support functions
void Body::init (sid2_usage_t &usage)
{
    m_pages = 0;
    memset (usage.memory, 0, sizeof (usage.memory));
}

bool Body::read (FILE *file, sid2_usage_t &usage, uint_least32_t length)
{
    if (!length)
        return Chunk::read (file, usage, length);

    for (m_pages = 0; length; m_pages++)
    {
        uint8_t page = 0;
        if (!_read (file, &page, sizeof(uint8_t), length))
            return false;

        // Check for a normal information termination byte.  If no extended
        // information it may optionally be present, else it is compulsory
        if (!page && m_pages)
            break;

        m_usage[m_pages].page = page;
        if (!_read (file, m_usage[m_pages].flags, sizeof(uint8_t) * 0x100, length))
            return false;
    }

    // Extract usage information
    for (int i = 0; i < m_pages; i++)
    {
        usage_t &data = m_usage[i];
        int addr = (int) data.page << 8;
        data.extended = false;
        for (int j = 0; j < 0x100; j++)
        {
            sid_usage_t::memflags_t flags = data.flags[j];
            usage.memory[addr++] = flags & ~SID_EXTENSION;
            data.extended |= (flags & SID_EXTENSION) != 0;
        }
    }

    {   // Add load image flag
        int end = (int) usage.end + 1;
        for (int addr = usage.start; addr < end; addr++)
            usage.memory[addr] |= SID_LOAD_IMAGE;
    }
    return Chunk::read (file, usage, length);
}

bool Body::write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length)
{
    m_pages = 0;

    // Evaluate normal body contents
    {for (int i = 0; i < 0x100; i++)
    {
        for (int j = 0; j < 0x100; j++)
        {
            if (!(usage.memory[(i << 8) | j] & ~SID_LOAD_IMAGE))
                continue;
           
            int addr = i << 8;
            usage_t &data = m_usage[m_pages++];
            data.extended = false;
            for (j = 0; j < 0x100; j++)
            {
                sid_usage_t::memflags_t flags = usage.memory[addr++];
                // SID_LOAD_IMAGE not saved as is pointless (information
                // provided by other means) allowing re-use of that bit
                data.flags[j] = (uint8_t) flags & (0xff & ~SID_LOAD_IMAGE);
                // Deal with extended flags
                if (SMM_EX_FLAGS && (flags > 0xff))
                {
                    data.extended  = true;
                    data.flags[j] |= SID_EXTENSION;
                }
            }

            data.page = (uint8_t) i;
        }
    }}

    // Write normal body contents
    for (int i = 0; i < m_pages; i++)
    {
        if (!_write (file, &m_usage[i].page, sizeof(uint8_t) * 0x101, length))
            return false;
    }

    // This is a termination byte picked up by some
    // older decoding routines as place to stop decoding
    // so they don't process the extended information
    // as normal data
    {
        uint8_t tmp = 0;
        if (!_write (file, &tmp, sizeof (tmp), length))
            return false;
    }
    return Chunk::write (file, usage, length);    
}


//-----------------------------------------------------------------------------
// Body extended flags functions
bool Body_extended_flags::read (FILE *file, sid2_usage_t &usage, uint_least32_t length)
{
    uint8_t *flags = 0;
    int count = 0, extension = 0;
    int pages = m_body.m_pages;

    if (!SMM_EX_FLAGS)
    {   // Skip the extended flags
        if (fseek (file, (long) length, SEEK_CUR) < 0)
            return false;
        length = 0;
        return Chunk::read (file, usage, length);
    }

    for (int i = 0; i < pages; i++)
    {
        Body::usage_t &data = m_body.m_usage[i];
        if (!data.extended)
            continue;
        // Search for extended flags
        for (int j = 0; j < 0x100; j++)
        {
            if (!(data.flags[j] & SID_EXTENSION))
                continue;
            if (!count)
            {
                if (!recall (file, count, extension, length))
                    return false;
                flags = m_flags;
            }

            {   // Form flags
                sid_usage_t::memflags_t f = 0;
                for (int x = 0; x < extension; x++)
                {
                    f  |= *flags++;
                    f <<= 8;
                }
                usage.memory[((int) data.page << 8) | j] |= f;
            }
            count -= extension;
        }
    }

    // Make sure all extended bytes were processed
    if (count)
        return false;

    return Chunk::read (file, usage, length);
}

bool Body_extended_flags::write (FILE *file, const sid2_usage_t &usage, uint_least32_t &length)
{
    uint8_t *flags = 0;
    int count = 0, extension = 0;

    int pages = m_body.m_pages;
    for (int i = 0; i < pages; i++)
    {
        Body::usage_t &data = m_body.m_usage[i];
        if (!data.extended)
            continue;
        // Search for extended flags
        for (int j = 0; j < 0x100; j++)
        {
            if (!(data.flags[j] & SID_EXTENSION))
                continue;

            {   // Calculate extension
                uint8_t tmp[SMM_EX_FLAGS + 1];
                sid_usage_t::memflags_t f = usage.memory[((int) data.page << 8) | j];
                int x;
                for (x = 0; x < SMM_EX_FLAGS; x++)
                {
                    f >>= 8;
                    // If no more flags then end
                    if (!f)
                        break;
                    tmp[x] = (uint8_t) f & 0xff;
                }

                // Is the extension the same as used last time
                if ((x != extension) || (count == 0x100))
                {
                    if (!store (file, count, extension, length))
                        return false;
                    flags = m_flags;
                    count = 0;
                    extension = x;
                }

                // Store extended flags
                while (x > 0)
                    *flags++ = tmp[--x];
            }
            count++;
        }
    }
    if (!store (file, count, extension, length))
        return false;
    return Chunk::write (file, usage, length);
}

bool Body_extended_flags::used (const sid2_usage_t &)
{
    int  pages    = m_body.m_pages;
    bool extended = false;
    for (int i = 0; i < pages; i++)
        extended |= m_body.m_usage[i].extended;
    return extended;
}

// Import the extended flags section from a file.  Note the there maybe more extended
// flags (or data) than are supported by this reader
bool Body_extended_flags::recall (FILE *file, int &count, int &extension, uint_least32_t &length)
{
    int offset = 0;
    uint8_t tmp = 0;

    // Read the flags count followed by flag extension size
    if (!_read (file, &tmp, sizeof (tmp), length))
        return false;
    extension = (int) tmp + 1;
    if (!_read (file, &tmp, sizeof (tmp), length))
        return false;
    count = (int) tmp + 1;

    // Calculate the offset
    offset = extension - SMM_EX_FLAGS;
    if (offset < 0)
        offset = 0;
    if (extension > SMM_EX_FLAGS)
        extension = SMM_EX_FLAGS;

    // Read extended flags for upto 1 page
    memset (m_flags, 0, sizeof (m_flags));
    uint8_t *flags = m_flags + (SMM_EX_FLAGS - extension);
    for (int c = count; c-- > 0;)
    {
        // Get to usefull data
        if (offset)
        {
            if (offset > length)
                return false;
            if (fseek (file, (long) offset, SEEK_CUR) < 0)
                return false;
            length -= offset;
        }
        if (!_read (file, flags, sizeof(uint8_t) * extension, length))
            return false;
        flags += extension;
    }
    return true;
}

// Export the extended flags section to a file.
bool Body_extended_flags::store (FILE *file, int count, int extension, uint_least32_t &length)
{
    if (count)
    {
        uint8_t tmp = (uint8_t) (extension - 1) & 0xff;
        if (!_write (file, &tmp, sizeof(uint8_t), length))
            return false;
        tmp = (uint8_t) (count - 1) & 0xff;
        if (!_write (file, &tmp, sizeof(uint8_t), length))
            return false;
        if (!_write (file, m_flags, sizeof(uint8_t) * (count * extension), length))
            return false;
    }
    return true;
}
