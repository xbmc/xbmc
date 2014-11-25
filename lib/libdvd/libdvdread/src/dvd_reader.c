/*
 * Copyright (C) 2001-2004 Billy Biggs <vektor@dumbterm.net>,
 *                         Håkan Hjort <d95hjort@dtek.chalmers.se>,
 *                         Björn Englund <d4bjorn@dtek.chalmers.se>
 *
 * This file is part of libdvdread.
 *
 * libdvdread is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * libdvdread is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with libdvdread; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h> /* For the timing of dvdcss_title crack. */
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#ifndef WIN32
#include <paths.h>
#endif

#define WITH_CACHE

/* misc win32 helpers */
#ifdef WIN32
#ifndef HAVE_GETTIMEOFDAY
/* replacement gettimeofday implementation */
#include <sys/timeb.h>
static inline int _private_gettimeofday( struct timeval *tv, void *tz )
{
  struct timeb t;
  ftime( &t );
  tv->tv_sec = t.time;
  tv->tv_usec = t.millitm * 1000;
  return 0;
}
#define gettimeofday(TV, TZ) _private_gettimeofday((TV), (TZ))
#endif
#include <io.h> /* read() */
#define lseek64 _lseeki64
#endif

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || defined(__bsdi__) || defined(__APPLE__)
#define SYS_BSD 1
#endif

#if defined(__sun)
#include <sys/mnttab.h>
#elif defined(SYS_BSD)
#include <fstab.h>
#elif defined(__linux__)
#include <mntent.h>
#include <paths.h>
#endif

#include "dvdread/dvd_udf.h"
#include "dvd_input.h"
#include "dvdread/dvd_reader.h"
#include "md5.h"

#define DEFAULT_UDF_CACHE_LEVEL 1

struct dvd_reader_s {
    /* Basic information. */
    int isImageFile;

    /* Hack for keeping track of the css status.
     * 0: no css, 1: perhaps (need init of keys), 2: have done init */
    int css_state;
    int css_title; /* Last title that we have called dvdinpute_title for. */

    /* Information required for an image file. */
    dvd_input_t dev;

    /* Information required for a directory path drive. */
    char *path_root;

    /* Filesystem cache */
    int udfcache_level; /* 0 - turned off, 1 - on */
    void *udfcache;
};

#define TITLES_MAX 9

struct dvd_file_s {
    /* Basic information. */
    dvd_reader_t *dvd;

    /* Hack for selecting the right css title. */
    int css_title;

    /* Information required for an image file. */
    uint32_t lb_start;
    uint32_t seek_pos;

#ifdef WITH_CACHE
    char cache[DVD_VIDEO_LB_LEN];
    uint32_t lb_cache;
#endif
    /* Information required for a directory path drive. */
    size_t title_sizes[ TITLES_MAX ];
    dvd_input_t title_devs[ TITLES_MAX ];

    /* Calculated at open-time, size in blocks. */
    ssize_t filesize;
};

int UDFReadBlocksRaw( dvd_reader_t *device, uint32_t lb_number,
                     size_t block_count, unsigned char *data,
                     int encrypted );

/**
 * Set the level of caching on udf
 * level = 0 (no caching)
 * level = 1 (caching filesystem info)
 */
int DVDUDFCacheLevel(dvd_reader_t *device, int level)
{
  struct dvd_reader_s *dev = (struct dvd_reader_s *)device;

  if(level > 0) {
    level = 1;
  } else if(level < 0) {
    return dev->udfcache_level;
  }

  dev->udfcache_level = level;

  return level;
}

void *GetUDFCacheHandle(dvd_reader_t *device)
{
  struct dvd_reader_s *dev = (struct dvd_reader_s *)device;

  return dev->udfcache;
}

void SetUDFCacheHandle(dvd_reader_t *device, void *cache)
{
  struct dvd_reader_s *dev = (struct dvd_reader_s *)device;

  dev->udfcache = cache;
}



/* Loop over all titles and call dvdcss_title to crack the keys. */
static int initAllCSSKeys( dvd_reader_t *dvd )
{
    struct timeval all_s, all_e;
    struct timeval t_s, t_e;
    char filename[ MAX_UDF_FILE_NAME_LEN ];
    uint32_t start, len;
    int title;

    char *nokeys_str = getenv("DVDREAD_NOKEYS");
    if(nokeys_str != NULL)
      return 0;

    fprintf( stderr, "\n" );
    fprintf( stderr, "libdvdread: Attempting to retrieve all CSS keys\n" );
    fprintf( stderr, "libdvdread: This can take a _long_ time, "
	     "please be patient\n\n" );
    gettimeofday(&all_s, NULL);

    for( title = 0; title < 100; title++ ) {
	gettimeofday( &t_s, NULL );
	if( title == 0 ) {
	    sprintf( filename, "/VIDEO_TS/VIDEO_TS.VOB" );
	} else {
	    sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, 0 );
	}
	start = UDFFindFile( dvd, filename, &len );
	if( start != 0 && len != 0 ) {
	    /* Perform CSS key cracking for this title. */
	    fprintf( stderr, "libdvdread: Get key for %s at 0x%08x\n",
		     filename, start );
	    if( dvdinput_title( dvd->dev, (int)start ) < 0 ) {
		fprintf( stderr, "libdvdread: Error cracking CSS key for %s (0x%08x)\n", filename, start);
	    }
	    gettimeofday( &t_e, NULL );
	    fprintf( stderr, "libdvdread: Elapsed time %ld\n",
		     (long int) t_e.tv_sec - t_s.tv_sec );
	}

	if( title == 0 ) continue;

	gettimeofday( &t_s, NULL );
	sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, 1 );
	start = UDFFindFile( dvd, filename, &len );
	if( start == 0 || len == 0 ) break;

	/* Perform CSS key cracking for this title. */
	fprintf( stderr, "libdvdread: Get key for %s at 0x%08x\n",
		 filename, start );
	if( dvdinput_title( dvd->dev, (int)start ) < 0 ) {
	    fprintf( stderr, "libdvdread: Error cracking CSS key for %s (0x%08x)!!\n", filename, start);
	}
	gettimeofday( &t_e, NULL );
	fprintf( stderr, "libdvdread: Elapsed time %ld\n",
		 (long int) t_e.tv_sec - t_s.tv_sec );
    }
    title--;

    fprintf( stderr, "libdvdread: Found %d VTS's\n", title );
    gettimeofday(&all_e, NULL);
    fprintf( stderr, "libdvdread: Elapsed time %ld\n",
	     (long int) all_e.tv_sec - all_s.tv_sec );

    return 0;
}



/**
 * Open a DVD image or block device file.
 */
