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
** memguard.c
**
** memory allocation wrapper routines
**
** NOTE: based on code (c) 1998 the Retrocade group
** $Id: memguard.c,v 1.8 2000/06/26 04:54:48 matt Exp $
*/

#include "types.h"

/* undefine macro definitions, so we get real calls */
#undef malloc
#undef free

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "memguard.h"
#include "log.h"


/* Maximum number of allocated blocks at any one time */
#define  MAX_BLOCKS        16384

/* Memory block structure */
typedef struct memblock_s
{
   void  *block_addr;
   int   block_size;
   char  *file_name;
   int   line_num;
} memblock_t;

boolean mem_debug = TRUE;           /* debugging flag */


#ifdef NOFRENDO_DEBUG

static int mem_blockcount = 0;   /* allocated block count */
static memblock_t *mem_record = NULL;

#define  GUARD_STRING   "GgUuAaRrDdSsTtRrIiNnGgBbLlOoCcKk"
#define  GUARD_LENGTH   64          /* before and after allocated block */


/*
** Check the memory guard to make sure out of bounds writes have not
** occurred.
*/
static boolean mem_checkguardblock(void *data, int guard_size)
{
   uint8 *orig, *chk, *blk;
   int i, alloc_size;

   /* get the original pointer */
   orig = (((uint8 *) data) - guard_size);

   /* get the size */
   alloc_size = *((uint32 *) orig);

   /* now skip past the size */
   blk = orig + sizeof(uint32);

   /* check leading guard string */
   chk = GUARD_STRING;
   for (i = sizeof(uint32); i < guard_size; i++)
   {
      if (0 == *chk)
         chk = GUARD_STRING;
      if (*blk != *chk)
         return FALSE;
      chk++;
      blk++;
   }

   /* check end of block */
   chk = GUARD_STRING;
   blk = ((uint8 *) data) + alloc_size;
   for (i = 0; i < guard_size; i++)
   {
      if (0 == *chk)
         chk = GUARD_STRING;
      if (*blk != *chk)
         return FALSE;
      chk++;
      blk++;
   }

   /* we're okay! */
   return TRUE;
}

/* free a guard block */
static void mem_freeguardblock(void *data, int guard_size)
{
   uint8 *orig = (((uint8 *) data) - guard_size);

   free(orig);
}

/* fill in the memory guard, advance the pointer to the 'real' memory */
static void *mem_guardblock(int alloc_size, int guard_size)
{
   void *orig;
   uint8 *blk, *chk;
   int i;

   /* allocate memory */
   orig = calloc(alloc_size + (guard_size * 2), 1);
   if (NULL == orig)
      return NULL;

   blk = ((uint8 *) orig);
   
   /* store the size of the newly allocated block*/
   *((uint32 *) blk) = alloc_size;

   /* skip past the size */
   blk += sizeof(uint32);

   /* put guard string at beginning of block */
   chk = GUARD_STRING;
   for (i = sizeof(uint32); i < guard_size; i++)
   {
      if (0 == *chk)
         chk = GUARD_STRING;
      *blk++ = *chk++;
   }

   /* check end of block */
   chk = GUARD_STRING;
   blk = guard_size + (uint8 *) orig + alloc_size;
   for (i = 0; i < guard_size; i++)
   {
      if (0 == *chk)
         chk = GUARD_STRING;
      *blk++ = *chk++;
   }

   return (void *) (guard_size + (uint8 *) orig);
}


/* Allocate a bunch of memory to keep track of all memory blocks */
static void mem_init(void)
{
   if (mem_record)
   {
      free(mem_record);
      mem_record = NULL;
   }

   mem_record = calloc(MAX_BLOCKS * sizeof(memblock_t), 1);
   ASSERT(mem_record);
}

/* add a block of memory to the master record */
static void mem_addblock(void *data, int block_size, char *file, int line)
{
   int i;

   for (i = 0; i < MAX_BLOCKS; i++)
   {
      if (NULL == mem_record[i].block_addr)
      {
         mem_record[i].block_addr = data;
         mem_record[i].block_size = block_size;
         mem_record[i].file_name = file;
         mem_record[i].line_num = line;
         return;
      }
   }

   ASSERT_MSG("out of memory blocks.");
}

/* find an entry in the block record and delete it */
static void mem_deleteblock(void *data, char *file, int line)
{
   int i;
   char fail[256];

   for (i = 0; i < MAX_BLOCKS; i++)
   {
      if (data == mem_record[i].block_addr)
      {
         if (FALSE == mem_checkguardblock(mem_record[i].block_addr, GUARD_LENGTH))
         {
            sprintf(fail, "mem_deleteblock 0x%08X at line %d of %s -- block corrupt",
                    (uint32) data, line, file);
            ASSERT_MSG(fail);
         }

         memset(&mem_record[i], 0, sizeof(memblock_t));
         return;
      }
   }

   sprintf(fail, "mem_deleteblock 0x%08X at line %d of %s -- block not found",
           (uint32) data, line, file);
   ASSERT_MSG(fail);
}
#endif /* NOFRENDO_DEBUG */

