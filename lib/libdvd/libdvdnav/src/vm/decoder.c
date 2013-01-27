/*
 * Copyright (C) 2000, 2001 Martin Norbäck, Håkan Hjort
 *               2002-2004 the dvdnav project
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>  /* For memset */
#include <sys/time.h>
#include <dvdread/nav_types.h>
#include <dvdread/ifo_types.h> /* vm_cmd_t */

#include "dvdnav/dvdnav.h"
#include "remap.h"
#include "decoder.h"
#include "vm.h"
#include "vmcmd.h"
#include "dvdnav_internal.h"

uint32_t vm_getbits(command_t *command, int32_t start, int32_t count) {
  uint64_t result = 0;
  uint64_t bit_mask = 0;
  uint64_t examining = 0;
  int32_t  bits;

  if (count == 0) return 0;

  if ( ((start - count) < -1) ||
       (count > 32) ||
       (start > 63) ||
       (count < 0) ||
       (start < 0) ) {
    fprintf(MSG_OUT, "libdvdnav: Bad call to vm_getbits. Parameter out of range\n");
    abort();
  }
  /* all ones, please */
  bit_mask = ~bit_mask;
  bit_mask >>= 63 - start;
  bits = start + 1 - count;
  examining = ((bit_mask >> bits) << bits );
  command->examined |= examining;
  result = (command->instruction & bit_mask) >> bits;
  return (uint32_t) result;
}

static uint16_t get_GPRM(registers_t* registers, uint8_t reg) {
  if (registers->GPRM_mode[reg] & 0x01) {
    struct timeval current_time, time_offset;
    uint16_t result;
    /* Counter mode */
    /* fprintf(MSG_OUT, "libdvdnav: Getting counter %d\n",reg);*/
    gettimeofday(&current_time, NULL);
    time_offset.tv_sec = current_time.tv_sec - registers->GPRM_time[reg].tv_sec;
    time_offset.tv_usec = current_time.tv_usec - registers->GPRM_time[reg].tv_usec;
    if (time_offset.tv_usec < 0) {
      time_offset.tv_sec--;
      time_offset.tv_usec += 1000000;
    }
    result = (uint16_t) (time_offset.tv_sec & 0xffff);
    registers->GPRM[reg]=result;
    return result;

  } else {
    /* Register mode */
    return registers->GPRM[reg];
  }

}

static void set_GPRM(registers_t* registers, uint8_t reg, uint16_t value) {
  if (registers->GPRM_mode[reg] & 0x01) {
    struct timeval current_time;
    /* Counter mode */
    /* fprintf(MSG_OUT, "libdvdnav: Setting counter %d\n",reg); */
    gettimeofday(&current_time, NULL);
    registers->GPRM_time[reg] = current_time;
    registers->GPRM_time[reg].tv_sec -= value;
  }
  registers->GPRM[reg] = value;
}

/* Eval register code, can either be system or general register.
   SXXX_XXXX, where S is 1 if it is system register. */
static uint16_t eval_reg(command_t* command, uint8_t reg) {
  if(reg & 0x80) {
    if ((reg & 0x1f) == 20) {
      fprintf(MSG_OUT, "libdvdnav: Suspected RCE Region Protection!!!\n");
    }
    return command->registers->SPRM[reg & 0x1f]; /*  FIXME max 24 not 32 */
  } else {
    return get_GPRM(command->registers, reg & 0x0f) ;
  }
}

/* Eval register or immediate data.
   AAAA_AAAA BBBB_BBBB, if immediate use all 16 bits for data else use
   lower eight bits for the system or general purpose register. */
static uint16_t eval_reg_or_data(command_t* command, int32_t imm, int32_t start) {
  if(imm) { /*  immediate */
    return vm_getbits(command, start, 16);
  } else {
    return eval_reg(command, vm_getbits(command, (start - 8), 8));
  }
}

/* Eval register or immediate data.
   xBBB_BBBB, if immediate use all 7 bits for data else use
   lower four bits for the general purpose register number. */
