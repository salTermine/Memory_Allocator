#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sfmm.h"

#include <errno.h>


/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */

Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(sizeof(int));
    *x = 4;
    cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(short));
    sf_free(pointer);
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
    sf_footer *sfFooter = (sf_footer *) (pointer - 8 + (sfHeader->block_size << 4));
    cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, PaddingSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini) {
    void *pointer = sf_malloc(sizeof(char));
    pointer = pointer - 8;
    sf_header *sfHeader = (sf_header *) pointer;
    cr_assert(sfHeader->padding_size == 15, "Header padding size is incorrect for malloc of a single char!\n");
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    memset(x, 0, 4);
    cr_assert(freelist_head->next == NULL);
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
    int *x = sf_malloc(4);
    int *y = sf_malloc(4);
    memset(y, 0xFF, 4);
    sf_free(x);
    cr_assert(freelist_head == (void*)x-8);
    sf_free_header *headofx = (void*) x-8;

    sf_footer *footofx = (void*)headofx + (headofx->header.block_size << 4) - 8;

    // All of the below should be true if there was no coalescing
    cr_assert(headofx->header.alloc == 0);
    cr_assert((headofx->header.block_size << 4) == 32);
    cr_assert(headofx->header.padding_size == 0);

    cr_assert(footofx->alloc == 0);
    cr_assert((footofx->block_size << 4) == 32);
}

/*
//############################################
// STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
// DO NOT DELETE THESE COMMENTS
//############################################
*/

Test(sf_memsuite, Malloc_2_pages, .init = sf_mem_init, .fini = sf_mem_fini) 
{
    printf("TEST 7\n");
    void *x = sf_malloc(4096 * 2 - 16);
    sf_varprint(x);
    sf_snapshot(1);
}

Test(sf_memsuite, Malloc_3_pages, .init = sf_mem_init, .fini = sf_mem_fini) 
{
    printf("TEST 8\n");
    void *x = sf_malloc(4096 * 3 - 16);
    sf_varprint(x);
    sf_snapshot(1);
}

Test(sf_memsuite, Malloc_3_free_1, .init = sf_mem_init, .fini = sf_mem_fini) 
{
    printf("TEST 9\n");
    void *x = sf_malloc(15);
    memset(x, 0, 15);
    void *y = sf_malloc(sizeof(int));
    void *z = sf_malloc(sizeof(int));

    sf_free(y);

    sf_free_header *headofx = (void*)x - 8;
    cr_assert(headofx->header.padding_size == 1);
    cr_assert((headofx->header.block_size << 4) == 32);

    cr_assert((freelist_head->header.block_size << 4) == 32);
    sf_free_header *headofz = (void*) z-8;
    cr_assert((headofz->header.block_size << 4) == 32);
}

Test(sf_memsuite, Malloc_4_pages, .init = sf_mem_init, .fini = sf_mem_fini) 
{
    printf("TEST 10\n");
    void* x = sf_malloc(4096 * 4);
    memset(x, 1, 4096 * 4);
    sf_free(x);
    cr_assert(freelist_head->header.block_size << 4 == (4096 * 4));
}

Test(sf_memsuite, Malloc_5_pages, .init = sf_mem_init, .fini = sf_mem_fini) 
{
    printf("TEST 11\n");
    void* x = sf_malloc(4096 * 5);
    memset(x, 1, 4096 * 5);
    sf_free(x);
    cr_assert(freelist_head->header.block_size << 4 == (4096 * 4));
}
Test(sf_memsuite, Malloc_print_allocations, .init = sf_mem_init, .fini = sf_mem_fini) 
{
    printf("TEST 12\n");
    void *x = sf_malloc(sizeof(int));
    void *y = sf_malloc(sizeof(int));
    void *z = sf_malloc(sizeof(int));
    void *q = sf_malloc(sizeof(int));
    void *r = sf_malloc(sizeof(int));
    sf_free(x);
    sf_free(q);
    sf_free(y);
    sf_free(r);
    sf_free(z);

    info stat;
    sf_info(&stat);
    printf("internal:%li\n", stat.internal);
    printf("external:%li\n", stat.external);
    printf("allocations:%li\n", stat.allocations);
    printf("frees:%li\n", stat.frees);
    printf("coalesce:%li\n", stat.coalesce);

    cr_assert(stat.frees == 5);
    cr_assert(stat.allocations == 5);

}

Test(sf_memsuite, free, .init = sf_mem_init, .fini = sf_mem_fini)
{
    printf("TEST 13\n");
    sf_malloc(4);
    void *x = sf_malloc(sizeof(int));
    void *y = sf_malloc(sizeof(int));
    void *z = sf_malloc(4);
    memset(z, 0, 4);

    sf_free(x);
    cr_assert(freelist_head == x - 8);
    sf_free(y);
    sf_free(z);
    sf_blockprint(freelist_head);
    cr_assert(((freelist_head->header.block_size << 4)== 4064));

}

Test(sf_memsuite, free_second, .init = sf_mem_init, .fini = sf_mem_fini)
{
    printf("TEST 14\n");

    void *z = sf_malloc(4);
    memset(z, 0, 4);
    void *zz = sf_malloc(4);
    memset(zz, 0, 4);
    void *zzz = sf_malloc(4);
    memset(zzz, 0, 4);

    sf_free(zzz);
    sf_free(z);
    sf_free(zz);

    sf_footer* freelist_foot = (void*)freelist_head + (freelist_head->header.block_size << 4) - 8;
    cr_assert(freelist_foot->alloc == 0);
    
    cr_assert(freelist_head->next != NULL);
    cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, fourteen, .init = sf_mem_init, .fini = sf_mem_fini)
{
    printf("TEST 15\n");

    void *x = sf_malloc(sizeof(int));
    void *y = sf_malloc(sizeof(int));

    sf_header *headofx = (sf_header*)x - 8;

    sf_free(x + 8);
    sf_free(y);

    cr_assert(headofx->alloc == 0);
    sf_varprint(headofx);
    cr_assert((headofx->block_size >> 4) == 4096);
    sf_footer *xfooter = (sf_footer*)x + headofx->block_size - 8;
    cr_assert(xfooter->alloc == 0);
}

Test(sf_memsuite, sf_info, .init = sf_mem_init, .fini = sf_mem_fini) 
{
    printf("TEST 16\n");

    sf_malloc(sizeof(int));
    void* x = sf_malloc(64);
    memset(x, 0, 64);
    sf_free(x);

    info stat;
    sf_info(&stat);
    printf("internal:%li\n", stat.internal);
    printf("external:%li\n", stat.external);
    printf("allocations:%li\n", stat.allocations);
    printf("frees:%li\n", stat.frees);
    printf("coalesce:%li\n", stat.coalesce);

    cr_assert(stat.frees == 1);
    cr_assert(stat.allocations == 2);
}


//-----------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------

