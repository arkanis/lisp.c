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

void test_instructions(bytecode_t *actual_bytecode, instruction_t *expected_bytecode, char *body){
	size_t expected_bc_length = 0;
	while(expected_bytecode[expected_bc_length].op != BC_NULL)
		expected_bc_length++;
	
	test(actual_bytecode->length == expected_bc_length, "sample: %s, expected a bytecode length of %d but got %d",
		body, expected_bc_length, actual_bytecode->length);
	for(size_t i = 0; i < actual_bytecode->length; i++)
		test_instruction(expected_bytecode[i], actual_bytecode->code[i], i, body);
}

atom_t *test_sample(char *body, instruction_t *expected_bytecode){
	scanner_t scan = scan_open_string(body);
	atom_t *ast = read_atom(&scan);
	scan_close(&scan);
	
	atom_t *runtime_lambda = eval_atom(ast, env);
	test(runtime_lambda->type == T_RUNTIME_LAMBDA, "sample: %s, expected a runtime lambda atom, got type %d",
		body, runtime_lambda->type);
	
	test_instructions(&runtime_lambda->cl->bytecode, expected_bytecode, body);
	
	return runtime_lambda;
}


void test_self_evaling_atoms(){
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
	atom_t *atom = test_sample("(lambda () \"foo\")", (instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_atom(atom->cl->literal_table.atoms[0], (atom_t){T_STR, .str = "foo"}, 0, "(lambda () \"foo\")");
}

void test_nums_in_literal_tables(){
	// 65546 = 2^16 + 10, just a number over the 16 bit limit. Therefore it does not fit into an instruction
	// and has to be stored in the literal table.
	atom_t *atom = test_sample("(lambda () 65546)", (instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_atom(atom->cl->literal_table.atoms[0], (atom_t){T_NUM, .num = 65546}, 0, "number atom was not correctly stored in the literal table");
}


void test_begin(){
	test_sample("(lambda () nil true false)", (instruction_t[]){
		(instruction_t){BC_PUSH_NIL},
		(instruction_t){BC_DROP},
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_DROP},
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
}

void test_args_in_own_frame(){
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
}

void test_locals_in_own_frame(){
	atom_t *rl = test_sample("(lambda () (define foo 42) (define bar nil))", (instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 42},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 0},  // leaves current atom on stack (as return value)
		(instruction_t){BC_DROP},  // from begin
		(instruction_t){BC_PUSH_NIL},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 1},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test( rl->cl->comp_data->var_count == 2, "variable count was not updated properly");
	test( strcmp(rl->cl->comp_data->names[0], "foo") == 0, "variable name was not correctly added to the name list");
	test( strcmp(rl->cl->comp_data->names[1], "bar") == 0, "variable name was not correctly added to the name list");
}

void test_nested_compilation(){
	char *code = "(lambda (x y z) \
		(define outer 42) \
		(define foo (lambda (a b c) \
			(define inner 17) \
			(begin \
				y \
				outer \
				c \
				inner \
			) \
		)) \
	)";
	atom_t *rl = test_sample(code, (instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 42},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 0},
		(instruction_t){BC_DROP},
		(instruction_t){BC_LAMBDA, .offset = 0, .index = 0},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 1},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test( rl->cl->comp_data->arg_count == 3, "variable count was not updated properly");
	test( rl->cl->comp_data->var_count == 2, "variable count was not updated properly");
	test( strcmp(rl->cl->comp_data->names[0], "x") == 0, "variable name was not correctly added to the name list");
	test( strcmp(rl->cl->comp_data->names[1], "y") == 0, "variable name was not correctly added to the name list");
	test( strcmp(rl->cl->comp_data->names[2], "z") == 0, "variable name was not correctly added to the name list");
	test( strcmp(rl->cl->comp_data->names[3], "outer") == 0, "variable name was not correctly added to the name list");
	test( strcmp(rl->cl->comp_data->names[4], "foo") == 0, "variable name was not correctly added to the name list");
	
	atom_t *child_cl = rl->cl->literal_table.atoms[0];
	test( child_cl != NULL, "no child lambda was compiled!");
	test( child_cl->type == T_COMPILED_LAMBDA, "expected compiled lambda (type %d) got type %d", T_COMPILED_LAMBDA, child_cl->type);
	test_instructions(&child_cl->bytecode, (instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 17},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 0},
		(instruction_t){BC_DROP},
		(instruction_t){BC_PUSH_ARG, .offset = 1, .index = 1},
		(instruction_t){BC_DROP},
		(instruction_t){BC_PUSH_VAR, .offset = 1, .index = 0},
		(instruction_t){BC_DROP},
		(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 2},
		(instruction_t){BC_DROP},
		(instruction_t){BC_PUSH_VAR, .offset = 0, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, "nested body");
	test( child_cl->comp_data->arg_count == 3, "variable count was not updated properly");
	test( child_cl->comp_data->var_count == 1, "variable count was not updated properly");
	test( strcmp(child_cl->comp_data->names[0], "a") == 0, "variable name was not correctly added to the name list");
	test( strcmp(child_cl->comp_data->names[1], "b") == 0, "variable name was not correctly added to the name list");
	test( strcmp(child_cl->comp_data->names[2], "c") == 0, "variable name was not correctly added to the name list");
	test( strcmp(child_cl->comp_data->names[3], "inner") == 0, "variable name was not correctly added to the name list");
}

void test_env_lookup_on_unknown_vars(){
	atom_t *rl = test_sample("(lambda () foo)", (instruction_t[]){
		(instruction_t){BC_PUSH_FROM_ENV, .offset = 0, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_atom(rl->cl->literal_table.atoms[0], (atom_t){T_SYM, .str = "foo"}, 0, "(lambda () foo)");
	
	rl = test_sample("(lambda (n) (if (= n 1) 1 (* n (fac (- n 1))) ))", (instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 0},
		(instruction_t){BC_PUSH_NUM, .num = 1},
		(instruction_t){BC_EQ},
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
			// true case
			(instruction_t){BC_PUSH_NUM, .num = 1},
		(instruction_t){BC_JUMP, .jump_offset = 7},
			// false case
			// args for *
				(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 0},
				(instruction_t){BC_PUSH_FROM_ENV, .offset = 0, .index = 0},  // look up the fac lambda itself
				// args for fac
					(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 0},
					(instruction_t){BC_PUSH_NUM, .num = 1},
				(instruction_t){BC_SUB},
				(instruction_t){BC_CALL, .num = 1},
			(instruction_t){BC_MUL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test_atom(rl->cl->literal_table.atoms[0], (atom_t){T_SYM, .str = "fac"}, 0, "fac lambda");
}

void test_self_recursion(){
	char *code = "(lambda () \
		\
		(define fac (lambda (n) \
			(if (= n 1) \
				1 \
				(* n (fac (- n 1))) \
			) \
		)) \
		\
		(fac 7) \
	)";
	atom_t *rl = test_sample(code, (instruction_t[]){
		(instruction_t){BC_LAMBDA, .offset = 0, .index = 0},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 0},
		(instruction_t){BC_DROP}, // from implicit begin
		(instruction_t){BC_PUSH_VAR, .offset = 0, .index = 0},
		(instruction_t){BC_PUSH_NUM, .num = 7},
		(instruction_t){BC_CALL, .num = 1},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
	test( rl->cl->comp_data->arg_count == 0, "variable count was not updated properly");
	test( rl->cl->comp_data->var_count == 1, "variable count was not updated properly");
	test( strcmp(rl->cl->comp_data->names[0], "fac") == 0, "variable name was not correctly added to the name list");
	
	atom_t *child_cl = rl->cl->literal_table.atoms[0];
	test( child_cl != NULL, "no child lambda was compiled!");
	test( child_cl->type == T_COMPILED_LAMBDA, "expected compiled lambda (type %d) got type %d", T_COMPILED_LAMBDA, child_cl->type);
	test_instructions(&child_cl->bytecode, (instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 0},
		(instruction_t){BC_PUSH_NUM, .num = 1},
		(instruction_t){BC_EQ},
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
			// true case
			(instruction_t){BC_PUSH_NUM, .num = 1},
		(instruction_t){BC_JUMP, .jump_offset = 7},
			// false case
			// args for *
				(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 0},
				(instruction_t){BC_PUSH_VAR, .offset = 1, .index = 0},  // look up the fac runtime lambda itself
				// args for fac
					(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 0},
					(instruction_t){BC_PUSH_NUM, .num = 1},
				(instruction_t){BC_SUB},
				(instruction_t){BC_CALL, .num = 1},
			(instruction_t){BC_MUL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, "nested body");
	test( child_cl->comp_data->arg_count == 1, "variable count was not updated properly");
	test( child_cl->comp_data->var_count == 0, "variable count was not updated properly");
}

void test_quote(){
	test_sample("(lambda () (quote c))", (instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .offset = 0, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	});
}

void test_if(){
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

void test_math(){
	// TODO
}

void test_comparators(){
	// TODO
}

int main(){
	memory_init();
	env = env_alloc(NULL);
	register_buildins_in(env);
	env_def(env, "__compile_lambdas", true_atom());
	
	test_self_evaling_atoms();
	test_nums_in_literal_tables();
	test_begin();
	test_args_in_own_frame();
	test_locals_in_own_frame();
	test_nested_compilation();
	test_env_lookup_on_unknown_vars();
	
	test_self_recursion();
	
	test_quote();
	test_if();
	test_math();
	test_comparators();
	
	return show_test_report();
}