/*
 * Copyright (C) 2000, 2001 Björn Englund <d4bjorn@dtek.chalmers.se>,
 *                          Håkan Hjort <d95hjort@dtek.chalmers.se>
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

#pragma once

//#include <inttypes.h>
#include "dvd_reader.h"


#undef ATTRIBUTE_PACKED

#if defined(__GNUC__)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
#if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)) && !defined(__clang__)
#define ATTRIBUTE_PACKED __attribute__((packed, gcc_struct))
#else
#define ATTRIBUTE_PACKED __attribute__((packed))
#endif
#define PRAGMA_PACK 0
#endif
#endif

#if !defined(ATTRIBUTE_PACKED)
#define ATTRIBUTE_PACKED
#define PRAGMA_PACK 1
#endif

#if PRAGMA_PACK
#pragma pack(1)
#endif


/**
 * Common
 *
 * The following structures are used in both the VMGI and VTSI.
 */


/**
 * DVD Time Information.
 */
typedef struct {
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t frame_u; /* The two high bits are the frame rate. */
} ATTRIBUTE_PACKED dvd_time_t;

/**
 * Type to store per-command data.
 */
typedef struct {
  uint8_t bytes[8];
} ATTRIBUTE_PACKED vm_cmd_t;
#define COMMAND_DATA_SIZE 8U


/**
 * Video Attributes.
 */
typedef struct {
  unsigned char mpeg_version         : 2;
  unsigned char video_format         : 2;
  unsigned char display_aspect_ratio : 2;
  unsigned char permitted_df         : 2;

  unsigned char line21_cc_1          : 1;
  unsigned char line21_cc_2          : 1;
  unsigned char unknown1             : 1;
  unsigned char bit_rate             : 1;

  unsigned char picture_size         : 2;
  unsigned char letterboxed          : 1;
  unsigned char film_mode            : 1;
} ATTRIBUTE_PACKED video_attr_t;

/**
 * Audio Attributes.
 */
typedef struct {
  unsigned char audio_format           : 3;
  unsigned char multichannel_extension : 1;
  unsigned char lang_type              : 2;
  unsigned char application_mode       : 2;

  unsigned char quantization           : 2;
  unsigned char sample_frequency       : 2;
  unsigned char unknown1               : 1;
  unsigned char channels               : 3;
  uint16_t lang_code;
  uint8_t  lang_extension;
  uint8_t  code_extension;
  uint8_t unknown3;
  union {
    struct ATTRIBUTE_PACKED {
      unsigned char unknown4           : 1;
      unsigned char channel_assignment : 3;
      unsigned char version            : 2;
      unsigned char mc_intro           : 1; /* probably 0: true, 1:false */
      unsigned char mode               : 1; /* Karaoke mode 0: solo 1: duet */
    } karaoke;
    struct ATTRIBUTE_PACKED {
      unsigned char unknown5           : 4;
      unsigned char dolby_encoded      : 1; /* suitable for surround decoding */
      unsigned char unknown6           : 3;
    } surround;
  } ATTRIBUTE_PACKED app_info;
} ATTRIBUTE_PACKED audio_attr_t;


/**
 * MultiChannel Extension
 */
typedef struct {
  unsigned char zero1 : 7;
  unsigned char ach0_gme : 1;

  unsigned char zero2 : 7;
  unsigned char ach1_gme : 1;

  unsigned char zero3 : 4;
  unsigned char ach2_gv1e : 1;
  unsigned char ach2_gv2e : 1;
  unsigned char ach2_gm1e : 1;
  unsigned char ach2_gm2e : 1;

  unsigned char zero4 : 4;
  unsigned char ach3_gv1e : 1;
  unsigned char ach3_gv2e : 1;
  unsigned char ach3_gmAe : 1;
  unsigned char ach3_se2e : 1;

  unsigned char zero5 : 4;
  unsigned char ach4_gv1e : 1;
  unsigned char ach4_gv2e : 1;
  unsigned char ach4_gmBe : 1;
  unsigned char ach4_seBe : 1;
  uint8_t zero6[19];
} ATTRIBUTE_PACKED multichannel_ext_t;


