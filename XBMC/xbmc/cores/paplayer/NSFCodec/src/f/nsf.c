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
** nsf.c
**
** NSF loading/saving related functions
** $Id: nsf.c,v 1.14 2000/07/05 14:54:45 matt Exp $
*/

#include <stdio.h>
#include <string.h>
#include "types.h"
#include "nsf.h"
#include "log.h"
#include "nes6502.h"
#include "nes_apu.h"
#include "vrcvisnd.h"
#include "vrc7_snd.h"
#include "mmc5_snd.h"
#include "fds_snd.h"

/* TODO: bleh! should encapsulate in NSF */
#define  MAX_ADDRESS_HANDLERS    32
static nes6502_memread nsf_readhandler[MAX_ADDRESS_HANDLERS];
static nes6502_memwrite nsf_writehandler[MAX_ADDRESS_HANDLERS];

static nsf_t *cur_nsf = NULL;

static void nsf_setcontext(nsf_t *nsf)
{
   ASSERT(nsf);
   cur_nsf = nsf;
}

static uint8 read_mirrored_ram(uint32 address)
{
   return cur_nsf->cpu->mem_page[0][address & 0x7FF];
}

static void write_mirrored_ram(uint32 address, uint8 value)
{
   cur_nsf->cpu->mem_page[0][address & 0x7FF] = value;
}

/* can be used for both banked and non-bankswitched NSFs */
static void nsf_bankswitch(uint32 address, uint8 value)
{
   int cpu_page;
   uint8 *offset;

   cpu_page = address & 0x0F;
   offset = (cur_nsf->data - (cur_nsf->load_addr & 0x0FFF)) + (value << 12);

   nes6502_getcontext(cur_nsf->cpu);
   cur_nsf->cpu->mem_page[cpu_page] = offset;
   nes6502_setcontext(cur_nsf->cpu);
}

static nes6502_memread default_readhandler[] =
{
   { 0x0800, 0x1FFF, read_mirrored_ram },
   { 0x4000, 0x4017, apu_read },
   { -1,     -1,     NULL }
};

static nes6502_memwrite default_writehandler[] =
{
   { 0x0800, 0x1FFF, write_mirrored_ram },
   { 0x4000, 0x4017, apu_write },
   { 0x5FF6, 0x5FFF, nsf_bankswitch },
   { -1,     -1,     NULL}
};

static uint8 invalid_read(uint32 address)
{
#ifdef NOFRENDO_DEBUG
   log_printf("filthy NSF read from $%04X\n", address);
#endif /* NOFRENDO_DEBUG */

   return 0xFF;
}

static void invalid_write(uint32 address, uint8 value)
{
#ifdef NOFRENDO_DEBUG
   log_printf("filthy NSF tried to write $%02X to $%04X\n", value, address);
#endif /* NOFRENDO_DEBUG */
}

/* set up the address handlers that the CPU uses */
static void build_address_handlers(nsf_t *nsf)
{
   int count, num_handlers;

   memset(nsf_readhandler, 0, sizeof(nsf_readhandler));
   memset(nsf_writehandler, 0, sizeof(nsf_writehandler));
 
   num_handlers = 0;
   for (count = 0; num_handlers < MAX_ADDRESS_HANDLERS; count++, num_handlers++)
   {
      if (NULL == default_readhandler[count].read_func)
         break;

      memcpy(&nsf_readhandler[num_handlers], &default_readhandler[count],
             sizeof(nes6502_memread));
   }

   if (nsf->apu->ext)
   {
      if (NULL != nsf->apu->ext->mem_read)
      {
         for (count = 0; num_handlers < MAX_ADDRESS_HANDLERS; count++, num_handlers++)
         {
            if (NULL == nsf->apu->ext->mem_read[count].read_func)
               break;

            memcpy(&nsf_readhandler[num_handlers], &nsf->apu->ext->mem_read[count],
                   sizeof(nes6502_memread));
         }
      }
   }

   /* catch-all for bad reads */
   nsf_readhandler[num_handlers].min_range = 0x2000; /* min address */
   nsf_readhandler[num_handlers].max_range = 0x5BFF; /* max address */
   nsf_readhandler[num_handlers].read_func = invalid_read; /* handler */
   num_handlers++;
   nsf_readhandler[num_handlers].min_range = -1;
   nsf_readhandler[num_handlers].max_range = -1;
   nsf_readhandler[num_handlers].read_func = NULL;
   num_handlers++;
   ASSERT(num_handlers <= MAX_ADDRESS_HANDLERS);

   num_handlers = 0;
   for (count = 0; num_handlers < MAX_ADDRESS_HANDLERS; count++, num_handlers++)
   {
      if (NULL == default_writehandler[count].write_func)
         break;

      memcpy(&nsf_writehandler[num_handlers], &default_writehandler[count],
             sizeof(nes6502_memwrite));
   }

   if (nsf->apu->ext)
   {
      if (NULL != nsf->apu->ext->mem_write)
      {
         for (count = 0; num_handlers < MAX_ADDRESS_HANDLERS; count++, num_handlers++)
         {
            if (NULL == nsf->apu->ext->mem_write[count].write_func)
               break;

            memcpy(&nsf_writehandler[num_handlers], &nsf->apu->ext->mem_write[count],
                   sizeof(nes6502_memwrite));
         }
      }
   }

   /* catch-all for bad writes */
   nsf_writehandler[num_handlers].min_range = 0x2000; /* min address */
   nsf_writehandler[num_handlers].max_range = 0x5BFF; /* max address */
   nsf_writehandler[num_handlers].write_func = invalid_write; /* handler */
   num_handlers++;
   /* protect region at $8000-$FFFF */
   nsf_writehandler[num_handlers].min_range = 0x8000; /* min address */
   nsf_writehandler[num_handlers].max_range = 0xFFFF; /* max address */
   nsf_writehandler[num_handlers].write_func = invalid_write; /* handler */
   num_handlers++;
   nsf_writehandler[num_handlers].min_range = -1;
   nsf_writehandler[num_handlers].max_range = -1;
   nsf_writehandler[num_handlers].write_func = NULL;
   num_handlers++;
   ASSERT(num_handlers <= MAX_ADDRESS_HANDLERS);
}

