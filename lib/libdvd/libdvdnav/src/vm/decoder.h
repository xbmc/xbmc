/*
 * Copyright (C) 2000, 2001 Martin Norbäck, Håkan Hjort
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

#ifndef LIBDVDNAV_DECODER_H
#define LIBDVDNAV_DECODER_H

/* link command types */
typedef enum {
  LinkNoLink  = 0,

  LinkTopC    = 1,
  LinkNextC   = 2,
  LinkPrevC   = 3,

  LinkTopPG   = 5,
  LinkNextPG  = 6,
  LinkPrevPG  = 7,

  LinkTopPGC  = 9,
  LinkNextPGC = 10,
  LinkPrevPGC = 11,
  LinkGoUpPGC = 12,
  LinkTailPGC = 13,

  LinkRSM     = 16,

  LinkPGCN,
  LinkPTTN,
  LinkPGN,
  LinkCN,

  Exit,

  JumpTT, /* 22 */
  JumpVTS_TT,
  JumpVTS_PTT,

  JumpSS_FP,
  JumpSS_VMGM_MENU,
  JumpSS_VTSM,
  JumpSS_VMGM_PGC,

  CallSS_FP, /* 29 */
  CallSS_VMGM_MENU,
  CallSS_VTSM,
  CallSS_VMGM_PGC,

  PlayThis
} link_cmd_t;

/* a link's data set */
typedef struct {
  link_cmd_t command;
  uint16_t   data1;
  uint16_t   data2;
  uint16_t   data3;
} link_t;

/* the VM registers */
typedef struct {
  uint16_t SPRM[24];
  uint16_t GPRM[16];
  uint8_t  GPRM_mode[16];  /* Need to have some thing to indicate normal/counter mode for every GPRM */
  struct timeval GPRM_time[16]; /* For counter mode */
} registers_t;

/* a VM command data set */
typedef struct {
  uint64_t instruction;
  uint64_t examined;
  registers_t *registers;
} command_t;

/* the big VM function, executing the given commands and writing
 * the link where to continue, the return value indicates if a jump
 * has been performed */
int32_t vmEval_CMD(vm_cmd_t commands[], int32_t num_commands,
	       registers_t *registers, link_t *return_values);

/* extracts some bits from the command */
uint32_t vm_getbits(command_t* command, int32_t start, int32_t count);

#ifdef TRACE
/* for debugging: prints a link in readable form */
void vm_print_link(link_t value);

/* for debugging: dumps VM registers */
void vm_print_registers( registers_t *registers );
#endif

#endif /* LIBDVDNAV_DECODER_H */