/**
 * Subpicture Attributes.
 */
typedef struct {
  /*
   * type: 0 not specified
   *       1 language
   *       2 other
   * coding mode: 0 run length
   *              1 extended
   *              2 other
   * language: indicates language if type == 1
   * lang extension: if type == 1 contains the lang extension
   */
  unsigned char code_mode : 3;
  unsigned char zero1     : 3;
  unsigned char type      : 2;
  uint8_t  zero2;
  uint16_t lang_code;
  uint8_t  lang_extension;
  uint8_t  code_extension;
} ATTRIBUTE_PACKED subp_attr_t;



/**
 * PGC Command Table.
 */
typedef struct {
  uint16_t nr_of_pre;
  uint16_t nr_of_post;
  uint16_t nr_of_cell;
  uint16_t last_byte;
  vm_cmd_t *pre_cmds;
  vm_cmd_t *post_cmds;
  vm_cmd_t *cell_cmds;
} ATTRIBUTE_PACKED pgc_command_tbl_t;
#define PGC_COMMAND_TBL_SIZE 8U

/**
 * PGC Program Map
 */
typedef uint8_t pgc_program_map_t;

/**
 * Cell Playback Information.
 */
typedef struct {
  unsigned char block_mode : 2;
  unsigned char block_type : 2;
  unsigned char seamless_play : 1;
  unsigned char interleaved : 1;
  unsigned char stc_discontinuity : 1;
  unsigned char seamless_angle : 1;
  unsigned char zero_1 : 1;
  unsigned char playback_mode : 1; /**< When set, enter StillMode after each VOBU */
  unsigned char restricted : 1; /**< ?? drop out of fastforward? */
  unsigned char cell_type : 5; /** for karaoke, reserved otherwise */
  uint8_t still_time;
  uint8_t cell_cmd_nr;
  dvd_time_t playback_time;
  uint32_t first_sector;
  uint32_t first_ilvu_end_sector;
  uint32_t last_vobu_start_sector;
  uint32_t last_sector;
} ATTRIBUTE_PACKED cell_playback_t;

#define BLOCK_TYPE_NONE         0x0
#define BLOCK_TYPE_ANGLE_BLOCK  0x1

#define BLOCK_MODE_NOT_IN_BLOCK 0x0
#define BLOCK_MODE_FIRST_CELL   0x1
#define BLOCK_MODE_IN_BLOCK     0x2
#define BLOCK_MODE_LAST_CELL    0x3

/**
 * Cell Position Information.
 */
typedef struct {
  uint16_t vob_id_nr;
  uint8_t  zero_1;
  uint8_t  cell_nr;
} ATTRIBUTE_PACKED cell_position_t;

/**
 * User Operations.
 */
typedef struct {
  unsigned char zero : 7; /* 25-31 */
  unsigned char video_pres_mode_change : 1; /* 24 */

  unsigned char karaoke_audio_pres_mode_change : 1; /* 23 */
  unsigned char angle_change : 1;
  unsigned char subpic_stream_change : 1;
  unsigned char audio_stream_change : 1;
  unsigned char pause_on : 1;
  unsigned char still_off : 1;
  unsigned char button_select_or_activate : 1;
  unsigned char resume : 1; /* 16 */

  unsigned char chapter_menu_call : 1; /* 15 */
  unsigned char angle_menu_call : 1;
  unsigned char audio_menu_call : 1;
  unsigned char subpic_menu_call : 1;
  unsigned char root_menu_call : 1;
  unsigned char title_menu_call : 1;
  unsigned char backward_scan : 1;
  unsigned char forward_scan : 1; /* 8 */

  unsigned char next_pg_search : 1; /* 7 */
  unsigned char prev_or_top_pg_search : 1;
  unsigned char time_or_chapter_search : 1;
  unsigned char go_up : 1;
  unsigned char stop : 1;
  unsigned char title_play : 1;
  unsigned char chapter_search_or_play : 1;
  unsigned char title_or_time_play : 1; /* 0 */
} ATTRIBUTE_PACKED user_ops_t;

/**
 * Program Chain Information.
 */
