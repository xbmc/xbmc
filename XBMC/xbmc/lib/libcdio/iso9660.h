/*
    $Id$

    Copyright (C) 2000 Herbert Valerio Riedel <hvr@gnu.org>
    Copyright (C) 2003, 2004 Rocky Bernstein <rocky@panix.com>

    See also iso9660.h by Eric Youngdale (1993).

    Copyright 1993 Yggdrasil Computing, Incorporated
    Copyright (c) 1999,2000 J. Schilling

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
/*!
 * \file iso9660.h 
 * \brief Header for libiso9660: the ISO-9660 filesystem library.
*/


#ifndef __CDIO_ISO9660_H__
#define __CDIO_ISO9660_H__

#include <cdio/cdio.h>
#include <cdio/xa.h>

#include <time.h>

#define	_delta(from, to)	((to) - (from) + 1)

#define MIN_TRACK_SIZE 4*75
#define MIN_ISO_SIZE MIN_TRACK_SIZE

/*!
   An ISO filename is: "abcde.eee;1" -> <filename> '.' <ext> ';' <version #>

    For ISO-9660 Level 1, the maximum needed string length is:

\verbatim  
  	 30 chars (filename + ext)
    +	  2 chars ('.' + ';')
    +	  5 chars (strlen("32767"))
    +	  1 null byte
   ================================
    =	 38 chars
\endverbatim 
*/
#define LEN_ISONAME     31
#define MAX_ISONAME     37

#define MAX_ISOPATHNAME 255

/*! \def Max # characters in an perparer id. */
#define ISO_MAX_PREPARER_ID 128

/*! \def Max # characters in an publisher id. */
#define ISO_MAX_PUBLISHER_ID 128

/*! \def Max # characters in an application id. */
#define ISO_MAX_APPLICATION_ID 128

/*! \def Max # characters in an system id. */
#define ISO_MAX_SYSTEM_ID 32

/*! \def Max # characters in an volume id. */
#define ISO_MAX_VOLUME_ID 32

/*! \def Max # characters in an volumeset id. */
#define ISO_MAX_VOLUMESET_ID 128

/*
 * ISO 9660 directory flags.
 */
#define	ISO_FILE	  0	/**< Not really a flag...		  */
#define	ISO_EXISTENCE	  1	/**< Do not make existence known (hidden) */
#define	ISO_DIRECTORY	  2	/**< This file is a directory		  */
#define	ISO_ASSOCIATED	  4	/**< This file is an associated file	  */
#define	ISO_RECORD	  8	/**< Record format in extended attr. != 0 */
#define	ISO_PROTECTION	 16	/**< No read/execute perm. in ext. attr.  */
#define	ISO_DRESERVED1	 32	/**< Reserved bit 5			  */
#define	ISO_DRESERVED2	 64	/**< Reserved bit 6			  */
#define	ISO_MULTIEXTENT	128	/**< Not final entry of a mult. ext. file */

/* Volume descriptor types */
#define ISO_VD_PRIMARY             1
#define ISO_VD_SUPPLEMENTARY	   2  /**< Used by Joliet */
#define ISO_VD_END	         255

#define ISO_PVD_SECTOR  16      /**< Sector of Primary Volume Descriptor */
#define ISO_EVD_SECTOR  17      /**< Sector of End Volume Descriptor */

#define ISO_STANDARD_ID      "CD001" /**< String inside track identifying an 
                                        ISO 9660 filesystem. */
#define ISO_BLOCKSIZE           2048 /**< Number of bytes in an ISO
                                        9660 block */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum strncpy_pad_check {
  ISO9660_NOCHECK = 0,
  ISO9660_7BIT,
  ISO9660_ACHARS,
  ISO9660_DCHARS
};

PRAGMA_BEGIN_PACKED

/*! 
  \brief ISO-9660 shorter-format time structure.
  
  @see iso9660_dtime
 */
