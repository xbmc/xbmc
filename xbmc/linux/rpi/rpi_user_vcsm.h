/*
 *  Copyright (C) 2015-2016 Raspberry Pi (Trading) Ltd.
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

/* VideoCore Shared Memory - user interface library.
**
** This library provides all the necessary abstraction for any application to
** make use of the shared memory service which is distributed accross a kernel
** driver and a videocore service.
**
** It is an application design decision to choose or not to use this service.
**
** The logical flow of operations that a user application needs to follow when
** using this service is:
**
**       1) Initialize the service.
**       2) Allocate shared memory blocks.
**       3) Start using the allocated blocks.
**          - In order to gain ownership on a block, lock the allocated block,
**            locking a block returns a valid address that the user application
**            can access.
**          - When finished with using the block for the current execution cycle
**            or function, and so when giving up the ownership, unlock the block.
**       4) A block can be locked/unlocked as many times required - within or outside
**          of - a specific execution context.
**       5) To completely release an allocated block, free it.
**       6) If the service is no longer required, terminate it.
**
**
** Some generic considerations:

** Allocating memory blocks.
**
**   Memory blocks can be allocated in different manners depending on the cache
**   behavior desired.  A given block can either be:

**       - Allocated in a non cached fashion all the way through host and videocore.
**       - Allocated in a cached fashion on host OR videocore.
**       - Allocated in a cached fashion on host AND videocore.
**
**   It is an application decision to determine how to allocate a block.  Evidently
**   if the application will be doing substantial read/write accesses to a given block,
**   it is recommended to allocate the block at least in a 'host cached' fashion for
**   better results.
**
**
** Locking memory blocks.
**
**   When the memory block has been allocated in a host cached fashion, locking the
**   memory block (and so taking ownership of it) will trigger a cache invalidation.
**
**   For the above reason and when using host cached allocation, it is important that
**   an application properly implements the lock/unlock mechanism to ensure cache will
**   stay coherent, otherwise there is no guarantee it will at all be.
**
**   It is possible to dynamically change the host cache behavior (ie cached or non
**   cached) of a given allocation without needing to free and re-allocate the block.
**   This feature can be useful for such application which requires access to the block
**   only at certain times and not otherwise.  By changing the cache behavior dynamically
**   the application can optimize performances for a given duration of use.
**   Such dynamic cache behavior remapping only applies to host cache and not videocore
**   cache.  If one requires to change the videocore cache behavior, then a new block
**   must be created to replace the old one.
**
**   On successful locking, a valid pointer is returned that the application can use
**   to access to data inside the block.  There is no guarantee that the pointer will
**   stay valid following the unlock action corresponding to this lock.
**
**
** Unocking memory blocks.
**
**   When the memory block has been allocated in a host cached fashion, unlocking the
**   memory block (and so forgiving its ownership) will trigger a cache flush unless
**   explicitely asked not to flush the cache for performances reasons.
**
**   For the above reason and when using host cached allocation, it is important that
**   an application properly implements the lock/unlock mechanism to ensure cache will
**   stay coherent, otherwise there is no guarantee it will at all be.
**
**
** A complete API is defined below.
*/

