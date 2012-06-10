#include <stdlib.h>

#include "bytecode_generator.h"

bytecode_t bcg_init(){
	return (bytecode_t){ .length = 0, .code = NULL };
}

void bcg_destroy(bytecode_t *bc){
	free(bc->code);
	bc->code = NULL;
	bc->length = 0;
}

void bcg_gen(bytecode_t *bc, int instruction){
	bc->length++;
	bc->code = realloc(bc->code, bc->length * sizeof(bc->code[0]));
	bc->code[bc->length-1] = instruction;
}

size_t bcg_gen_with_arg(bytecode_t *bc, int instruction, int arg){
	bc->length += 2;
	bc->code = realloc(bc->code, bc->length * sizeof(bc->code[0]));
	bc->code[bc->length-2] = instruction;
	bc->code[bc->length-1] = arg;
	return bc->length-1;
}

/**
 * Sets the value of the jump offset at the bytecode index `jump_offset_index` (this is
 * the value returned by `bcg_gen_with_arg()` when generating a jump instruction).
 * 
 * The offset is calculated as the difference between the jump offset index (not the
 * jump instruction!) and the index of the instruction generated next. In effect this is
 * the number of words between the jump offset and its target. After this patch the
 * jump will jump to the instruction generated after this call.
 */
void bcg_backpatch_target_in(bytecode_t *bc, size_t jump_offset_index){
	size_t target_index = bc->length;
	bc->code[jump_offset_index] = target_index - jump_offset_index - 1;
}