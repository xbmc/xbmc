/*
 * Copyright (C) 2000, 2001 Björn Englund, Håkan Hjort
 *
 * This file is part of libdvdnav, a DVD navigation library. It is a modified
 * file originally part of the Ogle DVD player project.
 * 
 * libdvdnav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * libdvdnav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with libdvdnav; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/*
 * Various useful structs and enums for DVDs.
 */

#ifndef LIBDVDNAV_DVD_TYPES_H
#define LIBDVDNAV_DVD_TYPES_H

/*
 * DVD Menu ID
 * (see dvdnav_menu_call())
 */
typedef enum {
  /* When used in VTS domain, DVD_MENU_Escape behaves like DVD_MENU_Root,
   * but from within a menu domain, DVD_MENU_Escape resumes playback. */
  DVD_MENU_Escape     = 0,
  DVD_MENU_Title      = 2,
  DVD_MENU_Root       = 3,
  DVD_MENU_Subpicture = 4,
  DVD_MENU_Audio      = 5,
  DVD_MENU_Angle      = 6,
  DVD_MENU_Part       = 7
} DVDMenuID_t;


/*
 * Structure containing info on highlight areas
 * (see dvdnav_get_highlight_area())
 */
typedef struct {
  uint32_t palette;     /* The CLUT entries for the highlight palette 
			   (4-bits per entry -> 4 entries) */
  uint16_t sx,sy,ex,ey; /* The start/end x,y positions */
  uint32_t pts;         /* Highlight PTS to match with SPU */

  /* button number for the SPU decoder/overlaying engine */
  uint32_t buttonN;
} dvdnav_highlight_area_t;


/* the following types are currently unused */

//XBMC Needs some of these
#if 1

/* Domain */
typedef enum {
  DVD_DOMAIN_FirstPlay,  /* First Play Domain */
  DVD_DOMAIN_VMG,        /* Video Manager Domain */
  DVD_DOMAIN_VTSMenu,    /* Video Title Set Menu Domain */
  DVD_DOMAIN_VTSTitle,   /* Video Title Set Domain */
  DVD_DOMAIN_Stop        /* Stop Domain */
} DVDDomain_t;

/* User operation permissions */
typedef enum {
  UOP_FLAG_TitleOrTimePlay            = 0x00000001, 
  UOP_FLAG_ChapterSearchOrPlay        = 0x00000002, 
  UOP_FLAG_TitlePlay                  = 0x00000004, 
  UOP_FLAG_Stop                       = 0x00000008,  
  UOP_FLAG_GoUp                       = 0x00000010,
  UOP_FLAG_TimeOrChapterSearch        = 0x00000020, 
  UOP_FLAG_PrevOrTopPGSearch          = 0x00000040,  
  UOP_FLAG_NextPGSearch               = 0x00000080,   
  UOP_FLAG_ForwardScan                = 0x00000100,  
  UOP_FLAG_BackwardScan               = 0x00000200,
  UOP_FLAG_TitleMenuCall              = 0x00000400,
  UOP_FLAG_RootMenuCall               = 0x00000800,
  UOP_FLAG_SubPicMenuCall             = 0x00001000,
  UOP_FLAG_AudioMenuCall              = 0x00002000,
  UOP_FLAG_AngleMenuCall              = 0x00004000,
  UOP_FLAG_ChapterMenuCall            = 0x00008000,
  UOP_FLAG_Resume                     = 0x00010000,
  UOP_FLAG_ButtonSelectOrActivate     = 0x00020000,
  UOP_FLAG_StillOff                   = 0x00040000,
  UOP_FLAG_PauseOn                    = 0x00080000,
  UOP_FLAG_AudioStreamChange          = 0x00100000,
  UOP_FLAG_SubPicStreamChange         = 0x00200000,
  UOP_FLAG_AngleChange                = 0x00400000,
  UOP_FLAG_KaraokeAudioPresModeChange = 0x00800000,
  UOP_FLAG_VideoPresModeChange        = 0x01000000 
} DVDUOP_t;

