#ifndef HW_MALLOC_H
#define HW_MALLOC_H

#define POW2TO5 32
#define POW2TO6 64
#define POW2TO7 128
#define POW2TO8 256
#define POW2TO9 512
#define POW2TO10 1024
#define POW2TO11 2048
#define POW2TO12 4096
#define POW2TO13 8192
#define POW2TO14 16384
#define POW2TO15 32768

#include <stdbool.h>
#include <unistd.h>

typedef size_t chunk_ptr_t;

typedef struct {
    int prev_chunk_size : 31;
    int curr_chunk_size : 31;
    bool allocate_flag : 1;
    bool mmap_flag :1;
} chunk_info_t;

typedef struct node {
    chunk_info_t chuck_info;
    struct node * prev;
    struct node * next;
} chunk_header_t;

void *hw_malloc(size_t bytes);
int hw_free(void *mem);
void * get_start_sbrk(void);

chunk_header_t * bin[11];

void * start_brk;
bool first_time;

void split_to_index(int index);
int get_proper_bin(size_t bytes);
size_t correspond_bytes(int index);
void printList(chunk_header_t * head);
void printBins();
void * heap_allocate(int index);
chunk_header_t * popNode(chunk_header_t ** head);
void merge(int index);
void * split_to_allocate(int index);
void * normal_allocate(int index);
int deleteNode(chunk_header_t ** head, void * del);
void * mmap_allocate(size_t bytes);
void mmap_insertSorted(chunk_header_t ** head, chunk_header_t * theNode);
void insertSorted(chunk_header_t ** head, chunk_header_t * theNode);
chunk_header_t * newNode(void * address, int prev, int curr, int alloc, int mmap);

chunk_header_t * mmap_alloc_list;
chunk_header_t * allocated_heap;
#endif