/* allocates memory and clears it */
#ifdef NOFRENDO_DEBUG
void *_my_malloc(int size, char *file, int line)
#else
void *_my_malloc(int size)
#endif
{
   void *temp;
   char fail[256];

#ifdef NOFRENDO_DEBUG
   if (NULL == mem_record && FALSE != mem_debug)
      mem_init();

   if (FALSE != mem_debug)
      temp = mem_guardblock(size, GUARD_LENGTH);
   else
#endif /* NOFRENDO_DEBUG */
      temp = calloc(sizeof(uint8), size);

   if (NULL == temp)
   {
#ifdef NOFRENDO_DEBUG
      sprintf(fail, "malloc: out of memory at line %d of %s.  block size: %d\n",
              line, file, size);
#else
      sprintf(fail, "malloc: out of memory.  block size: %d\n", size);
#endif
      ASSERT_MSG(fail);
   }

#ifdef NOFRENDO_DEBUG
   if (FALSE != mem_debug)
      mem_addblock(temp, size, file, line);

   mem_blockcount++;
#endif

   return temp;
}

/* free a pointer allocated with my_malloc */
#ifdef NOFRENDO_DEBUG
void _my_free(void **data, char *file, int line)
#else
void _my_free(void **data)
#endif
{
   char fail[256];

   if (NULL == data || NULL == *data
       || (unsigned long)-1 == (unsigned long) *data || (unsigned long)-1 == (unsigned long) data)
   {
#ifdef NOFRENDO_DEBUG
      sprintf(fail, "free: attempted to free NULL pointer at line %d of %s\n",
              line, file);
#else
      sprintf(fail, "free: attempted to free NULL pointer.\n");
#endif
      ASSERT_MSG(fail);
   }

#ifdef NOFRENDO_DEBUG
   /* if this is true, we are in REAL trouble */
   if (0 == mem_blockcount)
   {
      ASSERT_MSG("free: attempted to free memory when no blocks available");
   }

   if (FALSE != mem_debug)
      mem_deleteblock(*data, file, line);

   mem_blockcount--; /* dec our block count */

   if (FALSE != mem_debug)
      mem_freeguardblock(*data, GUARD_LENGTH);
   else
#endif /* NOFRENDO_DEBUG */
      free(*data);

   *data = NULL; /* NULL our source */
}

/* check for orphaned memory handles */
void mem_checkleaks(void)
{
#ifdef NOFRENDO_DEBUG
   int i;

   if (FALSE == mem_debug)
      return;

   if (mem_blockcount)
   {
      log_printf("memory leak - %d unfreed block%s\n\n", mem_blockcount, 
         mem_blockcount == 1 ? "" : "s");

      for (i = 0; i < MAX_BLOCKS; i++)
      {
         if (mem_record[i].block_addr)
         {
            log_printf("addr: 0x%08X, size: %d, line %d of %s%s\n",
                    (uint32) mem_record[i].block_addr,
                    mem_record[i].block_size,
                    mem_record[i].line_num,
                    mem_record[i].file_name,
                    (FALSE == mem_checkguardblock(mem_record[i].block_addr, GUARD_LENGTH))
                    ? " -- block corrupt" : "");
         }
      }
   }
   else
      log_printf("no memory leaks\n");
#endif
}

void mem_checkblocks(void)
{
#ifdef NOFRENDO_DEBUG
   int i;

   if (FALSE == mem_debug)
      return;

   for (i = 0; i < MAX_BLOCKS; i++)
   {
      if (mem_record[i].block_addr)
      {
         if (FALSE == mem_checkguardblock(mem_record[i].block_addr, GUARD_LENGTH))
         {
            log_printf("addr: 0x%08X, size: %d, line %d of %s -- block corrupt\n",
                    (uint32) mem_record[i].block_addr,
                    mem_record[i].block_size,
                    mem_record[i].line_num,
                    mem_record[i].file_name);
         }
      }
   }
#endif /* NOFRENDO_DEBUG */
}

/*
** $Log: memguard.c,v $
** Revision 1.8  2000/06/26 04:54:48  matt
** simplified and made more robust
**
** Revision 1.7  2000/06/12 01:11:41  matt
** cleaned up some error output for win32
**
** Revision 1.6  2000/06/09 15:12:25  matt
** initial revision
**
*/