static dvd_reader_t *DVDOpenImageFile( const char *location, int have_css )
{
    dvd_reader_t *dvd;
    dvd_input_t dev;

    dev = dvdinput_open( location );
    if( !dev ) {
	fprintf( stderr, "libdvdread: Can't open %s for reading\n", location );
	return NULL;
    }

    dvd = (dvd_reader_t *) malloc( sizeof( dvd_reader_t ) );
    if( !dvd ) {
        dvdinput_close(dev);
        return NULL;
    }
    dvd->isImageFile = 1;
    dvd->dev = dev;
    dvd->path_root = NULL;

    dvd->udfcache_level = DEFAULT_UDF_CACHE_LEVEL;
    dvd->udfcache = NULL;

    if( have_css ) {
      /* Only if DVDCSS_METHOD = title, a bit if it's disc or if
     * DVDCSS_METHOD = key but region mismatch. Unfortunately we
       * don't have that information. */

      dvd->css_state = 1; /* Need key init. */
    }
    dvd->css_title = 0;

    return dvd;
}

static dvd_reader_t *DVDOpenPath( const char *path_root )
{
    dvd_reader_t *dvd;

    dvd = (dvd_reader_t *) malloc( sizeof( dvd_reader_t ) );
    if( !dvd ) return NULL;
    dvd->isImageFile = 0;
    dvd->dev = 0;
    dvd->path_root = strdup( path_root );
    if(!dvd->path_root) {
      free(dvd);
      return 0;
    }
    dvd->udfcache_level = DEFAULT_UDF_CACHE_LEVEL;
    dvd->udfcache = NULL;

    dvd->css_state = 0; /* Only used in the UDF path */
    dvd->css_title = 0; /* Only matters in the UDF path */

    return dvd;
}

#if defined(__sun)
/* /dev/rdsk/c0t6d0s0 (link to /devices/...)
   /vol/dev/rdsk/c0t6d0/??
   /vol/rdsk/<name> */
static char *sun_block2char( const char *path )
{
    char *new_path;

    /* Must contain "/dsk/" */
    if( !strstr( path, "/dsk/" ) ) return (char *) strdup( path );

    /* Replace "/dsk/" with "/rdsk/" */
    new_path = malloc( strlen(path) + 2 );
    strcpy( new_path, path );
    strcpy( strstr( new_path, "/dsk/" ), "" );
    strcat( new_path, "/rdsk/" );
    strcat( new_path, strstr( path, "/dsk/" ) + strlen( "/dsk/" ) );

    return new_path;
}
#endif

#if defined(SYS_BSD)
/* FreeBSD /dev/(r)(a)cd0c (a is for atapi), recommended to _not_ use r
   update: FreeBSD and DragonFly no longer uses the prefix so don't add it.
   OpenBSD /dev/rcd0c, it needs to be the raw device
   NetBSD  /dev/rcd0[d|c|..] d for x86, c (for non x86), perhaps others
   Darwin  /dev/rdisk0,  it needs to be the raw device
   BSD/OS  /dev/sr0c (if not mounted) or /dev/rsr0c ('c' any letter will do)
   returns a string allocated with strdup. It should be freed when no longer
   used. */
static char *bsd_block2char( const char *path )
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
  return (char *) strdup( path );
#else
    char *new_path;

    /* If it doesn't start with "/dev/" or does start with "/dev/r" exit */
  if( strncmp( path, "/dev/",  5 ) || !strncmp( path, "/dev/r", 6 ) )
      return (char *) strdup( path );

    /* Replace "/dev/" with "/dev/r" */
    new_path = malloc( strlen(path) + 2 );
    strcpy( new_path, "/dev/r" );
    strcat( new_path, path + strlen( "/dev/" ) );

    return new_path;
#endif /* __FreeBSD__ || __DragonFly__ */
}
#endif


