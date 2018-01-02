/*****************************************************************
|
|      File Test Program 3
|
|      (c) 2005-2016 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Neptune.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|   globals
+---------------------------------------------------------------------*/
const unsigned int BUFFER_SIZE = 8192;
const unsigned int MAX_RANDOM = 123;
typedef struct {
    NPT_Position  position;
    unsigned int  size;
    unsigned char fill_value;
} BufferInfo;
const NPT_UInt64 TARGET_SIZE = 0x112345678;
NPT_Array<BufferInfo> Buffers;

#define CHECK(x) \
do {\
  if (!(x)) {\
    fprintf(stderr, "ERROR line %d\n", __LINE__);\
    return NPT_FAILURE;\
  }\
} while(0)\

/*----------------------------------------------------------------------
|   TestLargeFiles
+---------------------------------------------------------------------*/
static NPT_Result
TestLargeFiles(const char* filename)
{
    // create enough buffers to fill up to the target size
    NPT_UInt64 total_size = 0;
    while (total_size < TARGET_SIZE) {
        unsigned int random = NPT_System::GetRandomInteger() % MAX_RANDOM;
        unsigned int buffer_size = 4096-MAX_RANDOM/2+random;
        BufferInfo buffer_info;
        buffer_info.position = total_size;
        buffer_info.size = buffer_size;
        buffer_info.fill_value = NPT_System::GetRandomInteger()%256;
        Buffers.Add(buffer_info);
        
        total_size += buffer_size;
    }
    unsigned char* buffer = new unsigned char[BUFFER_SIZE];
    unsigned int progress = 0;
    
    // write random buffers
    printf("Writing sequential random-size buffers\n");
    NPT_File test_file(filename);
    NPT_Result result = test_file.Open(NPT_FILE_OPEN_MODE_WRITE | NPT_FILE_OPEN_MODE_CREATE | NPT_FILE_OPEN_MODE_TRUNCATE);
    CHECK(result == NPT_SUCCESS);
    NPT_OutputStreamReference output_stream;
    result = test_file.GetOutputStream(output_stream);
    CHECK(result == NPT_SUCCESS);
    
    for (unsigned int i=0; i<Buffers.GetItemCount(); i++) {
        unsigned int new_progress = (100*i)/Buffers.GetItemCount();
        if (new_progress != progress) {
            printf("\rProgress: %d%%", new_progress);
            fflush(stdout);
            progress = new_progress;
        }
        
        BufferInfo& buffer_info = Buffers[i];
        NPT_SetMemory(buffer, buffer_info.fill_value, buffer_info.size);
        
        
        NPT_Position cursor;
        result = output_stream->Tell(cursor);
        CHECK(result == NPT_SUCCESS);
        CHECK(cursor == buffer_info.position);
        
        result = output_stream->WriteFully(buffer, buffer_info.size);
        CHECK(result == NPT_SUCCESS);
        
        if ((buffer_info.fill_value % 7) == 0) {
            output_stream->Flush();
        }
        
        result = output_stream->Tell(cursor);
        CHECK(result == NPT_SUCCESS);
        CHECK(cursor == buffer_info.position+buffer_info.size);
    }
    
    output_stream = NULL;
    test_file.Close();
    
    // read random buffers
    printf("\nReading sequential random-size buffers\n");
    result = test_file.Open(NPT_FILE_OPEN_MODE_READ);
    CHECK(result == NPT_SUCCESS);
    NPT_InputStreamReference input_stream;
    result = test_file.GetInputStream(input_stream);
    CHECK(result == NPT_SUCCESS);
    
    for (unsigned int i=0; i<Buffers.GetItemCount(); i++) {
        unsigned int new_progress = (100*i)/Buffers.GetItemCount();
        if (new_progress != progress) {
            printf("\rProgress: %d%%", new_progress);
            fflush(stdout);
            progress = new_progress;
        }

        BufferInfo& buffer_info = Buffers[i];

        NPT_Position cursor;
        result = input_stream->Tell(cursor);
        CHECK(result == NPT_SUCCESS);
        CHECK(cursor == buffer_info.position);
        
        result = input_stream->ReadFully(buffer, buffer_info.size);
        CHECK(result == NPT_SUCCESS);
        
        for (unsigned int x=0; x<buffer_info.size; x++) {
            CHECK(buffer[x] == buffer_info.fill_value);
        }
        
        result = input_stream->Tell(cursor);
        CHECK(result == NPT_SUCCESS);
        CHECK(cursor == buffer_info.position+buffer_info.size);
    }
    input_stream = NULL;
    test_file.Close();

    // read random buffers
    printf("\nReading random-access random-size buffers\n");
    result = test_file.Open(NPT_FILE_OPEN_MODE_READ);
    CHECK(result == NPT_SUCCESS);
    result = test_file.GetInputStream(input_stream);
    CHECK(result == NPT_SUCCESS);
    
    for (unsigned int i=0; i<Buffers.GetItemCount()*5; i++) {
        unsigned int new_progress = (100*i)/(5*Buffers.GetItemCount());
        if (new_progress != progress) {
            printf("\rProgress: %d%%", new_progress);
            fflush(stdout);
            progress = new_progress;
        }
    
        unsigned int buffer_index = NPT_System::GetRandomInteger()%Buffers.GetItemCount();
        BufferInfo& buffer_info = Buffers[buffer_index];

        result = input_stream->Seek(buffer_info.position);
        CHECK(result == NPT_SUCCESS);

        NPT_Position cursor;
        result = input_stream->Tell(cursor);
        CHECK(result == NPT_SUCCESS);
        CHECK(cursor == buffer_info.position);
        
        result = input_stream->ReadFully(buffer, buffer_info.size);
        CHECK(result == NPT_SUCCESS);
        
        for (unsigned int x=0; x<buffer_info.size; x++) {
            CHECK(buffer[x] == buffer_info.fill_value);
        }
        
        result = input_stream->Tell(cursor);
        CHECK(result == NPT_SUCCESS);
        CHECK(cursor == buffer_info.position+buffer_info.size);
    }

    printf("\nSUCCESS!\n");
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int argc, char** argv)
{
    const char* output_filename = "largefile.bin";
    if (argc > 1) {
        output_filename = argv[1];
    }

    TestLargeFiles(output_filename);
    
    return 0;
}
