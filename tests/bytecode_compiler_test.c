#include <stdbool.h>
#include <string.h>

#include "test_utils.h"
#include "test_bytecode_utils.h"

#include "../memory.h"
#include "../reader.h"
#include "../eval.h"
#include "../buildins.h"
#include "../bytecode_compiler.h"


env_t *env;

atom_t *test_sample(char *body, instruction_t *expected_bytecode){
	scanner_t scan = scan_open_string(body);
	atom_t *ast = read_atom(&scan);
	scan_close(&scan);
	
	atom_t *runtime_lambda = eval_atom(ast, env);
	test(runtime_lambda->type == T_RUNTIME_LAMBDA, "sample: %s, expected a runtime lambda atom, got type %d",
		body, runtime_lambda->type);
	
	size_t expected_bc_length = 0;
	while(expected_bytecode[expected_bc_length].op != BC_NULL)
		expected_bc_length++;
	
	test(runtime_lambda->cl->bytecode.length == expected_bc_length, "sample: %s, expected a bytecode length of %d but got %d",
		body, expected_bc_length, runtime_lambda->cl->bytecode.length);
	for(size_t i = 0; i < runtime_lambda->cl->bytecode.length; i++)
		test_instruction(expected_bytecode[i], runtime_lambda->cl->bytecode.code[i], i, body);
	
	return runtime_lambda;
}


void test_compiler(){
	atom_t *atom = NULL;
	
	test_sample("(lambda () nil)", (instruction_t[]){
		(instruction_t){BC_PUSH_NIL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_sample("(lambda () true)", (instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_sample("(lambda () false)", (instruction_t[]){
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_sample("(lambda () 42)", (instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 42},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	atom = test_sample("(lambda () \"foo\")", (instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_atom(atom->cl->literal_table.atoms[0], (atom_t){T_STR, .str = "foo"}, 0, "(lambda () \"foo\")");
	
	test_sample("(lambda (a) a)", (instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_sample("(lambda (a b c) c)", (instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 2},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	
	test_sample("(lambda () (quote c))", (instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .offset = 0, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	
	test_sample("(lambda () (if true 42 17))", (instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
		(instruction_t){BC_PUSH_NUM, .num = 42},
		(instruction_t){BC_JUMP, .jump_offset = 1},
		(instruction_t){BC_PUSH_NUM, .num = 17},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
}


int main(){
	memory_init();
	env = env_alloc(NULL);
	register_buildins_in(env);
	env_set(env, "__compile_lambdas", true_atom());
	
	test_compiler();
	
	return show_test_report();
}