typedef struct {
  uint16_t zero_1;
  uint8_t  nr_of_programs;
  uint8_t  nr_of_cells;
  dvd_time_t playback_time;
  user_ops_t prohibited_ops;
  uint16_t audio_control[8]; /* New type? */
  uint32_t subp_control[32]; /* New type? */
  uint16_t next_pgc_nr;
  uint16_t prev_pgc_nr;
  uint16_t goup_pgc_nr;
  uint8_t  pg_playback_mode;
  uint8_t still_time;
  uint32_t palette[16]; /* New type struct {zero_1, Y, Cr, Cb} ? */
  uint16_t command_tbl_offset;
  uint16_t program_map_offset;
  uint16_t cell_playback_offset;
  uint16_t cell_position_offset;
  pgc_command_tbl_t *command_tbl;
  pgc_program_map_t  *program_map;
  cell_playback_t *cell_playback;
  cell_position_t *cell_position;
  int      ref_count;
} ATTRIBUTE_PACKED pgc_t;
#define PGC_SIZE 236U

/**
 * Program Chain Information Search Pointer.
 */
typedef struct {
  uint8_t  entry_id;
  unsigned char block_mode : 2;
  unsigned char block_type : 2;
  unsigned char zero_1 : 4;
  uint16_t ptl_id_mask;
  uint32_t pgc_start_byte;
  pgc_t *pgc;
} ATTRIBUTE_PACKED pgci_srp_t;
#define PGCI_SRP_SIZE 8U

/**
 * Program Chain Information Table.
 */
typedef struct {
  uint16_t nr_of_pgci_srp;
  uint16_t zero_1;
  uint32_t last_byte;
  pgci_srp_t *pgci_srp;
  int      ref_count;
} ATTRIBUTE_PACKED pgcit_t;
#define PGCIT_SIZE 8U

/**
 * Menu PGCI Language Unit.
 */
typedef struct {
  uint16_t lang_code;
  uint8_t  lang_extension;
  uint8_t  exists;
  uint32_t lang_start_byte;
  pgcit_t *pgcit;
} ATTRIBUTE_PACKED pgci_lu_t;
#define PGCI_LU_SIZE 8U

/**
 * Menu PGCI Unit Table.
 */
typedef struct {
  uint16_t nr_of_lus;
  uint16_t zero_1;
  uint32_t last_byte;
  pgci_lu_t *lu;
} ATTRIBUTE_PACKED pgci_ut_t;
#define PGCI_UT_SIZE 8U

/**
 * Cell Address Information.
 */
typedef struct {
  uint16_t vob_id;
  uint8_t  cell_id;
  uint8_t  zero_1;
  uint32_t start_sector;
  uint32_t last_sector;
} ATTRIBUTE_PACKED cell_adr_t;

/**
 * Cell Address Table.
 */
typedef struct {
  uint16_t nr_of_vobs; /* VOBs */
  uint16_t zero_1;
  uint32_t last_byte;
  cell_adr_t *cell_adr_table;  /* No explicit size given. */
} ATTRIBUTE_PACKED c_adt_t;
#define C_ADT_SIZE 8U

/**
 * VOBU Address Map.
 */
typedef struct {
  uint32_t last_byte;
  uint32_t *vobu_start_sectors;
} ATTRIBUTE_PACKED vobu_admap_t;
#define VOBU_ADMAP_SIZE 4U




/**
 * VMGI
 *
 * The following structures relate to the Video Manager.
 */

/**
 * Video Manager Information Management Table.
 */
