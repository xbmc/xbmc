/* Copyright (C) 2003 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

/* Key cache variable structures */

#ifndef _keycache_h
#define _keycache_h
C_MODE_START

/* declare structures that is used by st_key_cache */

struct st_block_link;
typedef struct st_block_link BLOCK_LINK;
struct st_keycache_page;
typedef struct st_keycache_page KEYCACHE_PAGE;
struct st_hash_link;
typedef struct st_hash_link HASH_LINK;

/* info about requests in a waiting queue */
typedef struct st_keycache_wqueue
{
  struct st_my_thread_var *last_thread;  /* circular list of waiting threads */
} KEYCACHE_WQUEUE;

#define CHANGED_BLOCKS_HASH 128             /* must be power of 2 */

/*
  The key cache structure
  It also contains read-only statistics parameters.
*/   

typedef struct st_key_cache
{
  my_bool key_cache_inited;
  my_bool in_resize;             /* true during resize operation             */
  my_bool resize_in_flush;       /* true during flush of resize operation    */
  my_bool can_be_used;           /* usage of cache for read/write is allowed */
  size_t key_cache_mem_size;      /* specified size of the cache memory       */
  uint key_cache_block_size;     /* size of the page buffer of a cache block */
  ulong min_warm_blocks;         /* min number of warm blocks;               */
  ulong age_threshold;           /* age threshold for hot blocks             */
  ulonglong keycache_time;       /* total number of block link operations    */
  uint hash_entries;             /* max number of entries in the hash table  */
  int hash_links;                /* max number of hash links                 */
  int hash_links_used;           /* number of hash links currently used      */
  int disk_blocks;               /* max number of blocks in the cache        */
  ulong blocks_used; /* maximum number of concurrently used blocks */
  ulong blocks_unused; /* number of currently unused blocks */
  ulong blocks_changed;          /* number of currently dirty blocks         */
  ulong warm_blocks;             /* number of blocks in warm sub-chain       */
  ulong cnt_for_resize_op;       /* counter to block resize operation        */
  long blocks_available;      /* number of blocks available in the LRU chain */
  HASH_LINK **hash_root;         /* arr. of entries into hash table buckets  */
  HASH_LINK *hash_link_root;     /* memory for hash table links              */
  HASH_LINK *free_hash_list;     /* list of free hash links                  */
  BLOCK_LINK *free_block_list;   /* list of free blocks */
  BLOCK_LINK *block_root;        /* memory for block links                   */
  uchar HUGE_PTR *block_mem;     /* memory for block buffers                 */
  BLOCK_LINK *used_last;         /* ptr to the last block of the LRU chain   */
  BLOCK_LINK *used_ins;          /* ptr to the insertion block in LRU chain  */
  pthread_mutex_t cache_lock;    /* to lock access to the cache structure    */
  KEYCACHE_WQUEUE resize_queue;  /* threads waiting during resize operation  */
  /*
    Waiting for a zero resize count. Using a queue for symmetry though
    only one thread can wait here.
  */
  KEYCACHE_WQUEUE waiting_for_resize_cnt;
  KEYCACHE_WQUEUE waiting_for_hash_link; /* waiting for a free hash link     */
  KEYCACHE_WQUEUE waiting_for_block;    /* requests waiting for a free block */
  BLOCK_LINK *changed_blocks[CHANGED_BLOCKS_HASH]; /* hash for dirty file bl.*/
  BLOCK_LINK *file_blocks[CHANGED_BLOCKS_HASH];    /* hash for other file bl.*/

  /*
    The following variables are and variables used to hold parameters for
    initializing the key cache.
  */

  ulonglong param_buff_size;    /* size the memory allocated for the cache  */
  ulong param_block_size;       /* size of the blocks in the key cache      */
  ulong param_division_limit;   /* min. percentage of warm blocks           */
  ulong param_age_threshold;    /* determines when hot block is downgraded  */

  /* Statistics variables. These are reset in reset_key_cache_counters(). */
  ulong global_blocks_changed;	/* number of currently dirty blocks         */
  ulonglong global_cache_w_requests;/* number of write requests (write hits) */
  ulonglong global_cache_write;     /* number of writes from cache to files  */
  ulonglong global_cache_r_requests;/* number of read requests (read hits)   */
  ulonglong global_cache_read;      /* number of reads from files to cache   */

  int blocks;                   /* max number of blocks in the cache        */
  my_bool in_init;		/* Set to 1 in MySQL during init/resize     */
} KEY_CACHE;

/* The default key cache */
extern KEY_CACHE dflt_key_cache_var, *dflt_key_cache;

extern int init_key_cache(KEY_CACHE *keycache, uint key_cache_block_size,
			  size_t use_mem, uint division_limit,
			  uint age_threshold);
extern int resize_key_cache(KEY_CACHE *keycache, uint key_cache_block_size,
			    size_t use_mem, uint division_limit,
			    uint age_threshold);
extern void change_key_cache_param(KEY_CACHE *keycache, uint division_limit,
				   uint age_threshold);
extern uchar *key_cache_read(KEY_CACHE *keycache,
                            File file, my_off_t filepos, int level,
                            uchar *buff, uint length,
			    uint block_length,int return_buffer);
extern int key_cache_insert(KEY_CACHE *keycache,
                            File file, my_off_t filepos, int level,
                            uchar *buff, uint length);
extern int key_cache_write(KEY_CACHE *keycache,
                           File file, my_off_t filepos, int level,
                           uchar *buff, uint length,
			   uint block_length,int force_write);
extern int flush_key_blocks(KEY_CACHE *keycache,
                            int file, enum flush_type type);
extern void end_key_cache(KEY_CACHE *keycache, my_bool cleanup);

/* Functions to handle multiple key caches */
extern my_bool multi_keycache_init(void);
extern void multi_keycache_free(void);
extern KEY_CACHE *multi_key_cache_search(uchar *key, uint length);
extern my_bool multi_key_cache_set(const uchar *key, uint length,
				   KEY_CACHE *key_cache);
extern void multi_key_cache_change(KEY_CACHE *old_data,
				   KEY_CACHE *new_data);
extern int reset_key_cache_counters(const char *name,
                                    KEY_CACHE *key_cache);
C_MODE_END
#endif /* _keycache_h */
