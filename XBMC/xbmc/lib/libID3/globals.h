// -*- C++ -*-
/* $Id$

 * id3lib: a C++ library for creating and manipulating id3v1/v2 tags
 * Copyright 1999, 2000 Scott Thomas Haug
 * Copyright 2002 Thijmen Klok (thijmen@id3lib.org)

 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

 * The id3lib authors encourage improvements and optimisations to be sent to
 * the id3lib coordinator.  Please see the README file for details on where to
 * send such submissions.  See the AUTHORS file for a list of people who have
 * contributed to id3lib.  See the ChangeLog file for a list of changes to
 * id3lib.  These files are distributed with id3lib at
 * http://download.sourceforge.net/id3lib/
 */

/** This file defines common macros, types, constants, and enums used
 ** throughout id3lib.
 **/

#ifndef _ID3LIB_GLOBALS_H_
#define _ID3LIB_GLOBALS_H_

#include <stdlib.h>
#include "sized_types.h"

/* id3lib version.
 * we prefix variable declarations so they can
 * properly get exported in windows dlls.
 */
#ifdef WIN32
#  define LINKOPTION_STATIC         1 //both for use and creation of static lib
#  define LINKOPTION_CREATE_DYNAMIC 2 //should only be used by prj/id3lib.dsp
#  define LINKOPTION_USE_DYNAMIC    3 //if your project links id3lib dynamic
#  ifndef ID3LIB_LINKOPTION
#    pragma message("*** NOTICE *** (not a real error)")
#    pragma message("* You should include a define in your project which reflect how you link the library")
#    pragma message("* If you use id3lib.lib or libprj/id3lib.dsp (you link static) you should add")
#    pragma message("* ID3LIB_LINKOPTION=1 to your preprocessor definitions of your project.")
#    pragma message("* If you use id3lib.dll (you link dynamic) you should add ID3LIB_LINKOPTION=3")
#    pragma message("* to your preprocessor definitions of your project.")
#    pragma message("***")
#    error read message above or win32.readme.first.txt
#  else
#    if (ID3LIB_LINKOPTION == LINKOPTION_CREATE_DYNAMIC)
       //used for creating a dynamic dll
#      define ID3_C_EXPORT extern _declspec(dllexport)
#      define ID3_CPP_EXPORT __declspec(dllexport)
#      define CCONV __stdcall // Added for VB & Delphi Compatibility - By FrogPrince Advised By Lothar
#    endif
#    if (ID3LIB_LINKOPTION == LINKOPTION_STATIC)
       //used for creating a static lib and using a static lib
#      define ID3_C_EXPORT
#      define ID3_CPP_EXPORT
#      define CCONV
#    endif
#    if (ID3LIB_LINKOPTION == LINKOPTION_USE_DYNAMIC)
       //used for those that do not link static and are using the dynamic dll by including a id3lib header
#      define ID3_C_EXPORT extern _declspec(dllimport)
#      define ID3_CPP_EXPORT __declspec(dllimport) //functions like these shouldn't be used by vb and delphi,
#      define CCONV __stdcall // Added for VB & Delphi Compatibility - By FrogPrince Advised By Lothar
#    endif
#  endif
#else /* !WIN32 */
#  define ID3_C_EXPORT
#  define ID3_CPP_EXPORT
#  define CCONV
#endif /* !WIN32 */

#define ID3_C_VAR extern

#ifndef __cplusplus

typedef int bool;
#  define false (0)
#  define true (!false)

#endif /* __cplusplus */

ID3_C_VAR const char * const ID3LIB_NAME;
ID3_C_VAR const char * const ID3LIB_RELEASE;
ID3_C_VAR const char * const ID3LIB_FULL_NAME;
ID3_C_VAR const int          ID3LIB_MAJOR_VERSION;
ID3_C_VAR const int          ID3LIB_MINOR_VERSION;
ID3_C_VAR const int          ID3LIB_PATCH_VERSION;
ID3_C_VAR const int          ID3LIB_INTERFACE_AGE;
ID3_C_VAR const int          ID3LIB_BINARY_AGE;

#define ID3_TAGID               "ID3"
#define ID3_TAGIDSIZE           (3)
#define ID3_TAGHEADERSIZE       (10)

