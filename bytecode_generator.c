#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "bytecode_generator.h"

bytecode_t bcg_init(){
	return (bytecode_t){ .length = 0, .code = NULL };
}

void bcg_destroy(bytecode_t *bc){
	gc_free(bc->code);
	bc->code = NULL;
	bc->length = 0;
}

size_t bcg_gen(bytecode_t *bc, instruction_t instruction){
	bc->length++;
	bc->code = gc_realloc(bc->code, bc->length * sizeof(bc->code[0]));
	bc->code[bc->length-1] = instruction;
	
	
	if (instruction.op == BC_LOAD_NIL)
		printf("bc %p: BC_LOAD_NIL\n", bc);
	else if (instruction.op == BC_LOAD_TRUE)
		printf("bc %p: BC_LOAD_TRUE\n", bc);
	else if (instruction.op == BC_LOAD_FALSE)
		printf("bc %p: BC_LOAD_FALSE\n", bc);
	else if (instruction.op == BC_LOAD_NUM)
		printf("bc %p: BC_LOAD_NUM num %d\n", bc, instruction.num);
	else if (instruction.op == BC_LOAD_LITERAL)
		printf("bc %p: BC_LOAD_LITERAL index %d, offset %d\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_LOAD_ARG)
		printf("bc %p: BC_LOAD_ARG index %d, offset %d\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_LOAD_LOCAL)
		printf("bc %p: BC_LOAD_LOCAL index %d, offset %d\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_STORE_LOCAL)
		printf("bc %p: BC_STORE_LOCAL index %d, offset %d\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_LOAD_ENV)
		printf("bc %p: BC_LOAD_ENV index %d (offset %d, unused)\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_STORE_ENV)
		printf("bc %p: BC_STORE_ENV index %d (offset %d, unused)\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_DROP)
		printf("bc %p: BC_DROP\n", bc);
	else if (instruction.op == BC_LOAD_LAMBDA)
		printf("bc %p: BC_LOAD_LAMBDA index %d, offset %d\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_CALL)
		printf("bc %p: BC_CALL num %d\n", bc, instruction.num);
	else if (instruction.op == BC_RETURN)
		printf("bc %p: BC_RETURN\n", bc);
	else if (instruction.op == BC_JUMP)
		printf("bc %p: BC_JUMP jump offset %d\n", bc, instruction.jump_offset);
	else if (instruction.op == BC_JUMP_IF_FALSE)
		printf("bc %p: BC_JUMP_IF_FALSE jump offset %d\n", bc, instruction.jump_offset);
	else if (instruction.op == BC_ADD)
		printf("bc %p: BC_ADD\n", bc);
	else if (instruction.op == BC_SUB)
		printf("bc %p: BC_SUB\n", bc);
	else if (instruction.op == BC_MUL)
		printf("bc %p: BC_MUL\n", bc);
	else if (instruction.op == BC_DIV)
		printf("bc %p: BC_DIV\n", bc);
	else if (instruction.op == BC_MOD)
		printf("bc %p: BC_MOD\n", bc);
	else if (instruction.op == BC_EQ)
		printf("bc %p: BC_EQ\n", bc);
	else if (instruction.op == BC_LT)
		printf("bc %p: BC_LT\n", bc);
	else if (instruction.op == BC_GT)
		printf("bc %p: BC_GT\n", bc);
	else if (instruction.op == BC_CONS)
		printf("bc %p: BC_CONS\n", bc);
	else if (instruction.op == BC_FIRST)
		printf("bc %p: BC_FIRST\n", bc);
	else if (instruction.op == BC_REST)
		printf("bc %p: BC_REST\n", bc);
	
	
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