dvd_reader_t *DVDOpen( const char *ppath )
{
    struct stat fileinfo;
  int ret, have_css, retval, cdir = -1;
	dvd_reader_t *ret_val = NULL;
    char *dev_name = NULL;
  char *path = NULL, *new_path = NULL, *path_copy = NULL;

#if defined(_WIN32) || defined(__OS2__)
	int len;
#endif

    if( ppath == NULL )
    goto DVDOpen_error;

	path = strdup(ppath);
    if( path == NULL )
    goto DVDOpen_error;

    /* Try to open libdvdcss or fall back to standard functions */
    have_css = dvdinput_setup();

#if defined(_WIN32) || defined(__OS2__)
	/* Strip off the trailing \ if it is not a drive */
	len = strlen(path);
	if ((len > 1) &&
		(path[len - 1] == '\\')  &&
		(path[len - 2] != ':'))
	{
		path[len-1] = '\0';
	}
#endif

    ret = stat( path, &fileinfo );

    if( ret < 0 ) {

        /* maybe "host:port" url? try opening it with acCeSS library */
        if( strchr(path,':') ) {
			ret_val = DVDOpenImageFile( path, have_css );
			free(path);
	        return ret_val;
        }

	/* If we can't stat the file, give up */
	fprintf( stderr, "libdvdread: Can't stat %s\n", path );
	perror("");
    goto DVDOpen_error;
    }

    /* First check if this is a block/char device or a file*/
    if( S_ISBLK( fileinfo.st_mode ) ||
	S_ISCHR( fileinfo.st_mode ) ||
	S_ISREG( fileinfo.st_mode ) ) {

	/**
	 * Block devices and regular files are assumed to be DVD-Video images.
	 */
    dvd_reader_t *dvd = NULL;
#if defined(__sun)
    dev_name = sun_block2char( path );
#elif defined(SYS_BSD)
    dev_name = bsd_block2char( path );
#else
    dev_name = strdup( path );
#endif
    dvd = DVDOpenImageFile( dev_name, have_css );
    free( dev_name );
	free(path);
    return dvd;
    } else if( S_ISDIR( fileinfo.st_mode ) ) {
	dvd_reader_t *auth_drive = 0;
#if defined(SYS_BSD)
	struct fstab* fe;
#elif defined(__sun) || defined(__linux__)
	FILE *mntfile;
#endif

	/* XXX: We should scream real loud here. */
    if( !(path_copy = strdup( path ) ) )
      goto DVDOpen_error;

#if !defined(WIN32) && !defined(_XBMC) /* don't have fchdir, and getcwd( NULL, ... ) is strange */
              /* Also WIN32 does not have symlinks, so we don't need this bit of code. */
			  /* XBMC also doesn't need symlink resolution */

    /* Resolve any symlinks and get the absolute dir name. */
	{
      if( ( cdir  = open( ".", O_RDONLY ) ) >= 0 ) {
        if( chdir( path_copy ) == -1 ) {
          goto DVDOpen_error;
        }
		new_path = malloc(PATH_MAX+1);
		if(!new_path) {
          goto DVDOpen_error;
        }
        if( getcwd( new_path, PATH_MAX ) == NULL ) {
          goto DVDOpen_error;
		}
        retval = fchdir( cdir );
		close( cdir );
        cdir = -1;
        if( retval == -1 ) {
          goto DVDOpen_error;
        }
		    path_copy = new_path;
        new_path = NULL;
	    }
	}
#endif

	/**
	 * If we're being asked to open a directory, check if that directory
	 * is the mountpoint for a DVD-ROM which we can use instead.
	 */

	if( strlen( path_copy ) > 1 ) {
      if( path_copy[ strlen( path_copy ) - 1 ] == '/' ) {
		path_copy[ strlen( path_copy ) - 1 ] = '\0';
	}
    }

#if defined(_WIN32) || defined(__OS2__)
    if(strlen(path_copy) > TITLES_MAX) {
      if(!strcasecmp(&(path_copy[strlen( path_copy ) - TITLES_MAX]),
                       "\\video_ts"))
        path_copy[strlen(path_copy) - (TITLES_MAX-1)] = '\0';
    }
#endif
	if( strlen( path_copy ) > TITLES_MAX ) {
	    if( !strcasecmp( &(path_copy[ strlen( path_copy ) - TITLES_MAX ]),
			     "/video_ts" ) ) {
	      path_copy[ strlen( path_copy ) - TITLES_MAX ] = '\0';
	    }
	}

	if(path_copy[0] == '\0') {
	    path_copy[0] = '/';
	    path_copy[1] = '\0';
	}

#if defined(SYS_BSD)
	if( ( fe = getfsfile( path_copy ) ) ) {
	    dev_name = bsd_block2char( fe->fs_spec );
	    fprintf( stderr,
		     "libdvdread: Attempting to use device %s"
		     " mounted on %s for CSS authentication\n",
		     dev_name,
		     fe->fs_file );
	    auth_drive = DVDOpenImageFile( dev_name, have_css );
	}
#elif defined(__sun)
	mntfile = fopen( MNTTAB, "r" );
	if( mntfile ) {
	    struct mnttab mp;
	    int res;

	    while( ( res = getmntent( mntfile, &mp ) ) != -1 ) {
		if( res == 0 && !strcmp( mp.mnt_mountp, path_copy ) ) {
		    dev_name = sun_block2char( mp.mnt_special );
		    fprintf( stderr,
			     "libdvdread: Attempting to use device %s"
			     " mounted on %s for CSS authentication\n",
			     dev_name,
			     mp.mnt_mountp );
		    auth_drive = DVDOpenImageFile( dev_name, have_css );
		    break;
		}
	    }
	    fclose( mntfile );
	}
#elif defined(__linux__)
        mntfile = fopen( _PATH_MOUNTED, "r" );
        if( mntfile ) {
            struct mntent *me;

            while( ( me = getmntent( mntfile ) ) ) {
                if( !strcmp( me->mnt_dir, path_copy ) ) {
		    fprintf( stderr,
			     "libdvdread: Attempting to use device %s"
			     " mounted on %s for CSS authentication\n",
			     me->mnt_fsname,
			     me->mnt_dir );
                    auth_drive = DVDOpenImageFile( me->mnt_fsname, have_css );
		    dev_name = strdup(me->mnt_fsname);
                    break;
                }
            }
            fclose( mntfile );
	}
#elif defined(_WIN32) || defined(__OS2__)
#ifdef __OS2__
    /* Use DVDOpenImageFile() only if it is a drive */
    if(isalpha(path[0]) && path[1] == ':' &&
        ( !path[2] ||
          ((path[2] == '\\' || path[2] == '/') && !path[3])))
#endif
    auth_drive = DVDOpenImageFile( path, have_css );
#endif

#if !defined(_WIN32) && !defined(__OS2__)
	if( !dev_name ) {
	  fprintf( stderr, "libdvdread: Couldn't find device name.\n" );
	} else if( !auth_drive ) {
	    fprintf( stderr, "libdvdread: Device %s inaccessible, "
		     "CSS authentication not available.\n", dev_name );
	}
#else
	if( !auth_drive ) {
	    fprintf( stderr, "libdvdread: Device %s inaccessible, "
                 "CSS authentication not available.\n", path );
	}
#endif

	free( dev_name );
    dev_name = NULL;
	free( path_copy );
    path_copy = NULL;

        /**
         * If we've opened a drive, just use that.
         */
	if( auth_drive ) {
		free(path);
		return auth_drive;
	}
        /**
         * Otherwise, we now try to open the directory tree instead.
         */
        ret_val = DVDOpenPath( path );
		free( path );
		return ret_val;
    }

DVDOpen_error:
    /* If it's none of the above, screw it. */
    fprintf( stderr, "libdvdread: Could not open %s\n", path );
  if( path != NULL )
	free( path );
  if ( path_copy != NULL )
    free( path_copy );
  if ( cdir >= 0 )
    close( cdir );
  if ( new_path != NULL )
    free( new_path );
    return NULL;
}

void DVDClose( dvd_reader_t *dvd )
{
    if( dvd ) {
        if( dvd->dev ) dvdinput_close( dvd->dev );
        if( dvd->path_root ) free( dvd->path_root );
	if( dvd->udfcache ) FreeUDFCache( dvd->udfcache );
        free( dvd );
    }
}

/**
 * Open an unencrypted file on a DVD image file.
 */
static dvd_file_t *DVDOpenFileUDF( dvd_reader_t *dvd, char *filename )
{
    uint32_t start, len;
    dvd_file_t *dvd_file;

    start = UDFFindFile( dvd, filename, &len );
    if( !start ) {
      fprintf( stderr, "libdvdnav:DVDOpenFileUDF:UDFFindFile %s failed\n", filename );
      return NULL;
    }

    dvd_file = (dvd_file_t *) malloc( sizeof( dvd_file_t ) );
    if( !dvd_file ) {
      fprintf( stderr, "libdvdnav:DVDOpenFileUDF:malloc failed\n" );
      return NULL;
    }
    dvd_file->dvd = dvd;
    dvd_file->lb_start = start;
    dvd_file->seek_pos = 0;
    memset( dvd_file->title_sizes, 0, sizeof( dvd_file->title_sizes ) );
    memset( dvd_file->title_devs, 0, sizeof( dvd_file->title_devs ) );
    dvd_file->filesize = len / DVD_VIDEO_LB_LEN;

#ifdef WITH_CACHE
    dvd_file->lb_cache = -1;
#endif

    return dvd_file;
}