struct	iso9660_dtime {
  uint8_t 	dt_year;
  uint8_t 	dt_month;  /**< Has value in range 1..12. Note starts
                              at 1, not 0 like a tm struct. */
  uint8_t	dt_day;
  uint8_t	dt_hour;
  uint8_t	dt_minute;
  uint8_t	dt_second;
  int8_t	dt_gmtoff; /**< GMT values -48 .. + 52 in 15 minute
                              intervals */
} GNUC_PACKED;

typedef struct iso9660_dtime  iso9660_dtime_t;

/*! 
  \brief ISO-9660 longer-format time structure.
  
  @see iso9660_ltime
 */
struct	iso9660_ltime {
  char	 lt_year	[_delta(   1,	4)];   /**< Add 1900 to value
                                                    for the Julian
                                                    year */
  char	 lt_month	[_delta(   5,	6)];   /**< Has value in range
                                                  1..12. Note starts
                                                  at 1, not 0 like a
                                                  tm struct. */
  char	 lt_day		[_delta(   7,	8)];
  char	 lt_hour	[_delta(   9,	10)];
  char	 lt_minute	[_delta(  11,	12)];
  char	 lt_second	[_delta(  13,	14)];
  char	 lt_hsecond	[_delta(  15,	16)];  /**<! The value is in
                                                  units of 1/100's of
                                                  a second */
  int8_t lt_gmtoff	[_delta(  17,	17)];
} GNUC_PACKED;

typedef struct iso9660_ltime  iso9660_ltime_t;

/*! 
  \brief ISO-9660 Primary Volume Descriptor.
 */
struct iso9660_pvd {
  uint8_t          type;                         /**< 711 encoded */
  char             id[5];
  uint8_t          version;                      /**< 711 encoded */
  char             unused1[1];
  char             system_id[ISO_MAX_SYSTEM_ID]; /**< each char is an achar */
  char             volume_id[ISO_MAX_VOLUME_ID]; /**< each char is a dchar */
  char             unused2[8];
  uint64_t         volume_space_size;            /**< 733 encoded */
  char             unused3[32];
  uint32_t         volume_set_size;              /**< 723 encoded */
  uint32_t         volume_sequence_number;       /**< 723 encoded */
  uint32_t         logical_block_size;           /**< 723 encoded */
  uint64_t         path_table_size;              /**< 733 encoded */
  uint32_t         type_l_path_table;            /**< 731 encoded */
  uint32_t         opt_type_l_path_table;        /**< 731 encoded */
  uint32_t         type_m_path_table;            /**< 732 encoded */
  uint32_t         opt_type_m_path_table;        /**< 732 encoded */
  char             root_directory_record[34];    /**< See section 9.1 of
                                                    ISO 9660 spec. */
  char             volume_set_id[ISO_MAX_VOLUMESET_ID];    /**< dchars */
  char             publisher_id[ISO_MAX_PUBLISHER_ID];     /**< achars */
  char             preparer_id[ISO_MAX_PREPARER_ID];       /**< achars */
  char             application_id[ISO_MAX_APPLICATION_ID]; /**< achars */
  char             copyright_file_id[37];     /**< See section 7.5 of
                                                 ISO 9660 spec. Each char is 
                                                 a dchar */
  char             abstract_file_id[37];      /**< See section 7.5 of 
                                                 ISO 9660 spec. Each char is 
                                                 a dchar */
  char             bibliographic_file_id[37]; /**< See section 7.5 of 
                                                 ISO 9660 spec. Each char is
                                                 a dchar. */
  iso9660_ltime_t  creation_date;             /**< See section 8.4.26.1 of
                                                 ISO 9660 spec. */
  iso9660_ltime_t  modification_date;         /**< See section 8.4.26.1 of 
                                                 ISO 9660 spec. */
  iso9660_ltime_t  expiration_date;           /**< See section 8.4.26.1 of
                                                 ISO 9660 spec. */
  iso9660_ltime_t  effective_date;            /**< See section 8.4.26.1 of
                                                 ISO 9660 spec. */
  uint8_t          file_structure_version;    /**< 711 encoded */
  char             unused4[1];
  char             application_data[512];
  char             unused5[653];
} GNUC_PACKED;

