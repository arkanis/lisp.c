#include <stdbool.h>
#include <string.h>

#include "test_utils.h"
#include "../memory.h"
#include "../reader.h"
#include "../eval.h"
#include "../bytecode_compiler.h"


void test_compiler(){
	atom_t *atom = NULL;
	env_t *env = env_alloc(NULL);
	register_compiler_buildins_in(env);
	
	atom_t *test_sample(char *body, int64_t *expected_bytecode){
		scanner_t scan = scan_open_string(body);
		atom = read_atom(&scan);
		scan_close(&scan);
		
		atom_t *compiled_lambda = eval_atom(atom, env);
		test(compiled_lambda->type == T_COMPILED_LAMBDA, "sample: %s, expected a compiled lambda atom, got type %d",
			body, compiled_lambda->type);
		
		size_t expected_bc_length = 0;
		while(expected_bytecode[expected_bc_length] != BC_NULL)
			expected_bc_length++;
		
		test(compiled_lambda->bytecode.length == expected_bc_length, "sample: %s, expected a bytecode length of %d but got %d",
			body, expected_bc_length, compiled_lambda->bytecode.length);
		for(size_t i = 0; i < compiled_lambda->bytecode.length; i++){
			test(compiled_lambda->bytecode.code[i] == expected_bytecode[i], "sample: %s, got wrong bytecode at index %d, expected %d but got %d",
				body, i, expected_bytecode[i], compiled_lambda->bytecode.code[i]);
		}
		
		return compiled_lambda;
	}
	
	test_sample("(lambda_compile () nil)", (int64_t[]){BC_PUSH_NIL, BC_RETURN, BC_NULL});
	test_sample("(lambda_compile () true)", (int64_t[]){BC_PUSH_TRUE, BC_RETURN, BC_NULL});
	test_sample("(lambda_compile () false)", (int64_t[]){BC_PUSH_FALSE, BC_RETURN, BC_NULL});
	test_sample("(lambda_compile () 42)", (int64_t[]){BC_PUSH_NUM, 42, BC_RETURN, BC_NULL});
	atom = test_sample("(lambda_compile () \"foo\")", (int64_t[]){BC_PUSH_LITERAL, 0, BC_RETURN, BC_NULL});
	test(atom->literal_table.atoms[0]->type == T_STR, "...");
	test(strcmp(atom->literal_table.atoms[0]->str, "foo") == 0, "...");
	test_sample("(lambda_compile (a) a)", (int64_t[]){BC_PUSH_ARG, 0, BC_RETURN, BC_NULL});
	test_sample("(lambda_compile (a b c) c)", (int64_t[]){BC_PUSH_ARG, 2, BC_RETURN, BC_NULL});
}


int main(){
	memory_init();
	test_compiler();
	return show_test_report();
}