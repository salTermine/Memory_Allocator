#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "sfmm.h"
#include <criterion/criterion.h>

#define WSIZE 8
#define DSIZE 16
#define PAGE_SIZE (1 << 12)
#define MAX(x,y) ((x) > (y)? (x) : (y))

static void place(void *bp, size_t adjusted_size);
static void *find_fit(size_t adjusted_size);
static void *insert_at_head(void *ptr);
static void remove_block(void *ptr);
static void coalesce(void *ptr);
static void *extend_heap();
//static void *extend_heap(size_t words);

/**
 * All functions you make for the assignment must be implemented in this file.
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */

sf_free_header* freelist_head = NULL;
sf_free_header* free_block = NULL;
sf_free_header* prev_block = NULL;
sf_free_header* next_block = NULL;
sf_header* block_header = NULL;
sf_header* block_footer = NULL;
sf_header* allocated_header = NULL;
static info my_info = {0};

size_t i_need_a_drink = 0;
void *heap_begin = NULL;
void *heap_end = NULL;

//--------------------------------------------------------------------------
void *sf_malloc(size_t size)
{
	void *heap_start = NULL;
	size_t adjusted_size = 0;
	size_t extend_size = 0;
	char *bp = NULL;
	i_need_a_drink = size;

	/* CHECK THE SIZE REQUESTED IF 0 RETURN NULL */
	if(size == 0)
	{
		return NULL;
	}
	/* IF SIZE IS LESS THEN THE MINIMUM ALLOWED THEN ADJUST */
	if(size < DSIZE)
	{
		adjusted_size = 2 * DSIZE;
	}
	else
	{
		adjusted_size = DSIZE * (( size + (DSIZE) + (DSIZE - 1)) / DSIZE);
	}


	/* CHECK IF FREELIST_HEAD = NULL */
	if(freelist_head == NULL)
	{
		/* SET HEAP_START TO START OF THE HEAP */
		heap_start = sf_sbrk(0);

		if(heap_begin == NULL)
		{
			heap_begin = heap_start;
			heap_end = heap_begin + PAGE_SIZE;
		}
		else
		{
			heap_end += PAGE_SIZE;
		}

		if((long)(sf_sbrk(1)) == -1)
		{
			return NULL;
		}

		/* SET UP FREELIST INFO */
		freelist_head = heap_start;
		freelist_head->header.alloc = 0;
		freelist_head->header.block_size = PAGE_SIZE >> 4;
		freelist_head->next = NULL;
		freelist_head->prev = NULL;
		sf_footer *free_footer = (void*)freelist_head + PAGE_SIZE - 8;
		free_footer->block_size = PAGE_SIZE >> 4;
		free_footer->alloc = 0;
	}

	/* SEARCH THE FREELIST FOR A SPOT, PLACE IF SPOT IS FOUND */
	if((bp = find_fit(adjusted_size)) != NULL)
	{
		place(bp, adjusted_size);
		return bp + 8;
	}

	extend_size = MAX(adjusted_size, PAGE_SIZE);

	if((bp = extend_heap(extend_size/WSIZE)) == NULL)
	{
		return NULL;
	}
	place(bp, adjusted_size);
	return bp + 8;
}
//--------------------------------------------------------------------------
void *find_fit(size_t adjusted_size)
{
	void *bp;

	for(bp = freelist_head; bp != NULL; bp = ((sf_free_header*)bp)-> next)
	{
		if(adjusted_size <= (((sf_free_header*)bp)->header.block_size << 4))
		{
			return bp;
		}
	}

	return NULL;
}
//--------------------------------------------------------------------------
void place(void *bp, size_t adjusted_size)
{
	free_block = bp;

	remove_block(free_block);

	/* IF THE SIZE OF REQUEST IS EQUAL TO WHAT WE HAVE */
	if(((free_block->header.block_size << 4)- adjusted_size) < 32)
	{
		allocated_header = (sf_header*)free_block;
		allocated_header->alloc = 1;
		allocated_header->padding_size = adjusted_size  - 16 - i_need_a_drink; 

		my_info.internal += (allocated_header->padding_size + 16);

		sf_footer *allocated_footer = (void*)allocated_header + (allocated_header->block_size << 4) - 8;
		allocated_footer->alloc = 1;
		allocated_footer->block_size = allocated_header->block_size;
		my_info.allocations += 1;
	}
	else
	{
		allocated_header = (sf_header*)bp;
		size_t free_block_size = allocated_header->block_size - (adjusted_size >> 4);

		allocated_header->alloc = 1;
		allocated_header->block_size = adjusted_size >> 4;
		allocated_header->padding_size = adjusted_size  - 16 - i_need_a_drink;

		my_info.internal += (allocated_header->padding_size + 16);

		sf_footer *allocated_footer = (void*)allocated_header + adjusted_size - 8;
		allocated_footer->alloc = 1;
		allocated_footer->block_size = (adjusted_size >> 4);

		free_block = (void*)allocated_footer + 8;
		free_block->header.alloc = 0;
		free_block->header.block_size = free_block_size;
		free_block->next = NULL;
		free_block->prev = NULL;
		sf_footer *free_footer = (void*)free_block + (free_block->header.block_size << 4) - 8;
		free_footer->block_size = free_block->header.block_size;

		my_info.allocations += 1;

		insert_at_head(free_block);
	}	
}
//--------------------------------------------------------------------------
static void *extend_heap()
{
	sf_free_header *bp = NULL;

	bp = sf_sbrk(0);

	if((long)(sf_sbrk(PAGE_SIZE)) == -1)
	{
		return NULL;
	}	

	/* SET UP FREELIST INFO */
	bp->header.alloc = 0;
	bp->header.block_size = PAGE_SIZE >> 4;
	bp->next = NULL;
	bp->prev = NULL;
	sf_footer *free_footer = (void*)bp + PAGE_SIZE - 8;
	free_footer->block_size = PAGE_SIZE >> 4;
	free_footer->alloc = 0;

	heap_end += PAGE_SIZE;
	coalesce(bp);

	return freelist_head;
}
//--------------------------------------------------------------------------
void sf_free(void *bp)
{
	if(bp == NULL)
	{
		return;
	}

	block_header = bp - 8;

	/* CHECK IF HEADER FOOTER MATCH */
	block_footer = (sf_header*)freelist_head;
	block_footer = (void*)block_header + (block_header->block_size << 4) - 8;

	if(block_header->alloc && block_footer->alloc)
	{
		if(block_header->block_size == block_footer->block_size)
		{
			sf_free_header* block = (void*)block_header;
			block->header.alloc = 0;
			block->header.block_size = block_header->block_size;
			block->header.padding_size = 0;
			block->next = NULL;
			block->prev = NULL;

			sf_footer *block_foot = (void*)block + (block->header.block_size << 4) - 8;
			block_foot->alloc = 0;
			block_foot->block_size = block->header.block_size;

			my_info.frees += 1;
			coalesce(block);
		}
	}
	else
	{
		return;
	}
	return;
 }
