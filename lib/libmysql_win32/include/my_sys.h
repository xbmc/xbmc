/* Copyright (C) 2000-2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef _my_sys_h
#define _my_sys_h
C_MODE_START

#ifdef HAVE_AIOWAIT
#include <sys/asynch.h>			/* Used by record-cache */
typedef struct my_aio_result {
  aio_result_t result;
  int	       pending;
} my_aio_result;
#endif

#ifndef THREAD
extern int NEAR my_errno;		/* Last error in mysys */
#else
#include <my_pthread.h>
#endif

#include <m_ctype.h>                    /* for CHARSET_INFO */
#include <stdarg.h>
#include <typelib.h>

#define MYSYS_PROGRAM_USES_CURSES()  { error_handler_hook = my_message_curses;	mysys_uses_curses=1; }
#define MYSYS_PROGRAM_DONT_USE_CURSES()  { error_handler_hook = my_message_no_curses; mysys_uses_curses=0;}
#define MY_INIT(name);		{ my_progname= name; my_init(); }

#define MY_FILE_ERROR	((size_t) -1)

	/* General bitmaps for my_func's */
#define MY_FFNF		1	/* Fatal if file not found */
#define MY_FNABP	2	/* Fatal if not all bytes read/writen */
#define MY_NABP		4	/* Error if not all bytes read/writen */
#define MY_FAE		8	/* Fatal if any error */
#define MY_WME		16	/* Write message on error */
#define MY_WAIT_IF_FULL 32	/* Wait and try again if disk full error */
#define MY_IGNORE_BADFD 32      /* my_sync: ignore 'bad descriptor' errors */
#define MY_SYNC_DIR     1024    /* my_create/delete/rename: sync directory */
#define MY_RAID         64      /* Support for RAID */
#define MY_FULL_IO     512      /* For my_read - loop intil I/O is complete */
#define MY_DONT_CHECK_FILESIZE 128 /* Option to init_io_cache() */
#define MY_LINK_WARNING 32	/* my_redel() gives warning if links */
#define MY_COPYTIME	64	/* my_redel() copys time */
#define MY_DELETE_OLD	256	/* my_create_with_symlink() */
#define MY_RESOLVE_LINK 128	/* my_realpath(); Only resolve links */
#define MY_HOLD_ORIGINAL_MODES 128  /* my_copy() holds to file modes */
#define MY_REDEL_MAKE_BACKUP 256
#define MY_SEEK_NOT_DONE 32	/* my_lock may have to do a seek */
#define MY_DONT_WAIT	64	/* my_lock() don't wait if can't lock */
#define MY_ZEROFILL	32	/* my_malloc(), fill array with zero */
#define MY_ALLOW_ZERO_PTR 64	/* my_realloc() ; zero ptr -> malloc */
#define MY_FREE_ON_ERROR 128	/* my_realloc() ; Free old ptr on error */
#define MY_HOLD_ON_ERROR 256	/* my_realloc() ; Return old ptr on error */
#define MY_DONT_OVERWRITE_FILE 1024	/* my_copy: Don't overwrite file */
#define MY_THREADSAFE 2048      /* my_seek(): lock fd mutex */
#define MY_SYNC       4096      /* my_copy(): sync dst file */

#define MY_CHECK_ERROR	1	/* Params to my_end; Check open-close */
#define MY_GIVE_INFO	2	/* Give time info about process*/
#define MY_DONT_FREE_DBUG 4     /* Do not call DBUG_END() in my_end() */

#define MY_REMOVE_NONE    0     /* Params for modify_defaults_file */
#define MY_REMOVE_OPTION  1
#define MY_REMOVE_SECTION 2

#define ME_HIGHBYTE	8	/* Shift for colours */
#define ME_NOCUR	1	/* Don't use curses message */
#define ME_OLDWIN	2	/* Use old window */
#define ME_BELL		4	/* Ring bell then printing message */
#define ME_HOLDTANG	8	/* Don't delete last keys */
#define ME_WAITTOT	16	/* Wait for errtime secs of for a action */
#define ME_WAITTANG	32	/* Wait for a user action  */
#define ME_NOREFRESH	64	/* Dont refresh screen */
#define ME_NOINPUT	128	/* Dont use the input libary */
#define ME_COLOUR1	((1 << ME_HIGHBYTE))	/* Possibly error-colours */
#define ME_COLOUR2	((2 << ME_HIGHBYTE))
#define ME_COLOUR3	((3 << ME_HIGHBYTE))
#define ME_FATALERROR   1024    /* Fatal statement error */
#define ME_NO_WARNING_FOR_ERROR 2048 /* Don't push a warning for error */
#define ME_NO_SP_HANDLER 4096 /* Don't call stored routine error handlers */

	/* Bits in last argument to fn_format */
#define MY_REPLACE_DIR		1	/* replace dir in name with 'dir' */
#define MY_REPLACE_EXT		2	/* replace extension with 'ext' */
#define MY_UNPACK_FILENAME	4	/* Unpack name (~ -> home) */
#define MY_PACK_FILENAME	8	/* Pack name (home -> ~) */
#define MY_RESOLVE_SYMLINKS	16	/* Resolve all symbolic links */
#define MY_RETURN_REAL_PATH	32	/* return full path for file */
#define MY_SAFE_PATH		64	/* Return NULL if too long path */
#define MY_RELATIVE_PATH	128	/* name is relative to 'dir' */
#define MY_APPEND_EXT           256     /* add 'ext' as additional extension*/


	/* My seek flags */
#define MY_SEEK_SET	0
#define MY_SEEK_CUR	1
#define MY_SEEK_END	2

	/* Some constants */
