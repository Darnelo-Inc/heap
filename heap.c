#include <stdio.h>
#include <stdbool.h>

struct heapchunk_t {
    int size;
    bool inuse;
    struct heapchink_t *next;
}

struct heapinfo_t {
    struct heapchunk_t *start;
} 

int main(int argc, char *argv[]) {
    printf("Hello world!\n");

	return 0;
}
