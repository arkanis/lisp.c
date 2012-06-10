#ifndef _BYTECODE_GENERATOR_H
#define _BYTECODE_GENERATOR_H

#include <stdint.h>
#include "bytecode.h"

typedef struct {
	size_t length;
	int64_t *code;
} bytecode_t;

bytecode_t bcg_init();
void bcg_destroy(bytecode_t *bc);
void bcg_gen(bytecode_t *bc, int instruction);
size_t bcg_gen_with_arg(bytecode_t *bc, int instruction, int arg);
void bcg_backpatch_target_in(bytecode_t *bc, size_t addr_index);

#endif