/** String used for the description field of a comment tag converted from an
 ** id3v1 tag to an id3v2 tag
 **
 ** \sa #ID3V1_Tag
 **/
#define STR_V1_COMMENT_DESC "ID3v1 Comment"


typedef       unsigned char   uchar;
typedef long  unsigned int    luint;

typedef uint16                unicode_t;
typedef uint16                flags_t;

#define NULL_UNICODE ((unicode_t) '\0')

/* These macros are used to make the C and C++ declarations for enums and
 * structs have the same syntax.  Basically, it allows C users to refer to an
 * enum or a struct without prepending enum/struct.
 */
#ifdef __cplusplus
#  define ID3_ENUM(E)   enum   E
#  define ID3_STRUCT(S) struct S
#else
#  define ID3_ENUM(E)   typedef enum   _ ## E E; enum   _ ## E
#  define ID3_STRUCT(S) typedef struct _ ## S S; struct _ ## S
#endif

/** \enum ID3_TextEnc
 ** Enumeration of the types of text encodings: ascii or unicode
 **/
ID3_ENUM(ID3_TextEnc)
{
  ID3TE_NONE = -1,
  ID3TE_ISO8859_1,
  ID3TE_UTF16,
  ID3TE_UTF16BE,
  ID3TE_UTF8,
  ID3TE_NUMENCODINGS,
  ID3TE_ASCII = ID3TE_ISO8859_1, // do not use this -> use ID3TE_IS_SINGLE_BYTE_ENC(enc) instead
  ID3TE_UNICODE = ID3TE_UTF16    // do not use this -> use ID3TE_IS_DOUBLE_BYTE_ENC(enc) instead
};

#define ID3TE_IS_SINGLE_BYTE_ENC(enc)    ((enc) == ID3TE_ISO8859_1 || (enc) == ID3TE_UTF8)
#define ID3TE_IS_DOUBLE_BYTE_ENC(enc)    ((enc) == ID3TE_UTF16 || (enc) == ID3TE_UTF16BE)

/** Enumeration of the various id3 specifications
 **/
ID3_ENUM(ID3_V1Spec)
{
  ID3V1_0 = 0,
  ID3V1_1,
  ID3V1_NUMSPECS
};

ID3_ENUM(ID3_V2Spec)
{
  ID3V2_UNKNOWN = -1,
  ID3V2_2_0 = 0,
  ID3V2_2_1,
  ID3V2_3_0,
  ID3V2_4_0,
  ID3V2_EARLIEST = ID3V2_2_0,
  ID3V2_LATEST = ID3V2_3_0
};

/** The various types of tags that id3lib can handle
 **/
ID3_ENUM(ID3_TagType)
{
  ID3TT_NONE       =      0,   /**< Represents an empty or non-existant tag */
  ID3TT_ID3V1      = 1 << 0,   /**< Represents an id3v1 or id3v1.1 tag */
  ID3TT_ID3V2      = 1 << 1,   /**< Represents an id3v2 tag */
  ID3TT_LYRICS3    = 1 << 2,   /**< Represents a Lyrics3 tag */
  ID3TT_LYRICS3V2  = 1 << 3,   /**< Represents a Lyrics3 v2.00 tag */
  ID3TT_MUSICMATCH = 1 << 4,   /**< Represents a MusicMatch tag */
   /**< Represents a Lyrics3 tag (for backwards compatibility) */
  ID3TT_LYRICS     = ID3TT_LYRICS3,
  /** Represents both id3 tags: id3v1 and id3v2 */
  ID3TT_ID3        = ID3TT_ID3V1 | ID3TT_ID3V2,
  /** Represents all possible types of tags */
  ID3TT_ALL        = ~ID3TT_NONE,
  /** Represents all tag types that can be prepended to a file */
  ID3TT_PREPENDED  = ID3TT_ID3V2,
  /** Represents all tag types that can be appended to a file */
  ID3TT_APPENDED   = ID3TT_ALL & ~ID3TT_ID3V2
};

/**
 ** Enumeration of the different types of fields in a frame.
 **/
