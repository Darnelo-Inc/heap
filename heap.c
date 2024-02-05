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
    
    printf("%p\n", start);    
    
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

    printf("Found chunk: %p\n", chunk);

    chunk->size = size;
    chunk->inuse = true;

    struct heapchunk_t *next = (struct heapchunk_t*)(((char*)chunk) + size);

    heap.start = next;
    heap.avail -= (sizeof(struct heapchunk_t) + size);
    
    printf("Setting %p->prevsize to %d\n", next, size);
    next->size = heap.avail;    
    next->prevsize = size;

    return (void*)(((char*)chunk) + sizeof(struct heapchunk_t));
}

heap_e heap_free(void* data) {
    struct heapchunk_t *chunk = &((struct heapchunk_t*)data)[-1];
    printf("chunk->size: %d\n", chunk->size);
    
    struct heapchunk_t *prevchunk = NULL;
    if(chunk->prevsize) {
        printf("prevsize: %d\n", chunk->prevsize);
        prevchunk = (struct heapchunk_t*)(((char*)chunk) - chunk->prevsize);
    }

    if(prevchunk && !prevchunk->inuse) {
        printf("The chunk behind us is freeeee\n");
        prevchunk->size = (chunk->size) + sizeof(struct heapchunk_t);
        struct heapchunk_t *oldfirst = heap.start;
        heap.start = prevchunk;
        prevchunk->next = oldfirst; 
    } else {
        struct heapchunk_t *oldfirst = heap.start;
        heap.start = chunk;
        chunk->next = oldfirst;    
        chunk->inuse = false;
    }

    return HEAP_SUCCESS;
}

int main(int argc, char *argv[]) {
     printf("heap.start: %d, heap.avail: %d\n", heap.start, heap.avail);

     if(init_heap(&heap) == HEAP_FAILURE) {
        printf("Failed to init heap\n");
        return -1;
    }
    
    char *a = heap_alloc(32);
    printf("Got %p for a\n", a);

    char *b = heap_alloc(32);
    printf("Got %p for b\n", b);

    heap_free(a);
    heap_free(b);    

    char *c = heap_alloc(64);
    printf("Got %p for c\n", c);
    
    char *d = heap_alloc(64);
    printf("Got %p for d\n", d);

	return 0;
}