/* Evaluates gprm or data depending on bit, data is in byte n */
static uint16_t eval_reg_or_data_2(command_t* command,
				   int32_t imm, int32_t start) {
  if(imm) /* immediate */
    return vm_getbits(command, (start - 1), 7);
  else
    return get_GPRM(command->registers, (vm_getbits(command, (start - 4), 4)) );
}


/* Compare data using operation, return result from comparison.
   Helper function for the different if functions. */
static int32_t eval_compare(uint8_t operation, uint16_t data1, uint16_t data2) {
  switch(operation) {
    case 1:
      return data1 & data2;
    case 2:
      return data1 == data2;
    case 3:
      return data1 != data2;
    case 4:
      return data1 >= data2;
    case 5:
      return data1 >  data2;
    case 6:
      return data1 <= data2;
    case 7:
      return data1 <  data2;
  }
  fprintf(MSG_OUT, "libdvdnav: eval_compare: Invalid comparison code\n");
  return 0;
}


/* Evaluate if version 1.
   Has comparison data in byte 3 and 4-5 (immediate or register) */
static int32_t eval_if_version_1(command_t* command) {
  uint8_t op = vm_getbits(command, 54, 3);
  if(op) {
    return eval_compare(op, eval_reg(command, vm_getbits(command, 39, 8)),
                            eval_reg_or_data(command, vm_getbits(command, 55, 1), 31));
  }
  return 1;
}

/* Evaluate if version 2.
   This version only compares register which are in byte 6 and 7 */
static int32_t eval_if_version_2(command_t* command) {
  uint8_t op = vm_getbits(command, 54, 3);
  if(op) {
    return eval_compare(op, eval_reg(command, vm_getbits(command, 15, 8)),
                            eval_reg(command, vm_getbits(command, 7, 8)));
  }
  return 1;
}

/* Evaluate if version 3.
   Has comparison data in byte 2 and 6-7 (immediate or register) */
static int32_t eval_if_version_3(command_t* command) {
  uint8_t op = vm_getbits(command, 54, 3);
  if(op) {
    return eval_compare(op, eval_reg(command, vm_getbits(command, 47, 8)),
                            eval_reg_or_data(command, vm_getbits(command, 55, 1), 15));
  }
  return 1;
}

/* Evaluate if version 4.
   Has comparison data in byte 1 and 4-5 (immediate or register)
   The register in byte 1 is only the lowe nibble (4 bits) */
static int32_t eval_if_version_4(command_t* command) {
  uint8_t op = vm_getbits(command, 54, 3);
  if(op) {
    return eval_compare(op, eval_reg(command, vm_getbits(command, 51, 4)),
                            eval_reg_or_data(command, vm_getbits(command, 55, 1), 31));
  }
  return 1;
}

/* Evaluate special instruction.... returns the new row/line number,
   0 if no new row and 256 if Break. */
static int32_t eval_special_instruction(command_t* command, int32_t cond) {
  int32_t line, level;

  switch(vm_getbits(command, 51, 4)) {
    case 0: /*  NOP */
      line = 0;
      return cond ? line : 0;
    case 1: /*  Goto line */
      line = vm_getbits(command, 7, 8);
      return cond ? line : 0;
    case 2: /*  Break */
      /*  max number of rows < 256, so we will end this set */
      line = 256;
      return cond ? 256 : 0;
    case 3: /*  Set temporary parental level and goto */
      line = vm_getbits(command, 7, 8);
      level = vm_getbits(command, 11, 4);
      if(cond) {
	/*  This always succeeds now, if we want real parental protection */
	/*  we need to ask the user and have passwords and stuff. */
	command->registers->SPRM[13] = level;
      }
      return cond ? line : 0;
  }
  return 0;
}

/* Evaluate link by subinstruction.
   Return 1 if link, or 0 if no link
   Actual link instruction is in return_values parameter */