#define MY_WAIT_FOR_USER_TO_FIX_PANIC	60	/* in seconds */
#define MY_WAIT_GIVE_USER_A_MESSAGE	10	/* Every 10 times of prev */
#define MIN_COMPRESS_LENGTH		50	/* Don't compress small bl. */
#define DFLT_INIT_HITS  3

	/* root_alloc flags */
#define MY_KEEP_PREALLOC	1
#define MY_MARK_BLOCKS_FREE     2  /* move used to free list and reuse them */

	/* Internal error numbers (for assembler functions) */
#define MY_ERRNO_EDOM		33
#define MY_ERRNO_ERANGE		34

	/* Bits for get_date timeflag */
#define GETDATE_DATE_TIME	1
#define GETDATE_SHORT_DATE	2
#define GETDATE_HHMMSSTIME	4
#define GETDATE_GMT		8
#define GETDATE_FIXEDLENGTH	16

	/* defines when allocating data */
#ifdef SAFEMALLOC
#define my_malloc(SZ,FLAG) _mymalloc((SZ), __FILE__, __LINE__, FLAG )
#define my_malloc_ci(SZ,FLAG) _mymalloc((SZ), sFile, uLine, FLAG )
#define my_realloc(PTR,SZ,FLAG) _myrealloc((PTR), (SZ), __FILE__, __LINE__, FLAG )
#define my_checkmalloc() _sanity( __FILE__, __LINE__ )
#define my_free(PTR,FLAG) _myfree((PTR), __FILE__, __LINE__,FLAG)
#define my_memdup(A,B,C) _my_memdup((A),(B), __FILE__,__LINE__,C)
#define my_strdup(A,C) _my_strdup((A), __FILE__,__LINE__,C)
#define my_strndup(A,B,C) _my_strndup((A),(B),__FILE__,__LINE__,C)
#define TRASH(A,B) bfill(A, B, 0x8F)
#define QUICK_SAFEMALLOC sf_malloc_quick=1
#define NORMAL_SAFEMALLOC sf_malloc_quick=0
extern uint sf_malloc_prehunc,sf_malloc_endhunc,sf_malloc_quick;
extern ulonglong sf_malloc_mem_limit;

#define CALLER_INFO_PROTO   , const char *sFile, uint uLine
#define CALLER_INFO         , __FILE__, __LINE__
#define ORIG_CALLER_INFO    , sFile, uLine
#else
#define my_checkmalloc()
#undef TERMINATE
#define TERMINATE(A,B) {}
#define QUICK_SAFEMALLOC
#define NORMAL_SAFEMALLOC
extern void *my_malloc(size_t Size,myf MyFlags);
#define my_malloc_ci(SZ,FLAG) my_malloc( SZ, FLAG )
extern void *my_realloc(void *oldpoint, size_t Size, myf MyFlags);
extern void my_no_flags_free(void *ptr);
extern void *my_memdup(const void *from,size_t length,myf MyFlags);
extern char *my_strdup(const char *from,myf MyFlags);
extern char *my_strndup(const char *from, size_t length,
				   myf MyFlags);
/* we do use FG (as a no-op) in below so that a typo on FG is caught */
#define my_free(PTR,FG) ((void)FG,my_no_flags_free(PTR))
#define CALLER_INFO_PROTO   /* nothing */
#define CALLER_INFO         /* nothing */
#define ORIG_CALLER_INFO    /* nothing */
#define TRASH(A,B) /* nothing */
#endif

#if defined(ENABLED_DEBUG_SYNC)
extern void (*debug_sync_C_callback_ptr)(const char *, size_t);
#define DEBUG_SYNC_C(_sync_point_name_) do {                            \
    if (debug_sync_C_callback_ptr != NULL)                              \
      (*debug_sync_C_callback_ptr)(STRING_WITH_LEN(_sync_point_name_)); } \
  while(0)
#else
#define DEBUG_SYNC_C(_sync_point_name_)
#endif /* defined(ENABLED_DEBUG_SYNC) */

#ifdef HAVE_LARGE_PAGES
extern uint my_get_large_page_size(void);
extern uchar * my_large_malloc(size_t size, myf my_flags);
extern void my_large_free(uchar * ptr, myf my_flags);
#else
#define my_get_large_page_size() (0)
#define my_large_malloc(A,B) my_malloc_lock((A),(B))
#define my_large_free(A,B) my_free_lock((A),(B))
#endif /* HAVE_LARGE_PAGES */

#ifdef HAVE_ALLOCA
#if defined(_AIX) && !defined(__GNUC__) && !defined(_AIX43)
#pragma alloca
#endif /* _AIX */
#if defined(__MWERKS__)
#undef alloca
#define alloca _alloca
#endif /* __MWERKS__ */
#if defined(__GNUC__) && !defined(HAVE_ALLOCA_H) && ! defined(alloca)
#define alloca __builtin_alloca
#endif /* GNUC */
#define my_alloca(SZ) alloca((size_t) (SZ))
#define my_afree(PTR) {}
#else
#define my_alloca(SZ) my_malloc(SZ,MYF(0))
#define my_afree(PTR) my_free(PTR,MYF(MY_WME))
#endif /* HAVE_ALLOCA */

#ifndef errno				/* did we already get it? */
#ifdef HAVE_ERRNO_AS_DEFINE
#include <errno.h>			/* errno is a define */
#else
extern int errno;			/* declare errno */
#endif
#endif					/* #ifndef errno */
extern char *home_dir;			/* Home directory for user */
extern const char *my_progname;		/* program-name (printed in errors) */
extern char NEAR curr_dir[];		/* Current directory for user */
extern int (*error_handler_hook)(uint my_err, const char *str,myf MyFlags);
extern int (*fatal_error_handler_hook)(uint my_err, const char *str,
				       myf MyFlags);
extern uint my_file_limit;
extern ulong my_thread_stack_size;

#ifdef HAVE_LARGE_PAGES
extern my_bool my_use_large_pages;
extern uint    my_large_page_size;
#endif