/* Parental Level */
typedef enum {
  DVD_PARENTAL_LEVEL_1 = 1,
  DVD_PARENTAL_LEVEL_2 = 2,
  DVD_PARENTAL_LEVEL_3 = 3,
  DVD_PARENTAL_LEVEL_4 = 4,
  DVD_PARENTAL_LEVEL_5 = 5,
  DVD_PARENTAL_LEVEL_6 = 6,
  DVD_PARENTAL_LEVEL_7 = 7,
  DVD_PARENTAL_LEVEL_8 = 8,
  DVD_PARENTAL_LEVEL_None = 15
} DVDParentalLevel_t;

/* Language ID (ISO-639 language code) */
typedef uint16_t DVDLangID_t;

/* Country ID (ISO-3166 country code) */
typedef uint16_t DVDCountryID_t;

/* Register */
typedef uint16_t DVDRegister_t;
typedef enum {
  DVDFalse = 0,
  DVDTrue = 1
} DVDBool_t; 
typedef DVDRegister_t DVDGPRMArray_t[16];
typedef DVDRegister_t DVDSPRMArray_t[24];

/* Navigation */
typedef int DVDStream_t;
typedef int DVDPTT_t;
typedef int DVDTitle_t;

/* Angle number (1-9 or default?) */
typedef int DVDAngle_t;

/* Timecode */
typedef struct {
  uint8_t Hours;
  uint8_t Minutes;
  uint8_t Seconds;
  uint8_t Frames;
} DVDTimecode_t;

/* Subpicture stream number (0-31,62,63) */
typedef int DVDSubpictureStream_t;  

/* Audio stream number (0-7, 15(none)) */
typedef int DVDAudioStream_t;  

/* The audio application mode */
typedef enum {
  DVD_AUDIO_APP_MODE_None     = 0,
  DVD_AUDIO_APP_MODE_Karaoke  = 1,
  DVD_AUDIO_APP_MODE_Surround = 2,
  DVD_AUDIO_APP_MODE_Other    = 3
} DVDAudioAppMode_t;

/* The audio format */
typedef enum {
  DVD_AUDIO_FORMAT_AC3        = 0,
  DVD_AUDIO_FORMAT_UNKNOWN_1  = 1,
  DVD_AUDIO_FORMAT_MPEG       = 2,
  DVD_AUDIO_FORMAT_MPEG2_EXT  = 3,
  DVD_AUDIO_FORMAT_LPCM       = 4,
  DVD_AUDIO_FORMAT_UNKNOWN_5  = 5,
  DVD_AUDIO_FORMAT_DTS        = 6,
  DVD_AUDIO_FORMAT_SDDS       = 7
} DVDAudioFormat_t;

/* Audio language extension */
typedef enum {
  DVD_AUDIO_LANG_EXT_NotSpecified       = 0,
  DVD_AUDIO_LANG_EXT_NormalCaptions     = 1,
  DVD_AUDIO_LANG_EXT_VisuallyImpaired   = 2,
  DVD_AUDIO_LANG_EXT_DirectorsComments1 = 3,
  DVD_AUDIO_LANG_EXT_DirectorsComments2 = 4
} DVDAudioLangExt_t;

/* Subpicture language extension */
typedef enum {
  DVD_SUBPICTURE_LANG_EXT_NotSpecified  = 0,
  DVD_SUBPICTURE_LANG_EXT_NormalCaptions  = 1,
  DVD_SUBPICTURE_LANG_EXT_BigCaptions  = 2,
  DVD_SUBPICTURE_LANG_EXT_ChildrensCaptions  = 3,
  DVD_SUBPICTURE_LANG_EXT_NormalCC  = 5,
  DVD_SUBPICTURE_LANG_EXT_BigCC  = 6,
  DVD_SUBPICTURE_LANG_EXT_ChildrensCC  = 7,
  DVD_SUBPICTURE_LANG_EXT_Forced  = 9,
  DVD_SUBPICTURE_LANG_EXT_NormalDirectorsComments  = 13,
  DVD_SUBPICTURE_LANG_EXT_BigDirectorsComments  = 14,
  DVD_SUBPICTURE_LANG_EXT_ChildrensDirectorsComments  = 15,
} DVDSubpictureLangExt_t;  

