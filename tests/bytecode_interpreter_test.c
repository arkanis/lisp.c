#include "test_utils.h"
#include "test_bytecode_utils.h"

#include "../memory.h"
#include "../bytecode_interpreter.h"
#include "../bytecode_compiler.h"

env_t *env;
bytecode_interpreter_t interpreter;

//
// Helpers
//

static atom_t* compiled(instruction_t *bytecode, atom_t **literal_table, uint16_t arg_count, uint16_t var_count){
	size_t bytecode_length = 0;
	while(bytecode[bytecode_length].op != BC_NULL)
		bytecode_length++;
	
	size_t literal_table_length = 0;
	if (literal_table != NULL){
		while(literal_table[literal_table_length] != NULL)
			literal_table_length++;
	}
	
	return compiled_lambda_atom_alloc((bytecode_t){
		.length = bytecode_length,
		.code = bytecode
	}, (atom_list_t){
		.length = literal_table_length,
		.atoms = literal_table
	}, arg_count, var_count);
}

static void test_compiled_sample(atom_t *cl_atom, atom_t *args, atom_t *expected_result){
	atom_t *rl = runtime_lambda_atom_alloc(cl_atom, scope_env_alloc(env));
	atom_t *result = bci_eval(interpreter, rl, args, env);
	test_atom(result, *expected_result, 0, "interpreter returned wrong atom");
	test(interpreter->stack->length == 0, "the stack was not empty after execution, %d atoms left", interpreter->stack->length);
}

static void test_sample(instruction_t *bytecode, atom_t **literal_table, atom_t *args, size_t arg_count, atom_t *expected_result){
	atom_t *cl_atom = compiled(bytecode, literal_table, arg_count, 0);
	test_compiled_sample(cl_atom, args, expected_result);
}


//
// Test cases
//

void test_simple_instructions(){
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NIL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, nil_atom());
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, true_atom());
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, false_atom());
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 17},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, num_atom_alloc(17));
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_DROP},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, true_atom());
}

void test_push_literal(){
	atom_t *atom = num_atom_alloc(29);
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		atom,
		NULL
	}, nil_atom(), 0, atom);
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .index = 2},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		true_atom(),
		nil_atom(),
		atom,
		nil_atom(),
		NULL
	}, nil_atom(), 0, atom);
}

void test_push_arg(){
	atom_t *atom = num_atom_alloc(29);
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .index = 0, .offset = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL,
		pair_atom_alloc(atom, nil_atom()), 1,
	atom);
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .index = 2, .offset = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL,
		pair_atom_alloc(false_atom(), pair_atom_alloc(true_atom(), pair_atom_alloc(atom, nil_atom()))), 3,
	atom);
}

void test_branching(){
	// bytecode for: if(true) 42 else 17
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
		(instruction_t){BC_PUSH_NUM, .num = 42},
		(instruction_t){BC_JUMP, .jump_offset = 1},
		(instruction_t){BC_PUSH_NUM, .num = 17},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, num_atom_alloc(42));
	
	// bytecode for: if(false) 42 else 17
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
		(instruction_t){BC_PUSH_NUM, .num = 42},
		(instruction_t){BC_JUMP, .jump_offset = 1},
		(instruction_t){BC_PUSH_NUM, .num = 17},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, num_atom_alloc(17));
}

void test_math_instructions(){
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_PUSH_NUM, .num = 8},
		(instruction_t){BC_ADD},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, num_atom_alloc(12));
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_PUSH_NUM, .num = 8},
		(instruction_t){BC_SUB},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, num_atom_alloc(-4));
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_PUSH_NUM, .num = 8},
		(instruction_t){BC_MUL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, num_atom_alloc(32));
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 8},
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_DIV},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, num_atom_alloc(2));
}

void test_comparators(){
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_EQ},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, true_atom());
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_PUSH_NUM, .num = 8},
		(instruction_t){BC_EQ},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, false_atom());
}