static int32_t eval_link_subins(command_t* command, int32_t cond, link_t *return_values) {
  uint16_t button = vm_getbits(command, 15, 6);
  uint8_t  linkop = vm_getbits(command, 4, 5);

  if(linkop > 0x10)
    return 0;    /*  Unknown Link by Sub-Instruction command */

  /*  Assumes that the link_cmd_t enum has the same values as the LinkSIns codes */
  return_values->command = linkop;
  return_values->data1 = button;
  return cond;
}


/* Evaluate link instruction.
   Return 1 if link, or 0 if no link
   Actual link instruction is in return_values parameter */
static int32_t eval_link_instruction(command_t* command, int32_t cond, link_t *return_values) {
  uint8_t op = vm_getbits(command, 51, 4);

  switch(op) {
    case 1:
	return eval_link_subins(command, cond, return_values);
    case 4:
	return_values->command = LinkPGCN;
	return_values->data1   = vm_getbits(command, 14, 15);
	return cond;
    case 5:
	return_values->command = LinkPTTN;
	return_values->data1 = vm_getbits(command, 9, 10);
	return_values->data2 = vm_getbits(command, 15, 6);
	return cond;
    case 6:
	return_values->command = LinkPGN;
	return_values->data1 = vm_getbits(command, 6, 7);
	return_values->data2 = vm_getbits(command, 15, 6);
	return cond;
    case 7:
	return_values->command = LinkCN;
	return_values->data1 = vm_getbits(command, 7, 8);
	return_values->data2 = vm_getbits(command, 15, 6);
	return cond;
  }
  return 0;
}


/* Evaluate a jump instruction.
   returns 1 if jump or 0 if no jump
   actual jump instruction is in return_values parameter */
static int32_t eval_jump_instruction(command_t* command, int32_t cond, link_t *return_values) {

  switch(vm_getbits(command, 51, 4)) {
    case 1:
      return_values->command = Exit;
      return cond;
    case 2:
      return_values->command = JumpTT;
      return_values->data1 = vm_getbits(command, 22, 7);
      return cond;
    case 3:
      return_values->command = JumpVTS_TT;
      return_values->data1 = vm_getbits(command, 22, 7);
      return cond;
    case 5:
      return_values->command = JumpVTS_PTT;
      return_values->data1 = vm_getbits(command, 22, 7);
      return_values->data2 = vm_getbits(command, 41, 10);
      return cond;
    case 6:
      switch(vm_getbits(command, 23, 2)) {
        case 0:
          return_values->command = JumpSS_FP;
          return cond;
        case 1:
          return_values->command = JumpSS_VMGM_MENU;
          return_values->data1 =  vm_getbits(command, 19, 4);
          return cond;
        case 2:
          return_values->command = JumpSS_VTSM;
          return_values->data1 =  vm_getbits(command, 31, 8);
          return_values->data2 =  vm_getbits(command, 39, 8);
          return_values->data3 =  vm_getbits(command, 19, 4);
          return cond;
        case 3:
          return_values->command = JumpSS_VMGM_PGC;
          return_values->data1 =  vm_getbits(command, 46, 15);
          return cond;
        }
      break;
    case 8:
      switch(vm_getbits(command, 23, 2)) {
        case 0:
          return_values->command = CallSS_FP;
          return_values->data1 = vm_getbits(command, 31, 8);
          return cond;
        case 1:
          return_values->command = CallSS_VMGM_MENU;
          return_values->data1 = vm_getbits(command, 19, 4);
          return_values->data2 = vm_getbits(command, 31, 8);
          return cond;
        case 2:
          return_values->command = CallSS_VTSM;
          return_values->data1 = vm_getbits(command, 19, 4);
          return_values->data2 = vm_getbits(command, 31, 8);
          return cond;
        case 3:
          return_values->command = CallSS_VMGM_PGC;
          return_values->data1 = vm_getbits(command, 46, 15);
          return_values->data2 = vm_getbits(command, 31, 8);
          return cond;
      }
      break;
  }
  return 0;
}

/* Evaluate a set sytem register instruction
   May contain a link so return the same as eval_link */
