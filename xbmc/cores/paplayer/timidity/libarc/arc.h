/*
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef ___LIBARC_H_
#define ___LIBARC_H_

/* Archive library */

#include "url.h"
#include "mblock.h"

#define ARC_LIB_VERSION "2.0.1"
#define ARC_DEFLATE_LEVEL 6	/* 1:Compress faster .. 9:Compress better */
#define ARC_ENTRY_HASHSIZE 63



/*
 * Interfaces
 */

extern char **expand_archive_names(int *nfiles_in_out, char **files);
/* Regist all archive files in `files_in_out', and expand the archive */

extern URL url_arc_open(char *name);
/* Open input stream from archive.  `name' format must be "filename#entry".
 */

extern void free_archive_files(void);
/* Call once at the last */

/* utilities */
extern int skip_gzip_header(URL url);
extern int parse_gzip_header_bytes(char *gz, long maxparse, int *hdrsiz);
extern int get_archive_type(char *archive_name);
extern void *arc_compress(void *buff, long bufsiz,
			  int compress_level, long *compressed_size);
extern void *arc_decompress(void *buff, long bufsiz, long *decompressed_size);
extern int arc_case_wildmat(char *text, char *p);
extern int arc_wildmat(char *text, char *p);
extern void (* arc_error_handler)(char *error_message);


/*
 * Internal library usage only
 */
typedef struct _ArchiveEntryNode
{
    struct _ArchiveEntryNode *next; /* next entry */
    char *name; /* Name of this entry */

    int comptype;		/* Compression/Encoding type */
    long compsize;		/* Compressed size */
    long origsize;		/* Uncompressed size */
    long start;		/* Offset start point */
    void *cache;		/* Cached data */
} ArchiveEntryNode;

typedef struct _ArchiveHandler {
    int isfile;
    URL url;	/* Input stream */
    int counter;/* counter to extract the entry*/
    long pos;
} ArchiveHandler;

extern ArchiveHandler arc_handler;
extern ArchiveEntryNode *arc_parse_entry(URL url, int archive_type);
extern ArchiveEntryNode *new_entry_node(char *name, int len);
extern ArchiveEntryNode *next_tar_entry(void);
extern ArchiveEntryNode *next_zip_entry(void);
extern ArchiveEntryNode *next_lzh_entry(void);
extern ArchiveEntryNode *next_mime_entry(void);
extern void free_entry_node(ArchiveEntryNode *entry);

/* Compression/Encoding type */
enum
{
    ARCHIVEC_STORED,		/* No compression */
    ARCHIVEC_PATHNAME,		/* Pathname (Contents exists there) */
    ARCHIVEC_COMPRESSED,	/* Compressed */
    ARCHIVEC_PACKED,		/* Packed */
    ARCHIVEC_DEFLATED,		/* Deflate */
    ARCHIVEC_SHRUNKED,		/* Shrunked */
    ARCHIVEC_REDUCED1,		/* Reduced with compression factor 1 */
    ARCHIVEC_REDUCED2,		/* Reduced with compression factor 2 */
    ARCHIVEC_REDUCED3,		/* Reduced with compression factor 3 */
    ARCHIVEC_REDUCED4,		/* Reduced with compression factor 4 */
    ARCHIVEC_IMPLODED,		/* Implode base-tag */
    ARCHIVEC_IMPLODED_LIT8,	/* 8K sliding window (coded) */
    ARCHIVEC_IMPLODED_LIT4,	/* 4K sliding window (coded) */
    ARCHIVEC_IMPLODED_NOLIT8,	/* 8K sliding window (uncoded) */
    ARCHIVEC_IMPLODED_NOLIT4,	/* 4K sliding window (uncoded) */
    ARCHIVEC_LZHED,		/* LZH base-tag */
    ARCHIVEC_LZHED_LH0,		/* -lh0- (ARCHIVE_STORED) */
    ARCHIVEC_LZHED_LH1,		/* -lh1- */
    ARCHIVEC_LZHED_LH2,		/* -lh2- */
    ARCHIVEC_LZHED_LH3,		/* -lh3- */
    ARCHIVEC_LZHED_LH4,		/* -lh4- */
    ARCHIVEC_LZHED_LH5,		/* -lh5- */
    ARCHIVEC_LZHED_LZS,		/* -lzs- */
    ARCHIVEC_LZHED_LZ5,		/* -lz5- */
    ARCHIVEC_LZHED_LZ4,		/* -lz4- (ARCHIVE_STORED) */
    ARCHIVEC_LZHED_LHD,		/* -lhd- (Directory, No compression data) */
    ARCHIVEC_LZHED_LH6,		/* -lh6- */
    ARCHIVEC_LZHED_LH7,		/* -lh7- */

    /* Encode for MIME */
    ARCHIVEC_UU,		/* uu encoded */
    ARCHIVEC_B64,		/* base64 encoded */
    ARCHIVEC_QS,		/* quoted string encoded */
    ARCHIVEC_HQX		/* HQX encoded */
};

/* archive_type */
enum
{
    ARCHIVE_TAR,
    ARCHIVE_TGZ,
    ARCHIVE_ZIP,
    ARCHIVE_LZH,
    ARCHIVE_DIR,
    ARCHIVE_MIME,
    ARCHIVE_NEWSGROUP
};

#endif /* ___LIBARC_H_ */