/*! 
  \brief ISO-9660 Supplimentary Volume Descriptor. 

  This is used for Joliet Extentions and is almost the same as the
  the primary descriptor but two unused fields, "unused1" and "unused3
  become "flags and "escape_sequences" respectively.
*/
struct iso9660_svd {
  uint8_t          type;                         /**< 711 encoded */
  char             id[5];
  uint8_t          version;                      /**< 711 encoded */
  char             flags;			 /**< 853 */
  char             system_id[ISO_MAX_SYSTEM_ID]; /**< each char is an achar */
  char             volume_id[ISO_MAX_VOLUME_ID]; /**< each char is a dchar */
  char             unused2[8];
  uint64_t         volume_space_size;            /**< 733 encoded */
  char             escape_sequences[32];         /**< 856 */
  uint32_t         volume_set_size;              /**< 723 encoded */
  uint32_t         volume_sequence_number;       /**< 723 encoded */
  uint32_t         logical_block_size;           /**< 723 encoded */
  uint64_t         path_table_size;              /**< 733 encoded */
  uint32_t         type_l_path_table;            /**< 731 encoded */
  uint32_t         opt_type_l_path_table;        /**< 731 encoded */
  uint32_t         type_m_path_table;            /**< 732 encoded */
  uint32_t         opt_type_m_path_table;        /**< 732 encoded */
  char             root_directory_record[34];    /**< See section 9.1 of
                                                    ISO 9660 spec. */
  char             volume_set_id[ISO_MAX_VOLUMESET_ID];    /**< dchars */
  char             publisher_id[ISO_MAX_PUBLISHER_ID];     /**< achars */
  char             preparer_id[ISO_MAX_PREPARER_ID];       /**< achars */
  char             application_id[ISO_MAX_APPLICATION_ID]; /**< achars */
  char             copyright_file_id[37];     /**< See section 7.5 of
                                                 ISO 9660 spec. Each char is 
                                                 a dchar */
  char             abstract_file_id[37];      /**< See section 7.5 of 
                                                 ISO 9660 spec. Each char is 
                                                 a dchar */
  char             bibliographic_file_id[37]; /**< See section 7.5 of 
                                                 ISO 9660 spec. Each char is
                                                 a dchar. */
  iso9660_ltime_t  creation_date;             /**< See section 8.4.26.1 of
                                                 ISO 9660 spec. */
  iso9660_ltime_t  modification_date;         /**< See section 8.4.26.1 of 
                                                 ISO 9660 spec. */
  iso9660_ltime_t  expiration_date;           /**< See section 8.4.26.1 of
                                                 ISO 9660 spec. */
  iso9660_ltime_t  effective_date;            /**< See section 8.4.26.1 of
                                                 ISO 9660 spec. */
  uint8_t          file_structure_version;    /**< 711 encoded */
  char             unused4[1];
  char             application_data[512];
  char             unused5[653];
};

typedef struct iso9660_dir  iso9660_dir_t;
typedef struct iso9660_pvd  iso9660_pvd_t;
typedef struct iso9660_svd  iso9660_svd_t;
typedef struct iso9660_stat iso9660_stat_t;

#ifndef  EMPTY_ARRAY_SIZE
#define EMPTY_ARRAY_SIZE 0
#endif


/*! \brief Format of an ISO-9660 directory record 

 This structure may have an odd length depending on how many
 characters there are in the filename!  Some compilers (e.g. on
 Sun3/mc68020) pad the structures to an even length.  For this reason,
 we cannot use sizeof (struct iso_path_table) or sizeof (struct
 iso_directory_record) to compute on disk sizes.  Instead, we use
 offsetof(..., name) and add the name size.  See mkisofs.h of the 
 cdrtools package.

  @see iso9660_stat
*/
struct iso9660_dir {
  uint8_t          length;            /*! 711 encoded */
  uint8_t          xa_length;         /*! 711 encoded */
  uint64_t         extent;            /*! 733 encoded */
  uint64_t         size;              /*! 733 encoded */
  iso9660_dtime_t  recording_time;    /*! 7 711-encoded units */
  uint8_t          file_flags;
  uint8_t          file_unit_size;    /*! 711 encoded */
  uint8_t          interleave_gap;    /*! 711 encoded */
  uint32_t volume_sequence_number;    /*! 723 encoded */
  uint8_t          filename_len;      /*! 711 encoded */
  char             filename[EMPTY_ARRAY_SIZE];
} GNUC_PACKED;