/* charsets */
extern MYSQL_PLUGIN_IMPORT CHARSET_INFO *default_charset_info;
extern MYSQL_PLUGIN_IMPORT CHARSET_INFO *all_charsets[256];
extern CHARSET_INFO compiled_charsets[];

/* statistics */
extern ulong	my_file_opened,my_stream_opened, my_tmp_file_created;
extern ulong    my_file_total_opened;
extern uint	mysys_usage_id;
extern my_bool	my_init_done;

					/* Point to current my_message() */
extern void (*my_sigtstp_cleanup)(void),
					/* Executed before jump to shell */
	    (*my_sigtstp_restart)(void),
	    (*my_abort_hook)(int);
					/* Executed when comming from shell */
extern MYSQL_PLUGIN_IMPORT int NEAR my_umask;		/* Default creation mask  */
extern int NEAR my_umask_dir,
	   NEAR my_recived_signals,	/* Signals we have got */
	   NEAR my_safe_to_handle_signal, /* Set when allowed to SIGTSTP */
	   NEAR my_dont_interrupt;	/* call remember_intr when set */
extern my_bool NEAR mysys_uses_curses, my_use_symdir;
extern size_t sf_malloc_cur_memory, sf_malloc_max_memory;

extern ulong	my_default_record_cache_size;
extern my_bool NEAR my_disable_locking,NEAR my_disable_async_io,
               NEAR my_disable_flush_key_blocks, NEAR my_disable_symlinks;
extern char	wild_many,wild_one,wild_prefix;
extern const char *charsets_dir;
/* from default.c */
extern char *my_defaults_extra_file;
extern const char *my_defaults_group_suffix;
extern const char *my_defaults_file;

extern my_bool timed_mutexes;

typedef struct wild_file_pack	/* Struct to hold info when selecting files */
{
  uint		wilds;		/* How many wildcards */
  uint		not_pos;	/* Start of not-theese-files */
  char *	*wild;		/* Pointer to wildcards */
} WF_PACK;

enum loglevel {
   ERROR_LEVEL,
   WARNING_LEVEL,
   INFORMATION_LEVEL
};

enum cache_type
{
  TYPE_NOT_SET= 0, READ_CACHE, WRITE_CACHE,
  SEQ_READ_APPEND		/* sequential read or append */,
  READ_FIFO, READ_NET,WRITE_NET};

enum flush_type
{
  FLUSH_KEEP,           /* flush block and keep it in the cache */
  FLUSH_RELEASE,        /* flush block and remove it from the cache */
  FLUSH_IGNORE_CHANGED, /* remove block from the cache */
  /*
    As my_disable_flush_pagecache_blocks is always 0, the following option
    is strictly equivalent to FLUSH_KEEP
  */
  FLUSH_FORCE_WRITE
};

typedef struct st_record_cache	/* Used when cacheing records */
{
  File file;
  int	rc_seek,error,inited;
  uint	rc_length,read_length,reclength;
  my_off_t rc_record_pos,end_of_file;
  uchar *rc_buff,*rc_buff2,*rc_pos,*rc_end,*rc_request_pos;
#ifdef HAVE_AIOWAIT
  int	use_async_io;
  my_aio_result aio_result;
#endif
  enum cache_type type;
} RECORD_CACHE;

enum file_type
{
  UNOPEN = 0, FILE_BY_OPEN, FILE_BY_CREATE, STREAM_BY_FOPEN, STREAM_BY_FDOPEN,
  FILE_BY_MKSTEMP, FILE_BY_DUP
};

struct st_my_file_info
{
  char *		name;
  enum file_type	type;
#if defined(THREAD) && !defined(HAVE_PREAD)
  pthread_mutex_t	mutex;
#endif
};

extern struct st_my_file_info *my_file_info;

typedef struct st_dynamic_array
{
  uchar *buffer;
  uint elements,max_element;
  uint alloc_increment;
  uint size_of_element;
} DYNAMIC_ARRAY;

typedef struct st_my_tmpdir
{
  DYNAMIC_ARRAY full_list;
  char **list;
  uint cur, max;
#ifdef THREAD
  pthread_mutex_t mutex;
#endif
} MY_TMPDIR;

typedef struct st_dynamic_string
{
  char *str;
  size_t length,max_length,alloc_increment;
} DYNAMIC_STRING;

struct st_io_cache;
typedef int (*IO_CACHE_CALLBACK)(struct st_io_cache*);

#ifdef THREAD
typedef struct st_io_cache_share
{
  pthread_mutex_t       mutex;           /* To sync on reads into buffer. */
  pthread_cond_t        cond;            /* To wait for signals. */
  pthread_cond_t        cond_writer;     /* For a synchronized writer. */
  /* Offset in file corresponding to the first byte of buffer. */
  my_off_t              pos_in_file;
  /* If a synchronized write cache is the source of the data. */
  struct st_io_cache    *source_cache;
  uchar                 *buffer;         /* The read buffer. */
  uchar                 *read_end;       /* Behind last valid byte of buffer. */
  int                   running_threads; /* threads not in lock. */
  int                   total_threads;   /* threads sharing the cache. */
  int                   error;           /* Last error. */
#ifdef NOT_YET_IMPLEMENTED
  /* whether the structure should be free'd */
  my_bool alloced;
#endif
} IO_CACHE_SHARE;
#endif