static int32_t eval_system_set(command_t* command, int32_t cond, link_t *return_values) {
  int32_t i;
  uint16_t data, data2;

  switch(vm_getbits(command, 59, 4)) {
    case 1: /*  Set system reg 1 &| 2 &| 3 (Audio, Subp. Angle) */
      for(i = 1; i <= 3; i++) {
        if(vm_getbits(command, 63 - ((2 + i)*8), 1)) {
          data = eval_reg_or_data_2(command, vm_getbits(command, 60, 1), (47 - (i*8)));
          if(cond) {
            command->registers->SPRM[i] = data;
          }
        }
      }
      break;
    case 2: /*  Set system reg 9 & 10 (Navigation timer, Title PGC number) */
      data = eval_reg_or_data(command, vm_getbits(command, 60, 1), 47);
      data2 = vm_getbits(command, 23, 8); /*  ?? size */
      if(cond) {
	command->registers->SPRM[9] = data; /*  time */
	command->registers->SPRM[10] = data2; /*  pgcN */
      }
      break;
    case 3: /*  Mode: Counter / Register + Set */
      data = eval_reg_or_data(command, vm_getbits(command, 60, 1), 47);
      data2 = vm_getbits(command, 19, 4);
      if(vm_getbits(command, 23, 1)) {
	command->registers->GPRM_mode[data2] |= 1; /* Set bit 0 */
      } else {
	command->registers->GPRM_mode[data2] &= ~ 0x01; /* Reset bit 0 */
      }
      if(cond) {
        set_GPRM(command->registers, data2, data);
      }
      break;
    case 6: /*  Set system reg 8 (Highlighted button) */
      data = eval_reg_or_data(command, vm_getbits(command, 60, 1), 31); /*  Not system reg!! */
      if(cond) {
	command->registers->SPRM[8] = data;
      }
      break;
  }
  if(vm_getbits(command, 51, 4)) {
    return eval_link_instruction(command, cond, return_values);
  }
  return 0;
}


/* Evaluate set operation
   Sets the register given to the value indicated by op and data.
   For the swap case the contents of reg is stored in reg2.
*/
static void eval_set_op(command_t* command, int32_t op, int32_t reg, int32_t reg2, int32_t data) {
  static const int32_t shortmax = 0xffff;
  int32_t     tmp;
  switch(op) {
    case 1:
      set_GPRM(command->registers, reg, data);
      break;
    case 2: /* SPECIAL CASE - SWAP! */
      set_GPRM(command->registers, reg2, get_GPRM(command->registers, reg));
      set_GPRM(command->registers, reg, data);
      break;
    case 3:
      tmp = get_GPRM(command->registers, reg) + data;
      if(tmp > shortmax) tmp = shortmax;
      set_GPRM(command->registers, reg, (uint16_t)tmp);
      break;
    case 4:
      tmp = get_GPRM(command->registers, reg) - data;
      if(tmp < 0) tmp = 0;
      set_GPRM(command->registers, reg, (uint16_t)tmp);
      break;
    case 5:
      tmp = get_GPRM(command->registers, reg) * data;
      if(tmp > shortmax) tmp = shortmax;
      set_GPRM(command->registers, reg, (uint16_t)tmp);
      break;
    case 6:
      if (data != 0) {
        set_GPRM(command->registers, reg, (get_GPRM(command->registers, reg) / data) );
      } else {
        set_GPRM(command->registers, reg, 0xffff); /* Avoid that divide by zero! */
      }
      break;
    case 7:
      if (data != 0) {
        set_GPRM(command->registers, reg, (get_GPRM(command->registers, reg) % data) );
      } else {
        set_GPRM(command->registers, reg, 0xffff); /* Avoid that divide by zero! */
      }
      break;
    case 8: /* SPECIAL CASE - RND! Return numbers between 1 and data. */
      set_GPRM(command->registers, reg, 1 + ((uint16_t) ((float) data * rand()/(RAND_MAX+1.0))) );
      break;
    case 9:
      set_GPRM(command->registers, reg, (get_GPRM(command->registers, reg) & data) );
      break;
    case 10:
      set_GPRM(command->registers, reg, (get_GPRM(command->registers, reg) | data) );
      break;
    case 11:
      set_GPRM(command->registers, reg, (get_GPRM(command->registers, reg) ^ data) );
      break;
  }
}