PRAGMA_END_PACKED

/*! \brief Unix stat-like version of iso9660_dir

   The iso9660_stat structure is not part of the ISO-9660
   specification. We use it for our to communicate information
   in a C-library friendly way, e.g struct tm time structures and
   a C-style filename string.

   @see iso9660_dir
*/
struct iso9660_stat { /* big endian!! */
  struct tm    tm;                    /**< time on entry */
  lsn_t        lsn;                   /**< start logical sector number */
  uint32_t     size;                  /**< total size in bytes */
  uint32_t     secsize;               /**< number of sectors allocated */
  iso9660_xa_t xa;                    /**< XA attributes */
  enum { _STAT_FILE = 1, _STAT_DIR = 2 } type;
  char         filename[EMPTY_ARRAY_SIZE]; /**< filename */
};


/** This is an opaque structure. */
typedef struct _iso9660 iso9660_t; 

/*!
  Open an ISO 9660 image for reading. Maybe in the future we will have
  flags and mode. NULL is returned on error.
*/
  iso9660_t *iso9660_open (const char *pathname /*flags, mode */);

/*!
  Close previously opened ISO 9660 image.
  True is unconditionally returned. If there was an error false would
  be returned.
*/
  bool iso9660_close (iso9660_t * p_iso);


/*!
  Seek to a position and then read n bytes. Size read is returned.
*/
  long int iso9660_iso_seek_read (const iso9660_t *p_iso, void *ptr, 
                                  lsn_t start, long int size);

/*!
  Read the Primary Volume Descriptor for an ISO 9660 image.
  True is returned if read, and false if there was an error.
*/
  bool iso9660_ifs_read_pvd (const iso9660_t *p_iso, 
                             /*out*/ iso9660_pvd_t *p_pvd);

/*!
  Read the Primary Volume Descriptor for a CD.
  True is returned if read, and false if there was an error.
*/
  bool iso9660_fs_read_mode2_pvd (const CdIo *p_cdio, 
                                  /*out*/ iso9660_pvd_t *p_pvd,
                                  bool b_form2);

/*====================================================
  Time conversion 
 ====================================================*/
/*!
  Set time in format used in ISO 9660 directory index record
  from a Unix time structure. */
  void iso9660_set_dtime (const struct tm *tm, 
                          /*out*/ iso9660_dtime_t *idr_date);


/*!
  Set "long" time in format used in ISO 9660 primary volume descriptor
  from a Unix time structure. */
  void iso9660_set_ltime (const struct tm *_tm, 
                          /*out*/ iso9660_ltime_t *p_pvd_date);

/*!
  Get Unix time structure from format use in an ISO 9660 directory index 
  record. Even though tm_wday and tm_yday fields are not explicitly in
  idr_date, they are calculated from the other fields.

  If tm is to reflect the localtime, set "use_localtime" true, otherwise
  tm will reported in GMT.
*/
  void iso9660_get_dtime (const iso9660_dtime_t *idr_date, bool use_localtime,
                          /*out*/ struct tm *tm);


/*====================================================
  Characters used in file and directory and manipulation
 ====================================================*/
/*!
   Return true if c is a DCHAR - a character that can appear in an an
   ISO-9600 level 1 directory name. These are the ASCII capital
   letters A-Z, the digits 0-9 and an underscore.
*/
bool iso9660_isdchar (int c);

/*!
   Return true if c is an ACHAR - 
   These are the DCHAR's plus some ASCII symbols including the space 
   symbol.   
*/
bool iso9660_isachar (int c);

/*!
   Convert ISO-9660 file name that stored in a directory entry into 
   what's usually listed as the file name in a listing.
   Lowercase name, and drop deal with trailing ;1's or .;1's or 
   ; version numbers.

   The length of the translated string is returned.
*/
int iso9660_name_translate(const char *oldname, char *newname);

