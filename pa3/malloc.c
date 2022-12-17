/**********************************************************************
 * Copyright (c) 2020
 *  Jinwoo Jeong <jjw8967@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdint.h>

#include "malloc.h"
#include "types.h"
#include "list_head.h"

#define ALIGNMENT 32
#define HDRSIZE sizeof(header_t)

#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

static LIST_HEAD(free_list); // Don't modify this line
static algo_t g_algo;        // Don't modify this line
static void *bp;             // Don't modify thie line

void *find_fit(size_t size)
{
    header_t *header;
    header_t *best;

    if(g_algo == FIRST_FIT)
    {
        list_for_each_entry(header, &free_list, list)
        {
            if(header->free && header->size >= size)
            {
                return header;
            }
        }
    }
    else if(g_algo == BEST_FIT)
    {
        if(list_empty(&free_list))
            return NULL;

        best = NULL;//list_first_entry(&free_list, header_t, list);
        list_for_each_entry(header, &free_list, list)
        {
            if(header->free && header->size >= size)
            {
                if(best)
                {
                    if(header->size < best->size)
                       best = header;
                }
                else
                {
                    best = header;
                }
            }
        }
        return best;
    }
    return NULL;
}

/***********************************************************************
 * extend_heap()
 *
 * DESCRIPTION
 *   allocate size of bytes of memory and returns a pointer to the
 *   allocated memory.
 *
 * RETURN VALUE
 *   Return a pointer to the allocated memory.
 */
void *my_malloc(size_t size)
{
    /* Implement this function */
    void *start_blk_point;
    header_t *ptr, *metadata;
    size_t origin_block_size;
    size_t data_blk_size = ALIGN(size);
    size_t blk_size = HDRSIZE + data_blk_size;

    if (size == 0)
            return NULL;

    ptr = (header_t*)find_fit(data_blk_size);

    if(ptr)
    {
        if(ptr->size != data_blk_size)
        {
            void *split_loc;
            //void *split_data_loc;

            //split block
            origin_block_size = ptr->size;
            ptr->free = false;
            ptr->size = data_blk_size;
            split_loc = (void *)ptr + HDRSIZE + data_blk_size;
            //split_data_loc = split_loc + HDRSIZE;

            metadata = (header_t*)split_loc;
            metadata->free = true;
            metadata->size = origin_block_size - data_blk_size - HDRSIZE;
            __list_add(&metadata->list, &ptr->list, ptr->list.next);
        }
        else
        {
            ptr->free = false;
            ptr->size = data_blk_size;
        }
        return ((void *)ptr + HDRSIZE);
    }
    else
    {
        //void *new;
        //allocation new block
        start_blk_point = sbrk(blk_size);
        //new = sbrk(0);
        metadata = (header_t*)start_blk_point;
        metadata->free = false;
        metadata->size = data_blk_size;

    ptr = list_last_entry(&free_list, header_t, list);
    if(ptr->free)
    {
            __list_add(&metadata->list, ptr->list.prev, &ptr->list);
    }
    else
        {
        list_add_tail(&metadata->list, &free_list); // warning
    }

        return ((void *)start_blk_point + HDRSIZE);
    }
    return NULL;
}

/***********************************************************************
 * my_realloc()
 *
 * DESCRIPTION
 *   tries to change the size of the allocation pointed to by ptr to
 *   size, and returns ptr. If there is not enough memory block,
 *   my_realloc() creates a new allocation, copies as much of the old
 *   data pointed to by ptr as will fit to the new allocation, frees
 *   the old allocation.
 *
 * RETURN VALUE
 *   Return a pointer to the reallocated memory
 */
void *my_realloc(void *ptr, size_t size)
{
    header_t *hdr;
    /* Implement this function */
    if(ptr==NULL)
        return my_malloc(size);

    hdr = (header_t *)(ptr-HDRSIZE);
    char buf[hdr->size < size ? hdr->size : size];
    memcpy(buf, ptr, hdr->size < size ? hdr->size : size);

    if(size == 0)
    {
        return NULL;
    }
    else
    {
        if(hdr->size == size):
            return ptr;

        if(hdr->size < size)
        {
                void *ptr2 = my_malloc(size);
                my_free(ptr);
                if(ptr2)
                {
                    memcpy(ptr2, buf, hdr->size < size ? hdr->size : size);
                    return ptr2;
                }
                else
                {
                    return NULL;
                }
         }
    }

    return NULL;
}

/***********************************************************************
 * my_free()
 *
 * DESCRIPTION
 *   deallocates the memory allocation pointed to by ptr.
 */
void my_free(void *ptr)
{
    /* Implement this function */
    header_t *metadata, *next, *prev;

    if (ptr == NULL)
            return;

    metadata = (header_t*)((void *)ptr - HDRSIZE);
    metadata->free = true;
    //next, prev merge
    next = list_next_entry(metadata, list);
    prev = list_prev_entry(metadata, list);

if(&next->list != &free_list)
{
    if(next->free)
    {
        metadata->size = HDRSIZE + next->size + metadata->size;
        list_del(&next->list);
    }
}

if(&prev->list != &free_list)
{
    if(prev->free)
    {
        prev->size = HDRSIZE + prev->size + metadata->size;
        list_del(&metadata->list);
    }
}

    return;
}

/*====================================================================*/
/*          ****** DO NOT MODIFY ANYTHING BELOW THIS LINE ******      */
/*          ****** BUT YOU MAY CALL SOME IF YOU WANT TO.. ******      */
/*          ****** EXCEPT TO mem_init() AND mem_deinit(). ******      */
void mem_init(const algo_t algo)
{
  g_algo = algo;
  bp = sbrk(0);
}

void mem_deinit()
{
  header_t *header;
  size_t size = 0;
  list_for_each_entry(header, &free_list, list) {
    size += HDRSIZE + header->size;
  }
  sbrk(-size);

  if (bp != sbrk(0)) {
    fprintf(stderr, "[Error] There is memory leak\n");
  }
}

void print_memory_layout()
{
  header_t *header;
  int cnt = 0;

  printf("===========================\n");
  list_for_each_entry(header, &free_list, list) {
    cnt++;
    printf("%c %ld\n", (header->free) ? 'F' : 'M', header->size);
  }

  printf("Number of block: %d\n", cnt);
  printf("===========================\n");
  return;
}
