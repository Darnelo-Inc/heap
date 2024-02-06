#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

typedef enum {
    HEAP_SUCCESS,
    HEAP_FAILURE,
} heap_e;

struct heapchunk_t {
    int prevsize;
    int size;
    bool inuse;
    struct heapchunk_t *next;
} __attribute__((aligned(16)));

struct heapinfo_t {
    struct heapchunk_t *start;
    int avail;
};

struct heapinfo_t heap = {0};

heap_e init_heap(struct heapinfo_t *heap) {
    void *start = mmap(NULL, getpagesize(), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(start == (void*)-1) {
        perror("mmap");
        return HEAP_FAILURE;
    }
    
    struct heapchunk_t *first = (struct heapchunk_t*)(start);
    first->size = getpagesize() - sizeof(struct heapchunk_t);
    first->inuse = false;
    first->next = NULL;
    first->prevsize = 0;    

    heap->start = first;
    heap->avail = first->size;

    return HEAP_SUCCESS;
}

void *heap_alloc(size_t size) {
    size = (size + 15) & ~0xf;

    if(heap.avail < size) {
        printf("You want too much from this life...\n");
        return (void*)-1;
    }

    struct heapchunk_t *chunk = heap.start;

    while(chunk != NULL && chunk->size < size) {
        chunk = chunk->next;
    }

    if(chunk == NULL) {
        printf("How did we get here?\n");
        return (void*)-1;
    }

    chunk->size = size;
    chunk->inuse = true;

    struct heapchunk_t *next = (struct heapchunk_t*)(((char*)chunk) + sizeof(struct heapchunk_t) + size);

    heap.start = next;
    heap.avail -= (sizeof(struct heapchunk_t) + size);
    
    next->size = heap.avail;    
    next->prevsize = size;

    return (void*)(((char*)chunk) + sizeof(struct heapchunk_t));
}

heap_e heap_free(void* data) {
    struct heapchunk_t *chunk = &((struct heapchunk_t*)data)[-1];
    
    struct heapchunk_t *prevchunk = NULL;
    if(chunk->prevsize) {
        prevchunk = (struct heapchunk_t*)(((char*)chunk) - chunk->prevsize - sizeof(struct heapchunk_t));
    }

    if(prevchunk && !prevchunk->inuse) {
        prevchunk->size += (chunk->size) + sizeof(struct heapchunk_t);
        struct heapchunk_t *oldfirst = heap.start;
        heap.start = prevchunk;
        prevchunk->next = oldfirst;
        heap.avail += chunk->size + sizeof(struct heapchunk_t); 
    } else {
        struct heapchunk_t *oldfirst = heap.start;
        heap.start = chunk;
        chunk->next = oldfirst;    
        heap.avail += chunk->size;
    }

    chunk->inuse = false;
    
    return HEAP_SUCCESS;
}

int main(int argc, char *argv[]) {
    if(init_heap(&heap) == HEAP_FAILURE) {
        printf("Failed to init heap\n");
        return -1;
    }
    
    printf("start avail: %x\n", heap.avail);

    char *a = heap_alloc(32);
    char *b = heap_alloc(32);
    char *c = heap_alloc(32);
    char *d = heap_alloc(32);
    
    heap_free(b);
    heap_free(c);
    heap_free(a);
    heap_free(d);
    
    printf("finish avail: %x\n", heap.avail);    
    printf("expected start-40-40-40-40+20+40+20+40==finish\n");
	
    return 0;
}