typedef struct st_io_cache		/* Used when cacheing files */
{
  /* Offset in file corresponding to the first byte of uchar* buffer. */
  my_off_t pos_in_file;
  /*
    The offset of end of file for READ_CACHE and WRITE_CACHE.
    For SEQ_READ_APPEND it the maximum of the actual end of file and
    the position represented by read_end.
  */
  my_off_t end_of_file;
  /* Points to current read position in the buffer */
  uchar	*read_pos;
  /* the non-inclusive boundary in the buffer for the currently valid read */
  uchar  *read_end;
  uchar  *buffer;				/* The read buffer */
  /* Used in ASYNC_IO */
  uchar  *request_pos;

  /* Only used in WRITE caches and in SEQ_READ_APPEND to buffer writes */
  uchar  *write_buffer;
  /*
    Only used in SEQ_READ_APPEND, and points to the current read position
    in the write buffer. Note that reads in SEQ_READ_APPEND caches can
    happen from both read buffer (uchar* buffer) and write buffer
    (uchar* write_buffer).
  */
  uchar *append_read_pos;
  /* Points to current write position in the write buffer */
  uchar *write_pos;
  /* The non-inclusive boundary of the valid write area */
  uchar *write_end;

  /*
    Current_pos and current_end are convenience variables used by
    my_b_tell() and other routines that need to know the current offset
    current_pos points to &write_pos, and current_end to &write_end in a
    WRITE_CACHE, and &read_pos and &read_end respectively otherwise
  */
  uchar  **current_pos, **current_end;
#ifdef THREAD
  /*
    The lock is for append buffer used in SEQ_READ_APPEND cache
    need mutex copying from append buffer to read buffer.
  */
  pthread_mutex_t append_buffer_lock;
  /*
    The following is used when several threads are reading the
    same file in parallel. They are synchronized on disk
    accesses reading the cached part of the file asynchronously.
    It should be set to NULL to disable the feature.  Only
    READ_CACHE mode is supported.
  */
  IO_CACHE_SHARE *share;
#endif
  /*
    A caller will use my_b_read() macro to read from the cache
    if the data is already in cache, it will be simply copied with
    memcpy() and internal variables will be accordinging updated with
    no functions invoked. However, if the data is not fully in the cache,
    my_b_read() will call read_function to fetch the data. read_function
    must never be invoked directly.
  */
  int (*read_function)(struct st_io_cache *,uchar *,size_t);
  /*
    Same idea as in the case of read_function, except my_b_write() needs to
    be replaced with my_b_append() for a SEQ_READ_APPEND cache
  */
  int (*write_function)(struct st_io_cache *,const uchar *,size_t);
  /*
    Specifies the type of the cache. Depending on the type of the cache
    certain operations might not be available and yield unpredicatable
    results. Details to be documented later
  */
  enum cache_type type;
  /*
    Callbacks when the actual read I/O happens. These were added and
    are currently used for binary logging of LOAD DATA INFILE - when a
    block is read from the file, we create a block create/append event, and
    when IO_CACHE is closed, we create an end event. These functions could,
    of course be used for other things
  */
  IO_CACHE_CALLBACK pre_read;
  IO_CACHE_CALLBACK post_read;
  IO_CACHE_CALLBACK pre_close;
  /*
    Counts the number of times, when we were forced to use disk. We use it to
    increase the binlog_cache_disk_use status variable.
  */
  ulong disk_writes;
  void* arg;				/* for use by pre/post_read */
  char *file_name;			/* if used with 'open_cached_file' */
  char *dir,*prefix;
  File file; /* file descriptor */
  /*
    seek_not_done is set by my_b_seek() to inform the upcoming read/write
    operation that a seek needs to be preformed prior to the actual I/O
    error is 0 if the cache operation was successful, -1 if there was a
    "hard" error, and the actual number of I/O-ed bytes if the read/write was
    partial.
  */
  int	seek_not_done,error;
  /* buffer_length is memory size allocated for buffer or write_buffer */
  size_t	buffer_length;
  /* read_length is the same as buffer_length except when we use async io */
  size_t  read_length;
  myf	myflags;			/* Flags used to my_read/my_write */
  /*
    alloced_buffer is 1 if the buffer was allocated by init_io_cache() and
    0 if it was supplied by the user.
    Currently READ_NET is the only one that will use a buffer allocated
    somewhere else
  */
  my_bool alloced_buffer;
#ifdef HAVE_AIOWAIT
  /*
    As inidicated by ifdef, this is for async I/O, which is not currently
    used (because it's not reliable on all systems)
  */
  uint inited;
  my_off_t aio_read_pos;
  my_aio_result aio_result;
#endif
} IO_CACHE;

typedef int (*qsort2_cmp)(const void *, const void *, const void *);

	/* defines for mf_iocache */

	/* Test if buffer is inited */
#define my_b_clear(info) (info)->buffer=0
#define my_b_inited(info) (info)->buffer
#define my_b_EOF INT_MIN

#define my_b_read(info,Buffer,Count) \
  ((info)->read_pos + (Count) <= (info)->read_end ?\
   (memcpy(Buffer,(info)->read_pos,(size_t) (Count)), \
    ((info)->read_pos+=(Count)),0) :\
   (*(info)->read_function)((info),Buffer,Count))

#define my_b_write(info,Buffer,Count) \
 ((info)->write_pos + (Count) <=(info)->write_end ?\
  (memcpy((info)->write_pos, (Buffer), (size_t)(Count)),\
   ((info)->write_pos+=(Count)),0) : \
   (*(info)->write_function)((info),(uchar *)(Buffer),(Count)))

#define my_b_get(info) \
  ((info)->read_pos != (info)->read_end ?\
   ((info)->read_pos++, (int) (uchar) (info)->read_pos[-1]) :\
   _my_b_get(info))

	/* my_b_write_byte dosn't have any err-check */
#define my_b_write_byte(info,chr) \
  (((info)->write_pos < (info)->write_end) ?\
   ((*(info)->write_pos++)=(chr)) :\
   (_my_b_write(info,0,0) , ((*(info)->write_pos++)=(chr))))

#define my_b_fill_cache(info) \
  (((info)->read_end=(info)->read_pos),(*(info)->read_function)(info,0,0))

