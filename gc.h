#ifndef _LISP_C_GC_H
#define _LISP_C_GC_H

// The simple _GC_H guard would prevent the real collectors gc.h from being included. Therefore use a more local guard.

#include <stddef.h>

void gc_init();
void *gc_alloc(size_t size);
void *gc_realloc(void *ptr, size_t size);
void gc_free(void *ptr);
size_t gc_heap_size();

#endif