ID3_ENUM(ID3_FieldID)
{
  ID3FN_NOFIELD = 0,    /**< No field */
  ID3FN_TEXTENC,        /**< Text encoding (unicode or ASCII) */
  ID3FN_TEXT,           /**< Text field */
  ID3FN_URL,            /**< A URL */
  ID3FN_DATA,           /**< Data field */
  ID3FN_DESCRIPTION,    /**< Description field */
  ID3FN_OWNER,          /**< Owner field */
  ID3FN_EMAIL,          /**< Email field */
  ID3FN_RATING,         /**< Rating field */
  ID3FN_FILENAME,       /**< Filename field */
  ID3FN_LANGUAGE,       /**< Language field */
  ID3FN_PICTURETYPE,    /**< Picture type field */
  ID3FN_IMAGEFORMAT,    /**< Image format field */
  ID3FN_MIMETYPE,       /**< Mimetype field */
  ID3FN_COUNTER,        /**< Counter field */
  ID3FN_ID,             /**< Identifier/Symbol field */
  ID3FN_VOLUMEADJ,      /**< Volume adjustment field */
  ID3FN_NUMBITS,        /**< Number of bits field */
  ID3FN_VOLCHGRIGHT,    /**< Volume chage on the right channel */
  ID3FN_VOLCHGLEFT,     /**< Volume chage on the left channel */
  ID3FN_PEAKVOLRIGHT,   /**< Peak volume on the right channel */
  ID3FN_PEAKVOLLEFT,    /**< Peak volume on the left channel */
  ID3FN_TIMESTAMPFORMAT,/**< SYLT Timestamp Format */
  ID3FN_CONTENTTYPE,    /**< SYLT content type */
  ID3FN_LASTFIELDID     /**< Last field placeholder */
};

/**
 ** Enumeration of the different types of frames recognized by id3lib
 **/