#define my_b_tell(info) ((info)->pos_in_file + \
			 (size_t) (*(info)->current_pos - (info)->request_pos))

#define my_b_get_buffer_start(info) (info)->request_pos 
#define my_b_get_bytes_in_buffer(info) (char*) (info)->read_end -   \
  (char*) my_b_get_buffer_start(info)
#define my_b_get_pos_in_file(info) (info)->pos_in_file

/* tell write offset in the SEQ_APPEND cache */
int      my_b_copy_to_file(IO_CACHE *cache, FILE *file);
my_off_t my_b_append_tell(IO_CACHE* info);
my_off_t my_b_safe_tell(IO_CACHE* info); /* picks the correct tell() */

#define my_b_bytes_in_cache(info) (size_t) (*(info)->current_end - \
					  *(info)->current_pos)

typedef uint32 ha_checksum;

/* Define the type of function to be passed to process_default_option_files */
typedef int (*Process_option_func)(void *ctx, const char *group_name,
                                   const char *option);

#include <my_alloc.h>


	/* Prototypes for mysys and my_func functions */

extern int my_copy(const char *from,const char *to,myf MyFlags);
extern int my_append(const char *from,const char *to,myf MyFlags);
extern int my_delete(const char *name,myf MyFlags);
extern int my_getwd(char * buf,size_t size,myf MyFlags);
extern int my_setwd(const char *dir,myf MyFlags);
extern int my_lock(File fd,int op,my_off_t start, my_off_t length,myf MyFlags);
extern void *my_once_alloc(size_t Size,myf MyFlags);
extern void my_once_free(void);
extern char *my_once_strdup(const char *src,myf myflags);
extern void *my_once_memdup(const void *src, size_t len, myf myflags);
extern File my_open(const char *FileName,int Flags,myf MyFlags);
extern File my_register_filename(File fd, const char *FileName,
				 enum file_type type_of_file,
				 uint error_message_number, myf MyFlags);
extern File my_create(const char *FileName,int CreateFlags,
		      int AccessFlags, myf MyFlags);
extern int my_close(File Filedes,myf MyFlags);
extern File my_dup(File file, myf MyFlags);
extern int my_mkdir(const char *dir, int Flags, myf MyFlags);
extern int my_readlink(char *to, const char *filename, myf MyFlags);
extern int my_is_symlink(const char *filename);
extern int my_realpath(char *to, const char *filename, myf MyFlags);
extern File my_create_with_symlink(const char *linkname, const char *filename,
				   int createflags, int access_flags,
				   myf MyFlags);
extern int my_delete_with_symlink(const char *name, myf MyFlags);
extern int my_rename_with_symlink(const char *from,const char *to,myf MyFlags);
extern int my_symlink(const char *content, const char *linkname, myf MyFlags);
extern size_t my_read(File Filedes,uchar *Buffer,size_t Count,myf MyFlags);
extern size_t my_pread(File Filedes,uchar *Buffer,size_t Count,my_off_t offset,
		     myf MyFlags);
extern int my_rename(const char *from,const char *to,myf MyFlags);
extern my_off_t my_seek(File fd,my_off_t pos,int whence,myf MyFlags);
extern my_off_t my_tell(File fd,myf MyFlags);
extern size_t my_write(File Filedes,const uchar *Buffer,size_t Count,
		     myf MyFlags);
extern size_t my_pwrite(File Filedes,const uchar *Buffer,size_t Count,
		      my_off_t offset,myf MyFlags);
extern size_t my_fread(FILE *stream,uchar *Buffer,size_t Count,myf MyFlags);
extern size_t my_fwrite(FILE *stream,const uchar *Buffer,size_t Count,
		      myf MyFlags);
extern my_off_t my_fseek(FILE *stream,my_off_t pos,int whence,myf MyFlags);
extern my_off_t my_ftell(FILE *stream,myf MyFlags);
extern void *_mymalloc(size_t uSize,const char *sFile,
                       uint uLine, myf MyFlag);
extern void *_myrealloc(void *pPtr,size_t uSize,const char *sFile,
		       uint uLine, myf MyFlag);
extern void * my_multi_malloc _VARARGS((myf MyFlags, ...));
extern void _myfree(void *pPtr,const char *sFile,uint uLine, myf MyFlag);
extern int _sanity(const char *sFile, uint uLine);
extern void *_my_memdup(const void *from, size_t length,
                        const char *sFile, uint uLine,myf MyFlag);
extern char * _my_strdup(const char *from, const char *sFile, uint uLine,
                         myf MyFlag);
extern char *_my_strndup(const char *from, size_t length,
                         const char *sFile, uint uLine,
                         myf MyFlag);

/* implemented in my_memmem.c */
extern void *my_memmem(const void *haystack, size_t haystacklen,
                       const void *needle, size_t needlelen);


#ifdef __WIN__
extern int my_access(const char *path, int amode);
extern File my_sopen(const char *path, int oflag, int shflag, int pmode);
#else
#define my_access access
#endif
extern int check_if_legal_filename(const char *path);
extern int check_if_legal_tablename(const char *path);

#if defined(__WIN__) && defined(__NT__)
extern int nt_share_delete(const char *name,myf MyFlags);
#define my_delete_allow_opened(fname,flags)  nt_share_delete((fname),(flags))
#else
#define my_delete_allow_opened(fname,flags)  my_delete((fname),(flags))
#endif

