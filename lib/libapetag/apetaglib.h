/********************************************************************
*    
* Copyright (c) 2002 Artur Polaczynski (Ar't)  All rights reserved.
*            <artii@o2.pl>        LGPL-2.1
*       $ArtId: apetaglib.h,v 1.30 2003/04/16 21:06:27 art Exp $
********************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as 
 * published by the Free Software Foundation; either version 2.1 
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#ifndef _APETAGLIB_H
#define _APETAGLIB_H

/** \file
    \brief All function related to apetag
*/

// Uncomment this line to enable debug messages
//#define APE_TAG_DEBUG

#ifdef __WATCOMC__  // Watcom don't like || in ifdef and use #if define() || define()
#define USE_CHSIZE
#define strcasecmp(a,b) stricmp(a,b)
#define index(a,b) strchr(a,b)
#endif

#ifdef __WIN32__
#define USE_CHSIZE
#define strcasecmp(a,b) stricmp(a,b)
#define index(a,b) strchr(a,b)
#define S_IRGRP S_IRUSR
#define S_IWGRP S_IWUSR
#endif

/**\{*/
#ifdef APE_TAG_DEBUG
#define PRINT_D(x) fprintf( stdout, x )
#define PRINT_D1(x,a) fprintf( stdout, x, a )
#define PRINT_D2(x,a,b) fprintf( stdout, x, a,b)
#define PRINT_D3(x,a,b,c) fprintf( stdout, x, a,b,c)
#define PRINT_D4(x,a,b,c,d) fprintf( stdout, x, a,b,c,d)
#define PRINT_D5(x,a,b,c,d,e) fprintf( stdout, x, a,b,c,d,e)
#define PRINT_D6(x,a,b,c,d,e,f) fprintf( stdout, x, a,b,c,d,e,f)
#define PRINT_D7(x,a,b,c,d,e,f,g) fprintf( stdout, x, a,b,c,d,e,f,g)
#define PRINT_D8(x,a,b,c,d,e,f,g,i) fprintf( stdout, x, a,b,c,d,e,f,g,i)
#define PRINT_D9(x,a,b,c,d,e,f,g,i,j) fprintf( stdout, x ,a,b,c,d,e,f,g,i,j )
#else
#define PRINT_D(x) 
#define PRINT_D1(x,a)
#define PRINT_D2(x,a,b) 
#define PRINT_D3(x,a,b,c)
#define PRINT_D4(x,a,b,c,d) 
#define PRINT_D5(x,a,b,c,d,e) 
#define PRINT_D6(x,a,b,c,d,e,f)
#define PRINT_D7(x,a,b,c,d,e,f,g) 
#define PRINT_D8(x,a,b,c,d,e,f,g,i) 
#define PRINT_D9(x,a,b,c,d,e,f,g,i,j) 
#endif /*APE_TAG_DEBUG*/

#define PRINT_ERR(x) fprintf( stderr, x )
#define PRINT_ERR1(x, a) fprintf( stderr, x ,a)
/**\}*/

/** version of apetaglib defined in one place */
#define APETAGLIB_VERSION "0.5pre1"


/*from winamp mpc plugin*/
/** \name frame names */
/**\{*/
#define APE_TAG_FIELD_TITLE             "Title"
#define APE_TAG_FIELD_SUBTITLE          "Subtitle"
#define APE_TAG_FIELD_ARTIST            "Artist"
#define APE_TAG_FIELD_ALBUM             "Album"
#define APE_TAG_FIELD_DEBUTALBUM        "Debut Album"
#define APE_TAG_FIELD_PUBLISHER         "Publisher"
#define APE_TAG_FIELD_CONDUCTOR         "Conductor"
#define APE_TAG_FIELD_COMPOSER          "Composer"
#define APE_TAG_FIELD_COMMENT           "Comment"
#define APE_TAG_FIELD_YEAR              "Year"
#define APE_TAG_FIELD_RECORDDATE        "Record Date"
#define APE_TAG_FIELD_RECORDLOCATION    "Record Location"
#define APE_TAG_FIELD_TRACK             "Track"
#define APE_TAG_FIELD_GENRE             "Genre"
#define APE_TAG_FIELD_COVER_ART_FRONT   "Cover Art (front)"
#define APE_TAG_FIELD_NOTES             "Notes"
#define APE_TAG_FIELD_LYRICS            "Lyrics"
#define APE_TAG_FIELD_COPYRIGHT         "Copyright"
#define APE_TAG_FIELD_PUBLICATIONRIGHT  "Publicationright"
#define APE_TAG_FIELD_FILE              "File"
#define APE_TAG_FIELD_MEDIA             "Media"
#define APE_TAG_FIELD_EANUPC            "EAN/UPC"
#define APE_TAG_FIELD_ISRC              "ISRC"
#define APE_TAG_FIELD_RELATED_URL       "Related"
#define APE_TAG_FIELD_ABSTRACT_URL      "Abstract"
#define APE_TAG_FIELD_BIBLIOGRAPHY_URL  "Bibliography"
#define APE_TAG_FIELD_BUY_URL           "Buy URL"
#define APE_TAG_FIELD_ARTIST_URL        "Artist URL"
#define APE_TAG_FIELD_PUBLISHER_URL     "Publisher URL"
#define APE_TAG_FIELD_FILE_URL          "File URL"
#define APE_TAG_FIELD_COPYRIGHT_URL     "Copyright URL"
#define APE_TAG_FIELD_INDEX             "Index"
#define APE_TAG_FIELD_INTROPLAY         "Introplay"
#define APE_TAG_FIELD_MJ_METADATA       "Media Jukebox Metadata"
#define APE_TAG_FIELD_DUMMY             "Dummy"
/**\}*/

