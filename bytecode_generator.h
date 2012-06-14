#ifndef _BYTECODE_GENERATOR_H
#define _BYTECODE_GENERATOR_H

#include <stdint.h>
#include "bytecode.h"

bytecode_t bcg_init();
void bcg_destroy(bytecode_t *bc);
size_t bcg_gen(bytecode_t *bc, uint8_t op, uint16_t offset, uint32_t index);
size_t bcg_gen_op(bytecode_t *bc, uint8_t op);
void bcg_backpatch_target_in(bytecode_t *bc, size_t index_of_jump_instruction);

#endif