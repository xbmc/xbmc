/*****************************************************************
|
|   Neptune - Zip Support
|
| Copyright (c) 2002-2008, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
 ****************************************************************/


/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptZip.h"
#include "NptLogging.h"
#include "NptUtils.h"

#include "zlib.h"

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.zip")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
static const NPT_UInt32 NPT_ZIP_END_OF_CENTRAL_DIRECTORY_SIGNATURE           = 0x06054b50;
static const NPT_UInt32 NPT_ZIP64_END_OF_CENTRAL_DIRECTORY_SIGNATURE         = 0x06064b50;
static const NPT_UInt32 NPT_ZIP64_END_OF_CENTRAL_DIRECTORY_LOCATOR_SIGNATURE = 0x07064b50;
static const NPT_UInt32 NPT_ZIP_CENTRAL_FILE_HEADER_SIGNATURE                = 0x02014b50;
static const NPT_UInt32 NPT_ZIP_LOCAL_FILE_HEADER_SIGNATURE                  = 0x04034b50;
static const NPT_UInt16 NPT_ZIP_EXT_DATA_TYPE_ZIP64                          = 0x0001;

static const NPT_UInt32 NPT_ZIP_MAX_DIRECTORY_SIZE = 0x1000000; // 16 MB
static const NPT_UInt32 NPT_ZIP_MAX_ENTRY_COUNT    = 0x100000;  // 1M entries

/*----------------------------------------------------------------------
|   NPT_ZipFile::NPT_ZipFile
+---------------------------------------------------------------------*/
NPT_ZipFile::NPT_ZipFile()
{
}

