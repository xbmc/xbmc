#ifndef _MSC_VER
#include <unistd.h>
#endif
#include "streamfile.h"
#include "util.h"

#ifdef _WIN32
#define IS_VALID_STREAM(stream)     (stream != NULL && (stream->_ptr != NULL))
#else
#define IS_VALID_STREAM(stream)     (stream)
#endif

typedef struct {
    STREAMFILE sf;
    FILE * infile;
    off_t offset;
    size_t validsize;
    uint8_t * buffer;
    size_t buffersize;
    char name[260];
#ifdef PROFILE_STREAMFILE
    size_t bytes_read;
    int error_count;
#endif
} STDIOSTREAMFILE;

static STREAMFILE * open_stdio_streamfile_buffer_by_FILE(FILE *infile,const char * const filename, size_t buffersize);

static size_t read_the_rest(uint8_t * dest, off_t offset, size_t length, STDIOSTREAMFILE * streamfile) {
    size_t length_read_total=0;

    /* is the beginning at least there? */
    if (offset >= streamfile->offset && offset < streamfile->offset+streamfile->validsize) {
        size_t length_read;
        off_t offset_into_buffer = offset-streamfile->offset;
        length_read = streamfile->validsize-offset_into_buffer;
        memcpy(dest,streamfile->buffer+offset_into_buffer,length_read);
        length_read_total += length_read;
        length -= length_read;
        offset += length_read;
        dest += length_read;
    }

    /* TODO: What would make more sense here is to read the whole request
     * at once into the dest buffer, as it must be large enough, and then
     * copy some part of that into our own buffer.
     * The destination buffer is supposed to be much smaller than the
     * STREAMFILE buffer, though. Maybe we should only ever return up
     * to the buffer size to avoid having to deal with things like this
     * which are outside of my intended use.
     */
    /* read as much of the beginning of the request as possible, proceed */
    while (length>0) {
        size_t length_to_read;
        size_t length_read=0;
        streamfile->validsize=0;
        if (fseeko(streamfile->infile,offset,SEEK_SET)) return length_read;
        streamfile->offset=offset;

        /* decide how much must be read this time */
        if (length>streamfile->buffersize) length_to_read=streamfile->buffersize;
        else length_to_read=length;

        /* always try to fill the buffer */
        length_read = fread(streamfile->buffer,1,streamfile->buffersize,streamfile->infile);
        streamfile->validsize=length_read;

#ifdef PROFILE_STREAMFILE
        if (ferror(streamfile->infile)) {
            clearerr(streamfile->infile);
            streamfile->error_count++;
        }

        streamfile->bytes_read += length_read;
#endif

        /* if we can't get enough to satisfy the request we give up */
        if (length_read < length_to_read) {
            memcpy(dest,streamfile->buffer,length_read);
            length_read_total+=length_read;
            return length_read_total;
        }

        /* use the new buffer */
        memcpy(dest,streamfile->buffer,length_to_read);
        length_read_total+=length_to_read;
        length-=length_to_read;
        dest+=length_to_read;
        offset+=length_to_read;
    }

    return length_read_total;
}

static size_t read_stdio(STDIOSTREAMFILE *streamfile,uint8_t * dest, off_t offset, size_t length)
{
    // read
    if (!streamfile || !dest || length<=0) return 0;

    /* if entire request is within the buffer */
    if (offset >= streamfile->offset && offset+length <= streamfile->offset+streamfile->validsize) {
        memcpy(dest,streamfile->buffer+(offset-streamfile->offset),length);
        return length;
    }

    {
        size_t length_read = read_the_rest(dest,offset,length,streamfile);
#if PROFILE_STREAMFILE
        if (length_read < length) 
            streamfile->error_count++;
#endif
        return length_read;
    }
}

static void close_stdio(STDIOSTREAMFILE * streamfile) {
    fclose(streamfile->infile);
    free(streamfile->buffer);
    free(streamfile);
}

static size_t get_size_stdio(STDIOSTREAMFILE * streamfile) {
    fseeko(streamfile->infile,0,SEEK_END);
    return ftello(streamfile->infile);
}

static off_t get_offset_stdio(STDIOSTREAMFILE *streamFile) {
    return streamFile->offset;
}

static void get_name_stdio(STDIOSTREAMFILE *streamfile,char *buffer,size_t length) {
    strcpy(buffer,streamfile->name);
}

#ifdef PROFILE_STREAMFILE
static size_t get_bytes_read_stdio(STDIOSTREAMFILE *streamFile) {
    return streamFile->bytes_read;
}
static size_t get_error_count_stdio(STDIOSTREAMFILE *streamFile) {
    return streamFile->error_count;
}
#endif