/**
 * Searches for <file> in directory <path>, ignoring case.
 * Returns 0 and full filename in <filename>.
 *     or -1 on file not found.
 *     or -2 on path not found.
 */
static int findDirFile( const char *path, const char *file, char *filename )
{
#if defined(_XBMC)
	struct stat fileinfo;

	// no emulated opendir function in xbmc, so we'll
	// check if the file exists by stat'ing it ...
	sprintf(filename, "%s%s%s", path,
                     ( ( path[ strlen( path ) - 1 ] == '/' ) ? "" : "/" ),
                     file );

	if (stat(filename, &fileinfo) == 0) return 0;
	
#else
    DIR *dir;
    struct dirent *ent;

    dir = opendir( path );
    if( !dir ) return -2;

    while( ( ent = readdir( dir ) ) != NULL ) {
        if( !strcasecmp( ent->d_name, file ) ) {
            sprintf( filename, "%s%s%s", path,
                     ( ( path[ strlen( path ) - 1 ] == '/' ) ? "" : "/" ),
                     ent->d_name );
            closedir( dir );
            return 0;
        }
    }
    closedir( dir );
#endif // _XBMC
    return -1;
}

static int findDVDFile( dvd_reader_t *dvd, const char *file, char *filename )
{
    char video_path[ PATH_MAX + 1 ];
    const char *nodirfile;
    int ret;

    /* Strip off the directory for our search */
    if( !strncasecmp( "/VIDEO_TS/", file, 10 ) ) {
        nodirfile = &(file[ 10 ]);
    } else {
        nodirfile = file;
    }

    ret = findDirFile( dvd->path_root, nodirfile, filename );
    if( ret < 0 ) {
        /* Try also with adding the path, just in case. */
        sprintf( video_path, "%s/VIDEO_TS/", dvd->path_root );
        ret = findDirFile( video_path, nodirfile, filename );
        if( ret < 0 ) {
            /* Try with the path, but in lower case. */
            sprintf( video_path, "%s/video_ts/", dvd->path_root );
            ret = findDirFile( video_path, nodirfile, filename );
            if( ret < 0 ) {
                return 0;
            }
        }
    }

    return 1;
}

/**
 * Open an unencrypted file from a DVD directory tree.
 */
static dvd_file_t *DVDOpenFilePath( dvd_reader_t *dvd, char *filename )
{
    char full_path[ PATH_MAX + 1 ];
    dvd_file_t *dvd_file;
    struct stat fileinfo;
    dvd_input_t dev;

    /* Get the full path of the file. */
    if( !findDVDFile( dvd, filename, full_path ) ) {
      fprintf( stderr, "libdvdnav:DVDOpenFilePath:findDVDFile %s failed\n", filename );
      return NULL;
    }

    dev = dvdinput_open( full_path );
    if( !dev ) {
      fprintf( stderr, "libdvdnav:DVDOpenFilePath:dvdinput_open %s failed\n", full_path );
      return NULL;
    }

    dvd_file = (dvd_file_t *) malloc( sizeof( dvd_file_t ) );
    if( !dvd_file ) {
      fprintf( stderr, "libdvdnav:DVDOpenFilePath:dvd_file malloc failed\n" );
      dvdinput_close(dev);
      return NULL;
    }
    dvd_file->dvd = dvd;
    dvd_file->lb_start = 0;
    dvd_file->seek_pos = 0;
    memset( dvd_file->title_sizes, 0, sizeof( dvd_file->title_sizes ) );
    memset( dvd_file->title_devs, 0, sizeof( dvd_file->title_devs ) );
    dvd_file->filesize = 0;

    if( stat( full_path, &fileinfo ) < 0 ) {
        fprintf( stderr, "libdvdread: Can't stat() %s.\n", filename );
        free( dvd_file );
        return NULL;
    }
    dvd_file->title_sizes[ 0 ] = fileinfo.st_size / DVD_VIDEO_LB_LEN;
    dvd_file->title_devs[ 0 ] = dev;
    dvd_file->filesize = dvd_file->title_sizes[ 0 ];

#ifdef WITH_CACHE
    dvd_file->lb_cache = -1;
#endif
    return dvd_file;
}

static dvd_file_t *DVDOpenVOBUDF( dvd_reader_t *dvd, int title, int menu )
{
    char filename[ MAX_UDF_FILE_NAME_LEN ];
    uint32_t start, len;
    dvd_file_t *dvd_file;

    if( title == 0 ) {
        sprintf( filename, "/VIDEO_TS/VIDEO_TS.VOB" );
    } else {
        sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, menu ? 0 : 1 );
    }
    start = UDFFindFile( dvd, filename, &len );
    if( start == 0 ) return NULL;

    dvd_file = (dvd_file_t *) malloc( sizeof( dvd_file_t ) );
    if( !dvd_file ) return NULL;
    dvd_file->dvd = dvd;
    /*Hack*/ dvd_file->css_title = title << 1 | menu;
    dvd_file->lb_start = start;
    dvd_file->seek_pos = 0;
    memset( dvd_file->title_sizes, 0, sizeof( dvd_file->title_sizes ) );
    memset( dvd_file->title_devs, 0, sizeof( dvd_file->title_devs ) );
    dvd_file->filesize = len / DVD_VIDEO_LB_LEN;

#ifdef WITH_CACHE
    dvd_file->lb_cache = -1;
#endif
    /* Calculate the complete file size for every file in the VOBS */
    if( !menu ) {
        int cur;

        for( cur = 2; cur < 10; cur++ ) {
            sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, cur );
            if( !UDFFindFile( dvd, filename, &len ) ) break;
            dvd_file->filesize += len / DVD_VIDEO_LB_LEN;
        }
    }

    if( dvd->css_state == 1 /* Need key init */ ) {
        initAllCSSKeys( dvd );
	dvd->css_state = 2;
    }
    /*
    if( dvdinput_title( dvd_file->dvd->dev, (int)start ) < 0 ) {
        fprintf( stderr, "libdvdread: Error cracking CSS key for %s\n",
		 filename );
    }
    */

    return dvd_file;
}