ID3_ENUM(ID3_FrameID)
{
  /* ???? */ ID3FID_NOFRAME = 0,       /**< No known frame */
  /* AENC */ ID3FID_AUDIOCRYPTO,       /**< Audio encryption */
  /* APIC */ ID3FID_PICTURE,           /**< Attached picture */
  /* ASPI */ ID3FID_AUDIOSEEKPOINT,    /**< Audio seek point index */
  /* COMM */ ID3FID_COMMENT,           /**< Comments */
  /* COMR */ ID3FID_COMMERCIAL,        /**< Commercial frame */
  /* ENCR */ ID3FID_CRYPTOREG,         /**< Encryption method registration */
  /* EQU2 */ ID3FID_EQUALIZATION2,     /**< Equalisation (2) */
  /* EQUA */ ID3FID_EQUALIZATION,      /**< Equalization */
  /* ETCO */ ID3FID_EVENTTIMING,       /**< Event timing codes */
  /* GEOB */ ID3FID_GENERALOBJECT,     /**< General encapsulated object */
  /* GRID */ ID3FID_GROUPINGREG,       /**< Group identification registration */
  /* IPLS */ ID3FID_INVOLVEDPEOPLE,    /**< Involved people list */
  /* LINK */ ID3FID_LINKEDINFO,        /**< Linked information */
  /* MCDI */ ID3FID_CDID,              /**< Music CD identifier */
  /* MLLT */ ID3FID_MPEGLOOKUP,        /**< MPEG location lookup table */
  /* OWNE */ ID3FID_OWNERSHIP,         /**< Ownership frame */
  /* PRIV */ ID3FID_PRIVATE,           /**< Private frame */
  /* PCNT */ ID3FID_PLAYCOUNTER,       /**< Play counter */
  /* POPM */ ID3FID_POPULARIMETER,     /**< Popularimeter */
  /* POSS */ ID3FID_POSITIONSYNC,      /**< Position synchronisation frame */
  /* RBUF */ ID3FID_BUFFERSIZE,        /**< Recommended buffer size */
  /* RVA2 */ ID3FID_VOLUMEADJ2,        /**< Relative volume adjustment (2) */
  /* RVAD */ ID3FID_VOLUMEADJ,         /**< Relative volume adjustment */
  /* RVRB */ ID3FID_REVERB,            /**< Reverb */
  /* SEEK */ ID3FID_SEEKFRAME,         /**< Seek frame */
  /* SIGN */ ID3FID_SIGNATURE,         /**< Signature frame */
  /* SYLT */ ID3FID_SYNCEDLYRICS,      /**< Synchronized lyric/text */
  /* SYTC */ ID3FID_SYNCEDTEMPO,       /**< Synchronized tempo codes */
  /* TALB */ ID3FID_ALBUM,             /**< Album/Movie/Show title */
  /* TBPM */ ID3FID_BPM,               /**< BPM (beats per minute) */
  /* TCOM */ ID3FID_COMPOSER,          /**< Composer */
  /* TCON */ ID3FID_CONTENTTYPE,       /**< Content type */
  /* TCOP */ ID3FID_COPYRIGHT,         /**< Copyright message */
  /* TDAT */ ID3FID_DATE,              /**< Date */
  /* TDEN */ ID3FID_ENCODINGTIME,      /**< Encoding time */
  /* TDLY */ ID3FID_PLAYLISTDELAY,     /**< Playlist delay */
  /* TDOR */ ID3FID_ORIGRELEASETIME,   /**< Original release time */
  /* TDRC */ ID3FID_RECORDINGTIME,     /**< Recording time */
  /* TDRL */ ID3FID_RELEASETIME,       /**< Release time */
  /* TDTG */ ID3FID_TAGGINGTIME,       /**< Tagging time */
  /* TIPL */ ID3FID_INVOLVEDPEOPLE2,   /**< Involved people list */
  /* TENC */ ID3FID_ENCODEDBY,         /**< Encoded by */
  /* TEXT */ ID3FID_LYRICIST,          /**< Lyricist/Text writer */
  /* TFLT */ ID3FID_FILETYPE,          /**< File type */
  /* TIME */ ID3FID_TIME,              /**< Time */
  /* TIT1 */ ID3FID_CONTENTGROUP,      /**< Content group description */
  /* TIT2 */ ID3FID_TITLE,             /**< Title/songname/content description */
  /* TIT3 */ ID3FID_SUBTITLE,          /**< Subtitle/Description refinement */
  /* TKEY */ ID3FID_INITIALKEY,        /**< Initial key */
  /* TLAN */ ID3FID_LANGUAGE,          /**< Language(s) */
  /* TLEN */ ID3FID_SONGLEN,           /**< Length */
  /* TMCL */ ID3FID_MUSICIANCREDITLIST,/**< Musician credits list */
  /* TMED */ ID3FID_MEDIATYPE,         /**< Media type */
  /* TMOO */ ID3FID_MOOD,              /**< Mood */
  /* TOAL */ ID3FID_ORIGALBUM,         /**< Original album/movie/show title */
  /* TOFN */ ID3FID_ORIGFILENAME,      /**< Original filename */
  /* TOLY */ ID3FID_ORIGLYRICIST,      /**< Original lyricist(s)/text writer(s) */
  /* TOPE */ ID3FID_ORIGARTIST,        /**< Original artist(s)/performer(s) */
  /* TORY */ ID3FID_ORIGYEAR,          /**< Original release year */
  /* TOWN */ ID3FID_FILEOWNER,         /**< File owner/licensee */
  /* TPE1 */ ID3FID_LEADARTIST,        /**< Lead performer(s)/Soloist(s) */
  /* TPE2 */ ID3FID_BAND,              /**< Band/orchestra/accompaniment */
  /* TPE3 */ ID3FID_CONDUCTOR,         /**< Conductor/performer refinement */
  /* TPE4 */ ID3FID_MIXARTIST,         /**< Interpreted, remixed, or otherwise modified by */
  /* TPOS */ ID3FID_PARTINSET,         /**< Part of a set */
  /* TPRO */ ID3FID_PRODUCEDNOTICE,    /**< Produced notice */
  /* TPUB */ ID3FID_PUBLISHER,         /**< Publisher */
  /* TRCK */ ID3FID_TRACKNUM,          /**< Track number/Position in set */
  /* TRDA */ ID3FID_RECORDINGDATES,    /**< Recording dates */
  /* TRSN */ ID3FID_NETRADIOSTATION,   /**< Internet radio station name */
  /* TRSO */ ID3FID_NETRADIOOWNER,     /**< Internet radio station owner */
  /* TSIZ */ ID3FID_SIZE,              /**< Size */
  /* TSOA */ ID3FID_ALBUMSORTORDER,    /**< Album sort order */
  /* TSOP */ ID3FID_PERFORMERSORTORDER,/**< Performer sort order */
  /* TSOT */ ID3FID_TITLESORTORDER,    /**< Title sort order */
  /* TSRC */ ID3FID_ISRC,              /**< ISRC (international standard recording code) */
  /* TSSE */ ID3FID_ENCODERSETTINGS,   /**< Software/Hardware and settings used for encoding */
  /* TSST */ ID3FID_SETSUBTITLE,       /**< Set subtitle */
  /* TXXX */ ID3FID_USERTEXT,          /**< User defined text information */
  /* TYER */ ID3FID_YEAR,              /**< Year */
  /* UFID */ ID3FID_UNIQUEFILEID,      /**< Unique file identifier */
  /* USER */ ID3FID_TERMSOFUSE,        /**< Terms of use */
  /* USLT */ ID3FID_UNSYNCEDLYRICS,    /**< Unsynchronized lyric/text transcription */
  /* WCOM */ ID3FID_WWWCOMMERCIALINFO, /**< Commercial information */
  /* WCOP */ ID3FID_WWWCOPYRIGHT,      /**< Copyright/Legal infromation */
  /* WOAF */ ID3FID_WWWAUDIOFILE,      /**< Official audio file webpage */
  /* WOAR */ ID3FID_WWWARTIST,         /**< Official artist/performer webpage */
  /* WOAS */ ID3FID_WWWAUDIOSOURCE,    /**< Official audio source webpage */
  /* WORS */ ID3FID_WWWRADIOPAGE,      /**< Official internet radio station homepage */
  /* WPAY */ ID3FID_WWWPAYMENT,        /**< Payment */
  /* WPUB */ ID3FID_WWWPUBLISHER,      /**< Official publisher webpage */
  /* WXXX */ ID3FID_WWWUSER,           /**< User defined URL link */
  /*      */ ID3FID_METACRYPTO,        /**< Encrypted meta frame (id3v2.2.x) */
  /*      */ ID3FID_METACOMPRESSION,   /**< Compressed meta frame (id3v2.2.1) */
  /* >>>> */ ID3FID_LASTFRAMEID        /**< Last field placeholder */
};