#define  NSF_ROUTINE_LOC   0x5000

/* sets up a simple loop that calls the desired routine and spins */
static void nsf_setup_routine(uint32 address, uint8 a_reg, uint8 x_reg)
{
   uint8 *mem;

   nes6502_getcontext(cur_nsf->cpu);
   mem = cur_nsf->cpu->mem_page[NSF_ROUTINE_LOC >> 12] + (NSF_ROUTINE_LOC & 0x0FFF);

   /* our lovely 4-byte 6502 NSF player */
   mem[0] = 0x20;            /* JSR address */
   mem[1] = address & 0xFF;
   mem[2] = address >> 8;
   mem[3] = 0xF2;            /* JAM (cpu kill op) */

   cur_nsf->cpu->pc_reg = NSF_ROUTINE_LOC;
   cur_nsf->cpu->a_reg = a_reg;
   cur_nsf->cpu->x_reg = x_reg;
   cur_nsf->cpu->y_reg = 0;
   cur_nsf->cpu->s_reg = 0xFF;

   nes6502_setcontext(cur_nsf->cpu);
}

/* retrieve any external soundchip driver */
static apuext_t *nsf_getext(nsf_t *nsf)
{
   switch (nsf->ext_sound_type)
   {
   case EXT_SOUND_VRCVI:
      return &vrcvi_ext;

   case EXT_SOUND_VRCVII:
      return &vrc7_ext;

   case EXT_SOUND_FDS:
      return &fds_ext;

   case EXT_SOUND_MMC5:
      return &mmc5_ext;

   case EXT_SOUND_NAMCO106:
   case EXT_SOUND_SUNSOFT_FME07:
   case EXT_SOUND_NONE:
   default:
      return NULL;
   }
}

static void nsf_inittune(nsf_t *nsf)
{
   uint8 bank, x_reg;
   uint8 start_bank, num_banks;

   memset(nsf->cpu->mem_page[0], 0, 0x800);
   memset(nsf->cpu->mem_page[6], 0, 0x1000);
   memset(nsf->cpu->mem_page[7], 0, 0x1000);

   if (nsf->bankswitched)
   {
      /* the first hack of the NSF spec! */
      if (EXT_SOUND_FDS == nsf->ext_sound_type)
      {
         nsf_bankswitch(0x5FF6, nsf->bankswitch_info[6]);
         nsf_bankswitch(0x5FF7, nsf->bankswitch_info[7]);
      }

      for (bank = 0; bank < 8; bank++)
         nsf_bankswitch(0x5FF8 + bank, nsf->bankswitch_info[bank]);
   }
   else
   {
      /* not bankswitched, just page in our standard stuff */
      ASSERT(nsf->load_addr + nsf->length <= 0x10000);

      /* avoid ripper filth */
      for (bank = 0; bank < 8; bank++)
         nsf_bankswitch(0x5FF8 + bank, bank);

      start_bank = nsf->load_addr >> 12;
      num_banks = ((nsf->load_addr + nsf->length - 1) >> 12) - start_bank + 1;

      for (bank = 0; bank < num_banks; bank++)
         nsf_bankswitch(0x5FF0 + start_bank + bank, bank);
   }

   /* determine PAL/NTSC compatibility shite */
   if (nsf->pal_ntsc_bits & NSF_DEDICATED_PAL)
      x_reg = 1;
   else   
      x_reg = 0;

   /* execute 1 frame or so; let init routine run free */
   nsf_setup_routine(nsf->init_addr, (uint8) (nsf->current_song - 1), x_reg);
   nes6502_execute((int) NES_FRAME_CYCLES);
}

