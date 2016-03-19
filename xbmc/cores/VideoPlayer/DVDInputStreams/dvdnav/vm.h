/*
 * Copyright (C) 2000, 2001 Håkan Hjort
 * Copyright (C) 2001 Rich Wareham <richwareham@users.sourceforge.net>
 * 
 * This file is part of libdvdnav, a DVD navigation library. It is modified
 * from a file originally part of the Ogle DVD player.
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

#ifndef LIBDVDNAV_VM_H
#define LIBDVDNAV_VM_H

#include "remap.h"
#include "dvdnav_internal.h"

/* DOMAIN enum */

typedef enum {
  FP_DOMAIN   = 1,
  VTS_DOMAIN  = 2,
  VMGM_DOMAIN = 4,
  VTSM_DOMAIN = 8
} domain_t;  

/**
 * State: SPRM, GPRM, Domain, pgc, pgN, cellN, ?
 */
typedef struct {
  registers_t registers;
  
  domain_t  domain;
  int       vtsN;         /* 0 is vmgm? */
  pgc_t    *pgc;          /* either this or 'int pgcN' is enough? */
  int       pgcN;         /* but provide pgcN for quick lookup */
  int       pgN;          /* is this needed? can allways fid pgN from cellN? */
  int       cellN;
  int32_t   cell_restart; /* get cell to restart */
  int       blockN;
  
  /* Resume info */
  int      rsm_vtsN;
  int      rsm_blockN;    /* of nav_packet */
  uint16_t rsm_regs[5];   /* system registers 4-8 */
  int      rsm_pgcN;
  int      rsm_cellN;
} dvd_state_t;

typedef struct vm_position_s {
  int16_t  button;        /* Button highlighted */
  int32_t  vts;           /* vts number to use */
  domain_t domain;        /* domain to use */
  int32_t  spu_channel;   /* spu channel to use */
  int32_t  angle_channel; /* angle channel to use */
  int32_t  audio_channel; /* audio channel to use */
  int32_t  hop_channel;   /* channel hopping. E.g menu button pressed */
#if 0
  /* currently unused */
  int32_t  title;         /* title number */
  int32_t  chapter;       /* chapter number */
#endif
  int32_t  cell;          /* cell number */
  int32_t  cell_restart;  /* get cell to restart */
  int32_t  cell_start;    /* sector number of start of current cell in use */
  int32_t  still;         /* is cell still */
  int32_t  block;         /* block number within cell in use */
} vm_position_t;

typedef struct {
  dvd_reader_t *dvd;
  ifo_handle_t *vmgi;
  ifo_handle_t *vtsi;
  dvd_state_t   state;
  int32_t       hop_channel;
  char          dvd_name[50];
  char          dvd_serial[15];
  remap_t      *map;
  int           stopped;
} vm_t;

/* magic number for seeking hops */
#define HOP_SEEK 0x1000


/*  Audio stream number */
#define AST_REG      registers.SPRM[1]
/*  Subpicture stream number */
#define SPST_REG     registers.SPRM[2]
/*  Angle number */
#define AGL_REG      registers.SPRM[3]
/*  Title Track Number */
#define TTN_REG      registers.SPRM[4]
/*  VTS Title Track Number */
#define VTS_TTN_REG  registers.SPRM[5]
/*  PGC Number for this Title Track */
#define TT_PGCN_REG  registers.SPRM[6]
/*  Current Part of Title (PTT) number for (One_Sequential_PGC_Title) */
#define PTTN_REG     registers.SPRM[7]
/*  Highlighted Button Number (btn nr 1 == value 1024) */
#define HL_BTNN_REG  registers.SPRM[8]
/*  Parental Level */
#define PTL_REG      registers.SPRM[13]

/* Initialisation & destruction */
vm_t *vm_new_vm(void);
void  vm_free_vm(vm_t *vm);

/* IFO access */
ifo_handle_t *vm_get_vmgi(vm_t *vm);
ifo_handle_t *vm_get_vtsi(vm_t *vm);

/* Reader Access */
dvd_reader_t *vm_get_dvd_reader(vm_t *vm);

/* Basic Handling */
int  vm_start(vm_t *vm);
void vm_stop(vm_t *vm);
int  vm_reset(vm_t *vm, const char *dvdroot);

/* copying and merging  - useful for try-running an operation */
vm_t *vm_new_copy(vm_t *vm);
void  vm_merge(vm_t *target, vm_t *source);
void  vm_free_copy(vm_t *vm);

/* regular playback */
void vm_position_get(vm_t *vm, vm_position_t *position);
void vm_get_next_cell(vm_t *vm);

/* Jumping - all these return 1, if a hop has been performed */
int vm_jump_pg(vm_t *vm, int pg);
int vm_jump_cell_block(vm_t *vm, int cell, int block);
int vm_jump_title_part(vm_t *vm, int title, int part);
int vm_jump_title_program(vm_t *vm, int title, int pgcn, int pgn);
int vm_jump_top_pg(vm_t *vm);
int vm_jump_next_pg(vm_t *vm);
int vm_jump_prev_pg(vm_t *vm);
int vm_jump_up(vm_t *vm);
int vm_jump_menu(vm_t *vm, DVDMenuID_t menuid);
int vm_jump_resume(vm_t *vm);
int vm_exec_cmd(vm_t *vm, vm_cmd_t *cmd);

/* getting information */
int vm_get_current_menu(vm_t *vm, int *menuid);
int vm_get_current_title_part(vm_t *vm, int *title_result, int *part_result);
int vm_get_audio_stream(vm_t *vm, int audioN);
int vm_get_subp_stream(vm_t *vm, int subpN, int mode);
int vm_get_audio_active_stream(vm_t *vm);
int vm_get_subp_active_stream(vm_t *vm, int mode);
void vm_get_angle_info(vm_t *vm, int *current, int *num_avail);
#if 0
/* currently unused */
void vm_get_audio_info(vm_t *vm, int *current, int *num_avail);
void vm_get_subp_info(vm_t *vm, int *current, int *num_avail);
#endif
void vm_get_video_res(vm_t *vm, int *width, int *height);
int  vm_get_video_aspect(vm_t *vm);
int  vm_get_video_scale_permission(vm_t *vm);
video_attr_t vm_get_video_attr(vm_t *vm);
audio_attr_t vm_get_audio_attr(vm_t *vm, int streamN);
subp_attr_t  vm_get_subp_attr(vm_t *vm, int streamN);
ifo_handle_t *vm_get_title_ifo(vm_t *vm, uint32_t title);
void vm_ifo_close(ifo_handle_t *ifo);

int vm_get_state(vm_t *vm, dvd_state_t *save_state);
int vm_set_state(vm_t *vm, dvd_state_t *save_state);

/* Uncomment for VM command tracing */
/* #define TRACE */
#ifdef TRACE
/* Debug */
void vm_position_print(vm_t *vm, vm_position_t *position);
#endif

/* XBMC added functions */
vm_t* dvdnav_get_vm(dvdnav_t *self);

#endif /* LIBDVDNAV_VM_H */