ID3_ENUM(ID3_V1Lengths)
{
  ID3_V1_LEN         = 128,
  ID3_V1_LEN_ID      =   3,
  ID3_V1_LEN_TITLE   =  30,
  ID3_V1_LEN_ARTIST  =  30,
  ID3_V1_LEN_ALBUM   =  30,
  ID3_V1_LEN_YEAR    =   4,
  ID3_V1_LEN_COMMENT =  30,
  ID3_V1_LEN_GENRE   =   1
};

ID3_ENUM(ID3_FieldFlags)
{
  ID3FF_NONE       =      0,
  ID3FF_CSTR       = 1 << 0,
  ID3FF_LIST       = 1 << 1,
  ID3FF_ENCODABLE  = 1 << 2,
  ID3FF_TEXTLIST   = ID3FF_CSTR | ID3FF_LIST | ID3FF_ENCODABLE
};

/** Enumeration of the types of field types */
ID3_ENUM(ID3_FieldType)
{
  ID3FTY_NONE           = -1,
  ID3FTY_INTEGER        = 0,
  ID3FTY_BINARY,
  ID3FTY_TEXTSTRING,
  ID3FTY_NUMTYPES
};

/**
 ** Predefined id3lib error types.
 **/
ID3_ENUM(ID3_Err)
{
  ID3E_NoError = 0,             /**< No error reported */
  ID3E_NoMemory,                /**< No available memory */
  ID3E_NoData,                  /**< No data to parse */
  ID3E_BadData,                 /**< Improperly formatted data */
  ID3E_NoBuffer,                /**< No buffer to write to */
  ID3E_SmallBuffer,             /**< Buffer is too small */
  ID3E_InvalidFrameID,          /**< Invalid frame id */
  ID3E_FieldNotFound,           /**< Requested field not found */
  ID3E_UnknownFieldType,        /**< Unknown field type */
  ID3E_TagAlreadyAttached,      /**< Tag is already attached to a file */
  ID3E_InvalidTagVersion,       /**< Invalid tag version */
  ID3E_NoFile,                  /**< No file to parse */
  ID3E_ReadOnly,                /**< Attempting to write to a read-only file */
  ID3E_zlibError                /**< Error in compression/uncompression */
};