void nsf_frame(nsf_t *nsf)
{
   nsf_setcontext(nsf); /* future expansion =) */
   apu_setcontext(nsf->apu);
   nes6502_setcontext(nsf->cpu);

   /* one frame of NES processing */
   nsf_setup_routine(nsf->play_addr, 0, 0);
   nes6502_execute((int) NES_FRAME_CYCLES);
}

/* Deallocate memory */
void nes_shutdown(nsf_t *nsf)
{
   int i;

   ASSERT(nsf);
   
   if (nsf->cpu)
   {
      if (nsf->cpu->mem_page[0])
         free(nsf->cpu->mem_page[0]);
      for (i = 5; i <= 7; i++)
      {
         if (nsf->cpu->mem_page[i])
            free(nsf->cpu->mem_page[i]);
      }
      free(nsf->cpu);
   }
}

void nsf_init(void)
{
   nes6502_init();
}

/* Initialize NES CPU, hardware, etc. */
static int nsf_cpuinit(nsf_t *nsf)
{
   int i;
   
   nsf->cpu = malloc(sizeof(nes6502_context));
   if (NULL == nsf->cpu)
      return -1;

   memset(nsf->cpu, 0, sizeof(nes6502_context));

   nsf->cpu->mem_page[0] = malloc(0x800);
   if (NULL == nsf->cpu->mem_page[0])
      return -1;

   /* allocate some space for the NSF "player" MMC5 EXRAM, and WRAM */
   for (i = 5; i <= 7; i++)
   {
      nsf->cpu->mem_page[i] = malloc(0x1000);
      if (NULL == nsf->cpu->mem_page[i])
         return -1;
   }

   nsf->cpu->read_handler = nsf_readhandler;
   nsf->cpu->write_handler = nsf_writehandler;

   return 0;
}

static void nsf_setup(nsf_t *nsf)
{
   int i;

   nsf->current_song = nsf->start_song;

   if (nsf->pal_ntsc_bits & NSF_DEDICATED_PAL)
   {
      if (nsf->pal_speed)
         nsf->playback_rate = 1000000 / nsf->pal_speed;
      else
         nsf->playback_rate = 50; /* 50 Hz */
   }
   else
   {
      if (nsf->ntsc_speed)
         nsf->playback_rate = 1000000 / nsf->ntsc_speed;
      else
         nsf->playback_rate = 60; /* 60 Hz */
   }

   nsf->bankswitched = FALSE;

   for (i = 0; i < 8; i++)
   {
      if (nsf->bankswitch_info[i])
      {
         nsf->bankswitched = TRUE;
         break;
      }
   }
}

#ifdef HOST_LITTLE_ENDIAN
#define  SWAP_16(x)  (x)
#else /* !HOST_LITTLE_ENDIAN */
#define  SWAP_16(x)  (((uint16) x >> 8) | (((uint16) x & 0xFF) << 8))
#endif /* !HOST_LITTLE_ENDIAN */

