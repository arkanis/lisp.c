#include <stdbool.h>

#include "test_utils.h"
#include "../bytecode_generator.h"


void test_generation(){
	bytecode_t bc = bcg_init();
	
	bcg_gen(&bc, BC_PUSH_NIL);
	bcg_gen(&bc, BC_PUSH_TRUE);
	bcg_gen(&bc, BC_PUSH_FALSE);
	
	test(bc.length == 3, "unexpected number of instructions: %d", bc.length);
	test(bc.code[0] == BC_PUSH_NIL, "got wrong instruction: %d", bc.code[0]);
	test(bc.code[1] == BC_PUSH_TRUE, "got wrong instruction: %d", bc.code[1]);
	test(bc.code[2] == BC_PUSH_FALSE, "got wrong instruction: %d", bc.code[2]);
	
	bcg_gen_with_arg(&bc, BC_PUSH_NUM, 42);
	test(bc.length == 5, "unexpected number of instructions: %d", bc.length);
	test(bc.code[3] == BC_PUSH_NUM, "got wrong instruction: %d", bc.code[3]);
	test(bc.code[4] == 42, "got wrong number value: %d", bc.code[4]);
	
	bcg_destroy(&bc);
}


void test_back_patching(){
	bytecode_t bc = bcg_init();
	
	// generate instructions for pseudocode: if(true) 42 else 17
					bcg_gen(&bc, BC_PUSH_TRUE);
	size_t false_offset =	bcg_gen_with_arg(&bc, BC_JUMP_IF_FALSE, 0);
					bcg_gen_with_arg(&bc, BC_PUSH_NUM, 42);
	size_t end_offset =	bcg_gen_with_arg(&bc, BC_JUMP, 0);
				bcg_backpatch_target_in(&bc, false_offset);
					bcg_gen_with_arg(&bc, BC_PUSH_NUM, 17);
				bcg_backpatch_target_in(&bc, end_offset);
	
	int expected_bytecode[9] = {
		BC_PUSH_TRUE,
		BC_JUMP_IF_FALSE, 4,
		BC_PUSH_NUM, 42,
		BC_JUMP, 2,
		BC_PUSH_NUM, 17
	};
	
	test(bc.length == 9, "unexpected number of instructions: %d", bc.length);
	for(size_t i = 0; i < 9; i++)
		test(bc.code[i] == expected_bytecode[i], "got wrong bytecode at index %d, expected %d, got %d", i, expected_bytecode[i], bc.code[i]);
	
	bcg_destroy(&bc);
}


int main(){
	test_generation();
	test_back_patching();
	return show_test_report();
}