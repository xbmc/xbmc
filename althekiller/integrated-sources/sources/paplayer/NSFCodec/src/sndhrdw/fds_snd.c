/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** fds_snd.c
**
** Famicom Disk System sound emulation
** $Id: fds_snd.c,v 1.3 2000/07/03 02:18:53 matt Exp $
*/

#include "../types.h"
#include "nes_apu.h"
#include "fds_snd.h"

static int32 fds_incsize = 0;

/* mix sound channels together */
static int32 fds_process(void)
{
   int32 output;
   output = 0;

   return output;
}

/* write to registers */
static void fds_write(uint32 address, uint8 value)
{
}

/* reset state of vrcvi sound channels */
static void fds_reset(void)
{
   fds_incsize = apu_getcyclerate();
}

static void fds_init(void)
{
}

/* TODO: bleh */
static void fds_shutdown(void)
{
}

static apu_memwrite fds_memwrite[] =
{
   { 0x4040, 0x4092, fds_write }, 
   {     -1,     -1, NULL }
};

apuext_t fds_ext = 
{
   fds_init,
   fds_shutdown,
   fds_reset,
   fds_process,
   NULL, /* no reads */
   fds_memwrite
};

/*
** $Log: fds_snd.c,v $
** Revision 1.3  2000/07/03 02:18:53  matt
** much better external module exporting
**
** Revision 1.2  2000/06/20 04:06:16  matt
** migrated external sound definition to apu module
**
** Revision 1.1  2000/06/20 00:06:47  matt
** initial revision
**
*/

