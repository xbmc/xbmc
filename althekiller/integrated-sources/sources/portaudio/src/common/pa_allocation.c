/*
 * $Id: pa_allocation.c 1097 2006-08-26 08:27:53Z rossb $
 * Portable Audio I/O Library allocation group implementation
 * memory allocation group for tracking allocation groups
 *
 * Based on the Open Source API proposed by Ross Bencina
 * Copyright (c) 1999-2002 Ross Bencina, Phil Burk
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * The text above constitutes the entire PortAudio license; however, 
 * the PortAudio community also makes the following non-binding requests:
 *
 * Any person wishing to distribute modifications to the Software is
 * requested to send the modifications to the original developer so that
 * they can be incorporated into the canonical version. It is also 
 * requested that these non-binding requests be included along with the 
 * license above.
 */

/** @file
 @ingroup common_src

 @brief Allocation Group implementation.
*/


#include "pa_allocation.h"
#include "pa_util.h"


/*
    Maintain 3 singly linked lists...
    linkBlocks: the buffers used to allocate the links
    spareLinks: links available for use in the allocations list
    allocations: the buffers currently allocated using PaUtil_ContextAllocateMemory()

    Link block size is doubled every time new links are allocated.
*/


#define PA_INITIAL_LINK_COUNT_    16

struct PaUtilAllocationGroupLink
{
    struct PaUtilAllocationGroupLink *next;
    void *buffer;
};

/*
    Allocate a block of links. The first link will have it's buffer member
    pointing to the block, and it's next member set to <nextBlock>. The remaining
    links will have NULL buffer members, and each link will point to
    the next link except the last, which will point to <nextSpare>
*/
static struct PaUtilAllocationGroupLink *AllocateLinks( long count,
        struct PaUtilAllocationGroupLink *nextBlock,
        struct PaUtilAllocationGroupLink *nextSpare )
{
    struct PaUtilAllocationGroupLink *result;
    int i;
    
    result = (struct PaUtilAllocationGroupLink *)PaUtil_AllocateMemory(
            sizeof(struct PaUtilAllocationGroupLink) * count );
    if( result )
    {
        /* the block link */
        result[0].buffer = result;
        result[0].next = nextBlock;

        /* the spare links */
        for( i=1; i<count; ++i )
        {
            result[i].buffer = 0;
            result[i].next = &result[i+1];
        }
        result[count-1].next = nextSpare;
    }
    
    return result;
}


PaUtilAllocationGroup* PaUtil_CreateAllocationGroup( void )
{
    PaUtilAllocationGroup* result = 0;
    struct PaUtilAllocationGroupLink *links;


    links = AllocateLinks( PA_INITIAL_LINK_COUNT_, 0, 0 );
    if( links != 0 )
    {
        result = (PaUtilAllocationGroup*)PaUtil_AllocateMemory( sizeof(PaUtilAllocationGroup) );
        if( result )
        {
            result->linkCount = PA_INITIAL_LINK_COUNT_;
            result->linkBlocks = &links[0];
            result->spareLinks = &links[1];
            result->allocations = 0;
        }
        else
        {
            PaUtil_FreeMemory( links );
        }
    }

    return result;
}


void PaUtil_DestroyAllocationGroup( PaUtilAllocationGroup* group )
{
    struct PaUtilAllocationGroupLink *current = group->linkBlocks;
    struct PaUtilAllocationGroupLink *next;

    while( current )
    {
        next = current->next;
        PaUtil_FreeMemory( current->buffer );
        current = next;
    }

    PaUtil_FreeMemory( group );
}


void* PaUtil_GroupAllocateMemory( PaUtilAllocationGroup* group, long size )
{
    struct PaUtilAllocationGroupLink *links, *link;
    void *result = 0;
    
    /* allocate more links if necessary */
    if( !group->spareLinks )
    {
        /* double the link count on each block allocation */
        links = AllocateLinks( group->linkCount, group->linkBlocks, group->spareLinks );
        if( links )
        {
            group->linkCount += group->linkCount;
            group->linkBlocks = &links[0];
            group->spareLinks = &links[1];
        }
    }

    if( group->spareLinks )
    {
        result = PaUtil_AllocateMemory( size );
        if( result )
        {
            link = group->spareLinks;
            group->spareLinks = link->next;

            link->buffer = result;
            link->next = group->allocations;

            group->allocations = link;
        }
    }

    return result;    
}


void PaUtil_GroupFreeMemory( PaUtilAllocationGroup* group, void *buffer )
{
    struct PaUtilAllocationGroupLink *current = group->allocations;
    struct PaUtilAllocationGroupLink *previous = 0;

    if( buffer == 0 )
        return;

    /* find the right link and remove it */
    while( current )
    {
        if( current->buffer == buffer )
        {
            if( previous )
            {
                previous->next = current->next;
            }
            else
            {
                group->allocations = current->next;
            }

            current->buffer = 0;
            current->next = group->spareLinks;
            group->spareLinks = current;

            break;
        }
        
        previous = current;
        current = current->next;
    }

    PaUtil_FreeMemory( buffer ); /* free the memory whether we found it in the list or not */
}


void PaUtil_FreeAllAllocations( PaUtilAllocationGroup* group )
{
    struct PaUtilAllocationGroupLink *current = group->allocations;
    struct PaUtilAllocationGroupLink *previous = 0;

    /* free all buffers in the allocations list */
    while( current )
    {
        PaUtil_FreeMemory( current->buffer );
        current->buffer = 0;

        previous = current;
        current = current->next;
    }

    /* link the former allocations list onto the front of the spareLinks list */
    if( previous )
    {
        previous->next = group->spareLinks;
        group->spareLinks = group->allocations;
        group->allocations = 0;
    }
}