static dvd_file_t *DVDOpenVOBPath( dvd_reader_t *dvd, int title, int menu )
{
    char filename[ MAX_UDF_FILE_NAME_LEN ];
    char full_path[ PATH_MAX + 1 ];
    struct stat fileinfo;
    dvd_file_t *dvd_file;
    int i;

    dvd_file = (dvd_file_t *) malloc( sizeof( dvd_file_t ) );
    if( !dvd_file ) return NULL;
    dvd_file->dvd = dvd;
    /*Hack*/ dvd_file->css_title = title << 1 | menu;
    dvd_file->lb_start = 0;
    dvd_file->seek_pos = 0;
    memset( dvd_file->title_sizes, 0, sizeof( dvd_file->title_sizes ) );
    memset( dvd_file->title_devs, 0, sizeof( dvd_file->title_devs ) );
    dvd_file->filesize = 0;

#ifdef WITH_CACHE
    dvd_file->lb_cache = -1;
#endif

    if( menu ) {
        dvd_input_t dev;

        if( title == 0 ) {
            sprintf( filename, "VIDEO_TS.VOB" );
        } else {
            sprintf( filename, "VTS_%02i_0.VOB", title );
        }
        if( !findDVDFile( dvd, filename, full_path ) ) {
            free( dvd_file );
            return NULL;
        }

        dev = dvdinput_open( full_path );
        if( dev == NULL ) {
            free( dvd_file );
            return NULL;
        }

        if( stat( full_path, &fileinfo ) < 0 ) {
            fprintf( stderr, "libdvdread: Can't stat() %s.\n", filename );
            dvdinput_close(dev);
            free( dvd_file );
            return NULL;
        }
        dvd_file->title_sizes[ 0 ] = fileinfo.st_size / DVD_VIDEO_LB_LEN;
        dvd_file->title_devs[ 0 ] = dev;
	dvdinput_title( dvd_file->title_devs[0], 0);
        dvd_file->filesize = dvd_file->title_sizes[ 0 ];

    } else {
        for( i = 0; i < TITLES_MAX; ++i ) {

            sprintf( filename, "VTS_%02i_%i.VOB", title, i + 1 );
            if( !findDVDFile( dvd, filename, full_path ) ) {
                break;
            }

            if( stat( full_path, &fileinfo ) < 0 ) {
                fprintf( stderr, "libdvdread: Can't stat() %s.\n", filename );
                break;
            }

            dvd_file->title_sizes[ i ] = fileinfo.st_size / DVD_VIDEO_LB_LEN;
            dvd_file->title_devs[ i ] = dvdinput_open( full_path );
	    dvdinput_title( dvd_file->title_devs[ i ], 0 );
            dvd_file->filesize += dvd_file->title_sizes[ i ];
        }
        if( !dvd_file->title_devs[ 0 ] ) {
            free( dvd_file );
            return NULL;
        }
    }

    return dvd_file;
}

dvd_file_t *DVDOpenFile( dvd_reader_t *dvd, int titlenum,
			 dvd_read_domain_t domain )
{
    char filename[ MAX_UDF_FILE_NAME_LEN ];

    /* Check arguments. */
    if( dvd == NULL || titlenum < 0 )
      return NULL;

    switch( domain ) {
    case DVD_READ_INFO_FILE:
        if( titlenum == 0 ) {
            sprintf( filename, "/VIDEO_TS/VIDEO_TS.IFO" );
        } else {
            sprintf( filename, "/VIDEO_TS/VTS_%02i_0.IFO", titlenum );
        }
        break;
    case DVD_READ_INFO_BACKUP_FILE:
        if( titlenum == 0 ) {
            sprintf( filename, "/VIDEO_TS/VIDEO_TS.BUP" );
        } else {
            sprintf( filename, "/VIDEO_TS/VTS_%02i_0.BUP", titlenum );
        }
        break;
    case DVD_READ_MENU_VOBS:
        if( dvd->isImageFile ) {
            return DVDOpenVOBUDF( dvd, titlenum, 1 );
        } else {
            return DVDOpenVOBPath( dvd, titlenum, 1 );
        }
        break;
    case DVD_READ_TITLE_VOBS:
        if( titlenum == 0 ) return 0;
        if( dvd->isImageFile ) {
            return DVDOpenVOBUDF( dvd, titlenum, 0 );
        } else {
            return DVDOpenVOBPath( dvd, titlenum, 0 );
        }
        break;
    default:
        fprintf( stderr, "libdvdread: Invalid domain for file open.\n" );
        return NULL;
    }

    if( dvd->isImageFile ) {
        return DVDOpenFileUDF( dvd, filename );
    } else {
        return DVDOpenFilePath( dvd, filename );
    }
}

void DVDCloseFile( dvd_file_t *dvd_file )
{
    int i;

    if( dvd_file ) {
    if( !dvd_file->dvd->isImageFile ) {
            for( i = 0; i < TITLES_MAX; ++i ) {
                if( dvd_file->title_devs[ i ] ) {
                    dvdinput_close( dvd_file->title_devs[i] );
                }
            }
        }

        free( dvd_file );
        dvd_file = 0;
    }
}

static int DVDFileStatVOBUDF( dvd_reader_t *dvd, int title,
                              int menu, dvd_stat_t *statbuf )
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  uint32_t size;
  off_t tot_size;
  off_t parts_size[ 9 ];
  int nr_parts = 0;
  int n;

  if( title == 0 )
    sprintf( filename, "/VIDEO_TS/VIDEO_TS.VOB" );
  else
    sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, menu ? 0 : 1 );

  if( !UDFFindFile( dvd, filename, &size ) )
    return -1;

  tot_size = size;
  nr_parts = 1;
  parts_size[ 0 ] = size;

  if( !menu ) {
    int cur;

    for( cur = 2; cur < 10; cur++ ) {
      sprintf( filename, "/VIDEO_TS/VTS_%02d_%d.VOB", title, cur );
      if( !UDFFindFile( dvd, filename, &size ) )
        break;

      parts_size[ nr_parts ] = size;
      tot_size += size;
      nr_parts++;
    }
  }

  statbuf->size = tot_size;
  statbuf->nr_parts = nr_parts;
  for( n = 0; n < nr_parts; n++ )
    statbuf->parts_size[ n ] = parts_size[ n ];

  return 0;
}


static int DVDFileStatVOBPath( dvd_reader_t *dvd, int title,
                               int menu, dvd_stat_t *statbuf )
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  char full_path[ PATH_MAX + 1 ];
  struct stat fileinfo;
  off_t tot_size;
  off_t parts_size[ 9 ];
  int nr_parts = 0;
  int n;

  if( title == 0 )
    sprintf( filename, "VIDEO_TS.VOB" );
  else
    sprintf( filename, "VTS_%02d_%d.VOB", title, menu ? 0 : 1 );

  if( !findDVDFile( dvd, filename, full_path ) )
    return -1;

  if( stat( full_path, &fileinfo ) < 0 ) {
    fprintf( stderr, "libdvdread: Can't stat() %s.\n", filename );
    return -1;
  }

  tot_size = fileinfo.st_size;
  nr_parts = 1;
  parts_size[ 0 ] = fileinfo.st_size;

  if( !menu ) {
    int cur;
    for( cur = 2; cur < 10; cur++ ) {
      sprintf( filename, "VTS_%02d_%d.VOB", title, cur );
      if( !findDVDFile( dvd, filename, full_path ) )
        break;

      if( stat( full_path, &fileinfo ) < 0 ) {
        fprintf( stderr, "libdvdread: Can't stat() %s.\n", filename );
        break;
      }

      parts_size[ nr_parts ] = fileinfo.st_size;
      tot_size += parts_size[ nr_parts ];
      nr_parts++;
    }
  }

  statbuf->size = tot_size;
  statbuf->nr_parts = nr_parts;
  for( n = 0; n < nr_parts; n++ )
    statbuf->parts_size[ n ] = parts_size[ n ];

  return 0;
}