#ifndef TERMINATE
extern void TERMINATE(FILE *file, uint flag);
#endif
extern void init_glob_errs(void);
extern void wait_for_free_space(const char *filename, int errors);
extern FILE *my_fopen(const char *FileName,int Flags,myf MyFlags);
extern FILE *my_fdopen(File Filedes,const char *name, int Flags,myf MyFlags);
extern int my_fclose(FILE *fd,myf MyFlags);
extern int my_chsize(File fd,my_off_t newlength, int filler, myf MyFlags);
extern int my_sync(File fd, myf my_flags);
extern int my_sync_dir(const char *dir_name, myf my_flags);
extern int my_sync_dir_by_file(const char *file_name, myf my_flags);
extern int my_error _VARARGS((int nr,myf MyFlags, ...));
extern int my_printf_error _VARARGS((uint my_err, const char *format,
				     myf MyFlags, ...))
				    ATTRIBUTE_FORMAT(printf, 2, 4);
extern int my_error_register(const char **errmsgs, int first, int last);
extern const char **my_error_unregister(int first, int last);
extern int my_message(uint my_err, const char *str,myf MyFlags);
extern int my_message_no_curses(uint my_err, const char *str,myf MyFlags);
extern int my_message_curses(uint my_err, const char *str,myf MyFlags);
extern my_bool my_init(void);
extern void my_end(int infoflag);
extern int my_redel(const char *from, const char *to, int MyFlags);
extern int my_copystat(const char *from, const char *to, int MyFlags);
extern char * my_filename(File fd);

#ifndef THREAD
extern void dont_break(void);
extern void allow_break(void);
#else
#define dont_break()
#define allow_break()
#endif

#ifdef EXTRA_DEBUG
void my_print_open_files(void);
#else
#define my_print_open_files()
#endif

extern my_bool init_tmpdir(MY_TMPDIR *tmpdir, const char *pathlist);
extern char *my_tmpdir(MY_TMPDIR *tmpdir);
extern void free_tmpdir(MY_TMPDIR *tmpdir);

extern void my_remember_signal(int signal_number,sig_handler (*func)(int));
extern size_t dirname_part(char * to,const char *name, size_t *to_res_length);
extern size_t dirname_length(const char *name);
#define base_name(A) (A+dirname_length(A))
extern int test_if_hard_path(const char *dir_name);
extern my_bool has_path(const char *name);
extern char *convert_dirname(char *to, const char *from, const char *from_end);
extern void to_unix_path(char * name);
extern char * fn_ext(const char *name);
extern char * fn_same(char * toname,const char *name,int flag);
extern char * fn_format(char * to,const char *name,const char *dir,
			   const char *form, uint flag);
extern size_t strlength(const char *str);
extern void pack_dirname(char * to,const char *from);
extern size_t normalize_dirname(char * to, const char *from);
extern size_t unpack_dirname(char * to,const char *from);
extern size_t cleanup_dirname(char * to,const char *from);
extern size_t system_filename(char * to,const char *from);
extern size_t unpack_filename(char * to,const char *from);
extern char * intern_filename(char * to,const char *from);
extern char * directory_file_name(char * dst, const char *src);
extern int pack_filename(char * to, const char *name, size_t max_length);
extern char * my_path(char * to,const char *progname,
			 const char *own_pathname_part);
extern char * my_load_path(char * to, const char *path,
			      const char *own_path_prefix);
extern int wild_compare(const char *str,const char *wildstr,
                        pbool str_is_pattern);
extern WF_PACK *wf_comp(char * str);
extern int wf_test(struct wild_file_pack *wf_pack,const char *name);
extern void wf_end(struct wild_file_pack *buffer);
extern my_bool array_append_string_unique(const char *str,
                                          const char **array, size_t size);
extern void get_date(char * to,int timeflag,time_t use_time);
extern void soundex(CHARSET_INFO *, char * out_pntr, char * in_pntr,
                    pbool remove_garbage);
extern int init_record_cache(RECORD_CACHE *info,size_t cachesize,File file,
			     size_t reclength,enum cache_type type,
			     pbool use_async_io);
extern int read_cache_record(RECORD_CACHE *info,uchar *to);
extern int end_record_cache(RECORD_CACHE *info);
extern int write_cache_record(RECORD_CACHE *info,my_off_t filepos,
			      const uchar *record,size_t length);
extern int flush_write_cache(RECORD_CACHE *info);
extern long my_clock(void);
extern sig_handler sigtstp_handler(int signal_number);
extern void handle_recived_signals(void);

extern sig_handler my_set_alarm_variable(int signo);
extern void my_string_ptr_sort(uchar *base,uint items,size_t size);
extern void radixsort_for_str_ptr(uchar* base[], uint number_of_elements,
				  size_t size_of_element,uchar *buffer[]);
extern qsort_t my_qsort(void *base_ptr, size_t total_elems, size_t size,
                        qsort_cmp cmp);
extern qsort_t my_qsort2(void *base_ptr, size_t total_elems, size_t size,
                         qsort2_cmp cmp, void *cmp_argument);
extern qsort2_cmp get_ptr_compare(size_t);
void my_store_ptr(uchar *buff, size_t pack_length, my_off_t pos);
my_off_t my_get_ptr(uchar *ptr, size_t pack_length);
extern int init_io_cache(IO_CACHE *info,File file,size_t cachesize,
			 enum cache_type type,my_off_t seek_offset,
			 pbool use_async_io, myf cache_myflags);
extern my_bool reinit_io_cache(IO_CACHE *info,enum cache_type type,
			       my_off_t seek_offset,pbool use_async_io,
			       pbool clear_cache);
extern void setup_io_cache(IO_CACHE* info);
extern int _my_b_read(IO_CACHE *info,uchar *Buffer,size_t Count);
#ifdef THREAD
extern int _my_b_read_r(IO_CACHE *info,uchar *Buffer,size_t Count);
extern void init_io_cache_share(IO_CACHE *read_cache, IO_CACHE_SHARE *cshare,
                                IO_CACHE *write_cache, uint num_threads);