/*----------------------------------------------------------------------
|   NPT_ZipFile::Parse
+---------------------------------------------------------------------*/
NPT_Result
NPT_ZipFile::Parse(NPT_InputStream& stream, NPT_ZipFile*& file)
{
    // defautl return value
    file = NULL;
    
    // check that we know the size of the stream
    NPT_LargeSize stream_size = 0;
    NPT_Result result = stream.GetSize(stream_size);
    if (NPT_FAILED(result)) {
        NPT_LOG_WARNING_1("cannot get stream size (%d)", result);
        return result;
    }
    if (stream_size < 22) {
        NPT_LOG_WARNING("input stream too short");
        return NPT_ERROR_INVALID_FORMAT;
    }
    
    // seek to the most likely start of the end of central directory record
    unsigned int max_eocdr_size = 22+65536;
    if (max_eocdr_size > stream_size) {
        max_eocdr_size = (unsigned int)stream_size;
    }
    unsigned char eocdr[22];
    bool record_found = false;
    NPT_Position position = 0;
    for (unsigned int i=0; i<max_eocdr_size; i++) {
        position = stream_size-22-i;
        result = stream.Seek(position);
        if (NPT_FAILED(result)) {
            NPT_LOG_WARNING_1("seek failed (%d)", result);
            return result;
        }
        result = stream.ReadFully(eocdr, 22);
        if (NPT_FAILED(result)) {
            NPT_LOG_WARNING_1("read failed (%d)", result);
            return result;
        }
        NPT_UInt32 signature = NPT_BytesToInt32Le(eocdr);
        if (signature == NPT_ZIP_END_OF_CENTRAL_DIRECTORY_SIGNATURE) {
            record_found = true;
            break;
        }
    }
    if (!record_found) {
        NPT_LOG_WARNING("eocd record not found at end of stream");
        return NPT_ERROR_INVALID_FORMAT;
    }
    
    // parse the eocdr
    NPT_UInt32   this_disk                = NPT_BytesToInt16Le(&eocdr[ 4]);
    NPT_UInt32   start_disk               = NPT_BytesToInt16Le(&eocdr[ 6]);
    NPT_UInt64   this_disk_entry_count    = NPT_BytesToInt16Le(&eocdr[ 8]);
    NPT_UInt64   total_entry_count        = NPT_BytesToInt16Le(&eocdr[10]);
    NPT_UInt64   central_directory_size   = NPT_BytesToInt32Le(&eocdr[12]);
    NPT_Position central_directory_offset = NPT_BytesToInt32Le(&eocdr[16]);

    // format check
    if (this_disk != 0 || start_disk != 0) {
        return NPT_ERROR_NOT_SUPPORTED;
    }
    if (this_disk_entry_count != total_entry_count) {
        return NPT_ERROR_NOT_SUPPORTED;
    }
    
    // check if this is a zip64 file
    if (central_directory_offset == 0xFFFFFFFF) {
        unsigned char zip64_locator[20];
        result = stream.Seek(position-20);
        if (NPT_FAILED(result)) {
            NPT_LOG_WARNING_1("seek failed (%d)", result);
            return result;
        }
        result = stream.ReadFully(zip64_locator, 20);
        if (NPT_FAILED(result)) {
            NPT_LOG_WARNING_1("read failed (%d)", result);
            return result;
        }
        
        NPT_UInt32 signature = NPT_BytesToInt32Le(&zip64_locator[0]);
        if (signature != NPT_ZIP64_END_OF_CENTRAL_DIRECTORY_LOCATOR_SIGNATURE) {
            NPT_LOG_WARNING("zip64 directory locator signature not found");
            return NPT_ERROR_INVALID_FORMAT;
        }
        NPT_UInt32 zip64_disk_start       = NPT_BytesToInt32Le(&zip64_locator[ 4]);
        NPT_UInt64 zip64_directory_offset = NPT_BytesToInt64Le(&zip64_locator[ 8]);
        NPT_UInt32 zip64_disk_count       = NPT_BytesToInt32Le(&zip64_locator[16]);

        // format check
        if (zip64_disk_start != 0 || zip64_disk_count != 1) {
            return NPT_ERROR_NOT_SUPPORTED;
        }
        
        // size check
        if (zip64_directory_offset > stream_size) {
            NPT_LOG_WARNING("zip64 directory offset too large");
            return NPT_ERROR_INVALID_FORMAT;
        }
        
        // load and parse the eocdr64
        unsigned char eocdr64[56];
        result = stream.Seek(zip64_directory_offset);
        if (NPT_FAILED(result)) {
            NPT_LOG_WARNING_1("seek failed (%d)", result);
            return result;
        }
        result = stream.ReadFully(eocdr64, 56);
        if (NPT_FAILED(result)) {
            NPT_LOG_WARNING_1("read failed (%d)", result);
            return result;
        }
        
        signature = NPT_BytesToInt32Le(&eocdr64[0]);
        if (signature != NPT_ZIP64_END_OF_CENTRAL_DIRECTORY_SIGNATURE) {
            NPT_LOG_WARNING("zip64 directory signature not found");
            return NPT_ERROR_INVALID_FORMAT;
        }
        
        this_disk                = NPT_BytesToInt32Le(&eocdr64[16]);
        start_disk               = NPT_BytesToInt32Le(&eocdr64[20]);
        this_disk_entry_count    = NPT_BytesToInt64Le(&eocdr64[24]);
        total_entry_count        = NPT_BytesToInt64Le(&eocdr64[32]);
        central_directory_size   = NPT_BytesToInt64Le(&eocdr64[40]);
        central_directory_offset = NPT_BytesToInt64Le(&eocdr64[48]);
    }
    
    // format check
    if (this_disk != 0 || start_disk != 0) {
        return NPT_ERROR_NOT_SUPPORTED;
    }
    if (this_disk_entry_count != total_entry_count) {
        return NPT_ERROR_NOT_SUPPORTED;
    }
    
    // check that the size looks reasonable
    if (central_directory_size > NPT_ZIP_MAX_DIRECTORY_SIZE) {
        NPT_LOG_WARNING("central directory larger than max supported");
        return NPT_ERROR_OUT_OF_RANGE;
    }
    if (total_entry_count > NPT_ZIP_MAX_ENTRY_COUNT) {
        NPT_LOG_WARNING("central directory larger than max supported");
        return NPT_ERROR_OUT_OF_RANGE;
    }
    
    // read the central directory
    NPT_DataBuffer central_directory_buffer;
    result = central_directory_buffer.SetDataSize((NPT_Size)central_directory_size);
    if (NPT_FAILED(result)) {
        NPT_LOG_WARNING_1("central directory too large (%lld)", central_directory_size);
        return result;
    }
    result = stream.Seek(central_directory_offset);
    if (NPT_FAILED(result)) {
        NPT_LOG_WARNING_1("seek failed (%d)", result);
        return result;
    }
    result = stream.ReadFully(central_directory_buffer.UseData(), (NPT_Size)central_directory_size);
    if (NPT_FAILED(result)) {
        NPT_LOG_WARNING_1("failed to read central directory (%d)", result);
        return result;
    }
    
    // create a new file object
    file = new NPT_ZipFile();
    file->m_Entries.Reserve((NPT_Cardinal)total_entry_count);
    
    // parse all entries
    const unsigned char* buffer = (const unsigned char*)central_directory_buffer.GetData();
    for (unsigned int i=0; i<total_entry_count; i++) {
        NPT_UInt32 signature = NPT_BytesToInt32Le(buffer);
        if (signature != NPT_ZIP_CENTRAL_FILE_HEADER_SIGNATURE) {
            NPT_LOG_WARNING("unexpected signature in central directory");
            break;
        }
        
        NPT_ZipFile::Entry entry(buffer);
        
        if (entry.m_DirectoryEntrySize > central_directory_size) {
            NPT_LOG_WARNING_1("entry size too large (%d)", entry.m_DirectoryEntrySize);
            break;
        }
        
        file->GetEntries().Add(entry);
        
        central_directory_size -= entry.m_DirectoryEntrySize;
        buffer                 += entry.m_DirectoryEntrySize;
    }
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_ZipFile::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result
NPT_ZipFile::GetInputStream(Entry& entry, NPT_InputStreamReference& zip_stream, NPT_InputStream*& file_stream)
{
    // default return value
    file_stream = NULL;
    
    // we don't support encrypted files
    if (entry.m_Flags & NPT_ZIP_FILE_FLAG_ENCRYPTED) {
        return NPT_ERROR_NOT_SUPPORTED;
    }
    
    // check that we support the compression method
#if NPT_CONFIG_ENABLE_ZIP
    if (entry.m_CompressionMethod != NPT_ZIP_FILE_COMPRESSION_METHOD_NONE &&
        entry.m_CompressionMethod != NPT_ZIP_FILE_COMPRESSION_METHOD_DEFLATE) {
        return NPT_ERROR_NOT_SUPPORTED;
    }
#else
    if (entry.m_CompressionMethod != NPT_ZIP_FILE_COMPRESSION_METHOD_NONE) {
        return NPT_ERROR_NOT_SUPPORTED;
    }
#endif

    // seek to the start of the file entry
    NPT_Result result = zip_stream->Seek(entry.m_RelativeOffset);
    if (NPT_FAILED(result)) {
        NPT_LOG_WARNING_1("seek failed (%d)", result);
        return result;
    }
    
    // read the fixed part of the header
    unsigned char header[30];
    result = zip_stream->ReadFully(header, 30);
    if (NPT_FAILED(result)) {
        NPT_LOG_WARNING_1("read failed (%d)", result);
        return result;
    }
    
    NPT_UInt16 file_name_length   = NPT_BytesToInt16Le(&header[26]);
    NPT_UInt16 extra_field_length = NPT_BytesToInt16Le(&header[28]);
    
    unsigned int header_size = 30+file_name_length+extra_field_length;
    NPT_LargeSize zip_stream_size = 0;
    zip_stream->GetSize(zip_stream_size);
    if (entry.m_RelativeOffset+header_size+entry.m_CompressedSize > zip_stream_size) {
        // something's wrong here
        return NPT_ERROR_INVALID_FORMAT;
    }
    
    file_stream = new NPT_SubInputStream(zip_stream, entry.m_RelativeOffset+header_size, entry.m_CompressedSize);
    
#if NPT_CONFIG_ENABLE_ZIP
    if (entry.m_CompressionMethod == NPT_ZIP_FILE_COMPRESSION_METHOD_DEFLATE) {
        NPT_InputStreamReference file_stream_ref(file_stream);
        file_stream = new NPT_ZipInflatingInputStream(file_stream_ref, true);
    }
#endif

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_ZipFile::Entry::Entry
+---------------------------------------------------------------------*/
NPT_ZipFile::Entry::Entry(const unsigned char* data)
{
    m_Flags                        = NPT_BytesToInt16Le(data+ 8);
    m_CompressionMethod            = NPT_BytesToInt16Le(data+10);
    m_Crc32                        = NPT_BytesToInt32Le(data+16);
    m_CompressedSize               = NPT_BytesToInt32Le(data+20);
    m_UncompressedSize             = NPT_BytesToInt32Le(data+24);
    m_DiskNumber                   = NPT_BytesToInt16Le(data+34);
    m_InternalFileAttributes       = NPT_BytesToInt32Le(data+36);
    m_ExternalFileAttributes       = NPT_BytesToInt32Le(data+38);
    m_RelativeOffset               = NPT_BytesToInt32Le(data+42);

    NPT_UInt16 file_name_length    = NPT_BytesToInt16Le(data+28);
    NPT_UInt16 extra_field_length  = NPT_BytesToInt16Le(data+30);
    NPT_UInt16 file_comment_length = NPT_BytesToInt16Le(data+32);
    
    // extract the file name
    m_Name.Assign((const char*)data+46, file_name_length);

    m_DirectoryEntrySize = 46+file_name_length+extra_field_length+file_comment_length;

    // check for a zip64 extension
    const unsigned char* ext_data = data+46+file_name_length;
    unsigned int ext_data_size = extra_field_length;
    while (ext_data_size >= 4) {
        unsigned int ext_id   = NPT_BytesToInt16Le(ext_data);
        unsigned int ext_size = NPT_BytesToInt16Le(ext_data+2);
        
        if (ext_id == NPT_ZIP_EXT_DATA_TYPE_ZIP64) {
            const unsigned char* local_data = ext_data+4;
            if (m_UncompressedSize == 0xFFFFFFFF) {
                m_UncompressedSize = NPT_BytesToInt64Le(local_data);
                local_data += 8;
            }
            if (m_CompressedSize == 0xFFFFFFFF) {
                m_CompressedSize = NPT_BytesToInt64Le(local_data);
                local_data += 8;
            }
            if (m_RelativeOffset == 0xFFFFFFFF) {
                m_RelativeOffset = NPT_BytesToInt64Le(local_data);
                local_data += 8;
            }
            if (m_DiskNumber == 0xFFFF) {
                m_DiskNumber = NPT_BytesToInt32Le(local_data);
                local_data += 4;
            }
        }
        
        ext_data      += 4+ext_size;
        if (ext_data_size >= 4+ext_size) {
            ext_data_size -= 4+ext_size;
        } else {
            ext_data_size = 0;
        }
    }
}

#if defined(NPT_CONFIG_ENABLE_ZIP)

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int NPT_ZIP_DEFAULT_BUFFER_SIZE = 4096;

/*----------------------------------------------------------------------
|   NPT_Zip::MapError
+---------------------------------------------------------------------*/
NPT_Result
NPT_Zip::MapError(int err)
{
    switch (err) {
        case Z_OK:            return NPT_SUCCESS;           
        case Z_STREAM_END:    return NPT_ERROR_EOS;
        case Z_DATA_ERROR:
        case Z_STREAM_ERROR:  return NPT_ERROR_INVALID_FORMAT;
        case Z_MEM_ERROR:     return NPT_ERROR_OUT_OF_MEMORY;
        case Z_VERSION_ERROR: return NPT_ERROR_INTERNAL;
        case Z_NEED_DICT:     return NPT_ERROR_NOT_SUPPORTED;
        default:              return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NPT_ZipInflateState
+---------------------------------------------------------------------*/
class NPT_ZipInflateState {
public:
    NPT_ZipInflateState(bool raw = false);
   ~NPT_ZipInflateState();
    z_stream m_Stream;
};

/*----------------------------------------------------------------------
|   NPT_ZipInflateState::NPT_ZipInflateState
+---------------------------------------------------------------------*/
NPT_ZipInflateState::NPT_ZipInflateState(bool raw)
{
    // initialize the state
    NPT_SetMemory(&m_Stream, 0, sizeof(m_Stream));

    // initialize the decompressor
    inflateInit2(&m_Stream, raw?-15:15+32); // 15 = default window bits, +32 = automatic header
}

/*----------------------------------------------------------------------
|   NPT_ZipInflateState::~NPT_ZipInflateState
+---------------------------------------------------------------------*/
NPT_ZipInflateState::~NPT_ZipInflateState()
{
    inflateEnd(&m_Stream);
}

/*----------------------------------------------------------------------
|   NPT_ZipDeflateState
+---------------------------------------------------------------------*/
class NPT_ZipDeflateState {
public:
    NPT_ZipDeflateState(int             compression_level,
                        NPT_Zip::Format format);
   ~NPT_ZipDeflateState();
    z_stream m_Stream;
};

/*----------------------------------------------------------------------
|   NPT_ZipDeflateState::NPT_ZipDeflateState
+---------------------------------------------------------------------*/
NPT_ZipDeflateState::NPT_ZipDeflateState(int             compression_level,
                                         NPT_Zip::Format format)
{
    // check parameters
    if (compression_level < NPT_ZIP_COMPRESSION_LEVEL_DEFAULT ||
        compression_level > NPT_ZIP_COMPRESSION_LEVEL_MAX) {
        compression_level = NPT_ZIP_COMPRESSION_LEVEL_DEFAULT;
    }

    // initialize the state
    NPT_SetMemory(&m_Stream, 0, sizeof(m_Stream));

    // initialize the compressor
    deflateInit2(&m_Stream, 
                compression_level,
                Z_DEFLATED,
                15 + (format == NPT_Zip::GZIP ? 16 : 0),
                8,
                Z_DEFAULT_STRATEGY);
}

/*----------------------------------------------------------------------
|   NPT_ZipDeflateState::~NPT_ZipDeflateState
+---------------------------------------------------------------------*/
NPT_ZipDeflateState::~NPT_ZipDeflateState()
{
    deflateEnd(&m_Stream);
}

/*----------------------------------------------------------------------
|   NPT_ZipInflatingInputStream::NPT_ZipInflatingInputStream
+---------------------------------------------------------------------*/
NPT_ZipInflatingInputStream::NPT_ZipInflatingInputStream(NPT_InputStreamReference& source, bool raw) :
    m_Source(source),
    m_Position(0),
    m_State(new NPT_ZipInflateState(raw)),
    m_Buffer(NPT_ZIP_DEFAULT_BUFFER_SIZE)
{
}

/*----------------------------------------------------------------------
|   NPT_ZipInflatingInputStream::~NPT_ZipInflatingInputStream
+---------------------------------------------------------------------*/
NPT_ZipInflatingInputStream::~NPT_ZipInflatingInputStream()
{
    delete m_State;
}

/*----------------------------------------------------------------------
|   NPT_ZipInflatingInputStream::Read
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ZipInflatingInputStream::Read(void*     buffer, 
                                  NPT_Size  bytes_to_read, 
                                  NPT_Size* bytes_read)
{
    // check state and parameters
    if (m_State == NULL) return NPT_ERROR_INVALID_STATE;
    if (buffer == NULL) return NPT_ERROR_INVALID_PARAMETERS;
    if (bytes_to_read == 0) return NPT_SUCCESS;
    
    // default values
    if (bytes_read) *bytes_read = 0;
    
    // setup the output buffer
    m_State->m_Stream.next_out  = (Bytef*)buffer;
    m_State->m_Stream.avail_out = (uInt)bytes_to_read;
    
    while (m_State->m_Stream.avail_out) {
        // decompress what we can
        int err = inflate(&m_State->m_Stream, Z_NO_FLUSH);
        
        if (err == Z_STREAM_END) {
            // we decompressed everything
            break;
        } else if (err == Z_OK) {
            // we got something
            continue;
        } else if (err == Z_BUF_ERROR) {
            // we need more input data
            NPT_Size   input_bytes_read = 0;
            NPT_Result result = m_Source->Read(m_Buffer.UseData(), m_Buffer.GetBufferSize(), &input_bytes_read);
            if (NPT_FAILED(result)) return result;

            // setup the input buffer
            m_Buffer.SetDataSize(input_bytes_read);
            m_State->m_Stream.next_in = m_Buffer.UseData();
            m_State->m_Stream.avail_in = m_Buffer.GetDataSize();
        
        } else {
            return NPT_Zip::MapError(err);
        }
    }
    
    // report how much we could decompress
    NPT_Size progress = bytes_to_read - m_State->m_Stream.avail_out;
    if (bytes_read) {
        *bytes_read = progress;
    }
    m_Position += progress;
    
    return progress == 0 ? NPT_ERROR_EOS:NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_ZipInflatingInputStream::Seek
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ZipInflatingInputStream::Seek(NPT_Position /* offset */)
{
    // we can't seek
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_ZipInflatingInputStream::Tell
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ZipInflatingInputStream::Tell(NPT_Position& offset)
{
    offset = m_Position;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_ZipInflatingInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ZipInflatingInputStream::GetSize(NPT_LargeSize& size)
{
    // the size is not predictable
    size = 0;
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_ZipInflatingInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ZipInflatingInputStream::GetAvailable(NPT_LargeSize& available)
{
    // we don't know
    available = 0;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_ZipDeflatingInputStream::NPT_ZipDeflatingInputStream
+---------------------------------------------------------------------*/
NPT_ZipDeflatingInputStream::NPT_ZipDeflatingInputStream(
    NPT_InputStreamReference& source,
    int                       compression_level,
    NPT_Zip::Format           format) :
    m_Source(source),
    m_Position(0),
    m_Eos(false),
    m_State(new NPT_ZipDeflateState(compression_level, format)),
    m_Buffer(NPT_ZIP_DEFAULT_BUFFER_SIZE)
{
}

/*----------------------------------------------------------------------
|   NPT_ZipDeflatingInputStream::~NPT_ZipDeflatingInputStream
+---------------------------------------------------------------------*/
NPT_ZipDeflatingInputStream::~NPT_ZipDeflatingInputStream()
{
    delete m_State;
}

/*----------------------------------------------------------------------
|   NPT_ZipDeflatingInputStream::Read
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ZipDeflatingInputStream::Read(void*     buffer, 
                                  NPT_Size  bytes_to_read, 
                                  NPT_Size* bytes_read)
{
    // check state and parameters
    if (m_State == NULL) return NPT_ERROR_INVALID_STATE;
    if (buffer == NULL) return NPT_ERROR_INVALID_PARAMETERS;
    if (bytes_to_read == 0) return NPT_SUCCESS;
    
    // default values
    if (bytes_read) *bytes_read = 0;
    
    // setup the output buffer
    m_State->m_Stream.next_out  = (Bytef*)buffer;
    m_State->m_Stream.avail_out = (uInt)bytes_to_read;
    
    while (m_State->m_Stream.avail_out) {
        // compress what we can
        int err = deflate(&m_State->m_Stream, m_Eos?Z_FINISH:Z_NO_FLUSH);
        
        if (err == Z_STREAM_END) {
            // we compressed everything
            break;
        } else if (err == Z_OK) {
            // we got something
            continue;
        } else if (err == Z_BUF_ERROR) {
            // we need more input data
            NPT_Size   input_bytes_read = 0;
            NPT_Result result = m_Source->Read(m_Buffer.UseData(), m_Buffer.GetBufferSize(), &input_bytes_read);
            if (result == NPT_ERROR_EOS) {
                m_Eos = true;
            } else {
                if (NPT_FAILED(result)) return result;
            }
            
            // setup the input buffer
            m_Buffer.SetDataSize(input_bytes_read);
            m_State->m_Stream.next_in = m_Buffer.UseData();
            m_State->m_Stream.avail_in = m_Buffer.GetDataSize();
        
        } else {
            return NPT_Zip::MapError(err);
        }
    }
    
    // report how much we could compress
    NPT_Size progress = bytes_to_read - m_State->m_Stream.avail_out;
    if (bytes_read) {
        *bytes_read = progress;
    }
    m_Position += progress;
    
    return progress == 0 ? NPT_ERROR_EOS:NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_ZipDeflatingInputStream::Seek
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ZipDeflatingInputStream::Seek(NPT_Position /* offset */)
{
    // we can't seek
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_ZipDeflatingInputStream::Tell
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ZipDeflatingInputStream::Tell(NPT_Position& offset)
{
    offset = m_Position;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_ZipDeflatingInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ZipDeflatingInputStream::GetSize(NPT_LargeSize& size)
{
    // the size is not predictable
    size = 0;
    return NPT_ERROR_NOT_SUPPORTED;
}

/*----------------------------------------------------------------------
|   NPT_ZipDeflatingInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result 
NPT_ZipDeflatingInputStream::GetAvailable(NPT_LargeSize& available)
{
    // we don't know
    available = 0;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Zip::Deflate
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Zip::Deflate(const NPT_DataBuffer& in,
                 NPT_DataBuffer&       out,
                 int                   compression_level,
                 Format                format /* = ZLIB */)
{
    // default return state
    out.SetDataSize(0);
    
    // check parameters
    if (compression_level < NPT_ZIP_COMPRESSION_LEVEL_DEFAULT ||
        compression_level > NPT_ZIP_COMPRESSION_LEVEL_MAX) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }
                
    // setup the stream
    z_stream stream;
    NPT_SetMemory(&stream, 0, sizeof(stream));
    stream.next_in   = (Bytef*)in.GetData();
    stream.avail_in  = (uInt)in.GetDataSize();
    
    // setup the memory functions
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    // initialize the compressor
    int err = deflateInit2(&stream, 
                           compression_level,
                           Z_DEFLATED,
                           15 + (format == GZIP ? 16 : 0),
                           8,
                           Z_DEFAULT_STRATEGY);
    if (err != Z_OK) return MapError(err);

    // reserve an output buffer known to be large enough
    out.Reserve((NPT_Size)deflateBound(&stream, stream.avail_in) + (format==GZIP?10:0));
    stream.next_out  = out.UseData();
    stream.avail_out = out.GetBufferSize();

    // decompress
    err = deflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        deflateEnd(&stream);
        return MapError(err);
    }
    
    // update the output size
    out.SetDataSize((NPT_Size)stream.total_out);

    // cleanup
    err = deflateEnd(&stream);
    return MapError(err);
}
                              
/*----------------------------------------------------------------------
|   NPT_Zip::Inflate
+---------------------------------------------------------------------*/                              
NPT_Result 
NPT_Zip::Inflate(const NPT_DataBuffer& in,
                 NPT_DataBuffer&       out,
                 bool                  raw)
{
    // assume an output buffer twice the size of the input plus a bit
    NPT_CHECK_WARNING(out.Reserve(32+2*in.GetDataSize()));
    
    // setup the stream
    z_stream stream;
    stream.next_in   = (Bytef*)in.GetData();
    stream.avail_in  = (uInt)in.GetDataSize();
    stream.next_out  = out.UseData();
    stream.avail_out = (uInt)out.GetBufferSize();

    // setup the memory functions
    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;
    stream.opaque = (voidpf)0;

    // initialize the decompressor
    int err = inflateInit2(&stream, raw?-15:15+32); // 15 = default window bits, +32 = automatic header
    if (err != Z_OK) return MapError(err);
    
    // decompress until the end
    do {
        err = inflate(&stream, Z_SYNC_FLUSH);
        if (err == Z_STREAM_END || err == Z_OK || err == Z_BUF_ERROR) {
            out.SetDataSize((NPT_Size)stream.total_out);
            if ((err == Z_OK && stream.avail_out == 0) || err == Z_BUF_ERROR) {
                // grow the output buffer
                out.Reserve(out.GetBufferSize()*2);
                stream.next_out = out.UseData()+stream.total_out;
                stream.avail_out = out.GetBufferSize()-(NPT_Size)stream.total_out;
            }
        }
    } while (err == Z_OK);
    
    // check for errors
    if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        return MapError(err);
    }
    
    // cleanup
    err = inflateEnd(&stream);
    return MapError(err);
}


/*----------------------------------------------------------------------
|   NPT_Zip::Deflate
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Zip::Deflate(NPT_File& in,
                 NPT_File& out,
                 int       compression_level,
                 Format    format /* = ZLIB */)
{
    // check parameters
    if (compression_level < NPT_ZIP_COMPRESSION_LEVEL_DEFAULT ||
        compression_level > NPT_ZIP_COMPRESSION_LEVEL_MAX) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }
    
    NPT_InputStreamReference input;
    NPT_CHECK(in.GetInputStream(input));
    NPT_OutputStreamReference output;
    NPT_CHECK(out.GetOutputStream(output));
    
    NPT_ZipDeflatingInputStream deflating_stream(input, compression_level, format);
    return NPT_StreamToStreamCopy(deflating_stream, *output.AsPointer());
}

#endif // NPT_CONFIG_ENABLE_ZIP