//--------------------------------------------------------------------------
void *sf_realloc(void *ptr, size_t size)
{
	block_header = ptr - 8;

	size_t a_size = size;
	/* IF SIZE IS LESS THEN THE MINIMUM ALLOWED THEN ADJUST */
	if(size < DSIZE)
	{
		a_size = 2 * DSIZE;
	}
	else
	{
		a_size = DSIZE * (( size + (DSIZE) + (DSIZE - 1)) / DSIZE);
	}
	// IF SIZE == 0 JUST RETURN
	if(a_size == 0)
	{
		return NULL;
	}
	// IF SIZE == BLOCK_SIZE JUST RETURN
	else if(a_size == (block_header->block_size << 4))
	{
		return block_header;
	}
	// IF SIZE IS LESS THEN BLOCK_SIZE
	else if(a_size < (block_header->block_size << 4))
	{
		if((block_header->block_size << 4) - a_size < 32)
		{
			return block_header;
		}
		else if((block_header->block_size << 4) - a_size >= 32)
		{
			block_header->block_size = a_size;
			block_header->padding_size = a_size - 16 - size;

			sf_footer *block_foot = (void*)block_header + (block_header->block_size << 4) - 8;
			block_foot->alloc = 1;
			block_foot->block_size = block_header->block_size;

			sf_free_header *nbp = (void*)free_block;
			nbp = (void*)(block_footer + 8);
			nbp->header.block_size = (block_header->block_size - a_size);
			nbp->header.alloc = 0;
			nbp->next = NULL;
			nbp->prev = NULL;
			nbp->prev = NULL;
			sf_footer *nbp_footer = (void*)free_block + (free_block->header.block_size << 4) - 8;
			nbp_footer->block_size = free_block->header.block_size;
			nbp_footer->alloc = 0;
			coalesce(nbp);
			return block_header;
		}
		else if(a_size > (block_header->block_size << 4))
		{
			void *bp;
			void *nbp;
			nbp = block_header + (block_header->block_size << 4);
			size_t total = (block_header->block_size << 4) + (((sf_header*)nbp)->block_size << 4);

			if(((sf_header*)nbp)->alloc == 0)
			{
				if(a_size > total)
				{
					bp = sf_malloc(size);
					memcpy(bp, (void*)block_header + 8, (block_header->block_size << 4) -16);
					sf_free(block_header);
					return bp;
				}
				else if(total - a_size >= 32)
				{
					block_header->block_size = a_size;
					block_header->padding_size = a_size - 16 - size;

					sf_footer *block_foot = (void*)block_header + (block_header->block_size << 4) - 8;
					block_foot->alloc = 1;
					block_foot->block_size = block_header->block_size;

					sf_free_header *nbp = (void*)free_block;
					nbp = (void*)(block_footer + 8);
					nbp->header.block_size = (block_header->block_size - a_size);
					nbp->header.alloc = 0;
					nbp->next = NULL;
					nbp->prev = NULL;
					nbp->prev = NULL;
					sf_footer *nbp_footer = (void*)free_block + (free_block->header.block_size << 4) - 8;
					nbp_footer->block_size = free_block->header.block_size;
					nbp_footer->alloc = 0;
					coalesce(nbp);
					return block_header;
				}
				else
				{
					block_header->block_size = a_size;
					block_header->padding_size = a_size - 16 - size;

					sf_footer *block_foot = (void*)block_header + (block_header->block_size << 4) - 8;
					block_foot->alloc = 1;
					block_foot->block_size = block_header->block_size;
					return block_header;
				}
			}
		}
	}
  	return block_header;
}
//--------------------------------------------------------------------------
int sf_info(info* meminfo)
{
	void *bp;

	if(meminfo == NULL)
	{
		return -1;
	}
	meminfo->internal = my_info.internal;

	for(bp = freelist_head; bp != NULL; bp = ((sf_free_header*)bp)-> next)
	{
		meminfo->external += my_info.external;
	}
	meminfo->external = my_info.external;
	meminfo->allocations = my_info.allocations;
	meminfo->frees = my_info.frees;
	meminfo->coalesce = my_info.coalesce;
  	return 0;
}
//--------------------------------------------------------------------------
void *insert_at_head(void *bp)
{
	free_block = bp;

	if(freelist_head == NULL)
	{
		freelist_head = free_block;
		return freelist_head;
	}

	free_block->prev = NULL;
	free_block->header.padding_size = 0;
	free_block->next = freelist_head;
	freelist_head->prev = free_block;
	freelist_head = free_block;

	return freelist_head;
}
//--------------------------------------------------------------------------
void remove_block(void *bp)
{
	sf_free_header* block_remove = (bp);

	if(block_remove->prev == NULL)
	{
		if(block_remove->next != NULL)
		{
			freelist_head = block_remove->next;
		}
		else
		{
			freelist_head = NULL;
		}
	}
	else if(block_remove->prev != NULL)
	{
		if(block_remove->next == NULL)
		{
			block_remove->prev->next = NULL;
		}
	}
	else
	{
		prev_block = block_remove->prev;
		next_block = block_remove->next;
		prev_block->next = next_block;
		next_block->prev = prev_block;
	}	
}
//--------------------------------------------------------------------------
void coalesce(void *bp)
{
	free_block = bp;

	sf_footer* prev_footer = (void*)free_block - 8;
	size_t prev_alloc = prev_footer->alloc;

	prev_block = (void*)prev_footer - (prev_footer->block_size << 4) + 8;

	sf_free_header* next_header = ((void*)free_block + (free_block->header.block_size << 4));
	size_t next_alloc = next_header->header.alloc;

	next_block = (sf_free_header*)next_header;
	next_block = next_header;


	/* IF PREV AND NEXT ARE ALLOCATED JUST ADD TO THE HEAD OF THE FREE LIST*/
	if((prev_alloc == 1 || free_block == heap_begin) && next_alloc == 1)
	{
		/* ADD TO THE FREELIST HEAD */
		insert_at_head(free_block);
	}
	/* IF PREV IS ALLOCATED AND THE NEXT IS FREE */
	else if(prev_alloc == 1 && next_alloc == 0)
	{
		/* REMOVE NEXT BLOCK FROM FREELIST */
		remove_block(next_header);

		/* COMBINE THE BLOCKS */
		free_block->header.block_size = free_block->header.block_size  + next_header->header.block_size;

		sf_footer* block_foot = (void*)free_block + (free_block->header.block_size << 4) - 8;
		block_foot->block_size = free_block->header.block_size;

		my_info.coalesce += 1;

		/* INSERT BLOCK AT THE HEAD OF THE LIST */
		insert_at_head(free_block); 
	}
	/* IF PREV IS FREE AND NEXT IS ALLOCATED */
	else if(prev_alloc == 0 && (heap_end == next_block || next_alloc == 1 ))
	{
		/* REMOVE PREV BLOCK FROM FREELIST */
		remove_block(prev_block);
		
		/* COMBINE THE BLOCKS */
		prev_block->header.block_size = free_block->header.block_size + prev_block->header.block_size;

		sf_footer* block_foot = (void*)prev_block + (prev_block->header.block_size << 4) - 8;
		block_foot->block_size = prev_block->header.block_size ;

		my_info.coalesce += 1;

		/* INSERT BLOCK AT THE HEAD OF THE LIST */
		insert_at_head(prev_block);
	}
	/* PREV AND NEXT ARE BOTH FREE */
	else if(prev_alloc == 0 && next_alloc == 0)
	{
		/* REMOVE BOTH BLOCKS */
		remove_block(prev_block);
		remove_block(next_block);

		/* COMBINE ALL BLOCKS */
		prev_block->header.block_size = prev_block->header.block_size + free_block->header.block_size + next_block->header.block_size;
		prev_footer->block_size = prev_block->header.block_size;

		my_info.coalesce += 1;

		/* INSERT BLOCK AT THE HEAD OF THE LIST */
		insert_at_head(prev_block);
	}
}
//--------------------------------------------------------------------------