/* Load a ROM image into memory */
nsf_t *nsf_load(char *filename, void *source, int length)
{
   FILE *fp = NULL;
   char *new_fn = NULL;
   nsf_t *temp_nsf;

   if (NULL == filename && NULL == source)
      return NULL;
 
   if (NULL == source)
   {
      fp = fopen(filename, "rb");

      /* Didn't find the file?  Maybe the .NSF extension was omitted */
      if (NULL == fp)
      {
         new_fn = malloc(strlen(filename) + 5);
         if (NULL == new_fn)
            return NULL;
         strcpy(new_fn, filename);

         if (NULL == strrchr(new_fn, '.'))
            strcat(new_fn, ".nsf");

         fp = fopen(new_fn, "rb");

         if (NULL == fp)
         {
            log_printf("could not find file '%s'\n", new_fn);
            free(new_fn);
            return NULL;
         }
      }
   }

   temp_nsf = malloc(sizeof(nsf_t));
   if (NULL == temp_nsf)
      return NULL;

   /* Read in the header */
   if (NULL == source)
      fread(temp_nsf, 1, NSF_HEADER_SIZE, fp);
   else
      memcpy(temp_nsf, source, NSF_HEADER_SIZE);

   if (memcmp(temp_nsf->id, NSF_MAGIC, 5))
   {
      if (NULL == source)
      {
         log_printf("%s is not an NSF format file\n", new_fn);
         fclose(fp);
         free(new_fn);
      }
      nsf_free(&temp_nsf);
      return NULL;
   }

   /* fixup endianness */
   temp_nsf->load_addr = SWAP_16(temp_nsf->load_addr);
   temp_nsf->init_addr = SWAP_16(temp_nsf->init_addr);
   temp_nsf->play_addr = SWAP_16(temp_nsf->play_addr);
   temp_nsf->ntsc_speed = SWAP_16(temp_nsf->ntsc_speed);
   temp_nsf->pal_speed = SWAP_16(temp_nsf->pal_speed);

   /* we're now at position 80h */
   if (NULL == source)
   {
      fseek(fp, 0, SEEK_END);
      temp_nsf->length = ftell(fp) - NSF_HEADER_SIZE;
   }
   else
   {
      temp_nsf->length = length - NSF_HEADER_SIZE;
   }

   /* Allocate NSF space, and load it up! */
   temp_nsf->data = malloc(temp_nsf->length);
   if (NULL == temp_nsf->data)
   {
      log_printf("error allocating memory for NSF data\n");
      nsf_free(&temp_nsf);
      return NULL;
   }

   /* seek to end of header, read in data */
   if (NULL == source)
   {
      fseek(fp, NSF_HEADER_SIZE, SEEK_SET);
      fread(temp_nsf->data, temp_nsf->length, 1, fp);

      fclose(fp);

      if (new_fn)
         free(new_fn);
   }
   else
      memcpy(temp_nsf->data, (uint8 *) source + NSF_HEADER_SIZE, length);

   /* Set up some variables */
   nsf_setup(temp_nsf);

   temp_nsf->apu = NULL; /* just make sure */

   if (nsf_cpuinit(temp_nsf))
   {
      nsf_free(&temp_nsf);
      return NULL;
   }

   return temp_nsf;
}

/* Free an NSF */
void nsf_free(nsf_t **nsf)
{
   if (*nsf)
   {
      if ((*nsf)->apu)
         apu_destroy((*nsf)->apu);

      nes_shutdown(*nsf);

      if ((*nsf)->data)
         free((*nsf)->data);

      free(*nsf);
   }
}

void nsf_setchan(nsf_t *nsf, int chan, boolean enabled)
{
   if (nsf)
   {
      nsf_setcontext(nsf);
      apu_setchan(chan, enabled);
   }
}

void nsf_playtrack(nsf_t *nsf, int track, int sample_rate, int sample_bits, boolean stereo)
{
   ASSERT(nsf);

   /* make this NSF the current context */
   nsf_setcontext(nsf);

   /* create the APU */
   if (nsf->apu)
      apu_destroy(nsf->apu);

   nsf->apu = apu_create(sample_rate, nsf->playback_rate, sample_bits, stereo);
   if (NULL == nsf->apu)
   {
      nsf_free(&nsf);
      return;
   }

   apu_setext(nsf->apu, nsf_getext(nsf));

   /* go ahead and init all the read/write handlers */
   build_address_handlers(nsf);

   /* convenience? */
   nsf->process = nsf->apu->process;

   nes6502_setcontext(nsf->cpu);

   if (track > nsf->num_songs)
      track = nsf->num_songs;
   else if (track < 1)
      track = 1;

   nsf->current_song = track;
   
   apu_reset();

   nsf_inittune(nsf);
}

void nsf_setfilter(nsf_t *nsf, int filter_type)
{
   if (nsf)
   {
      nsf_setcontext(nsf);
      apu_setfilter(filter_type);
   }
}

/*
** $Log: nsf.c,v $
** Revision 1.14  2000/07/05 14:54:45  matt
** fix for naughty Crystalis rip
**
** Revision 1.13  2000/07/04 04:59:38  matt
** removed DOS-specific stuff, fixed bug in address handlers
**
** Revision 1.12  2000/07/03 02:19:36  matt
** dynamic address range handlers, cleaner and faster
**
** Revision 1.11  2000/06/23 03:27:58  matt
** cleaned up external sound inteface
**
** Revision 1.10  2000/06/20 20:42:47  matt
** accuracy changes
**
** Revision 1.9  2000/06/20 00:05:58  matt
** changed to driver-based external sound generation
**
** Revision 1.8  2000/06/13 03:51:54  matt
** update API to take freq/sample data on nsf_playtrack
**
** Revision 1.7  2000/06/12 03:57:14  matt
** more robust checking for winamp plugin
**
** Revision 1.6  2000/06/12 01:13:00  matt
** added CPU/APU as members of the nsf struct
**
** Revision 1.5  2000/06/11 16:09:21  matt
** nsf_free is more robust
**
** Revision 1.4  2000/06/09 15:12:26  matt
** initial revision
**
*/