/*****************************************************************
|
|   Neptune - Buffered Byte Stream
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_BUFFERED_STREAMS_H_
#define _NPT_BUFFERED_STREAMS_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptStreams.h"
#include "NptTypes.h"
#include "NptConstants.h"
#include "NptStrings.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|   NPT_BufferedStream
+---------------------------------------------------------------------*/
const NPT_Size NPT_BUFFERED_BYTE_STREAM_DEFAULT_SIZE = 4096;

/*----------------------------------------------------------------------
|   NPT_BufferedByteStream
+---------------------------------------------------------------------*/
class NPT_BufferedInputStream : public NPT_InputStream
{
public:
    // constructors and destructor
    NPT_BufferedInputStream(NPT_InputStreamReference& stream,
                            NPT_Size buffer_size = NPT_BUFFERED_BYTE_STREAM_DEFAULT_SIZE);
    ~NPT_BufferedInputStream();

    // methods
    virtual NPT_Result ReadLine(NPT_String& line,
                                NPT_Size    max_chars = 4096);
    virtual NPT_Result ReadLine(char*     buffer, 
                                NPT_Size  buffer_size,
                                NPT_Size* chars_read);
    virtual NPT_Result SetBufferSize(NPT_Size size);

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer, 
                    NPT_Size  bytes_to_read, 
                    NPT_Size* bytes_read = NULL);
    NPT_Result Seek(NPT_Position offset);
    NPT_Result Tell(NPT_Position& offset);
    NPT_Result GetSize(NPT_Size& size);
    NPT_Result GetAvailable(NPT_Size& available);

private:
    // members
    NPT_InputStreamReference m_Source;
    bool                     m_SkipNewline;
    bool                     m_Eos;
    struct {
        NPT_Byte*    data;
        NPT_Position offset;
        NPT_Size     valid;
        NPT_Size     size;
    } m_Buffer;

    // methods
    NPT_Result FillBuffer();
    NPT_Result ReleaseBuffer();
};

typedef NPT_Reference<NPT_BufferedInputStream> NPT_BufferedInputStreamReference;

/*----------------------------------------------------------------------
|   NPT_StringOutputStream
+---------------------------------------------------------------------*/
class NPT_StringOutputStream : public NPT_OutputStream
{
public:
    // methods
    NPT_StringOutputStream(NPT_Size size = 4096);
    NPT_StringOutputStream(NPT_String* data);
    virtual ~NPT_StringOutputStream() ;

    const NPT_String* GetData() const { return m_Data; }

    // NPT_OutputStream methods
    NPT_Result Write(const void* buffer, NPT_Size bytes_to_write, NPT_Size* bytes_written = NULL);
    NPT_Result Flush();

    NPT_Result Seek(NPT_Position offset)  { NPT_COMPILER_UNUSED(offset); return NPT_FAILURE; }
    NPT_Result Tell(NPT_Position& offset) { NPT_COMPILER_UNUSED(offset); return NPT_FAILURE; }

protected:
    NPT_String* m_Data;
    bool        m_Local;
};

#endif // _NPT_BUFFERED_STREAMS_H_
