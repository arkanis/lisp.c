#ifndef _BYTECODE_COMPILER_H
#define _BYTECODE_COMPILER_H

#include "memory.h"

/*

Compiled lambda:
| constant table
| bytecode

Frame
| Locals (with args first)
| Stack

*/

typedef struct {
	size_t length;
	char *symbols[];
} symbol_list_t;

void register_compiler_buildins_in(env_t *env);

#endif