typedef struct {
  char     vmg_identifier[12];
  uint32_t vmg_last_sector;
  uint8_t  zero_1[12];
  uint32_t vmgi_last_sector;
  uint8_t  zero_2;
  uint8_t  specification_version;
  uint32_t vmg_category;
  uint16_t vmg_nr_of_volumes;
  uint16_t vmg_this_volume_nr;
  uint8_t  disc_side;
  uint8_t  zero_3[19];
  uint16_t vmg_nr_of_title_sets;  /* Number of VTSs. */
  char     provider_identifier[32];
  uint64_t vmg_pos_code;
  uint8_t  zero_4[24];
  uint32_t vmgi_last_byte;
  uint32_t first_play_pgc;
  uint8_t  zero_5[56];
  uint32_t vmgm_vobs;             /* sector */
  uint32_t tt_srpt;               /* sector */
  uint32_t vmgm_pgci_ut;          /* sector */
  uint32_t ptl_mait;              /* sector */
  uint32_t vts_atrt;              /* sector */
  uint32_t txtdt_mgi;             /* sector */
  uint32_t vmgm_c_adt;            /* sector */
  uint32_t vmgm_vobu_admap;       /* sector */
  uint8_t  zero_6[32];

  video_attr_t vmgm_video_attr;
  uint8_t  zero_7;
  uint8_t  nr_of_vmgm_audio_streams; /* should be 0 or 1 */
  audio_attr_t vmgm_audio_attr;
  audio_attr_t zero_8[7];
  uint8_t  zero_9[17];
  uint8_t  nr_of_vmgm_subp_streams; /* should be 0 or 1 */
  subp_attr_t  vmgm_subp_attr;
  subp_attr_t  zero_10[27];  /* XXX: how much 'padding' here? */
} ATTRIBUTE_PACKED vmgi_mat_t;

typedef struct {
  unsigned char zero_1 : 1;
  unsigned char multi_or_random_pgc_title : 1; /* 0: one sequential pgc title */
  unsigned char jlc_exists_in_cell_cmd : 1;
  unsigned char jlc_exists_in_prepost_cmd : 1;
  unsigned char jlc_exists_in_button_cmd : 1;
  unsigned char jlc_exists_in_tt_dom : 1;
  unsigned char chapter_search_or_play : 1; /* UOP 1 */
  unsigned char title_or_time_play : 1; /* UOP 0 */
} ATTRIBUTE_PACKED playback_type_t;

/**
 * Title Information.
 */
typedef struct {
  playback_type_t pb_ty;
  uint8_t  nr_of_angles;
  uint16_t nr_of_ptts;
  uint16_t parental_id;
  uint8_t  title_set_nr;
  uint8_t  vts_ttn;
  uint32_t title_set_sector;
} ATTRIBUTE_PACKED title_info_t;

/**
 * PartOfTitle Search Pointer Table.
 */
typedef struct {
  uint16_t nr_of_srpts;
  uint16_t zero_1;
  uint32_t last_byte;
  title_info_t *title;
} ATTRIBUTE_PACKED tt_srpt_t;
#define TT_SRPT_SIZE 8U


/**
 * Parental Management Information Unit Table.
 * Level 1 (US: G), ..., 7 (US: NC-17), 8
 */
#define PTL_MAIT_NUM_LEVEL 8
typedef uint16_t pf_level_t[PTL_MAIT_NUM_LEVEL];

/**
 * Parental Management Information Unit Table.
 */
typedef struct {
  uint16_t country_code;
  uint16_t zero_1;
  uint16_t pf_ptl_mai_start_byte;
  uint16_t zero_2;
  pf_level_t *pf_ptl_mai; /* table of (nr_of_vtss + 1), video_ts is first */
} ATTRIBUTE_PACKED ptl_mait_country_t;
#define PTL_MAIT_COUNTRY_SIZE 8U

/**
 * Parental Management Information Table.
 */
typedef struct {
  uint16_t nr_of_countries;
  uint16_t nr_of_vtss;
  uint32_t last_byte;
  ptl_mait_country_t *countries;
} ATTRIBUTE_PACKED ptl_mait_t;
#define PTL_MAIT_SIZE 8U

/**
 * Video Title Set Attributes.
 */
