// Use the Boehm-Demers-Weiser conservative garbage collector for now.
// This file has to be included before the local gc.h. Otherwise the macros will
// not be expanded properly (no idea why).
#include <gc.h>

#include "gc.h"


void gc_init(){
	GC_INIT();
}

void *gc_alloc(size_t size){
	return GC_MALLOC(size);
}

void *gc_realloc(void *ptr, size_t size){
	return GC_REALLOC(ptr, size);
}

void gc_free(void *ptr){
	// Do nothing for now
}

size_t gc_heap_size(){
	return GC_get_heap_size();
}