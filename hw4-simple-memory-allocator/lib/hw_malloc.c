#include <stdlib.h>
#include <stdio.h>
#include "hw_malloc.h"

#include <sys/mman.h>
#include <math.h>

#define MMAP_THRESHOLD 32 * 1024
#define HEAP_INITIAL 64 * 1024
#define SBRKERR (void *) - 1
void * ptr;
bool heap_flag;

void *hw_malloc(size_t bytes)
{
    // printf("MALLOC\n");
    void * data = NULL;
    // Require Bytes
    size_t require_byte = bytes + 24;
    if(require_byte > MMAP_THRESHOLD) {
        heap_flag = false;
        data = mmap_allocate(require_byte);
        return data;
    } else {
        heap_flag = true;
    }

    if(heap_flag) {
        if(!first_time) {
            if((ptr = sbrk(HEAP_INITIAL)) == SBRKERR) {
                printf("sbrk unsuccessful\n");
            } else {
                first_time = true;
                mmap_alloc_list = NULL;
                start_brk = ptr;
                // printf("%p\n", start_brk);
                ptr = sbrk(0);
                // printf("%p\n", ptr);
                int i;
                for(i = 0; i < 11; i ++) {
                    bin[i] = NULL;
                }
                int index = get_proper_bin(require_byte);
                // printf("index %d\n", index);
                split_to_index(index);
            }
        }
        // allocate bytes
        int index = get_proper_bin(require_byte);
        data = heap_allocate(index);
    }
    return data;
}

int hw_free(void *mem)
{
    void * address;
    address = mem;
    // printf("%p\n", address);
    address -= 24;
    size_t size = deleteNode(&mmap_alloc_list, address);
    // printf("size %ld\n", size);
    if(size != 0) {
        // delete mmap
        munmap(address, size);
        return size;
    } else {
        address = start_brk + (size_t)mem;
        // printf("%p\n", address);
        address -= 24;
        // printf("%p\n", address);
        int size = deleteNode(&allocated_heap, address);
        // printf("delete return size is %d\n", size);
        if(!size) {
            return 0;
        }
        int index = get_proper_bin(size);
        chunk_header_t * new_node = newNode(address, 0, size, 0, 0);
        insertSorted(&(bin[index]), new_node);
        merge(index);
        // printBins();
        return size;
    }
}

void * get_start_sbrk(void)
{
    return start_brk;
}

void * mmap_allocate(size_t bytes)
{
    void * p = mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    chunk_header_t * theNode = newNode(p, 0, bytes, true, true);
    mmap_insertSorted(&mmap_alloc_list, theNode);
    return (void *)theNode + 24;
}

void * heap_allocate(int index)
{
    void * data = NULL;
    // printList(bin[index]);
    if(bin[index] == NULL) {
        // have to split
        printf("you have to split\n");

        // find the lowest available chunk to split
        data = split_to_allocate(index);
    } else {
        data = normal_allocate(index);
    }
    return data;
}
void * normal_allocate(int index)
{
    chunk_header_t * pop = popNode(&(bin[index]));
    pop->chuck_info.allocate_flag = true;
    insertSorted(&allocated_heap, pop);
    return (void *)pop + 24;
}

void * split_to_allocate(int index)
{
    int i;
    int split_index = -1;
    void * tmp = get_start_sbrk() + 0x10000;
    for(i = index + 1; i < 11; i ++) {
        if(bin[i] != NULL && (void *)bin[i] < tmp) {
            tmp = bin[i];
            split_index = i;
        }
    }
    chunk_header_t * theRoot = bin[split_index];
    deleteNode(&(bin[split_index]), theRoot);

    split_index --;

    while(split_index>= index) {
        void * address = (void*)theRoot + correspond_bytes(split_index + 5);
        chunk_header_t * new_node = newNode(address, 0, correspond_bytes(split_index + 5), 0, 0);
        insertSorted(&(bin[split_index]),new_node);
        split_index--;
    }
    chunk_header_t * theNode = newNode(theRoot, 0, correspond_bytes(index + 5), true, 0);
    insertSorted(&allocated_heap, theNode);
    return (void *)theNode + 24;
}