ID3_ENUM(ID3_ContentType)
{
  ID3CT_OTHER = 0,
  ID3CT_LYRICS,
  ID3CT_TEXTTRANSCRIPTION,
  ID3CT_MOVEMENT,
  ID3CT_EVENTS,
  ID3CT_CHORD,
  ID3CT_TRIVIA
};

ID3_ENUM(ID3_PictureType)
{
  ID3PT_OTHER = 0,
  ID3PT_PNG32ICON = 1,     //  32x32 pixels 'file icon' (PNG only)
  ID3PT_OTHERICON = 2,     // Other file icon
  ID3PT_COVERFRONT = 3,    // Cover (front)
  ID3PT_COVERBACK = 4,     // Cover (back)
  ID3PT_LEAFLETPAGE = 5,   // Leaflet page
  ID3PT_MEDIA = 6,         // Media (e.g. lable side of CD)
  ID3PT_LEADARTIST = 7,    // Lead artist/lead performer/soloist
  ID3PT_ARTIST = 8,        // Artist/performer
  ID3PT_CONDUCTOR = 9,     // Conductor
  ID3PT_BAND = 10,         // Band/Orchestra
  ID3PT_COMPOSER = 11,     // Composer
  ID3PT_LYRICIST = 12,     // Lyricist/text writer
  ID3PT_REC_LOCATION = 13, // Recording Location
  ID3PT_RECORDING = 14,    // During recording
  ID3PT_PERFORMANCE = 15,  // During performance
  ID3PT_VIDEO = 16,        // Movie/video screen capture
  ID3PT_FISH = 17,         // A bright coloured fish
  ID3PT_ILLUSTRATION = 18, // Illustration
  ID3PT_ARTISTLOGO = 19,   // Band/artist logotype
  ID3PT_PUBLISHERLOGO = 20 // Publisher/Studio logotype
};

ID3_ENUM(ID3_TimeStampFormat)
{
  ID3TSF_FRAME  = 1,
  ID3TSF_MS
};

ID3_ENUM(MP3_BitRates)
{
  MP3BITRATE_FALSE = -1,
  MP3BITRATE_NONE = 0,
  MP3BITRATE_8K   = 8000,
  MP3BITRATE_16K  = 16000,
  MP3BITRATE_24K  = 24000,
  MP3BITRATE_32K  = 32000,
  MP3BITRATE_40K  = 40000,
  MP3BITRATE_48K  = 48000,
  MP3BITRATE_56K  = 56000,
  MP3BITRATE_64K  = 64000,
  MP3BITRATE_80K  = 80000,
  MP3BITRATE_96K  = 96000,
  MP3BITRATE_112K = 112000,
  MP3BITRATE_128K = 128000,
  MP3BITRATE_144K = 144000,
  MP3BITRATE_160K = 160000,
  MP3BITRATE_176K = 176000,
  MP3BITRATE_192K = 192000,
  MP3BITRATE_224K = 224000,
  MP3BITRATE_256K = 256000,
  MP3BITRATE_288K = 288000,
  MP3BITRATE_320K = 320000,
  MP3BITRATE_352K = 352000,
  MP3BITRATE_384K = 384000,
  MP3BITRATE_416K = 416000,
  MP3BITRATE_448K = 448000
};

ID3_ENUM(Mpeg_Layers)
{
  MPEGLAYER_FALSE = -1,
  MPEGLAYER_UNDEFINED,
  MPEGLAYER_III,
  MPEGLAYER_II,
  MPEGLAYER_I
};

ID3_ENUM(Mpeg_Version)
{
  MPEGVERSION_FALSE = -1,
  MPEGVERSION_2_5,
  MPEGVERSION_Reserved,
  MPEGVERSION_2,
  MPEGVERSION_1
};