/* Evaluate set instruction, combined with either Link or Compare. */
static void eval_set_version_1(command_t* command, int32_t cond) {
  uint8_t  op   = vm_getbits(command, 59, 4);
  uint8_t  reg  = vm_getbits(command, 35, 4); /* FIXME: This is different from vmcmd.c!!! */
  uint8_t  reg2 = vm_getbits(command, 19, 4);
  uint16_t data = eval_reg_or_data(command, vm_getbits(command, 60, 1), 31);

  if(cond) {
    eval_set_op(command, op, reg, reg2, data);
  }
}


/* Evaluate set instruction, combined with both Link and Compare. */
static void eval_set_version_2(command_t* command, int32_t cond) {
  uint8_t  op   = vm_getbits(command, 59, 4);
  uint8_t  reg  = vm_getbits(command, 51, 4);
  uint8_t  reg2 = vm_getbits(command, 35, 4); /* FIXME: This is different from vmcmd.c!!! */
  uint16_t data = eval_reg_or_data(command, vm_getbits(command, 60, 1), 47);

  if(cond) {
    eval_set_op(command, op, reg, reg2, data);
  }
}


/* Evaluate a command
   returns row number of goto, 0 if no goto, -1 if link.
   Link command in return_values */
static int32_t eval_command(uint8_t *bytes, registers_t* registers, link_t *return_values) {
  int32_t cond, res = 0;
  command_t command;
  command.instruction =( (uint64_t) bytes[0] << 56 ) |
        ( (uint64_t) bytes[1] << 48 ) |
        ( (uint64_t) bytes[2] << 40 ) |
        ( (uint64_t) bytes[3] << 32 ) |
        ( (uint64_t) bytes[4] << 24 ) |
        ( (uint64_t) bytes[5] << 16 ) |
        ( (uint64_t) bytes[6] <<  8 ) |
          (uint64_t) bytes[7] ;
  command.examined = 0;
  command.registers = registers;
  memset(return_values, 0, sizeof(link_t));

  switch(vm_getbits(&command, 63, 3)) { /* three first old_bits */
    case 0: /*  Special instructions */
      cond = eval_if_version_1(&command);
      res = eval_special_instruction(&command, cond);
      if(res == -1) {
	fprintf(MSG_OUT, "libdvdnav: Unknown Instruction!\n");
	abort();
      }
      break;
    case 1: /*  Link/jump instructions */
      if(vm_getbits(&command, 60, 1)) {
        cond = eval_if_version_2(&command);
        res = eval_jump_instruction(&command, cond, return_values);
      } else {
        cond = eval_if_version_1(&command);
        res = eval_link_instruction(&command, cond, return_values);
      }
      if(res)
	res = -1;
      break;
    case 2: /*  System set instructions */
      cond = eval_if_version_2(&command);
      res = eval_system_set(&command, cond, return_values);
      if(res)
	res = -1;
      break;
    case 3: /*  Set instructions, either Compare or Link may be used */
      cond = eval_if_version_3(&command);
      eval_set_version_1(&command, cond);
      if(vm_getbits(&command, 51, 4)) {
	res = eval_link_instruction(&command, cond, return_values);
      }
      if(res)
	res = -1;
      break;
    case 4: /*  Set, Compare -> Link Sub-Instruction */
      eval_set_version_2(&command, /*True*/ 1);
      cond = eval_if_version_4(&command);
      res = eval_link_subins(&command, cond, return_values);
      if(res)
	res = -1;
      break;
    case 5: /*  Compare -> (Set and Link Sub-Instruction) */
      /* FIXME: These are wrong. Need to be updated from vmcmd.c */
      cond = eval_if_version_4(&command);
      eval_set_version_2(&command, cond);
      res = eval_link_subins(&command, cond, return_values);
      if(res)
	res = -1;
      break;
    case 6: /*  Compare -> Set, allways Link Sub-Instruction */
      /* FIXME: These are wrong. Need to be updated from vmcmd.c */
      cond = eval_if_version_4(&command);
      eval_set_version_2(&command, cond);
      res = eval_link_subins(&command, /*True*/ 1, return_values);
      if(res)
	res = -1;
      break;
    default: /* Unknown command */
      fprintf(MSG_OUT, "libdvdnav: WARNING: Unknown Command=%x\n", vm_getbits(&command, 63, 3));
      abort();
  }
  /*  Check if there are bits not yet examined */

  if(command.instruction & ~ command.examined) {
    fprintf(MSG_OUT, "libdvdnav: decoder.c: [WARNING, unknown bits:");
    fprintf(MSG_OUT, " %08"PRIx64, (command.instruction & ~ command.examined) );
    fprintf(MSG_OUT, "]\n");
  }

  return res;
}

