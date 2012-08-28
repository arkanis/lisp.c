#include <stdlib.h>
#include <assert.h>

#include "bytecode_generator.h"

bytecode_t bcg_init(){
	return (bytecode_t){ .length = 0, .code = NULL };
}

void bcg_destroy(bytecode_t *bc){
	free(bc->code);
	bc->code = NULL;
	bc->length = 0;
}

size_t bcg_gen(bytecode_t *bc, instruction_t instruction){
	bc->length++;
	bc->code = realloc(bc->code, bc->length * sizeof(bc->code[0]));
	bc->code[bc->length-1] = instruction;
	return bc->length-1;
}

size_t bcg_gen_op(bytecode_t *bc, uint8_t op){
	return bcg_gen(bc, (instruction_t){op});
}


/**
 * Sets the value of the jump offset at the bytecode index `index_of_jump_instruction`
 * (this is the value returned by `bcg_gen()` when generating a jump instruction).
 * 
 * The offset is calculated as the difference between the jump instruction and the
 * index of the instruction generated next. In effect this is the number of instructions
 * between the jump offset and its target. After this patch the jump will jump to the
 * instruction generated after this call.
 * 
 * The function asserts that only jump instructions are patched.
 */
void bcg_backpatch_target_in(bytecode_t *bc, size_t index_of_jump_instruction){
	size_t target_index = bc->length;
	assert(bc->code[index_of_jump_instruction].op == BC_JUMP || bc->code[index_of_jump_instruction].op == BC_JUMP_IF_FALSE);
	bc->code[index_of_jump_instruction].jump_offset = target_index - index_of_jump_instruction - 1;
}