int DVDFileStat( dvd_reader_t *dvd, int titlenum,
                 dvd_read_domain_t domain, dvd_stat_t *statbuf )
{
  char filename[ MAX_UDF_FILE_NAME_LEN ];
  char full_path[ PATH_MAX + 1 ];
  struct stat fileinfo;
  uint32_t size;

  /* Check arguments. */
  if( dvd == NULL || titlenum < 0 ) {
    errno = EINVAL;
    return -1;
  }

  switch( domain ) {
  case DVD_READ_INFO_FILE:
    if( titlenum == 0 )
      sprintf( filename, "/VIDEO_TS/VIDEO_TS.IFO" );
    else
      sprintf( filename, "/VIDEO_TS/VTS_%02i_0.IFO", titlenum );

    break;
  case DVD_READ_INFO_BACKUP_FILE:
    if( titlenum == 0 )
      sprintf( filename, "/VIDEO_TS/VIDEO_TS.BUP" );
    else
      sprintf( filename, "/VIDEO_TS/VTS_%02i_0.BUP", titlenum );

    break;
  case DVD_READ_MENU_VOBS:
    if( dvd->isImageFile )
      return DVDFileStatVOBUDF( dvd, titlenum, 1, statbuf );
    else
      return DVDFileStatVOBPath( dvd, titlenum, 1, statbuf );

    break;
  case DVD_READ_TITLE_VOBS:
    if( titlenum == 0 )
      return -1;

    if( dvd->isImageFile )
      return DVDFileStatVOBUDF( dvd, titlenum, 0, statbuf );
    else
      return DVDFileStatVOBPath( dvd, titlenum, 0, statbuf );

    break;
  default:
    fprintf( stderr, "libdvdread: Invalid domain for file stat.\n" );
    errno = EINVAL;
    return -1;
  }

  if( dvd->isImageFile ) {
    if( UDFFindFile( dvd, filename, &size ) ) {
      statbuf->size = size;
      statbuf->nr_parts = 1;
      statbuf->parts_size[ 0 ] = size;
      return 0;
    }
  } else {
    if( findDVDFile( dvd, filename, full_path ) ) {
      if( stat( full_path, &fileinfo ) < 0 )
        fprintf( stderr, "libdvdread: Can't stat() %s.\n", filename );
      else {
        statbuf->size = fileinfo.st_size;
        statbuf->nr_parts = 1;
        statbuf->parts_size[ 0 ] = statbuf->size;
        return 0;
      }
    }
  }
  return -1;
}

/* Internal, but used from dvd_udf.c */
int UDFReadBlocksRaw( dvd_reader_t *device, uint32_t lb_number,
			 size_t block_count, unsigned char *data,
			 int encrypted )
{
   int ret;

   if( !device->dev ) {
     	fprintf( stderr, "libdvdread: Fatal error in block read.\n" );
	return 0;
   }

   ret = dvdinput_seek( device->dev, (int) lb_number );
   if( ret != (int) lb_number ) {
     	fprintf( stderr, "libdvdread: Can't seek to block %u\n", lb_number );
	return 0;
   }

   ret = dvdinput_read( device->dev, (char *) data,
			 (int) block_count, encrypted );
   return ret;
}

/* This is using a single input and starting from 'dvd_file->lb_start' offset.
 *
 * Reads 'block_count' blocks from 'dvd_file' at block offset 'offset'
 * into the buffer located at 'data' and if 'encrypted' is set
 * descramble the data if it's encrypted.  Returning either an
 * negative error or the number of blocks read. */
static int DVDReadBlocksUDF( dvd_file_t *dvd_file, uint32_t offset,
			     size_t block_count, unsigned char *data,
			     int encrypted )
{
    return UDFReadBlocksRaw( dvd_file->dvd, dvd_file->lb_start + offset,
			     block_count, data, encrypted );
}

/* This is using possibly several inputs and starting from an offset of '0'.
 *
 * Reads 'block_count' blocks from 'dvd_file' at block offset 'offset'
 * into the buffer located at 'data' and if 'encrypted' is set
 * descramble the data if it's encrypted.  Returning either an
 * negative error or the number of blocks read. */
static int DVDReadBlocksPath( dvd_file_t *dvd_file, unsigned int offset,
			      size_t block_count, unsigned char *data,
			      int encrypted )
{
    int i;
    int ret, ret2, off;

    ret = 0;
    ret2 = 0;
    for( i = 0; i < TITLES_MAX; ++i ) {
      if( !dvd_file->title_sizes[ i ] ) return 0; /* Past end of file */

        if( offset < dvd_file->title_sizes[ i ] ) {
            if( ( offset + block_count ) <= dvd_file->title_sizes[ i ] ) {
		off = dvdinput_seek( dvd_file->title_devs[ i ], (int)offset );
                if( off < 0 || off != (int)offset ) {
		    fprintf( stderr, "libdvdread: Can't seek to block %d\n",
			     offset );
		    return off < 0 ? off : 0;
		}
                ret = dvdinput_read( dvd_file->title_devs[ i ], data,
				     (int)block_count, encrypted );
                break;
            } else {
                size_t part1_size = dvd_file->title_sizes[ i ] - offset;
		/* FIXME: Really needs to be a while loop.
                 * (This is only true if you try and read >1GB at a time) */

                /* Read part 1 */
                off = dvdinput_seek( dvd_file->title_devs[ i ], (int)offset );
                if( off < 0 || off != (int)offset ) {
		    fprintf( stderr, "libdvdread: Can't seek to block %d\n",
			     offset );
		    return off < 0 ? off : 0;
		}
                ret = dvdinput_read( dvd_file->title_devs[ i ], data,
				     (int)part1_size, encrypted );
		if( ret < 0 ) return ret;
		/* FIXME: This is wrong if i is the last file in the set.
                 * also error from this read will not show in ret. */

		/* Does the next part exist? If not then return now. */
		if( i + 1 >= TITLES_MAX || !dvd_file->title_devs[ i + 1 ] )
                    return ret;

                /* Read part 2 */
                off = dvdinput_seek( dvd_file->title_devs[ i + 1 ], 0 );
                if( off < 0 || off != 0 ) {
		    fprintf( stderr, "libdvdread: Can't seek to block %d\n",
			     0 );
		    return off < 0 ? off : 0;
		}
                ret2 = dvdinput_read( dvd_file->title_devs[ i + 1 ],
				      data + ( part1_size
					       * (int64_t)DVD_VIDEO_LB_LEN ),
				      (int)(block_count - part1_size),
				      encrypted );
                if( ret2 < 0 ) return ret2;
		break;
            }
        } else {
            offset -= dvd_file->title_sizes[ i ];
        }
    }

    return ret + ret2;
}

