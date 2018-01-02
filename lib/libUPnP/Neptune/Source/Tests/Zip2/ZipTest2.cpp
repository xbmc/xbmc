/*****************************************************************
|
|      Zip Test Program 2
|
|      (c) 2001-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "NptDebug.h"

#if defined(WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

#define CHECK(x)                                        \
    do {                                                \
      if (!(x)) {                                       \
        fprintf(stderr, "ERROR line %d \n", __LINE__);  \
        error_hook();                                   \
        return -1;                                      \
      }                                                 \
    } while(0)


/*----------------------------------------------------------------------
|       error_hook
+---------------------------------------------------------------------*/
static void
error_hook() 
{
    fprintf(stderr, "STOPPING\n");
}

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    // usage: ZipTest2 zipfile [extract_source extract_dest]
    if (argc < 2) {
        fprintf(stderr, "ERROR: filename argument expected\n");
        return 1;
    }
    
    NPT_File file(argv[1]);
    
    const char* extract_source = NULL;
    const char* extract_dest = NULL;
    if (argc > 2) {
        if (argc != 4) {
            fprintf(stderr, "ERROR: extract_source and extract_dest arguments expected\n");
            return 1;
        }
        extract_source = argv[2];
        extract_dest   = argv[3];
    }
    
    NPT_Result result = file.Open(NPT_FILE_OPEN_MODE_READ);
    CHECK(NPT_SUCCEEDED(result));
    
    NPT_InputStreamReference input;
    file.GetInputStream(input);
    
    NPT_ZipFile* zipfile = NULL;
    result = NPT_ZipFile::Parse(*input.AsPointer(), zipfile);
    CHECK(NPT_SUCCEEDED(result));
    
    for (unsigned int i=0; i<zipfile->GetEntries().GetItemCount(); i++) {
        NPT_ZipFile::Entry& entry = zipfile->GetEntries()[i];
        printf("[%d] - size=%lld bytes, name=%s, compression=%d\n", i, (long long)entry.m_CompressedSize, entry.m_Name.GetChars(), entry.m_CompressionMethod);
    
        if (extract_source && entry.m_Name == extract_source) {
            NPT_File output(extract_dest);
            output.Open(NPT_FILE_OPEN_MODE_CREATE | NPT_FILE_OPEN_MODE_WRITE);
            NPT_OutputStreamReference out_stream;
            output.GetOutputStream(out_stream);
            NPT_InputStream* in_stream_p = NULL;
            NPT_ZipFile::GetInputStream(entry, input, in_stream_p);
            NPT_InputStreamReference in_stream(in_stream_p);
            NPT_StreamToStreamCopy(*in_stream, *out_stream);
            output.Close();
        }
    }
    
    return 0;
}
