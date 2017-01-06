#include <stdio.h>
#include <stdlib.h>

/* safer malloc */
void *my_malloc(int i) {
	void *mem;
	
	mem = (void*)malloc(i);
	if (mem == NULL) {
		printf("memory allocation failed!");
		exit(-1);
	}

	return mem;
}

/* safer realloc */
void *my_realloc(void *memblk, int i) {
	void *mem;
	printf("Doing realloc %d %p\n", i, memblk);
	mem = (void*)realloc(memblk, i);
    printf("Done realloc\n");
	if (mem == NULL) {
		printf("memory allocation failed!");
		exit(-1);
	}

	return mem;
}