#ifdef WITH_CACHE

/* returns true aslong as the sector isn't all zeros */
int DVDCheckSector(unsigned char *data, int offset)
{
  int i = 0;
  int32_t *p = (int32_t*)data + (DVD_VIDEO_LB_LEN>>2)*offset;
  for(;i<(DVD_VIDEO_LB_LEN>>2);i++) {
    if(*(p+i) != 0)
      break;
  }
  return (i!=(DVD_VIDEO_LB_LEN>>2));
}

int DVDReadBlocksCached( dvd_file_t *dvd_file, int offset, 
		       size_t block_count, unsigned char *data, int encrypted )
{
    int ret=0;
    /* Check arguments. */
    if( dvd_file == NULL || offset < 0 || data == NULL )
      return -1;

    if(encrypted & DVDINPUT_READ_DECRYPT) {
      /* Hack, and it will still fail for multiple opens in a threaded app ! */
      if( dvd_file->dvd->css_title != dvd_file->css_title ) {
        dvd_file->dvd->css_title = dvd_file->css_title;
        if( dvd_file->dvd->isImageFile ) {
	  dvdinput_title( dvd_file->dvd->dev, (int)dvd_file->lb_start );
        } 
        /* Here each vobu has it's own dvdcss handle, so no need to update 
        else {
	  dvdinput_title( dvd_file->title_devs[ 0 ], (int)dvd_file->lb_start );
        }*/
      }
    }

    /* check if first sector is in cache */
    int cachehit = 0;
    if( offset == dvd_file->lb_cache ) {
      memcpy( data, dvd_file->cache, DVD_VIDEO_LB_LEN );
      block_count--;
      offset++;
      data+=DVD_VIDEO_LB_LEN;
      cachehit = 1;
    }


    if( block_count > 0 )
    {
      if( dvd_file->dvd->isImageFile )
	        ret = DVDReadBlocksUDF( dvd_file, (uint32_t)offset, 
				            block_count, data, encrypted );
      else
	        ret = DVDReadBlocksPath( dvd_file, (unsigned int)offset, 
				            block_count, data, encrypted );      

      if(ret<0)
        return ret;

      /* here is a hack for drive wich don't handle layerchange properly */
      /* they start returning zero data while laser is shifting position */
      /* normally just doing a reread will get the correct data */
      if( dvd_file->dvd->isImageFile )
      {
        /* check sectors from the back */
        int count = ret; /* previous call could have returned fewer than requested */
        int i = count-1;
        for(;i>=0;i--)
          if(!DVDCheckSector(data, i)) break;

        if(i>=0) {
          fprintf( stderr, "libdvdread: potential layer change. %d zero sectors detected starting at %d!\n", i+1, offset);

          /* reread the invalid sectors */
          count = DVDReadBlocksUDF( dvd_file, (uint32_t)offset+i,
				            count-i, data+DVD_VIDEO_LB_LEN*i, encrypted );

          if(count<0)
            return count;
        }
      }

    }
    
    if(ret>0)
    { /* store last sector read into cache */
      dvd_file->lb_cache = offset+ret-1;
      memcpy( dvd_file->cache, data+(DVD_VIDEO_LB_LEN*(ret-1)), DVD_VIDEO_LB_LEN );
    }
    
    return (ssize_t)(ret+cachehit);
}
#endif

/* This is broken reading more than 2Gb at a time is ssize_t is 32-bit. */
ssize_t DVDReadBlocks( dvd_file_t *dvd_file, int offset,
		       size_t block_count, unsigned char *data )
{
    int ret;

    /* Check arguments. */
    if( dvd_file == NULL || offset < 0 || data == NULL )
      return -1;

#ifdef WITH_CACHE
    return (ssize_t)DVDReadBlocksCached( dvd_file, offset, block_count, data, DVDINPUT_READ_DECRYPT );
#endif

    /* Hack, and it will still fail for multiple opens in a threaded app ! */
    if( dvd_file->dvd->css_title != dvd_file->css_title ) {
      dvd_file->dvd->css_title = dvd_file->css_title;
      if( dvd_file->dvd->isImageFile ) {
	dvdinput_title( dvd_file->dvd->dev, (int)dvd_file->lb_start );
      }
      /* Here each vobu has it's own dvdcss handle, so no need to update
      else {
	dvdinput_title( dvd_file->title_devs[ 0 ], (int)dvd_file->lb_start );
      }*/
    }

    if( dvd_file->dvd->isImageFile ) {
	ret = DVDReadBlocksUDF( dvd_file, (uint32_t)offset,
				block_count, data, DVDINPUT_READ_DECRYPT );
    } else {
	ret = DVDReadBlocksPath( dvd_file, (unsigned int)offset,
				 block_count, data, DVDINPUT_READ_DECRYPT );
    }

    return (ssize_t)ret;
}

int32_t DVDFileSeek( dvd_file_t *dvd_file, int32_t offset )
{
    /* Check arguments. */
    if( dvd_file == NULL || offset < 0 )
       return -1;

    if( offset > dvd_file->filesize * DVD_VIDEO_LB_LEN ) {
       return -1;
    }
    dvd_file->seek_pos = (uint32_t) offset;
    return offset;
}

int DVDFileSeekForce(dvd_file_t *dvd_file, int offset, int force_size)
{
    /* Check arguments. */
    if( dvd_file == NULL || offset <= 0 )
        return -1;

    if( dvd_file->dvd->isImageFile ) {
        if( force_size < 0 )
            force_size = (offset - 1) / DVD_VIDEO_LB_LEN + 1;
        if( dvd_file->filesize < force_size ) {
            dvd_file->filesize = force_size;
            fprintf(stderr, "libdvdread: Ignored size of file indicated in UDF.\n");
        }
    }

    if( offset > dvd_file->filesize * DVD_VIDEO_LB_LEN )
        return -1;

    dvd_file->seek_pos = (uint32_t) offset;
    return offset;
}