void split_to_index(int index)
{
    int i = 10;
    void * root = get_start_sbrk();

    while(i >= index) {
        void * address = root + correspond_bytes(i + 5);
        chunk_header_t * new_node = newNode(address, 0, correspond_bytes(i + 5), 0, 0);
        insertSorted(&(bin[i]),new_node);
        i--;
    }
    chunk_header_t * new_node = newNode(root, 0, correspond_bytes(index + 5), 0, 0);
    insertSorted(&(bin[index]),new_node);
}

void merge(int index)
{
    chunk_header_t * curr = bin[index];
    if(curr->next == NULL) {
        return;
    }

    chunk_header_t * prev = NULL;
    while((void*)curr + curr->chuck_info.curr_chunk_size != curr->next && curr->next->next != NULL) {
        prev = curr;
        curr = curr->next;
    }

    if((void*)curr + curr->chuck_info.curr_chunk_size == curr->next) {
        if(curr->next->next == NULL) {
            if(prev == NULL) {
                curr->next = NULL;
                bin[index] = NULL;
            } else {
                curr->next = NULL;
                prev->next = NULL;
            }
            chunk_header_t * new_node = newNode(curr, 0, curr->chuck_info.curr_chunk_size * 2, 0, 0);
            curr = NULL;
            insertSorted(&(bin[index + 1]), new_node);
            // printf("index is %d\n", index +1);
            // printList(bin[index + 1]);
            if(index + 1 == 10) {
                return;
            } else {
                merge(index + 1);
            }
            return;
        } else {
            if(prev == NULL) {
                curr->next->next->prev = NULL;
                bin[index] = curr->next->next;
                curr->next = NULL;
            } else {
                curr->next->next->prev = prev;
                prev->next = curr->next->next;
                curr->next =NULL;
            }
            chunk_header_t * new_node = newNode(curr, 0, curr->chuck_info.curr_chunk_size * 2, 0, 0);
            curr = NULL;
            insertSorted(&(bin[index + 1]), new_node);
            // printf("index is %d\n", index +1);
            // printList(bin[index + 1]);
            if(index + 1 == 10) {
                return;
            } else {
                merge(index + 1);
            }
            return;
        }
    }
    return;
}

void insertSorted(chunk_header_t ** head, chunk_header_t * theNode)
{
    if(*head == NULL) {
        *head = theNode;
    } else if( *head > theNode) {
        theNode->next = *head;
        theNode->next->prev = theNode;
        *head = theNode;
    } else {
        chunk_header_t * curr = *head;
        while(curr->next !=NULL && curr->next < theNode) {
            curr = curr->next;
        }
        theNode->next = curr->next;
        if(curr->next !=NULL) {
            theNode->next->prev = theNode;
        }
        curr->next = theNode;
        theNode->prev = curr;
    }
}

void mmap_insertSorted(chunk_header_t ** head, chunk_header_t * theNode)
{
    if(*head == NULL) {
        *head = theNode;
    } else if( (*head)->chuck_info.curr_chunk_size > theNode->chuck_info.curr_chunk_size) {
        theNode->next = *head;
        theNode->next->prev = theNode;
        *head = theNode;
    } else {
        chunk_header_t * curr = *head;
        while(curr->next !=NULL && curr->next->chuck_info.curr_chunk_size < theNode->chuck_info.curr_chunk_size) {
            curr = curr->next;
        }
        theNode->next = curr->next;
        if(curr->next !=NULL) {
            theNode->next->prev = theNode;
        }
        curr->next = theNode;
        theNode->prev = curr;
    }
}

chunk_header_t * newNode(void * address, int prev, int curr, int alloc, int mmap)
{
    chunk_header_t * node = (void *)address;
    node->chuck_info.allocate_flag = alloc;
    node->chuck_info.mmap_flag = mmap;
    node->chuck_info.prev_chunk_size = prev;
    node->chuck_info.curr_chunk_size = curr;
    node->next = NULL;
    node->prev = NULL;

    return node;
}

