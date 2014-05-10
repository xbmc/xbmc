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

#ifndef _NPT_ZIP_H_
#define _NPT_ZIP_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptStreams.h"
#include "NptArray.h"
#include "NptFile.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class NPT_ZipInflateState;
class NPT_ZipDeflateState;

/*----------------------------------------------------------------------
|   NPT_ZipFile
+---------------------------------------------------------------------*/
const unsigned int NPT_ZIP_FILE_FLAG_ENCRYPTED = 0x01;
const unsigned int NPT_ZIP_FILE_COMPRESSION_METHOD_NONE    = 0x00;
const unsigned int NPT_ZIP_FILE_COMPRESSION_METHOD_DEFLATE = 0x08;

class NPT_ZipFile
{
public:
    // types
    class Entry {
    public:
        Entry(const unsigned char* data);
        NPT_String    m_Name;
        NPT_UInt16    m_Flags;
        NPT_UInt16    m_CompressionMethod;
        NPT_UInt32    m_Crc32;
        NPT_LargeSize m_CompressedSize;
        NPT_LargeSize m_UncompressedSize;
        NPT_UInt16    m_DiskNumber;
        NPT_UInt16    m_InternalFileAttributes;
        NPT_UInt32    m_ExternalFileAttributes;
        NPT_Position  m_RelativeOffset;
        NPT_UInt32    m_DirectoryEntrySize;
    };
    
    // class methods
    static NPT_Result Parse(NPT_InputStream& stream, NPT_ZipFile*& file);
    static NPT_Result GetInputStream(Entry& entry, NPT_InputStreamReference& zip_stream, NPT_InputStream*& file_stream);
    
    // accessors
    NPT_Array<Entry>& GetEntries() { return m_Entries; }
    
private:
    // constructor
    NPT_ZipFile();
    
    // members
    NPT_Array<Entry> m_Entries;
};

/*----------------------------------------------------------------------
|   NPT_Zip
+---------------------------------------------------------------------*/
const int NPT_ZIP_COMPRESSION_LEVEL_DEFAULT = -1;
const int NPT_ZIP_COMPRESSION_LEVEL_MIN     = 0;
const int NPT_ZIP_COMPRESSION_LEVEL_MAX     = 9;
const int NPT_ZIP_COMPRESSION_LEVEL_NONE    = 0;
class NPT_Zip 
{
public:
    // class methods
    static NPT_Result MapError(int err);

    /** 
     * Compressed data format
     */
    typedef enum {
        ZLIB,
        GZIP
    } Format;
        
    /**
     * Deflate (i.e compress) a buffer
     */
    static NPT_Result Deflate(const NPT_DataBuffer& in,
                              NPT_DataBuffer&       out,
                              int                   compression_level = NPT_ZIP_COMPRESSION_LEVEL_DEFAULT,
                              Format                format = ZLIB);
                              
    /**
     * Inflate (i.e decompress) a buffer
     */
    static NPT_Result Inflate(const NPT_DataBuffer& in,
                              NPT_DataBuffer&       out,
                              bool                  raw = false);
    
    /**
     * Deflate (i.e compress) a file
     */
    static NPT_Result Deflate(NPT_File& in,
                              NPT_File& out,
                              int       compression_level = NPT_ZIP_COMPRESSION_LEVEL_DEFAULT,
                              Format    format = GZIP);
    
};

/*----------------------------------------------------------------------
|   NPT_ZipInflatingInputStream
+---------------------------------------------------------------------*/
class NPT_ZipInflatingInputStream : public NPT_InputStream 
{
public:
    NPT_ZipInflatingInputStream(NPT_InputStreamReference& source, bool raw = false);
   ~NPT_ZipInflatingInputStream();
   
    // NPT_InputStream methods
    virtual NPT_Result Read(void*     buffer, 
                            NPT_Size  bytes_to_read, 
                            NPT_Size* bytes_read = NULL);
    virtual NPT_Result Seek(NPT_Position offset);
    virtual NPT_Result Tell(NPT_Position& offset);
    virtual NPT_Result GetSize(NPT_LargeSize& size);
    virtual NPT_Result GetAvailable(NPT_LargeSize& available);

private:
    NPT_InputStreamReference m_Source;
    NPT_Position             m_Position;
    NPT_ZipInflateState*     m_State;
    NPT_DataBuffer           m_Buffer;
};

/*----------------------------------------------------------------------
|   NPT_ZipInflatingOutputStream
+---------------------------------------------------------------------*/

/*----------------------------------------------------------------------
|   NPT_ZipDeflatingInputStream
+---------------------------------------------------------------------*/
class NPT_ZipDeflatingInputStream : public NPT_InputStream 
{
public:
    NPT_ZipDeflatingInputStream(NPT_InputStreamReference& source,
                                int                       compression_level = NPT_ZIP_COMPRESSION_LEVEL_DEFAULT,
                                NPT_Zip::Format           format = NPT_Zip::ZLIB);
   ~NPT_ZipDeflatingInputStream();
   
    // NPT_InputStream methods
    virtual NPT_Result Read(void*     buffer, 
                            NPT_Size  bytes_to_read, 
                            NPT_Size* bytes_read = NULL);
    virtual NPT_Result Seek(NPT_Position offset);
    virtual NPT_Result Tell(NPT_Position& offset);
    virtual NPT_Result GetSize(NPT_LargeSize& size);
    virtual NPT_Result GetAvailable(NPT_LargeSize& available);

private:
    NPT_InputStreamReference m_Source;
    NPT_Position             m_Position;
    bool                     m_Eos;
    NPT_ZipDeflateState*     m_State;
    NPT_DataBuffer           m_Buffer;
};

/*----------------------------------------------------------------------
|   NPT_ZipDeflatingOutputStream
+---------------------------------------------------------------------*/
/*class NPT_ZipDeflatingOutputStream : public NPT_OutputStream 
{
public:
    NPT_ZipDeflatingOutputStream(NPT_OutputStreamReference& source,
                                 int                        compression_level = NPT_ZIP_COMPRESSION_LEVEL_DEFAULT,
                                 NPT_Zip::Format            format = NPT_Zip::ZLIB);
   NPT_ZipDeflatingOutputStream();
   
    // NPT_OutputStream methods
    virtual NPT_Result Write(void*     buffer, 
                             NPT_Size  bytes_to_write, 
                             NPT_Size* bytes_written = NULL);
    virtual NPT_Result Seek(NPT_Position offset);
    virtual NPT_Result Tell(NPT_Position& offset);

private:
    NPT_OutputStreamReference m_Output;
    NPT_Position              m_Position;
    bool                      m_Eos;
    NPT_ZipDeflateState*      m_State;
    NPT_DataBuffer            m_Buffer;
}; */

#endif // _NPT_ZIP_H_