typedef struct {
  uint32_t last_byte;
  uint32_t vts_cat;

  video_attr_t vtsm_vobs_attr;
  uint8_t  zero_1;
  uint8_t  nr_of_vtsm_audio_streams; /* should be 0 or 1 */
  audio_attr_t vtsm_audio_attr;
  audio_attr_t zero_2[7];
  uint8_t  zero_3[16];
  uint8_t  zero_4;
  uint8_t  nr_of_vtsm_subp_streams; /* should be 0 or 1 */
  subp_attr_t vtsm_subp_attr;
  subp_attr_t zero_5[27];

  uint8_t  zero_6[2];

  video_attr_t vtstt_vobs_video_attr;
  uint8_t  zero_7;
  uint8_t  nr_of_vtstt_audio_streams;
  audio_attr_t vtstt_audio_attr[8];
  uint8_t  zero_8[16];
  uint8_t  zero_9;
  uint8_t  nr_of_vtstt_subp_streams;
  subp_attr_t vtstt_subp_attr[32];
} ATTRIBUTE_PACKED vts_attributes_t;
#define VTS_ATTRIBUTES_SIZE 542U
#define VTS_ATTRIBUTES_MIN_SIZE 356U

/**
 * Video Title Set Attribute Table.
 */
typedef struct {
  uint16_t nr_of_vtss;
  uint16_t zero_1;
  uint32_t last_byte;
  vts_attributes_t *vts;
  uint32_t *vts_atrt_offsets; /* offsets table for each vts_attributes */
} ATTRIBUTE_PACKED vts_atrt_t;
#define VTS_ATRT_SIZE 8U

/**
 * Text Data. (Incomplete)
 */
typedef struct {
  uint32_t last_byte;    /* offsets are relative here */
  uint16_t offsets[100]; /* == nr_of_srpts + 1 (first is disc title) */
#if 0
  uint16_t unknown; /* 0x48 ?? 0x48 words (16bit) info following */
  uint16_t zero_1;

  uint8_t type_of_info; /* ?? 01 == disc, 02 == Title, 04 == Title part */
  uint8_t unknown1;
  uint8_t unknown2;
  uint8_t unknown3;
  uint8_t unknown4; /* ?? always 0x30 language?, text format? */
  uint8_t unknown5;
  uint16_t offset; /* from first */

  char text[12]; /* ended by 0x09 */
#endif
} ATTRIBUTE_PACKED txtdt_t;

/**
 * Text Data Language Unit. (Incomplete)
 */
typedef struct {
  uint16_t lang_code;
  uint8_t zero_1;
  uint8_t
      char_set; /* 0x00 reserved Unicode, 0x01 ISO 646, 0x10 JIS Roman & JIS Kanji, 0x11 ISO 8859-1, 0x12 Shift JIS Kanji */
  uint32_t txtdt_start_byte;  /* prt, rel start of vmg_txtdt_mgi  */
  txtdt_t  *txtdt;
} ATTRIBUTE_PACKED txtdt_lu_t;
#define TXTDT_LU_SIZE 8U

/**
 * Text Data Manager Information. (Incomplete)
 */
typedef struct {
  char disc_name[12];
  uint16_t unknown1;
  uint16_t nr_of_language_units;
  uint32_t last_byte;
  txtdt_lu_t *lu;
} ATTRIBUTE_PACKED txtdt_mgi_t;
#define TXTDT_MGI_SIZE 20U


/**
 * VTS
 *
 * Structures relating to the Video Title Set (VTS).
 */

/**
 * Video Title Set Information Management Table.
 */
