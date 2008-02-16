/*****************************************************************
|
|   Neptune - Zip Support
|
|   (c) 2007 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_ZIP_H_
#define _NPT_ZIP_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptStreams.h"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class NPT_ZipInflateState;
class NPT_ZipDeflateState;

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
                              NPT_DataBuffer&       out);                       
};

/*----------------------------------------------------------------------
|   NPT_ZipInflatingInputStream
+---------------------------------------------------------------------*/
class NPT_ZipInflatingInputStream : public NPT_InputStream 
{
public:
    NPT_ZipInflatingInputStream(NPT_InputStreamReference& source);
   ~NPT_ZipInflatingInputStream();
   
    // NPT_InputStream methods
    virtual NPT_Result Read(void*     buffer, 
                            NPT_Size  bytes_to_read, 
                            NPT_Size* bytes_read = NULL);
    virtual NPT_Result Seek(NPT_Position offset);
    virtual NPT_Result Tell(NPT_Position& offset);
    virtual NPT_Result GetSize(NPT_Size& size);
    virtual NPT_Result GetAvailable(NPT_Size& available);

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
    virtual NPT_Result GetSize(NPT_Size& size);
    virtual NPT_Result GetAvailable(NPT_Size& available);

private:
    NPT_InputStreamReference m_Source;
    NPT_Position             m_Position;
    bool                     m_Eos;
    NPT_ZipDeflateState*     m_State;
    NPT_DataBuffer           m_Buffer;
};

/*----------------------------------------------------------------------
|   NPT_ZipDeflatingInputStream
+---------------------------------------------------------------------*/

#endif // _NPT_ZIP_H_