void printList(chunk_header_t * head)
{
    if(head == NULL) {
        printf("The List Is Empty\n");
        return;
    }
    while (head != NULL) {
        if(head->chuck_info.mmap_flag == 1) {
            printf("%p--------%d\n", (void *)head, head->chuck_info.curr_chunk_size);
        } else {
            printf("%p--------%d\n", (void *)((void *)head - get_start_sbrk()), head->chuck_info.curr_chunk_size);
        }
        head = head->next;
    }
    return;
}

void printBins()
{
    int i = 0;
    for(i = 0; i < 11; i++) {
        printf("Bin[%d]\n", i);
        printList(bin[i]);
    }
}

int deleteNode(chunk_header_t ** head, void * del)
{
    size_t size = 0;;
    /* base case */
    if (*head == NULL || del == NULL) {
        return 0;
    }

    /* If node to be deleted is head node */
    if (*head == del) {
        size = (*head)->chuck_info.curr_chunk_size;
        if((*head)->next != NULL) {
            *head = (*head)->next;
            (*head)->prev = NULL;
        } else {
            *head = NULL;
        }
        return size;
    } else if((*head) != del && (*head)->next == NULL) {
        return 0;
    }

    chunk_header_t * curr = *head;
    chunk_header_t * prev;
    while(curr->next != NULL && curr != del) {
        prev = curr;
        curr = curr->next;
    }

    if(curr == del) {
        prev->next = prev->next->next;
        if(prev->next != NULL) {
            prev->next->prev = prev;
        } else {
            prev->next = NULL;
        }
    } else {
        return 0;
    }
    size = curr->chuck_info.curr_chunk_size;
    curr->next = NULL;
    curr->prev = NULL;
    curr = NULL;
    return size;
}

chunk_header_t * popNode(chunk_header_t ** head)
{
    if(*head == NULL) {
        // nothing to pop
        return NULL;
    }

    if(*head != NULL && (*head)->next == NULL && (*head)->prev == NULL) {
        // this is the only node
        chunk_header_t * tmp = *head;
        *head = NULL;
        return tmp;
    } else {
        chunk_header_t * tmp = *head;
        *head = (*head)->next;
        (*head)->prev = NULL;
        tmp->next = NULL;
        tmp->prev = NULL;
        return tmp;
    }
}

int get_proper_bin(size_t bytes)
{
    if(bytes > 24 && bytes <= POW2TO5) return 0;
    else if(bytes > POW2TO5 && bytes <= POW2TO6) return 1;
    else if(bytes > POW2TO6 && bytes <= POW2TO7) return 2;
    else if(bytes > POW2TO7 && bytes <= POW2TO8) return 3;
    else if(bytes > POW2TO8 && bytes <= POW2TO9) return 4;
    else if(bytes > POW2TO9 && bytes <= POW2TO10) return 5;
    else if(bytes > POW2TO10 && bytes <= POW2TO11) return 6;
    else if(bytes > POW2TO11 && bytes <= POW2TO12) return 7;
    else if(bytes > POW2TO12 && bytes <= POW2TO13) return 8;
    else if(bytes > POW2TO13 && bytes <= POW2TO14) return 9;
    else if(bytes > POW2TO14 && bytes <= POW2TO15) return 10;
    else return -1;
}

size_t correspond_bytes(int index)
{
    switch(index) {
    case 5:
        return POW2TO5;
        break;
    case 6:
        return POW2TO6;
        break;
    case 7:
        return POW2TO7;
        break;
    case 8:
        return POW2TO8;
        break;
    case 9:
        return POW2TO9;
        break;
    case 10:
        return POW2TO10;
        break;
    case 11:
        return POW2TO11;
        break;
    case 12:
        return POW2TO12;
        break;
    case 13:
        return POW2TO13;
        break;
    case 14:
        return POW2TO14;
        break;
    case 15:
        return POW2TO15;
        break;
    default:
        return 0;
    }
}