/*!  
  Pad string src with spaces to size len and copy this to dst. If
  len is less than the length of src, dst will be truncated to the
  first len characters of src.

  src can also be scanned to see if it contains only ACHARs, DCHARs, 
  7-bit ASCII chars depending on the enumeration _check.

  In addition to getting changed, dst is the return value.
  Note: this string might not be NULL terminated.
 */
char *iso9660_strncpy_pad(char dst[], const char src[], size_t len, 
                          enum strncpy_pad_check _check);

/*=====================================================================
  file/dirname's 
======================================================================*/

/*!
  Check that pathname is a valid ISO-9660 directory name.

  A valid directory name should not start out with a slash (/), 
  dot (.) or null byte, should be less than 37 characters long, 
  have no more than 8 characters in a directory component 
  which is separated by a /, and consist of only DCHARs. 

  True is returned if pathname is valid.
 */
bool iso9660_dirname_valid_p (const char pathname[]);

/*!  
  Take pathname and a version number and turn that into a ISO-9660
  pathname.  (That's just the pathname followd by ";" and the version
  number. For example, mydir/file.ext -> MYDIR/FILE.EXT;1 for version
  1. The resulting ISO-9660 pathname is returned.
*/
char *iso9660_pathname_isofy (const char pathname[], uint16_t version);

/*!
  Check that pathname is a valid ISO-9660 pathname.  

  A valid pathname contains a valid directory name, if one appears and
  the filename portion should be no more than 8 characters for the
  file prefix and 3 characters in the extension (or portion after a
  dot). There should be exactly one dot somewhere in the filename
  portion and the filename should be composed of only DCHARs.
  
  True is returned if pathname is valid.
 */
bool iso9660_pathname_valid_p (const char pathname[]);

/*=====================================================================
  directory tree 
======================================================================*/

void
iso9660_dir_init_new (void *dir, uint32_t self, uint32_t ssize, 
                      uint32_t parent, uint32_t psize, 
                      const time_t *dir_time);

void
iso9660_dir_init_new_su (void *dir, uint32_t self, uint32_t ssize, 
                         const void *ssu_data, unsigned int ssu_size, 
                         uint32_t parent, uint32_t psize, 
                         const void *psu_data, unsigned int psu_size,
                         const time_t *dir_time);

void
iso9660_dir_add_entry_su (void *dir, const char filename[], uint32_t extent,
                          uint32_t size, uint8_t file_flags, 
                          const void *su_data,
                          unsigned int su_size, const time_t *entry_time);

unsigned int 
iso9660_dir_calc_record_size (unsigned int namelen, unsigned int su_len);

/*!
   Given a directory pointer, find the filesystem entry that contains
   lsn and return information about it.

   Returns stat_t of entry if we found lsn, or NULL otherwise.
 */
iso9660_stat_t *iso9660_find_fs_lsn(const CdIo *p_cdio, lsn_t lsn);


/*!
   Given a directory pointer, find the filesystem entry that contains
   lsn and return information about it.

   Returns stat_t of entry if we found lsn, or NULL otherwise.
 */
iso9660_stat_t *iso9660_find_ifs_lsn(const iso9660_t *iso, lsn_t lsn);


/*!
  Get file status for pathname into stat. NULL is returned on error.
 */
iso9660_stat_t *iso9660_fs_stat (const CdIo *obj, const char pathname[], 
                                 bool is_mode2);

/*!
  Get file status for pathname into stat. NULL is returned on error.
  pathname version numbers in the ISO 9660
  name are dropped, i.e. ;1 is removed and if level 1 ISO-9660 names
  are lowercased.
 */
iso9660_stat_t *iso9660_fs_stat_translate (const CdIo *obj, 
                                           const char pathname[], 
                                           bool is_mode2);

/*!
  Get file status for pathname into stat. NULL is returned on error.
 */
void *iso9660_ifs_stat (iso9660_t *iso, const char pathname[]);