typedef struct {
  char vts_identifier[12];
  uint32_t vts_last_sector;
  uint8_t  zero_1[12];
  uint32_t vtsi_last_sector;
  uint8_t  zero_2;
  uint8_t  specification_version;
  uint32_t vts_category;
  uint16_t zero_3;
  uint16_t zero_4;
  uint8_t  zero_5;
  uint8_t  zero_6[19];
  uint16_t zero_7;
  uint8_t  zero_8[32];
  uint64_t zero_9;
  uint8_t  zero_10[24];
  uint32_t vtsi_last_byte;
  uint32_t zero_11;
  uint8_t  zero_12[56];
  uint32_t vtsm_vobs;       /* sector */
  uint32_t vtstt_vobs;      /* sector */
  uint32_t vts_ptt_srpt;    /* sector */
  uint32_t vts_pgcit;       /* sector */
  uint32_t vtsm_pgci_ut;    /* sector */
  uint32_t vts_tmapt;       /* sector */
  uint32_t vtsm_c_adt;      /* sector */
  uint32_t vtsm_vobu_admap; /* sector */
  uint32_t vts_c_adt;       /* sector */
  uint32_t vts_vobu_admap;  /* sector */
  uint8_t  zero_13[24];

  video_attr_t vtsm_video_attr;
  uint8_t  zero_14;
  uint8_t  nr_of_vtsm_audio_streams; /* should be 0 or 1 */
  audio_attr_t vtsm_audio_attr;
  audio_attr_t zero_15[7];
  uint8_t  zero_16[17];
  uint8_t  nr_of_vtsm_subp_streams; /* should be 0 or 1 */
  subp_attr_t vtsm_subp_attr;
  subp_attr_t zero_17[27];
  uint8_t  zero_18[2];

  video_attr_t vts_video_attr;
  uint8_t  zero_19;
  uint8_t  nr_of_vts_audio_streams;
  audio_attr_t vts_audio_attr[8];
  uint8_t  zero_20[17];
  uint8_t  nr_of_vts_subp_streams;
  subp_attr_t vts_subp_attr[32];
  uint16_t zero_21;
  multichannel_ext_t vts_mu_audio_attr[8];
  /* XXX: how much 'padding' here, if any? */
} ATTRIBUTE_PACKED vtsi_mat_t;

/**
 * PartOfTitle Unit Information.
 */
typedef struct {
  uint16_t pgcn;
  uint16_t pgn;
} ATTRIBUTE_PACKED ptt_info_t;

/**
 * PartOfTitle Information.
 */
typedef struct {
  uint16_t nr_of_ptts;
  ptt_info_t *ptt;
} ATTRIBUTE_PACKED ttu_t;

/**
 * PartOfTitle Search Pointer Table.
 */
typedef struct {
  uint16_t nr_of_srpts;
  uint16_t zero_1;
  uint32_t last_byte;
  ttu_t  *title;
  uint32_t *ttu_offset; /* offset table for each ttu */
} ATTRIBUTE_PACKED vts_ptt_srpt_t;
#define VTS_PTT_SRPT_SIZE 8U


/**
 * Time Map Entry.
 */
/* Should this be bit field at all or just the uint32_t? */
typedef uint32_t map_ent_t;

/**
 * Time Map.
 */
typedef struct {
  uint8_t  tmu;   /* Time unit, in seconds */
  uint8_t  zero_1;
  uint16_t nr_of_entries;
  map_ent_t *map_ent;
} ATTRIBUTE_PACKED vts_tmap_t;
#define VTS_TMAP_SIZE 4U

/**
 * Time Map Table.
 */
typedef struct {
  uint16_t nr_of_tmaps;
  uint16_t zero_1;
  uint32_t last_byte;
  vts_tmap_t *tmap;
  uint32_t *tmap_offset; /* offset table for each tmap */
} ATTRIBUTE_PACKED vts_tmapt_t;
#define VTS_TMAPT_SIZE 8U


#if PRAGMA_PACK
#pragma pack()
#endif


/**
 * The following structure defines an IFO file.  The structure is divided into
 * two parts, the VMGI, or Video Manager Information, which is read from the
 * VIDEO_TS.[IFO,BUP] file, and the VTSI, or Video Title Set Information, which
 * is read in from the VTS_XX_0.[IFO,BUP] files.
 */
typedef struct {
  /* VMGI */
  vmgi_mat_t     *vmgi_mat;
  tt_srpt_t      *tt_srpt;
  pgc_t          *first_play_pgc;
  ptl_mait_t     *ptl_mait;
  vts_atrt_t     *vts_atrt;
  txtdt_mgi_t    *txtdt_mgi;

  /* Common */
  pgci_ut_t      *pgci_ut;
  c_adt_t        *menu_c_adt;
  vobu_admap_t   *menu_vobu_admap;

  /* VTSI */
  vtsi_mat_t     *vtsi_mat;
  vts_ptt_srpt_t *vts_ptt_srpt;
  pgcit_t        *vts_pgcit;
  vts_tmapt_t    *vts_tmapt;
  c_adt_t        *vts_c_adt;
  vobu_admap_t   *vts_vobu_admap;
} ifo_handle_t;