/* Evaluate a set of commands in the given register set (which is modified) */
int32_t vmEval_CMD(vm_cmd_t commands[], int32_t num_commands,
	       registers_t *registers, link_t *return_values) {
  int32_t i = 0;
  int32_t total = 0;

#ifdef TRACE
  /*  DEBUG */
  fprintf(MSG_OUT, "libdvdnav: Registers before transaction\n");
  vm_print_registers( registers );
  fprintf(MSG_OUT, "libdvdnav: Full list of commands to execute\n");
  for(i = 0; i < num_commands; i++)
    vm_print_cmd(i, &commands[i]);
  fprintf(MSG_OUT, "libdvdnav: --------------------------------------------\n");
  fprintf(MSG_OUT, "libdvdnav: Single stepping commands\n");
#endif

  i = 0;
  while(i < num_commands && total < 100000) {
    int32_t line;

#ifdef TRACE
    vm_print_cmd(i, &commands[i]);
#endif

    line = eval_command(&commands[i].bytes[0], registers, return_values);

    if (line < 0) { /*  Link command */
#ifdef TRACE
      fprintf(MSG_OUT, "libdvdnav: Registers after transaction\n");
      vm_print_registers( registers );
      fprintf(MSG_OUT, "libdvdnav: eval: Doing Link/Jump/Call\n");
#endif
      return 1;
    }

    if (line > 0) /*  Goto command */
      i = line - 1;
    else /*  Just continue on the next line */
      i++;

    total++;
  }

  memset(return_values, 0, sizeof(link_t));
#ifdef TRACE
  fprintf(MSG_OUT, "libdvdnav: Registers after transaction\n");
  vm_print_registers( registers );
#endif
  return 0;
}

#ifdef TRACE

static char *linkcmd2str(link_cmd_t cmd) {
  switch(cmd) {
  case LinkNoLink:
    return "LinkNoLink";
  case LinkTopC:
    return "LinkTopC";
  case LinkNextC:
    return "LinkNextC";
  case LinkPrevC:
    return "LinkPrevC";
  case LinkTopPG:
    return "LinkTopPG";
  case LinkNextPG:
    return "LinkNextPG";
  case LinkPrevPG:
    return "LinkPrevPG";
  case LinkTopPGC:
    return "LinkTopPGC";
  case LinkNextPGC:
    return "LinkNextPGC";
  case LinkPrevPGC:
    return "LinkPrevPGC";
  case LinkGoUpPGC:
    return "LinkGoUpPGC";
  case LinkTailPGC:
    return "LinkTailPGC";
  case LinkRSM:
    return "LinkRSM";
  case LinkPGCN:
    return "LinkPGCN";
  case LinkPTTN:
    return "LinkPTTN";
  case LinkPGN:
    return "LinkPGN";
  case LinkCN:
    return "LinkCN";
  case Exit:
    return "Exit";
  case JumpTT:
    return "JumpTT";
  case JumpVTS_TT:
    return "JumpVTS_TT";
  case JumpVTS_PTT:
    return "JumpVTS_PTT";
  case JumpSS_FP:
    return "JumpSS_FP";
  case JumpSS_VMGM_MENU:
    return "JumpSS_VMGM_MENU";
  case JumpSS_VTSM:
    return "JumpSS_VTSM";
  case JumpSS_VMGM_PGC:
    return "JumpSS_VMGM_PGC";
  case CallSS_FP:
    return "CallSS_FP";
  case CallSS_VMGM_MENU:
    return "CallSS_VMGM_MENU";
  case CallSS_VTSM:
    return "CallSS_VTSM";
  case CallSS_VMGM_PGC:
    return "CallSS_VMGM_PGC";
  case PlayThis:
    return "PlayThis";
  }
  return "*** (bug)";
}

