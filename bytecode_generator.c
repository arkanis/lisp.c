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

size_t bcg_gen(bytecode_t *bc, uint8_t op, uint16_t offset, uint32_t index){
	bc->length++;
	bc->code = realloc(bc->code, bc->length * sizeof(bc->code[0]));
	bc->code[bc->length-1] = (instruction_t){op, 0, offset, index};
	return bc->length-1;
}

size_t bcg_gen_op(bytecode_t *bc, uint8_t op){
	return bcg_gen(bc, op, 0, 0);
}


/**
 * Sets the value of the jump offset at the bytecode index `index_of_jump_instruction`
 * (this is the value returned by `bcg_gen()` when generating a jump instruction).
 * 
 * The offset is calculated as the difference between the jump instruction and the
 * index of the instruction generated next. In effect this is the number of words between
 * the jump offset and its target. After this patch the jump will jump to the instruction
 * generated after this call.
 */
void bcg_backpatch_target_in(bytecode_t *bc, size_t index_of_jump_instruction){
	size_t target_index = bc->length;
	bc->code[index_of_jump_instruction].index = target_index - index_of_jump_instruction - 1;
}