ID3_ENUM(Mp3_Frequencies)
{
  MP3FREQUENCIES_FALSE = -1,
  MP3FREQUENCIES_Reserved = 0,
  MP3FREQUENCIES_8000HZ = 8000,
  MP3FREQUENCIES_11025HZ = 11025,
  MP3FREQUENCIES_12000HZ = 12000,
  MP3FREQUENCIES_16000HZ = 16000,
  MP3FREQUENCIES_22050HZ = 22050,
  MP3FREQUENCIES_24000HZ = 24000,
  MP3FREQUENCIES_32000HZ = 32000,
  MP3FREQUENCIES_48000HZ = 48000,
  MP3FREQUENCIES_44100HZ = 44100,
};

ID3_ENUM(Mp3_ChannelMode)
{
  MP3CHANNELMODE_FALSE = -1,
  MP3CHANNELMODE_STEREO,
  MP3CHANNELMODE_JOINT_STEREO,
  MP3CHANNELMODE_DUAL_CHANNEL,
  MP3CHANNELMODE_SINGLE_CHANNEL
};

ID3_ENUM(Mp3_ModeExt)
{
  MP3MODEEXT_FALSE = -1,
  MP3MODEEXT_0,
  MP3MODEEXT_1,
  MP3MODEEXT_2,
  MP3MODEEXT_3
};

ID3_ENUM(Mp3_Emphasis)
{
  MP3EMPHASIS_FALSE = -1,
  MP3EMPHASIS_NONE,
  MP3EMPHASIS_50_15MS,
  MP3EMPHASIS_Reserved,
  MP3EMPHASIS_CCIT_J17
};

ID3_ENUM(Mp3_Crc)
{
  MP3CRC_ERROR_SIZE = -2,
  MP3CRC_MISMATCH = -1,
  MP3CRC_NONE = 0,
  MP3CRC_OK = 1
};

ID3_STRUCT(Mp3_Headerinfo)
{
  Mpeg_Layers layer;
  Mpeg_Version version;
  MP3_BitRates bitrate;
  Mp3_ChannelMode channelmode;
  Mp3_ModeExt modeext;
  Mp3_Emphasis emphasis;
  Mp3_Crc crc;
  uint32 vbr_bitrate;           // avg bitrate from xing header
  uint32 frequency;             // samplerate
  uint32 framesize;
  uint32 frames;                // nr of frames
  uint32 time;                  // nr of seconds in song
  bool privatebit;
  bool copyrighted;
  bool original;
};

#define ID3_NR_OF_V1_GENRES 148