static STREAMFILE *open_stdio(STDIOSTREAMFILE *streamFile,const char * const filename,size_t buffersize) {
    int newfd;
    FILE *newfile;
    STREAMFILE *newstreamFile;

    if (!filename)
        return NULL;
    // if same name, duplicate the file pointer we already have open
    if (!strcmp(streamFile->name,filename)) {
        if (IS_VALID_STREAM(streamFile->infile) && ((newfd = dup(fileno(streamFile->infile))) >= 0) &&
            (newfile = fdopen( newfd, "rb" ))) 
        {
            newstreamFile = open_stdio_streamfile_buffer_by_FILE(newfile,filename,buffersize);
            if (newstreamFile) { 
                return newstreamFile;
            }
            // failure, close it and try the default path (which will probably fail a second time)
            fclose(newfile);
        }
    }
    // a normal open, open a new file
    return open_stdio_streamfile_buffer(filename,buffersize);
}

static STREAMFILE * open_stdio_streamfile_buffer_by_FILE(FILE *infile,const char * const filename, size_t buffersize) {
    uint8_t * buffer;
    STDIOSTREAMFILE * streamfile;

    buffer = calloc(buffersize,1);
    if (!buffer) {
        return NULL;
    }

    streamfile = calloc(1,sizeof(STDIOSTREAMFILE));
    if (!streamfile) {
        free(buffer);
        return NULL;
    }

    streamfile->sf.read = (void*)read_stdio;
    streamfile->sf.get_size = (void*)get_size_stdio;
    streamfile->sf.get_offset = (void*)get_offset_stdio;
    streamfile->sf.get_name = (void*)get_name_stdio;
    streamfile->sf.get_realname = (void*)get_name_stdio;
    streamfile->sf.open = (void*)open_stdio;
    streamfile->sf.close = (void*)close_stdio;
#ifdef PROFILE_STREAMFILE
    streamfile->sf.get_bytes_read = (void*)get_bytes_read_stdio;
    streamfile->sf.get_error_count = (void*)get_error_count_stdio;
#endif

    streamfile->infile = infile;
    streamfile->buffersize = buffersize;
    streamfile->buffer = buffer;

    strcpy(streamfile->name,filename);

    return &streamfile->sf;
}

STREAMFILE * open_stdio_streamfile_buffer(const char * const filename, size_t buffersize) {
    FILE * infile;
    STREAMFILE *streamFile;

    infile = fopen(filename,"rb");
    if (!infile) return NULL;

    streamFile = open_stdio_streamfile_buffer_by_FILE(infile,filename,buffersize);
    if (!streamFile) {
        fclose(infile);
    }

    return streamFile;
}

/* Read a line into dst. The source files are MS-DOS style,
 * separated (not terminated) by CRLF. Return 1 if the full line was
 * retrieved (if it could fit in dst), 0 otherwise. In any case the result
 * will be properly terminated. The CRLF will be removed if there is one.
 * Return the number of bytes read (including CRLF line ending). Note that
 * this is not the length of the string, and could be larger than the buffer.
 * *line_done_ptr is set to 1 if the complete line was read into dst,
 * otherwise it is set to 0. line_done_ptr can be NULL if you aren't
 * interested in this info.
 */
size_t get_streamfile_dos_line(int dst_length, char * dst, off_t offset,
        STREAMFILE * infile, int *line_done_ptr)
{
    int i;
    off_t file_length = get_streamfile_size(infile);
    /* how many bytes over those put in the buffer were read */
    int extra_bytes = 0;

    if (line_done_ptr) *line_done_ptr = 0;

    for (i=0;i<dst_length-1 && offset+i < file_length;i++)
    {
        char in_char = read_8bit(offset+i,infile);
        /* check for end of line */
        if (in_char == 0x0d &&
                read_8bit(offset+i+1,infile) == 0x0a)
        {
            extra_bytes = 2;
            if (line_done_ptr) *line_done_ptr = 1;
            break;
        }

        dst[i]=in_char;
    }
    
    dst[i]='\0';

    /* did we fill the buffer? */
    if (i==dst_length) {
        /* did the bytes we missed just happen to be the end of the line? */
        if (read_8bit(offset+i,infile) == 0x0d &&
                read_8bit(offset+i+1,infile) == 0x0a)
        {
            extra_bytes = 2;
            /* if so be proud! */
            if (line_done_ptr) *line_done_ptr = 1;
        }
    }

    /* did we hit the file end? */
    if (offset+i == file_length)
    {
        /* then we did in fact finish reading the last line */
        if (line_done_ptr) *line_done_ptr = 1;
    }

    return i+extra_bytes;
}