#define APE_TAG_LIB_FIRST   "\02"    /**< is using by #apefrm_get to get first frame */
#define APE_TAG_LIB_NEXT    "\03"    /**< is using by #apefrm_get to get next frame you may check all frames this way */
#define APE_TAG_LIB_DEL_ALL "\04"    /**< is using by #apefrm_remove_real for removing all frames */


/** 
    \name #apetag_save flags
    \note default is #APE_TAG_V2 + #SAVE_NEW_OLD_APE_TAG + #SAVE_REMOVE_ID3V1
*/
/**\{*/
#define APE_TAG_V1            (1 <<  1)
#define APE_TAG_V2            (1 <<  2)
#define SAVE_NEW_APE_TAG      (1 <<  3)
#define SAVE_NEW_OLD_APE_TAG  (1 <<  4)
#define SAVE_REMOVE_ID3V1     (1 <<  5)
#define SAVE_CREATE_ID3V1_TAG (1 <<  6)
#define SAVE_FAKE_SAVE        (1 <<  7)
/* apetag_read(_fp) flags - default read all (ape,id3v1,id3v2(if compiled)) */
#define DONT_READ_TAG_APE     (1 <<  8)
#define DONT_READ_TAG_ID3V1   (1 <<  9)
#define DONT_READ_TAG_ID3V2   (1 << 10)
/**\}*/


/**
    \name #atl_return
    \brief return codes from all functions
    \{
*/
#define     ATL_OK        0    /**< not using :) */
#define     ATL_FOPEN     1    /**< can't open file */
#define     ATL_FREAD     2    /**< can't read from file */
#define     ATL_FWRITE    3    /**< can't write to file (written bytes != bytes to write) */
#define     ATL_MALOC     4    /**< can't allocate memory */
#define     ATL_BADARG    5    /**< bad function argument */
#define     ATL_NOINIT    6    /**< not inited struct by apetag_init */
/** \} */

/** 
    \struct tag
    \brief tag  structure 

    i you get this <b>don't</b> change anything. copy all values/strings  
*/
struct tag
{
    char *name;          /**< name of tag */
    char *value;         /**< value of tag */
    size_t sizeName;     /**< size of name in tag */
    size_t sizeValue;    /**< size of value in tag */
    unsigned long flags; /**< flags of tag */
};


/** 
    object apetag used to store information about tag.
    main object store **#tag , number of frames, current position for 
    APE_TAG_LIB_NEXT, etc
    \brief object apetag used to store information about tag
**/
typedef struct _ape_mem_cnt  apetag;


/* 
 *  function:
 *  apetag_*: for load/save/init file and structure
 *  apefrm_*: for add del edit one (or more) 
 *          frame(, field, entry) in tag 
 */

/* read file and add frames */
int
apetag_read (apetag *mem_cnt, char *filename, int flag) ;

/* read file and add frames */
int 
apetag_read_fp (apetag *mem_cnt, ape_file * fp, char *filename, int flag) ;

/* initialise new object #apetag and return */
apetag * 
apetag_init (void) ;

/* free #apetag object */
void 
apetag_free (apetag *mem_cnt) ;

/* save apetag to file */
int
apetag_save (char *filename, apetag *mem_cnt, int flag) ;


/* Add text frame */
int
apefrm_add (apetag *mem_cnt, unsigned long flags, char *name, char *value) ;

/* add binary frame */
int
apefrm_add_bin (apetag *mem_cnt, unsigned long flags,
                long sizeName, char *name, long sizeValue, char *value);

/* add frame if other (the same name) no exist */
int
apefrm_add_noreplace (apetag *mem_cnt, unsigned long flags, char *name, char *value) ;

/* search in apetag for name and return tag */
struct tag * 
apefrm_get (apetag *mem_cnt, char *name) ;

/* search in apetag for name and return string */
char * 
apefrm_getstr (apetag *mem_cnt, char *name) ;

/* remove frame from memory */
void 
apefrm_remove_real (apetag *mem_cnt, char *name) ;


/**
    \def apefrm_fake_remove(mem_cnt,name)
    \brief set frame to remove 
    \deprecated remove in 0.5
*/
#define     apefrm_fake_remove(mem_cnt,name)  apefrm_remove(mem_cnt,name)

/* set frame to remove */
void
apefrm_remove (apetag *mem_cnt, char *name);

/* read id3v1 and add frames */
int 
readtag_id3v1_fp (apetag *mem_cnt, ape_file * fp) ;

/** \name flags in frames and headers */ 
/**\{*/
//================================
#define HEADER_IS        0x80000000   
#define HEADER_NOT       0x00000000 
#define HEADER_THIS_IS   0x20000000  
//================================
#define FOOTER_IS        0x00000000
#define FOOTER_NOT       0x40000000
#define FOOTER_THIS_IS   0x00000000
//================================
#define ITEM_TEXT        0x00000000
#define ITEM_BIN         0x00000002   
#define ITEM_LINK        0x00000004
#define ITEM_URL         0x00000004 // for compability ITEM_LINK
//================================
#define TAG_RW           0x00000000
#define TAG_RO           0x00000001
/**\}*/

/* debug function print all tags exclude bin (print only size for bin) */
void
libapetag_print_mem_cnt (apetag *mem_cnt);

#endif /* _APETAGLIB_H */