extern void remove_io_thread(IO_CACHE *info);
#endif
extern int _my_b_seq_read(IO_CACHE *info,uchar *Buffer,size_t Count);
extern int _my_b_net_read(IO_CACHE *info,uchar *Buffer,size_t Count);
extern int _my_b_get(IO_CACHE *info);
extern int _my_b_async_read(IO_CACHE *info,uchar *Buffer,size_t Count);
extern int _my_b_write(IO_CACHE *info,const uchar *Buffer,size_t Count);
extern int my_b_append(IO_CACHE *info,const uchar *Buffer,size_t Count);
extern int my_b_safe_write(IO_CACHE *info,const uchar *Buffer,size_t Count);

extern int my_block_write(IO_CACHE *info, const uchar *Buffer,
			  size_t Count, my_off_t pos);
extern int my_b_flush_io_cache(IO_CACHE *info, int need_append_buffer_lock);

#define flush_io_cache(info) my_b_flush_io_cache((info),1)

extern int end_io_cache(IO_CACHE *info);
extern size_t my_b_fill(IO_CACHE *info);
extern void my_b_seek(IO_CACHE *info,my_off_t pos);
extern size_t my_b_gets(IO_CACHE *info, char *to, size_t max_length);
extern my_off_t my_b_filelength(IO_CACHE *info);
extern size_t my_b_printf(IO_CACHE *info, const char* fmt, ...);
extern size_t my_b_vprintf(IO_CACHE *info, const char* fmt, va_list ap);
extern my_bool open_cached_file(IO_CACHE *cache,const char *dir,
				 const char *prefix, size_t cache_size,
				 myf cache_myflags);
extern my_bool real_open_cached_file(IO_CACHE *cache);
extern void close_cached_file(IO_CACHE *cache);
File create_temp_file(char *to, const char *dir, const char *pfx,
		      int mode, myf MyFlags);
#define my_init_dynamic_array(A,B,C,D) init_dynamic_array2(A,B,NULL,C,D CALLER_INFO)
#define my_init_dynamic_array_ci(A,B,C,D) init_dynamic_array2(A,B,NULL,C,D ORIG_CALLER_INFO)
#define my_init_dynamic_array2(A,B,C,D,E) init_dynamic_array2(A,B,C,D,E CALLER_INFO)
#define my_init_dynamic_array2_ci(A,B,C,D,E) init_dynamic_array2(A,B,C,D,E ORIG_CALLER_INFO)
extern my_bool init_dynamic_array2(DYNAMIC_ARRAY *array,uint element_size,
                                   void *init_buffer, uint init_alloc, 
                                   uint alloc_increment
                                   CALLER_INFO_PROTO);
/* init_dynamic_array() function is deprecated */
extern my_bool init_dynamic_array(DYNAMIC_ARRAY *array,uint element_size,
                                  uint init_alloc,uint alloc_increment
                                  CALLER_INFO_PROTO);
extern my_bool insert_dynamic(DYNAMIC_ARRAY *array,uchar * element);
extern uchar *alloc_dynamic(DYNAMIC_ARRAY *array);
extern uchar *pop_dynamic(DYNAMIC_ARRAY*);
extern my_bool set_dynamic(DYNAMIC_ARRAY *array,uchar * element,uint array_index);
extern my_bool allocate_dynamic(DYNAMIC_ARRAY *array, uint max_elements);
extern void get_dynamic(DYNAMIC_ARRAY *array,uchar * element,uint array_index);
extern void delete_dynamic(DYNAMIC_ARRAY *array);
extern void delete_dynamic_element(DYNAMIC_ARRAY *array, uint array_index);
extern void freeze_size(DYNAMIC_ARRAY *array);
extern int  get_index_dynamic(DYNAMIC_ARRAY *array, uchar * element);
#define dynamic_array_ptr(array,array_index) ((array)->buffer+(array_index)*(array)->size_of_element)
#define dynamic_element(array,array_index,type) ((type)((array)->buffer) +(array_index))
#define push_dynamic(A,B) insert_dynamic((A),(B))
#define reset_dynamic(array) ((array)->elements= 0)
#define sort_dynamic(A,cmp) my_qsort((A)->buffer, (A)->elements, (A)->size_of_element, (cmp))

extern my_bool init_dynamic_string(DYNAMIC_STRING *str, const char *init_str,
				   size_t init_alloc,size_t alloc_increment);
extern my_bool dynstr_append(DYNAMIC_STRING *str, const char *append);
my_bool dynstr_append_mem(DYNAMIC_STRING *str, const char *append,
			  size_t length);
extern my_bool dynstr_append_os_quoted(DYNAMIC_STRING *str, const char *append,
                                       ...);
extern my_bool dynstr_set(DYNAMIC_STRING *str, const char *init_str);
extern my_bool dynstr_realloc(DYNAMIC_STRING *str, size_t additional_size);
extern my_bool dynstr_trunc(DYNAMIC_STRING *str, size_t n);
extern void dynstr_free(DYNAMIC_STRING *str);
#ifdef HAVE_MLOCK
extern void *my_malloc_lock(size_t length,myf flags);
extern void my_free_lock(void *ptr,myf flags);
#else
#define my_malloc_lock(A,B) my_malloc((A),(B))
#define my_free_lock(A,B) my_free((A),(B))
#endif
#define alloc_root_inited(A) ((A)->min_malloc != 0)
#define ALLOC_ROOT_MIN_BLOCK_SIZE (MALLOC_OVERHEAD + sizeof(USED_MEM) + 8)
#define clear_alloc_root(A) do { (A)->free= (A)->used= (A)->pre_alloc= 0; (A)->min_malloc=0;} while(0)
extern void init_alloc_root(MEM_ROOT *mem_root, size_t block_size,
			    size_t pre_alloc_size);
extern void *alloc_root(MEM_ROOT *mem_root, size_t Size);
extern void *multi_alloc_root(MEM_ROOT *mem_root, ...);
extern void free_root(MEM_ROOT *root, myf MyFLAGS);
extern void set_prealloc_root(MEM_ROOT *root, char *ptr);
extern void reset_root_defaults(MEM_ROOT *mem_root, size_t block_size,
                                size_t prealloc_size);