/* Karaoke Downmix mode */
typedef enum {
  DVD_KARAOKE_DOWNMIX_0to0 = 0x0001,
  DVD_KARAOKE_DOWNMIX_1to0 = 0x0002,
  DVD_KARAOKE_DOWNMIX_2to0 = 0x0004,
  DVD_KARAOKE_DOWNMIX_3to0 = 0x0008,
  DVD_KARAOKE_DOWNMIX_4to0 = 0x0010,
  DVD_KARAOKE_DOWNMIX_Lto0 = 0x0020,
  DVD_KARAOKE_DOWNMIX_Rto0 = 0x0040,
  DVD_KARAOKE_DOWNMIX_0to1 = 0x0100,
  DVD_KARAOKE_DOWNMIX_1to1 = 0x0200,
  DVD_KARAOKE_DOWNMIX_2to1 = 0x0400,
  DVD_KARAOKE_DOWNMIX_3to1 = 0x0800,
  DVD_KARAOKE_DOWNMIX_4to1 = 0x1000,
  DVD_KARAOKE_DOWNMIX_Lto1 = 0x2000,
  DVD_KARAOKE_DOWNMIX_Rto1 = 0x4000
} DVDKaraokeDownmix_t;
typedef int DVDKaraokeDownmixMask_t;

/* Display mode */
typedef enum {
  DVD_DISPLAY_MODE_ContentDefault = 0,
  DVD_DISPLAY_MODE_16x9 = 1,
  DVD_DISPLAY_MODE_4x3PanScan = 2,
  DVD_DISPLAY_MODE_4x3Letterboxed = 3  
} DVDDisplayMode_t;

typedef int DVDAudioSampleFreq_t;
typedef int DVDAudioSampleQuant_t;
typedef int DVDChannelNumber_t;

/* Audio attributes */
typedef struct {
  DVDAudioAppMode_t     AppMode;
  DVDAudioFormat_t      AudioFormat;
  DVDLangID_t           Language;
  DVDAudioLangExt_t     LanguageExtension;
  DVDBool_t             HasMultichannelInfo;
  DVDAudioSampleFreq_t  SampleFrequency;
  DVDAudioSampleQuant_t SampleQuantization;
  DVDChannelNumber_t    NumberOfChannels;
} DVDAudioAttributes_t;

/* Subpicture attributes */
typedef enum {
  DVD_SUBPICTURE_TYPE_NotSpecified = 0,
  DVD_SUBPICTURE_TYPE_Language     = 1,
  DVD_SUBPICTURE_TYPE_Other        = 2
} DVDSubpictureType_t;
typedef enum {
  DVD_SUBPICTURE_CODING_RunLength = 0,
  DVD_SUBPICTURE_CODING_Extended  = 1,
  DVD_SUBPICTURE_CODING_Other     = 2
} DVDSubpictureCoding_t;
typedef struct {
  DVDSubpictureType_t    Type;
  DVDSubpictureCoding_t  CodingMode;
  DVDLangID_t            Language;
  DVDSubpictureLangExt_t LanguageExtension;
} DVDSubpictureAttributes_t;

typedef int DVDVideoCompression_t;

/* Video attributes */
typedef struct {
  DVDBool_t PanscanPermitted;
  DVDBool_t LetterboxPermitted;
  int AspectX;
  int AspectY;
  int FrameRate;
  int FrameHeight;
  DVDVideoCompression_t Compression;
  DVDBool_t Line21Field1InGop;
  DVDBool_t Line21Field2InGop;
  int more_to_come;
} DVDVideoAttributes_t;

#endif

#endif /* LIBDVDNAV_DVD_TYPES_H */