void test_function_calls(){
	atom_t *ret_first_arg = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .index = 0, .offset = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, 1, 0);
	
	test_sample((instruction_t[]){
		(instruction_t){BC_LAMBDA, .index = 0, .offset = 0},
		(instruction_t){BC_PUSH_NUM, .num = 42},
		(instruction_t){BC_CALL, .num = 1},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		ret_first_arg,
		NULL
	}, nil_atom(), 0, num_atom_alloc(42));
	
	// factorial
	/*
		fac(n)
			if (n == 0)
				1
			else
				fac(n - 1) * n
	*/
	atom_t *fac = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .index = 0, .offset = 0},
		(instruction_t){BC_PUSH_NUM, .num = 1},
		(instruction_t){BC_EQ},  // n == 1
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
			(instruction_t){BC_PUSH_NUM, .num = 1},
		(instruction_t){BC_JUMP, .jump_offset = 7},
			(instruction_t){BC_LAMBDA, .index = 0},  // fak
				(instruction_t){BC_PUSH_ARG, .index = 0, .offset = 0}, // n
				(instruction_t){BC_PUSH_NUM, .num = 1},
				(instruction_t){BC_SUB},  // n - 1
			(instruction_t){BC_CALL, .num = 1},
			(instruction_t){BC_PUSH_ARG, .index = 0, .offset = 0}, // n for mul
			(instruction_t){BC_MUL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		nil_atom(),
		NULL
	}, 1, 0);
	// Patch the compiled lambda itself into its own literal table at index 0.
	// This allows the recursive call on itself.
	fac->literal_table.atoms[0] = fac;
	
	test_compiled_sample(fac, pair_atom_alloc(num_atom_alloc(2), nil_atom()), num_atom_alloc(2));
	test_compiled_sample(fac, pair_atom_alloc(num_atom_alloc(4), nil_atom()), num_atom_alloc(24));
	test_compiled_sample(fac, pair_atom_alloc(num_atom_alloc(12), nil_atom()), num_atom_alloc(479001600));
	test_compiled_sample(fac, pair_atom_alloc(num_atom_alloc(16), nil_atom()), num_atom_alloc(20922789888000));
	test_compiled_sample(fac, pair_atom_alloc(num_atom_alloc(20), nil_atom()), num_atom_alloc(2432902008176640000));
	// fac(21) results in an int64 overflow
}


void test_env_instructions(){
	atom_t *atom = num_atom_alloc(42);
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .index = 0},
		(instruction_t){BC_SAVE_ENV, .index = 1},
		(instruction_t){BC_DROP},
		(instruction_t){BC_PUSH_FROM_ENV, .index = 1},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		atom,
		sym_atom_alloc("from_bci"),
		NULL
	}, nil_atom(), 0, atom);
	
	test( env_get(env, "from_bci") == atom, "env did not contain the stored symbol");
}

void test_var_instructions(){
	atom_t *cl = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 43},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 0},
		(instruction_t){BC_DROP},
		(instruction_t){BC_PUSH_VAR, .offset = 0, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, 0, 1);
	
	test_compiled_sample(cl, nil_atom(), num_atom_alloc(43));
}

void test_arg_and_local_offset(){
	atom_t *test_atom = num_atom_alloc(44);
	
	// Function that just returns the first local of the outer scope
	atom_t *ret_first_outer_local = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_VAR, .offset = 1, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, 0, 0);
	
	// Saves first literal in the first local, then calls ret_first_outer_local()
	atom_t *cl = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .offset = 0, .index = 0},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 0},
		(instruction_t){BC_DROP},
		(instruction_t){BC_LAMBDA, .offset = 0, .index = 1},
		(instruction_t){BC_CALL, .num = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		test_atom,
		ret_first_outer_local,
		NULL
	}, 0, 1);
	
	test_compiled_sample(cl, nil_atom(), test_atom);
}

void test_lexical_scoping(){
	/* Idea:
	
	(define test (lambda ()
		(define right 45)
		(define ret_first_outer_local (lambda () right))
		(define stack_polluter (lambda ()
			(define wrong 17)
			(ret_first_outer_local)
		))
		(stack_polluter)
	))
	
	*/
	
	atom_t *ret_first_outer_local = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_VAR, .offset = 1, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, 0, 0);
	
	atom_t *stack_polluter = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 17},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 0},  // (define wrong 17)
		(instruction_t){BC_DROP},
		(instruction_t){BC_PUSH_VAR, .offset = 1, .index = 1},  // load ret_first_outer_local
		(instruction_t){BC_CALL, .num = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, 0, 1);
	
	atom_t *test = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 45},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 0},  // (define right 45)
		(instruction_t){BC_DROP},
		(instruction_t){BC_LAMBDA, .offset = 0, .index = 0},  //  create ret_first_outer_local lambda from literal table
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 1},  // (define ret_first_outer_local ...)
		(instruction_t){BC_DROP},
		(instruction_t){BC_LAMBDA, .offset = 0, .index = 1},  // create stack_polluter lambda from literal table
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 2},  // (define stack_polluter ...)
		(instruction_t){BC_DROP},
		(instruction_t){BC_PUSH_VAR, .offset = 0, .index = 2},
		(instruction_t){BC_CALL, .num = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		ret_first_outer_local,
		stack_polluter,
		NULL
	}, 0, 3);
	
	test_compiled_sample(test, nil_atom(), num_atom_alloc(45));
}