ssize_t DVDReadBytes( dvd_file_t *dvd_file, void *data, size_t byte_size )
{
    unsigned char *secbuf_base, *secbuf;
    unsigned int numsec, seek_sector, seek_byte;
    int ret;

    /* Check arguments. */
    if( dvd_file == NULL || data == NULL )
      return -1;

    seek_sector = dvd_file->seek_pos / DVD_VIDEO_LB_LEN;
    seek_byte   = dvd_file->seek_pos % DVD_VIDEO_LB_LEN;

    numsec = ( ( seek_byte + byte_size ) / DVD_VIDEO_LB_LEN ) +
      ( ( ( seek_byte + byte_size ) % DVD_VIDEO_LB_LEN ) ? 1 : 0 );

    secbuf_base = (unsigned char *) malloc( numsec * DVD_VIDEO_LB_LEN + 2048 );
    secbuf = (unsigned char *)(((uintptr_t)secbuf_base & ~((uintptr_t)2047)) + 2048);
    if( !secbuf_base ) {
	fprintf( stderr, "libdvdread: Can't allocate memory "
		 "for file read!\n" );
        return 0;
    }

#ifdef WITH_CACHE
	ret = DVDReadBlocksCached( dvd_file, (uint32_t) seek_sector, 
				(size_t) numsec, secbuf, DVDINPUT_NOFLAGS );
#else
    if( dvd_file->dvd->isImageFile ) {
	ret = DVDReadBlocksUDF( dvd_file, (uint32_t) seek_sector,
				(size_t) numsec, secbuf, DVDINPUT_NOFLAGS );
    } else {
	ret = DVDReadBlocksPath( dvd_file, seek_sector,
				 (size_t) numsec, secbuf, DVDINPUT_NOFLAGS );
    }
#endif

    if( ret != (int) numsec ) {
        free( secbuf_base );
        return ret < 0 ? ret : 0;
    }

    memcpy( data, &(secbuf[ seek_byte ]), byte_size );
    free( secbuf_base );

    DVDFileSeekForce(dvd_file, dvd_file->seek_pos + byte_size, -1);
    return byte_size;
}

ssize_t DVDFileSize( dvd_file_t *dvd_file )
{
    /* Check arguments. */
    if( dvd_file == NULL )
      return -1;

    return dvd_file->filesize;
}

int DVDDiscID( dvd_reader_t *dvd, unsigned char *discid )
{
    struct md5_ctx ctx;
    int title;
    int nr_of_files = 0;

    /* Check arguments. */
    if( dvd == NULL || discid == NULL )
      return 0;

    /* Go through the first 10 IFO:s, in order,
     * and md5sum them, i.e  VIDEO_TS.IFO and VTS_0?_0.IFO */
    md5_init_ctx( &ctx );
    for( title = 0; title < 10; title++ ) {
	dvd_file_t *dvd_file = DVDOpenFile( dvd, title, DVD_READ_INFO_FILE );
	if( dvd_file != NULL ) {
	    ssize_t bytes_read;
	    size_t file_size = dvd_file->filesize * DVD_VIDEO_LB_LEN;
	    char *buffer_base = malloc( file_size + 2048 );
	    char *buffer = (char *)(((uintptr_t)buffer_base & ~((uintptr_t)2047)) + 2048);

	    if( buffer_base == NULL ) {
	        DVDCloseFile( dvd_file );
		fprintf( stderr, "libdvdread: DVDDiscId, failed to "
			 "allocate memory for file read!\n" );
		return -1;
	    }

	    bytes_read = DVDReadBytes( dvd_file, buffer, file_size );
	    if( bytes_read != file_size ) {
		fprintf( stderr, "libdvdread: DVDDiscId read returned %zd bytes"
			 ", wanted %zd\n", bytes_read, file_size );
		DVDCloseFile( dvd_file );
		free( buffer_base );
		return -1;
	    }

	    md5_process_bytes( buffer, file_size,  &ctx );

	    DVDCloseFile( dvd_file );
	    free( buffer_base );
	    nr_of_files++;
	}
    }
    md5_finish_ctx( &ctx, discid );
    if(!nr_of_files)
      return -1;

    return 0;
}


int DVDISOVolumeInfo( dvd_reader_t *dvd,
		      char *volid, unsigned int volid_size,
		      unsigned char *volsetid, unsigned int volsetid_size )
{
  unsigned char *buffer, *buffer_base;
  int ret;

  /* Check arguments. */
  if( dvd == NULL )
    return 0;

  if( dvd->dev == NULL ) {
    /* No block access, so no ISO... */
    return -1;
  }

  buffer_base = malloc( DVD_VIDEO_LB_LEN + 2048 );
  buffer = (unsigned char *)(((uintptr_t)buffer_base & ~((uintptr_t)2047)) + 2048);

  if( buffer_base == NULL ) {
    fprintf( stderr, "libdvdread: DVDISOVolumeInfo, failed to "
	     "allocate memory for file read!\n" );
    return -1;
  }

  ret = UDFReadBlocksRaw( dvd, 16, 1, buffer, 0 );
  if( ret != 1 ) {
    fprintf( stderr, "libdvdread: DVDISOVolumeInfo, failed to "
	     "read ISO9660 Primary Volume Descriptor!\n" );
    free( buffer_base );
    return -1;
  }

  if( (volid != NULL) && (volid_size > 0) ) {
    unsigned int n;
    for(n = 0; n < 32; n++) {
      if(buffer[40+n] == 0x20) {
	break;
      }
    }

    if(volid_size > n+1) {
      volid_size = n+1;
    }

    memcpy(volid, &buffer[40], volid_size-1);
    volid[volid_size-1] = '\0';
  }

  if( (volsetid != NULL) && (volsetid_size > 0) ) {
    if(volsetid_size > 128) {
      volsetid_size = 128;
    }
    memcpy(volsetid, &buffer[190], volsetid_size);
  }
  free( buffer_base );
  return 0;
}


int DVDUDFVolumeInfo( dvd_reader_t *dvd,
		      char *volid, unsigned int volid_size,
		      unsigned char *volsetid, unsigned int volsetid_size )
{
  int ret;
  /* Check arguments. */
  if( dvd == NULL )
    return -1;

  if( dvd->dev == NULL ) {
    /* No block access, so no UDF VolumeSet Identifier */
    return -1;
  }

  if( (volid != NULL) && (volid_size > 0) ) {
    ret = UDFGetVolumeIdentifier(dvd, volid, volid_size);
    if(!ret) {
      return -1;
    }
  }
  if( (volsetid != NULL) && (volsetid_size > 0) ) {
    ret =  UDFGetVolumeSetIdentifier(dvd, volsetid, volsetid_size);
    if(!ret) {
      return -1;
    }
  }

  return 0;
}