extern char *strdup_root(MEM_ROOT *root,const char *str);
extern char *strmake_root(MEM_ROOT *root,const char *str,size_t len);
extern void *memdup_root(MEM_ROOT *root,const void *str, size_t len);
extern int get_defaults_options(int argc, char **argv,
                                char **defaults, char **extra_defaults,
                                char **group_suffix);
extern int my_load_defaults(const char *conf_file, const char **groups,
                            int *argc, char ***argv, const char ***);
extern int load_defaults(const char *conf_file, const char **groups,
                         int *argc, char ***argv);
extern int modify_defaults_file(const char *file_location, const char *option,
                                const char *option_value,
                                const char *section_name, int remove_option);
extern int my_search_option_files(const char *conf_file, int *argc,
                                  char ***argv, uint *args_used,
                                  Process_option_func func, void *func_ctx,
                                  const char **default_directories);
extern void free_defaults(char **argv);
extern void my_print_default_files(const char *conf_file);
extern void print_defaults(const char *conf_file, const char **groups);
extern my_bool my_compress(uchar *, size_t *, size_t *);
extern my_bool my_uncompress(uchar *, size_t , size_t *);
extern uchar *my_compress_alloc(const uchar *packet, size_t *len,
                                size_t *complen);
extern int packfrm(uchar *, size_t, uchar **, size_t *);
extern int unpackfrm(uchar **, size_t *, const uchar *);

extern ha_checksum my_checksum(ha_checksum crc, const uchar *mem,
                               size_t count);
extern void my_sleep(ulong m_seconds);
extern ulong crc32(ulong crc, const uchar *buf, uint len);
extern uint my_set_max_open_files(uint files);
void my_free_open_file_info(void);

extern time_t my_time(myf flags);
extern ulonglong my_getsystime(void);
extern ulonglong my_micro_time();
extern ulonglong my_micro_time_and_time(time_t *time_arg);
time_t my_time_possible_from_micro(ulonglong microtime);
extern my_bool my_gethwaddr(uchar *to);
extern int my_getncpus();

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>

#ifndef MAP_NOSYNC
#define MAP_NOSYNC      0
#endif
#ifndef MAP_NORESERVE
#define MAP_NORESERVE 0         /* For irix and AIX */
#endif

#ifdef HAVE_MMAP64
#define my_mmap(a,b,c,d,e,f)    mmap64(a,b,c,d,e,f)
#else
#define my_mmap(a,b,c,d,e,f)    mmap(a,b,c,d,e,f)
#endif
#define my_munmap(a,b)          munmap((a),(b))

#else
/* not a complete set of mmap() flags, but only those that nesessary */
#define PROT_READ        1
#define PROT_WRITE       2
#define MAP_NORESERVE    0
#define MAP_SHARED       0x0001
#define MAP_PRIVATE      0x0002
#define MAP_NOSYNC       0x0800
#define MAP_FAILED       ((void *)-1)
#define MS_SYNC          0x0000

#ifndef __NETWARE__
#define HAVE_MMAP
#endif

void *my_mmap(void *, size_t, int, int, int, my_off_t);
int my_munmap(void *, size_t);
#endif

/* my_getpagesize */
#ifdef HAVE_GETPAGESIZE
#define my_getpagesize()        getpagesize()
#else
int my_getpagesize(void);
#endif

int my_msync(int, void *, size_t, int);

/* character sets */
extern uint get_charset_number(const char *cs_name, uint cs_flags);
extern uint get_collation_number(const char *name);
extern const char *get_charset_name(uint cs_number);

extern CHARSET_INFO *get_charset(uint cs_number, myf flags);
extern CHARSET_INFO *get_charset_by_name(const char *cs_name, myf flags);
extern CHARSET_INFO *get_charset_by_csname(const char *cs_name,
					   uint cs_flags, myf my_flags);

extern my_bool resolve_charset(const char *cs_name,
                               CHARSET_INFO *default_cs,
                               CHARSET_INFO **cs);
extern my_bool resolve_collation(const char *cl_name,
                                 CHARSET_INFO *default_cl,
                                 CHARSET_INFO **cl);

extern char *get_charsets_dir(char *buf);
extern my_bool my_charset_same(CHARSET_INFO *cs1, CHARSET_INFO *cs2);
extern my_bool init_compiled_charsets(myf flags);
extern void add_compiled_collation(CHARSET_INFO *cs);
extern size_t escape_string_for_mysql(CHARSET_INFO *charset_info,
                                      char *to, size_t to_length,
                                      const char *from, size_t length);
#ifdef __WIN__
#define BACKSLASH_MBTAIL
/* File system character set */
extern CHARSET_INFO *fs_character_set(void);
#endif
extern size_t escape_quotes_for_mysql(CHARSET_INFO *charset_info,
                                      char *to, size_t to_length,
                                      const char *from, size_t length);

extern void thd_increment_bytes_sent(ulong length);
extern void thd_increment_bytes_received(ulong length);
extern void thd_increment_net_big_packet_count(ulong length);

#ifdef __WIN__
extern my_bool have_tcpip;		/* Is set if tcpip is used */

/* implemented in my_windac.c */

int my_security_attr_create(SECURITY_ATTRIBUTES **psa, const char **perror,
                            DWORD owner_rights, DWORD everybody_rights);

void my_security_attr_free(SECURITY_ATTRIBUTES *sa);

/* implemented in my_conio.c */
char* my_cgets(char *string, size_t clen, size_t* plen);

#endif
#ifdef __NETWARE__
void netware_reg_user(const char *ip, const char *user,
		      const char *application);
#endif

C_MODE_END
#endif /* _my_sys_h */