/*!
  Get file status for pathname into stat. NULL is returned on error.
  pathname version numbers in the ISO 9660
  name are dropped, i.e. ;1 is removed and if level 1 ISO-9660 names
  are lowercased.
 */
void *iso9660_ifs_stat_translate (iso9660_t *iso, const char pathname[]);



/*! 
  Read pathname (a directory) and return a list of iso9660_stat_t
  of the files inside that. The caller must free the returned result.
*/
void * iso9660_fs_readdir (const CdIo *obj, const char pathname[], bool mode2);

/*! 
  Read pathname (a directory) and return a list of iso9660_stat_t
  of the files inside that. The caller must free the returned result.
*/
void * iso9660_ifs_readdir (iso9660_t *iso, const char pathname[]);

/*!
  Return the PVD's application ID.
  NULL is returned if there is some problem in getting this. 
  */
const char * iso9660_get_application_id(const iso9660_pvd_t *p_pvd);


uint8_t iso9660_get_dir_len(const iso9660_dir_t *idr);

#if FIXME
uint8_t iso9660_get_dir_size(const iso9660_dir_t *idr);

lsn_t iso9660_get_dir_extent(const iso9660_dir_t *idr);
#endif

/*!
  Return the directory name stored in the iso9660_dir_t

  A string is allocated: the caller must deallocate.
*/
char * iso9660_dir_to_name (const iso9660_dir_t *iso9660_dir);
  
/*!
   Return a string containing the preparer id with trailing
   blanks removed.
*/
const char *iso9660_get_preparer_id(const iso9660_pvd_t *pvd);
  
/*!
   Return a string containing the PVD's publisher id with trailing
   blanks removed.
*/
const char *iso9660_get_publisher_id(const iso9660_pvd_t *pvd);

uint8_t iso9660_get_pvd_type(const iso9660_pvd_t *pvd);

const char * iso9660_get_pvd_id(const iso9660_pvd_t *pvd);

int iso9660_get_pvd_space_size(const iso9660_pvd_t *pvd);

int iso9660_get_pvd_block_size(const iso9660_pvd_t *pvd) ;

/*! Return the primary volume id version number (of pvd).
    If there is an error 0 is returned. 
 */
int iso9660_get_pvd_version(const iso9660_pvd_t *pvd) ;

/*!
   Return a string containing the PVD's system id with trailing
   blanks removed.
*/
const char *iso9660_get_system_id(const iso9660_pvd_t *pvd);
  

/*! Return the LSN of the root directory for pvd.
    If there is an error CDIO_INVALID_LSN is returned. 
 */
lsn_t iso9660_get_root_lsn(const iso9660_pvd_t *pvd);

/*!
  Return the PVD's volume ID.
*/
const char *iso9660_get_volume_id(const iso9660_pvd_t *pvd);

/*!
  Return the PVD's volumeset ID.
  NULL is returned if there is some problem in getting this. 
*/
const char *iso9660_get_volumeset_id(const iso9660_pvd_t *pvd);

/* pathtable */

/*! Zero's out pathable. Do this first. */
void iso9660_pathtable_init (void *pt);

unsigned int iso9660_pathtable_get_size (const void *pt);

uint16_t
iso9660_pathtable_l_add_entry (void *pt, const char name[], uint32_t extent,
                               uint16_t parent);

uint16_t
iso9660_pathtable_m_add_entry (void *pt, const char name[], uint32_t extent,
                               uint16_t parent);

/*=====================================================================
                 Volume Descriptors
======================================================================*/

void
iso9660_set_pvd (void *pd, const char volume_id[], const char application_id[],
                 const char publisher_id[], const char preparer_id[],
                 uint32_t iso_size, const void *root_dir, 
                 uint32_t path_table_l_extent, uint32_t path_table_m_extent,
                 uint32_t path_table_size, const time_t *pvd_time);

void 
iso9660_set_evd (void *pd);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CDIO_ISO9660_H__ */

/* 
 * Local variables:
 *  c-file-style: "gnu"
 *  tab-width: 8
 *  indent-tabs-mode: nil
 * End:
 */