void vm_print_link(link_t value) {
  char *cmd = linkcmd2str(value.command);

  switch(value.command) {
  case LinkNoLink:
  case LinkTopC:
  case LinkNextC:
  case LinkPrevC:
  case LinkTopPG:
  case LinkNextPG:
  case LinkPrevPG:
  case LinkTopPGC:
  case LinkNextPGC:
  case LinkPrevPGC:
  case LinkGoUpPGC:
  case LinkTailPGC:
  case LinkRSM:
    fprintf(MSG_OUT, "libdvdnav: %s (button %d)\n", cmd, value.data1);
    break;
  case LinkPGCN:
  case JumpTT:
  case JumpVTS_TT:
  case JumpSS_VMGM_MENU: /*  == 2 -> Title Menu */
  case JumpSS_VMGM_PGC:
    fprintf(MSG_OUT, "libdvdnav: %s %d\n", cmd, value.data1);
    break;
  case LinkPTTN:
  case LinkPGN:
  case LinkCN:
    fprintf(MSG_OUT, "libdvdnav: %s %d (button %d)\n", cmd, value.data1, value.data2);
    break;
  case Exit:
  case JumpSS_FP:
  case PlayThis: /*  Humm.. should we have this at all.. */
    fprintf(MSG_OUT, "libdvdnav: %s\n", cmd);
    break;
  case JumpVTS_PTT:
    fprintf(MSG_OUT, "libdvdnav: %s %d:%d\n", cmd, value.data1, value.data2);
    break;
  case JumpSS_VTSM:
    fprintf(MSG_OUT, "libdvdnav: %s vts %d title %d menu %d\n",
	    cmd, value.data1, value.data2, value.data3);
    break;
  case CallSS_FP:
    fprintf(MSG_OUT, "libdvdnav: %s resume cell %d\n", cmd, value.data1);
    break;
  case CallSS_VMGM_MENU: /*  == 2 -> Title Menu */
  case CallSS_VTSM:
    fprintf(MSG_OUT, "libdvdnav: %s %d resume cell %d\n", cmd, value.data1, value.data2);
    break;
  case CallSS_VMGM_PGC:
    fprintf(MSG_OUT, "libdvdnav: %s %d resume cell %d\n", cmd, value.data1, value.data2);
    break;
  }
 }

void vm_print_registers( registers_t *registers ) {
  int32_t i;
  fprintf(MSG_OUT, "libdvdnav:    #   ");
  for(i = 0; i < 24; i++)
    fprintf(MSG_OUT, " %2d |", i);
  fprintf(MSG_OUT, "\nlibdvdnav: SRPMS: ");
  for(i = 0; i < 24; i++)
    fprintf(MSG_OUT, "%04x|", registers->SPRM[i]);
  fprintf(MSG_OUT, "\nlibdvdnav: GRPMS: ");
  for(i = 0; i < 16; i++)
    fprintf(MSG_OUT, "%04x|", get_GPRM(registers, i) );
  fprintf(MSG_OUT, "\nlibdvdnav: Gmode: ");
  for(i = 0; i < 16; i++)
    fprintf(MSG_OUT, "%04x|", registers->GPRM_mode[i]);
  fprintf(MSG_OUT, "\nlibdvdnav: Gtime: ");
  for(i = 0; i < 16; i++)
    fprintf(MSG_OUT, "%04lx|", registers->GPRM_time[i].tv_sec & 0xffff);
  fprintf(MSG_OUT, "\n");
}

#endif

