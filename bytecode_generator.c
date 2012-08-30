#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

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
	
	/*
	if (instruction.op == BC_PUSH_NIL)
		printf("bc %p: BC_PUSH_NIL\n", bc);
	else if (instruction.op == BC_PUSH_TRUE)
		printf("bc %p: BC_PUSH_TRUE\n", bc);
	else if (instruction.op == BC_PUSH_FALSE)
		printf("bc %p: BC_PUSH_FALSE\n", bc);
	else if (instruction.op == BC_PUSH_NUM)
		printf("bc %p: BC_PUSH_NUM num %d\n", bc, instruction.num);
	else if (instruction.op == BC_PUSH_LITERAL)
		printf("bc %p: BC_PUSH_LITERAL index %d, offset %d\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_PUSH_ARG)
		printf("bc %p: BC_PUSH_ARG index %d, offset %d\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_PUSH_VAR)
		printf("bc %p: BC_PUSH_VAR index %d, offset %d\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_SAVE_VAR)
		printf("bc %p: BC_SAVE_VAR index %d, offset %d\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_PUSH_FROM_ENV)
		printf("bc %p: BC_PUSH_FROM_ENV index %d (offset %d, unused)\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_SAVE_ENV)
		printf("bc %p: BC_SAVE_ENV index %d (offset %d, unused)\n", bc, instruction.index, instruction.offset);
	else if (instruction.op == BC_DROP)
		printf("bc %p: BC_DROP\n", bc);
	else if (instruction.op == BC_LAMBDA)
		printf("bc %p: BC_LAMBDA index %d, offset %d\n", bc, instruction.index, instruction.offset);
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
	else if (instruction.op == BC_EQ)
		printf("bc %p: BC_EQ\n", bc);
	else if (instruction.op == BC_CONS)
		printf("bc %p: BC_CONS\n", bc);
	else if (instruction.op == BC_FIRST)
		printf("bc %p: BC_FIRST\n", bc);
	else if (instruction.op == BC_REST)
		printf("bc %p: BC_REST\n", bc);
	*/
	
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