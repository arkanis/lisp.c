#include <stdbool.h>

#include "test_utils.h"
#include "../bytecode_generator.h"


void test_generation(){
	bytecode_t bc = bcg_init();
	
	bcg_gen_op(&bc, BC_PUSH_NIL);
	bcg_gen_op(&bc, BC_PUSH_TRUE);
	bcg_gen_op(&bc, BC_PUSH_FALSE);
	
	test(bc.length == 3, "unexpected number of instructions: %d", bc.length);
	test(bc.code[0].op == BC_PUSH_NIL, "got wrong instruction: %d", bc.code[0].op);
	test(bc.code[1].op == BC_PUSH_TRUE, "got wrong instruction: %d", bc.code[1].op);
	test(bc.code[2].op == BC_PUSH_FALSE, "got wrong instruction: %d", bc.code[2].op);
	
	bcg_gen(&bc, BC_PUSH_LITERAL, 2, 7);
	test(bc.length == 4, "unexpected number of instructions: %d", bc.length);
	test(bc.code[3].op == BC_PUSH_LITERAL, "got wrong instruction: %d", bc.code[3].op);
	test(bc.code[3].offset == 2, "got wrong offset: %d", bc.code[3].offset);
	test(bc.code[3].index == 7, "got wrong index: %d", bc.code[3].index);
	
	bcg_destroy(&bc);
}


void test_back_patching(){
	bytecode_t bc = bcg_init();
	
	// generate instructions for pseudocode: if(true) 42 else 17
					bcg_gen_op(&bc, BC_PUSH_TRUE);
	size_t false_offset =	bcg_gen(&bc, BC_JUMP_IF_FALSE, 0, 0);
					bcg_gen(&bc, BC_PUSH_NUM, 0, 42);
	size_t end_offset =	bcg_gen(&bc, BC_JUMP, 0, 0);
				bcg_backpatch_target_in(&bc, false_offset);
					bcg_gen(&bc, BC_PUSH_NUM, 0, 17);
				bcg_backpatch_target_in(&bc, end_offset);
	
	instruction_t expected_bytecode[5] = {
		(instruction_t){ BC_PUSH_TRUE },
		(instruction_t){ BC_JUMP_IF_FALSE, .index = 2 },
		(instruction_t){ BC_PUSH_NUM, .index = 42 },
		(instruction_t){ BC_JUMP, .index = 1 },
		(instruction_t){ BC_PUSH_NUM, .index = 17 },
	};
	
	test(bc.length == 5, "unexpected number of instructions: %d", bc.length);
	for(size_t i = 0; i < 5; i++){
		test(bc.code[i].op == expected_bytecode[i].op, "got wrong op at index %d, expected %d, got %d",
			i, expected_bytecode[i].op, bc.code[i].op);
		test(bc.code[i].index == expected_bytecode[i].index, "got wrong op at index %d, expected %d, got %d",
			i, expected_bytecode[i].index, bc.code[i].index);
	}
	
	bcg_destroy(&bc);
}


int main(){
	test_generation();
	test_back_patching();
	return show_test_report();
}