void test_pair_instructions(){
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_PUSH_NIL},
		(instruction_t){BC_CONS},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, pair_atom_alloc(true_atom(), nil_atom()));
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_CONS},
		(instruction_t){BC_FIRST},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, true_atom());
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_CONS},
		(instruction_t){BC_REST},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), 0, false_atom());
}

void test_capturing(){
	/* Idea:
	
	generate_adders is supposed to generate n adder functions. Each adder function
	captures the current n (e.g. 3, 2, 1) and adds it's argument to it. Therefore each
	frame needs to be captured and the scoping has to work reliably.
	
	(define test (lambda ()
		(define generate_adders (lambda (n)
			(if (eq n 0)
				nil
			; else
				(cons
					(lambda (x) (plus x n))
					(generate_adders (minus n 1))
				)
			)
		))
		(define adders (generate_adders 3))
		(define add3 (first adders))
		(define add2 (first (rest adders)))
		(add2 (add3 7))
	))
	
	*/
	
	atom_t *adder = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 0},  // x
		(instruction_t){BC_PUSH_ARG, .offset = 1, .index = 0},  // n, needs to be captured
		(instruction_t){BC_ADD},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, 1, 0);
	
	atom_t *generate_adders = compiled((instruction_t[]){
		// (eq n 0)
			(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 0},  // n
			(instruction_t){BC_PUSH_NUM, .num = 0},  // 0
		(instruction_t){BC_EQ},
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
			// true case
			(instruction_t){BC_PUSH_NIL},
		(instruction_t){BC_JUMP, .jump_offset = 7},
			// false case
			// cons first: (lambda (x) (plus x n))
				(instruction_t){BC_LAMBDA, .offset = 0, .index = 0},
			// cons rest
				// load generate_adders
				(instruction_t){BC_PUSH_VAR, .offset = 1, .index = 0},
				// args for generate_adders
					// (minus n 1)
					(instruction_t){BC_PUSH_ARG, .offset = 0, .index = 0},
					(instruction_t){BC_PUSH_NUM, .num = 1},
					(instruction_t){BC_SUB},
				(instruction_t){BC_CALL, .num = 1},
			(instruction_t){BC_CONS},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		adder,
		NULL
	}, 1, 0);
	
	atom_t *test = compiled((instruction_t[]){
		(instruction_t){BC_LAMBDA, .offset = 0, .index = 0},  // create generate_adders lambda
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 0},  // (define generate_adders ...)
		(instruction_t){BC_DROP},
		
		(instruction_t){BC_PUSH_VAR, .offset = 0, .index = 0},
		(instruction_t){BC_PUSH_NUM, .num = 3},
		(instruction_t){BC_CALL, .num = 1},  // (generate_adders 3)
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 1},  // (define adders ...)
		(instruction_t){BC_DROP},
		
		(instruction_t){BC_PUSH_VAR, .offset = 0, .index = 1},
		(instruction_t){BC_FIRST},  // (frist adders)
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 2},  // (define add3 ...)
		(instruction_t){BC_DROP},
		
		(instruction_t){BC_PUSH_VAR, .offset = 0, .index = 1},
		(instruction_t){BC_REST},
		(instruction_t){BC_FIRST},
		(instruction_t){BC_SAVE_VAR, .offset = 0, .index = 3},  // (define add2 (first (rest adders)))
		(instruction_t){BC_DROP},
		
		(instruction_t){BC_PUSH_VAR, .offset = 0, .index = 3},  // load add2
			(instruction_t){BC_PUSH_VAR, .offset = 0, .index = 2},  // load add3
			(instruction_t){BC_PUSH_NUM, .num = 7},
			(instruction_t){BC_CALL, .num = 1},  // (add3 7)
		(instruction_t){BC_CALL, .num = 1},  // (add2 ...)
		
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		generate_adders,
		NULL
	}, 0, 4);
	
	test_compiled_sample(test, nil_atom(), num_atom_alloc(12));
}

int main(){
	memory_init();
	env = env_alloc(NULL);
	interpreter = bci_new(0);
	
	test_simple_instructions();
	test_push_literal();
	test_push_arg();
	test_branching();
	test_math_instructions();
	test_comparators();
	
	test_function_calls();
	test_env_instructions();
	test_var_instructions();
	test_arg_and_local_offset();
	
	test_lexical_scoping();
	
	test_pair_instructions();
	test_capturing();
	
	bci_destroy(interpreter);
	return show_test_report();
}