static const char *ID3_v1_genre_description[ID3_NR_OF_V1_GENRES] =
{
  "Blues",             //0
  "Classic Rock",      //1
  "Country",           //2
  "Dance",             //3
  "Disco",             //4
  "Funk",              //5
  "Grunge",            //6
  "Hip-Hop",           //7
  "Jazz",              //8
  "Metal",             //9
  "New Age",           //10
  "Oldies",            //11
  "Other",             //12
  "Pop",               //13
  "R&B",               //14
  "Rap",               //15
  "Reggae",            //16
  "Rock",              //17
  "Techno",            //18
  "Industrial",        //19
  "Alternative",       //20
  "Ska",               //21
  "Death Metal",       //22
  "Pranks",            //23
  "Soundtrack",        //24
  "Euro-Techno",       //25
  "Ambient",           //26
  "Trip-Hop",          //27
  "Vocal",             //28
  "Jazz+Funk",         //29
  "Fusion",            //30
  "Trance",            //31
  "Classical",         //32
  "Instrumental",      //33
  "Acid",              //34
  "House",             //35
  "Game",              //36
  "Sound Clip",        //37
  "Gospel",            //38
  "Noise",             //39
  "AlternRock",        //40
  "Bass",              //41
  "Soul",              //42
  "Punk",              //43
  "Space",             //44
  "Meditative",        //45
  "Instrumental Pop",  //46
  "Instrumental Rock", //47
  "Ethnic",            //48
  "Gothic",            //49
  "Darkwave",          //50
  "Techno-Industrial", //51
  "Electronic",        //52
  "Pop-Folk",          //53
  "Eurodance",         //54
  "Dream",             //55
  "Southern Rock",     //56
  "Comedy",            //57
  "Cult",              //58
  "Gangsta",           //59
  "Top 40",            //60
  "Christian Rap",     //61
  "Pop/Funk",          //62
  "Jungle",            //63
  "Native American",   //64
  "Cabaret",           //65
  "New Wave",          //66
  "Psychadelic",       //67
  "Rave",              //68
  "Showtunes",         //69
  "Trailer",           //70
  "Lo-Fi",             //71
  "Tribal",            //72
  "Acid Punk",         //73
  "Acid Jazz",         //74
  "Polka",             //75
  "Retro",             //76
  "Musical",           //77
  "Rock & Roll",       //78
  "Hard Rock",         //79
// following are winamp extentions
  "Folk",                  //80
  "Folk-Rock",             //81
  "National Folk",         //82
  "Swing",                 //83
  "Fast Fusion",           //84
  "Bebob",                 //85
  "Latin",                 //86
  "Revival",               //87
  "Celtic",                //88
  "Bluegrass",             //89
  "Avantgarde",            //90
  "Gothic Rock",           //91
  "Progressive Rock",      //92
  "Psychedelic Rock",      //93
  "Symphonic Rock",        //94
  "Slow Rock",             //95
  "Big Band",              //96
  "Chorus",                //97
  "Easy Listening",        //98
  "Acoustic",              //99
  "Humour",                //100
  "Speech",                //101
  "Chanson",               //102
  "Opera",                 //103
  "Chamber Music",         //104
  "Sonata",                //105
  "Symphony",              //106
  "Booty Bass",            //107
  "Primus",                //108
  "Porn Groove",           //109
  "Satire",                //110
  "Slow Jam",              //111
  "Club",                  //112
  "Tango",                 //113
  "Samba",                 //114
  "Folklore",              //115
  "Ballad",                //116
  "Power Ballad",          //117
  "Rhythmic Soul",         //118
  "Freestyle",             //119
  "Duet",                  //120
  "Punk Rock",             //121
  "Drum Solo",             //122
  "A capella",             //123
  "Euro-House",            //124
  "Dance Hall",            //125
  "Goa",                   //126
  "Drum & Bass",           //127
  "Club-House",            //128
  "Hardcore",              //129
  "Terror",                //130
  "Indie",                 //131
  "Britpop",               //132
  "Negerpunk",             //133
  "Polsk Punk",            //134
  "Beat",                  //135
  "Christian Gangsta Rap", //136
  "Heavy Metal",           //137
  "Black Metal",           //138
  "Crossover",             //139
  "Contemporary Christian",//140
  "Christian Rock ",       //141
  "Merengue",              //142
  "Salsa",                 //143
  "Trash Metal",           //144
  "Anime",                 //145
  "JPop",                  //146
  "Synthpop"               //147
};

#define ID3_V1GENRE2DESCRIPTION(x) (x < ID3_NR_OF_V1_GENRES && x >= 0) ? ID3_v1_genre_description[x] : NULL

#define MASK(bits) ((1 << (bits)) - 1)
#define MASK1 MASK(1)
#define MASK2 MASK(2)
#define MASK3 MASK(3)
#define MASK4 MASK(4)
#define MASK5 MASK(5)
#define MASK6 MASK(6)
#define MASK7 MASK(7)
#define MASK8 MASK(8)

/*
 * The following is borrowed from glib.h (http://www.gtk.org)
 */
#ifdef WIN32

/* On native Win32, directory separator is the backslash, and search path
 * separator is the semicolon.
 */
#  define ID3_DIR_SEPARATOR '\\'
#  define ID3_DIR_SEPARATOR_S "\\"
#  define ID3_SEARCHPATH_SEPARATOR ';'
#  define ID3_SEARCHPATH_SEPARATOR_S ";"

#else  /* !WIN32 */

#  ifndef _EMX_
/* Unix */

#    define ID3_DIR_SEPARATOR '/'
#    define ID3_DIR_SEPARATOR_S "/"
#    define ID3_SEARCHPATH_SEPARATOR ':'
#    define ID3_SEARCHPATH_SEPARATOR_S ":"

#  else
/* EMX/OS2 */

#    define ID3_DIR_SEPARATOR '/'
#    define ID3_DIR_SEPARATOR_S "/"
#    define ID3_SEARCHPATH_SEPARATOR ';'
#    define ID3_SEARCHPATH_SEPARATOR_S ";"

#  endif

#endif /* !WIN32 */

#ifndef NULL
#  define NULL ((void*) 0)
#endif

#endif /* _ID3LIB_GLOBALS_H_ */

