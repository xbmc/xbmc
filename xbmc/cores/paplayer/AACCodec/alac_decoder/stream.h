#ifndef STREAM_H
#define STREAM_H

/* stream.h */

//#include <stdint.h>

typedef struct stream_tTAG stream_t;

void stream_read(stream_t *stream, size_t len, void *buf);

int stream_read_int32(stream_t *stream);
unsigned int stream_read_uint32(stream_t *stream);

short stream_read_int16(stream_t *stream);
unsigned short stream_read_uint16(stream_t *stream);

char stream_read_int8(stream_t *stream);
unsigned char stream_read_uint8(stream_t *stream);

void stream_skip(stream_t *stream, size_t skip);

int stream_eof(stream_t *stream);

stream_t *stream_create_file(FILE *file,
                             int bigendian);
void stream_destroy(stream_t *stream);

// -- Additions by Arnie Pie
int	stream_seek(stream_t *stream, long val, int mode);
long stream_tell(stream_t *stream);
// -- End of additions

#endif /* STREAM_H */


