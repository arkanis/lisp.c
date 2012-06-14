#include <stdbool.h>
#include <string.h>

#include "test_utils.h"
#include "test_bytecode_utils.h"

#include "../memory.h"
#include "../reader.h"
#include "../eval.h"
#include "../bytecode_compiler.h"


void test_compiler(){
	atom_t *atom = NULL;
	env_t *env = env_alloc(NULL);
	register_compiler_buildins_in(env);
	
	atom_t *test_sample(char *body, instruction_t *expected_bytecode){
		scanner_t scan = scan_open_string(body);
		atom = read_atom(&scan);
		scan_close(&scan);
		
		atom_t *compiled_lambda = eval_atom(atom, env);
		test(compiled_lambda->type == T_COMPILED_LAMBDA, "sample: %s, expected a compiled lambda atom, got type %d",
			body, compiled_lambda->type);
		
		size_t expected_bc_length = 0;
		while(expected_bytecode[expected_bc_length].op != BC_NULL)
			expected_bc_length++;
		
		test(compiled_lambda->bytecode.length == expected_bc_length, "sample: %s, expected a bytecode length of %d but got %d",
			body, expected_bc_length, compiled_lambda->bytecode.length);
		for(size_t i = 0; i < compiled_lambda->bytecode.length; i++)
			test_instruction(expected_bytecode[i], compiled_lambda->bytecode.code[i], i, body);
		
		return compiled_lambda;
	}
	
	test_sample("(lambda_compile () nil)", (instruction_t[]){
		(instruction_t){BC_PUSH_NIL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_sample("(lambda_compile () true)", (instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_sample("(lambda_compile () false)", (instruction_t[]){
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_sample("(lambda_compile () 42)", (instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 42},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	atom = test_sample("(lambda_compile () \"foo\")", (instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_atom(atom->literal_table.atoms[0], (atom_t){T_STR, .str = "foo"}, 0, "(lambda_compile () \"foo\")");
	
	test_sample("(lambda_compile (a) a)", (instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .frame_offset = 0, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_sample("(lambda_compile (a b c) c)", (instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .frame_offset = 0, .index = 2},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
}


int main(){
	memory_init();
	test_compiler();
	return show_test_report();
}