#ifdef __cplusplus
extern "C"
{
#endif

/* Different status that can be dumped.
*/
typedef enum
{
   VCSM_STATUS_VC_WALK_ALLOC = 0,   // Walks *all* the allocation on videocore.
                                    // Result of the walk is seen in the videocore
                                    // log.
   VCSM_STATUS_HOST_WALK_MAP,       // Walks the *full* mapping allocation on host
                                    // driver (ie for all processes).  Result of
                                    // the walk is seen in the kernel log.
   VCSM_STATUS_HOST_WALK_PID_MAP,   // Walks the per process mapping allocation on host
                                    // driver (for current process).  Result of
                                    // the walk is seen in the kernel log.
   VCSM_STATUS_HOST_WALK_PID_ALLOC, // Walks the per process host allocation on host
                                    // driver (for current process).  Result of
                                    // the walk is seen in the kernel log.
   VCSM_STATUS_VC_MAP_ALL,          // Equivalent to both VCSM_STATUS_VC_WALK_ALLOC and
                                    // VCSM_STATUS_HOST_WALK_MAP.
                                    //
   VCSM_STATUS_NONE,                // Must be last - invalid.

} VCSM_STATUS_T;

/* Different kind of cache behavior.
*/
typedef enum
{
   VCSM_CACHE_TYPE_NONE = 0,        // No caching applies.
   VCSM_CACHE_TYPE_HOST,            // Allocation is cached on host (user space).
   VCSM_CACHE_TYPE_VC,              // Allocation is cached on videocore.
   VCSM_CACHE_TYPE_HOST_AND_VC,     // Allocation is cached on both host and videocore.

} VCSM_CACHE_TYPE_T;

/* Initialize the vcsm processing.
**
** Must be called once before attempting to do anything else.
**
** Returns 0 on success, -1 on error.
*/
int vcsm_init( void );


/* Terminates the vcsm processing.
**
** Must be called vcsm services are no longer needed, it will
** take care of removing any allocation under the current process
** control if deemed necessary.
*/
void vcsm_exit( void );


/* Queries the status of the the vcsm.
**
** Triggers dump of various kind of information, see the
** different variants specified in VCSM_STATUS_T.
**
** Pid is optional.
*/
void vcsm_status( VCSM_STATUS_T status, int pid );


/* Allocates a non-cached block of memory of size 'size' via the vcsm memory
** allocator.
**
** Returns:        0 on error
**                 a non-zero opaque handle on success.
**
** On success, the user must invoke vcsm_lock with the returned opaque
** handle to gain access to the memory associated with the opaque handle.
** When finished using the memory, the user calls vcsm_unlock_xx (see those
** function definition for more details on the one that can be used).
** 
** A well behaved application should make every attempt to lock/unlock
** only for the duration it needs to access the memory data associated with
** the opaque handle.
*/
unsigned int vcsm_malloc( unsigned int size, char *name );


/* Allocates a cached block of memory of size 'size' via the vcsm memory
** allocator, the type of caching requested is passed as argument of the
** function call.
**
** Returns:        0 on error
**                 a non-zero opaque handle on success.
**
** On success, the user must invoke vcsm_lock with the returned opaque
** handle to gain access to the memory associated with the opaque handle.
** When finished using the memory, the user calls vcsm_unlock_xx (see those
** function definition for more details on the one that can be used).
** 
** A well behaved application should make every attempt to lock/unlock
** only for the duration it needs to access the memory data associated with
** the opaque handle.
*/
unsigned int vcsm_malloc_cache( unsigned int size, VCSM_CACHE_TYPE_T cache, char *name );


/* Shares an allocated block of memory via the vcsm memory allocator.
**
** Returns:        0 on error
**                 a non-zero opaque handle on success.
**
** On success, the user must invoke vcsm_lock with the returned opaque
** handle to gain access to the memory associated with the opaque handle.
** When finished using the memory, the user calls vcsm_unlock_xx (see those
** function definition for more details on the one that can be used).
**
** A well behaved application should make every attempt to lock/unlock
** only for the duration it needs to access the memory data associated with
** the opaque handle.
*/
unsigned int vcsm_malloc_share( unsigned int handle );


/* Resizes a block of memory allocated previously by vcsm_alloc.
**
** Returns:        0 on success
**                 -errno on error.
**
** The handle must be unlocked by user prior to attempting any
** resize action.
**
** On error, the original size allocated against the handle
** remains available the same way it would be following a
** successful vcsm_malloc.
*/
int vcsm_resize( unsigned int handle, unsigned int new_size );


/* Frees a block of memory that was successfully allocated by
** a prior call the vcms_alloc.
**
** The handle should be considered invalid upon return from this
** call.
**
** Whether any memory is actually freed up or not as the result of
** this call will depends on many factors, if all goes well it will
** be freed.  If something goes wrong, the memory will likely end up
** being freed up as part of the vcsm_exit process.  In the end the
** memory is guaranteed to be freed one way or another.
*/
void vcsm_free( unsigned int handle );


/* Retrieves a videocore opaque handle from a mapped user address
** pointer.  The videocore handle will correspond to the actual
** memory mapped in videocore.
**
** Returns:        0 on error
**                 a non-zero opaque handle on success.
**
** Note: the videocore opaque handle is distinct from the user
**       opaque handle (allocated via vcsm_malloc) and it is only
**       significant for such application which knows what to do
**       with it, for the others it is just a number with little
**       use since nothing can be done with it (in particular
**       for safety reason it cannot be used to map anything).
*/
unsigned int vcsm_vc_hdl_from_ptr( void *usr_ptr );


/* Retrieves a videocore opaque handle from a opaque handle
** pointer.  The videocore handle will correspond to the actual
** memory mapped in videocore.
**
** Returns:        0 on error
**                 a non-zero opaque handle on success.
**
** Note: the videocore opaque handle is distinct from the user
**       opaque handle (allocated via vcsm_malloc) and it is only
**       significant for such application which knows what to do
**       with it, for the others it is just a number with little
**       use since nothing can be done with it (in particular
**       for safety reason it cannot be used to map anything).
*/
unsigned int vcsm_vc_hdl_from_hdl( unsigned int handle );


/* Retrieves a user opaque handle from a mapped user address
** pointer.
**
** Returns:        0 on error
**                 a non-zero opaque handle on success.
*/
unsigned int vcsm_usr_handle( void *usr_ptr );


/* Retrieves a mapped user address from an opaque user
** handle.
**
** Returns:        0 on error
**                 a non-zero address on success.
**
** On success, the address corresponds to the pointer
** which can access the data allocated via the vcsm_malloc
** call.
*/
void *vcsm_usr_address( unsigned int handle );


/* Locks the memory associated with this opaque handle.
**
** Returns:        NULL on error
**                 a valid pointer on success.
**
** A user MUST lock the handle received from vcsm_malloc
** in order to be able to use the memory associated with it.
**
** On success, the pointer returned is only valid within
** the lock content (ie until a corresponding vcsm_unlock_xx
** is invoked).
*/
void *vcsm_lock( unsigned int handle );


/* Locks the memory associated with this opaque handle.  The lock
** also gives a chance to update the *host* cache behavior of the
** allocated buffer if so desired.  The *videocore* cache behavior
** of the allocated buffer cannot be changed by this call and such
** attempt will be ignored.
**
** The system will attempt to honour the cache_update mode request,
** the cache_result mode will provide the final answer on which cache
** mode is really in use.  Failing to change the cache mode will not
** result in a failure to lock the buffer as it is an application
** decision to choose what to do if (cache_result != cache_update)
**
** The value returned in cache_result can only be considered valid if
** the returned pointer is non NULL.  The cache_result pointer may be
** NULL if the application does not care about the actual outcome of
** its action with regards to the cache behavior change.
**
** Returns:        NULL on error
**                 a valid pointer on success.
**
** A user MUST lock the handle received from vcsm_malloc
** in order to be able to use the memory associated with it.
**
** On success, the pointer returned is only valid within
** the lock content (ie until a corresponding vcsm_unlock_xx
** is invoked).
*/
void *vcsm_lock_cache( unsigned int handle,
                       VCSM_CACHE_TYPE_T cache_update,
                       VCSM_CACHE_TYPE_T *cache_result );


/* Unlocks the memory associated with this user mapped address.
**
** Returns:        0 on success
**                 -errno on error.
**
** After unlocking a mapped address, the user should no longer
** attempt to reference it.
*/
int vcsm_unlock_ptr( void *usr_ptr );


/* Unlocks the memory associated with this user mapped address.
** Apply special processing that would override the otherwise
** default behavior.
**
** If 'cache_no_flush' is specified:
**    Do not flush cache as the result of the unlock (if cache
**    flush was otherwise applicable in this case).
**
** Returns:        0 on success
**                 -errno on error.
**
** After unlocking a mapped address, the user should no longer
** attempt to reference it.
*/
int vcsm_unlock_ptr_sp( void *usr_ptr, int cache_no_flush );


/* Unlocks the memory associated with this user opaque handle.
**
** Returns:        0 on success
**                 -errno on error.
**
** After unlocking an opaque handle, the user should no longer
** attempt to reference the mapped addressed once associated
** with it.
*/
int vcsm_unlock_hdl( unsigned int handle );


/* Unlocks the memory associated with this user opaque handle.
** Apply special processing that would override the otherwise
** default behavior.
**
** If 'cache_no_flush' is specified:
**    Do not flush cache as the result of the unlock (if cache
**    flush was otherwise applicable in this case).
**
** Returns:        0 on success
**                 -errno on error.
**
** After unlocking an opaque handle, the user should no longer
** attempt to reference the mapped addressed once associated
** with it.
*/
int vcsm_unlock_hdl_sp( unsigned int handle, int cache_no_flush );

/* Clean and/or invalidate the memory associated with this user opaque handle
**
** Returns:        non-zero on error
**
** structure contains a list of flush/invalidate commands. Commands are:
** 0: nop
** 1: invalidate       given virtual range in L1/L2
** 2: clean            given virtual range in L1/L2
** 3: clean+invalidate given virtual range in L1/L2
** 4: flush all L1/L2
*/
struct vcsm_user_clean_invalid_s {
   struct {
      unsigned int cmd;
      unsigned int handle;
      unsigned int addr;
      unsigned int size;
   } s[8];
};

int vcsm_clean_invalid( struct vcsm_user_clean_invalid_s *s );

#ifdef __cplusplus
}
#endif

