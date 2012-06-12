#include <stdbool.h>

#include "test_utils.h"
#include "../memory.h"
#include "../reader.h"
#include "../eval.h"
#include "../bytecode_compiler.h"


void test_compiler(){
	env_t *env = env_alloc(NULL);
	register_compiler_buildins_in(env);
	
	void test_sample(char *body, int *expected_bytecode){
		scanner_t scan = scan_open_string(body);
		atom_t *atom = read_atom(&scan);
		scan_close(&scan);
		
		atom_t *compiled_lambda = eval_atom(atom, env);
		test(compiled_lambda->type == T_COMPILED_LAMBDA, "sample: %s, expected a compiled lambda atom, got type %d",
			body, compiled_lambda->type);
		
		size_t expected_bc_length = 0;
		while(expected_bytecode[expected_bc_length] != 0)
			expected_bc_length++;
		
		test(compiled_lambda->bytecode.length == expected_bc_length, "sample: %s, expected a bytecode length of %d but got %d",
			body, expected_bc_length, compiled_lambda->bytecode.length);
		for(size_t i = 0; i < compiled_lambda->bytecode.length; i++){
			test(compiled_lambda->bytecode.code[i] == expected_bytecode[i], "sample: %s, got wrong bytecode at index %d, expected %d but got %d",
				body, i, expected_bytecode[i], compiled_lambda->bytecode.code[i]);
		}
	}
	
	test_sample("(lambda_compile () nil)", (int[]){1, 0});
	test_sample("(lambda_compile () true)", (int[]){2, 0});
	test_sample("(lambda_compile () false)", (int[]){3, 0});
	test_sample("(lambda_compile () 42)", (int[]){4, 42, 0});
}


int main(){
	memory_init();
	test_compiler();
	return show_test_report();
}