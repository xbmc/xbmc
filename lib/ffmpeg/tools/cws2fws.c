/*
 * cws2fws by Alex Beregszaszi
 * This file is placed in the public domain.
 * Use the program however you see fit.
 *
 * This utility converts compressed Macromedia Flash files to uncompressed ones.
 */

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <zlib.h>

#ifdef DEBUG
#define dbgprintf printf
#else
#define dbgprintf(...)
#endif

int main(int argc, char *argv[])
{
    int fd_in, fd_out, comp_len, uncomp_len, i, last_out;
    char buf_in[1024], buf_out[65536];
    z_stream zstream;
    struct stat statbuf;

    if (argc < 3) {
        printf("Usage: %s <infile.swf> <outfile.swf>\n", argv[0]);
        return 1;
    }

    fd_in = open(argv[1], O_RDONLY);
    if (fd_in < 0) {
        perror("Error opening input file");
        return 1;
    }

    fd_out = open(argv[2], O_WRONLY | O_CREAT, 00644);
    if (fd_out < 0) {
        perror("Error opening output file");
        close(fd_in);
        return 1;
    }

    if (read(fd_in, &buf_in, 8) != 8) {
        printf("Header error\n");
        close(fd_in);
        close(fd_out);
        return 1;
    }

    if (buf_in[0] != 'C' || buf_in[1] != 'W' || buf_in[2] != 'S') {
        printf("Not a compressed flash file\n");
        return 1;
    }

    fstat(fd_in, &statbuf);
    comp_len   = statbuf.st_size;
    uncomp_len = buf_in[4] | (buf_in[5] << 8) | (buf_in[6] << 16) | (buf_in[7] << 24);

    printf("Compressed size: %d Uncompressed size: %d\n",
           comp_len - 4, uncomp_len - 4);

    // write out modified header
    buf_in[0] = 'F';
    if (write(fd_out, &buf_in, 8) < 8) {
        perror("Error writing output file");
        return 1;
    }

    zstream.zalloc = NULL;
    zstream.zfree  = NULL;
    zstream.opaque = NULL;
    inflateInit(&zstream);

    for (i = 0; i < comp_len - 8;) {
        int ret, len = read(fd_in, &buf_in, 1024);

        dbgprintf("read %d bytes\n", len);

        last_out = zstream.total_out;

        zstream.next_in   = &buf_in[0];
        zstream.avail_in  = len;
        zstream.next_out  = &buf_out[0];
        zstream.avail_out = 65536;

        ret = inflate(&zstream, Z_SYNC_FLUSH);
        if (ret != Z_STREAM_END && ret != Z_OK) {
            printf("Error while decompressing: %d\n", ret);
            inflateEnd(&zstream);
            return 1;
        }

        dbgprintf("a_in: %d t_in: %lu a_out: %d t_out: %lu -- %lu out\n",
                  zstream.avail_in, zstream.total_in, zstream.avail_out,
                  zstream.total_out, zstream.total_out - last_out);

        if (write(fd_out, &buf_out, zstream.total_out - last_out) <
            zstream.total_out - last_out) {
            perror("Error writing output file");
            return 1;
        }

        i += len;

        if (ret == Z_STREAM_END || ret == Z_BUF_ERROR)
            break;
    }

    if (zstream.total_out != uncomp_len - 8) {
        printf("Size mismatch (%lu != %d), updating header...\n",
               zstream.total_out, uncomp_len - 8);

        buf_in[0] =  (zstream.total_out + 8)        & 0xff;
        buf_in[1] = ((zstream.total_out + 8) >>  8) & 0xff;
        buf_in[2] = ((zstream.total_out + 8) >> 16) & 0xff;
        buf_in[3] = ((zstream.total_out + 8) >> 24) & 0xff;

        lseek(fd_out, 4, SEEK_SET);
        if (write(fd_out, &buf_in, 4) < 4) {
            perror("Error writing output file");
            return 1;
        }
    }

    inflateEnd(&zstream);
    close(fd_in);
    close(fd_out);
    return 0;
}
