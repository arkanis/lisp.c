#include "test_utils.h"
#include "test_bytecode_utils.h"

#include "../memory.h"
#include "../bytecode_interpreter.h"

env_t *env;
bytecode_interpreter_t interpreter;

//
// Helpers
//

static atom_t* compiled(instruction_t *bytecode, atom_t **literal_table){
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
	});
}

static void test_compiled_sample(atom_t *cl_atom, atom_t *args, atom_t *expected_result){
	atom_t *result = bci_eval(interpreter, cl_atom, args, env);
	test_atom(result, *expected_result, 0, "interpreter returned wrong atom");
	test(interpreter->stack->length == 0, "the stack was not empty after execution, %d atoms left", interpreter->stack->length);
}

static void test_sample(instruction_t *bytecode, atom_t **literal_table, atom_t *args, atom_t *expected_result){
	atom_t *cl_atom = compiled(bytecode, literal_table);
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
	}, NULL, nil_atom(), nil_atom());
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), true_atom());
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), false_atom());
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 17},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), num_atom_alloc(17));
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_TRUE},
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_DROP},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), true_atom());
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
	}, nil_atom(), atom);
	
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
	}, nil_atom(), atom);
}

void test_push_arg(){
	atom_t *atom = num_atom_alloc(29);
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL,
		pair_atom_alloc(atom, nil_atom()),
	atom);
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .index = 2},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL,
		pair_atom_alloc(false_atom(), pair_atom_alloc(true_atom(), pair_atom_alloc(atom, nil_atom()))),
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
	}, NULL, nil_atom(), num_atom_alloc(42));
	
	// bytecode for: if(false) 42 else 17
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_FALSE},
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
		(instruction_t){BC_PUSH_NUM, .num = 42},
		(instruction_t){BC_JUMP, .jump_offset = 1},
		(instruction_t){BC_PUSH_NUM, .num = 17},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), num_atom_alloc(17));
}

void test_math_instructions(){
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_PUSH_NUM, .num = 8},
		(instruction_t){BC_ADD},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), num_atom_alloc(12));
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_PUSH_NUM, .num = 8},
		(instruction_t){BC_SUB},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), num_atom_alloc(-4));
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_PUSH_NUM, .num = 8},
		(instruction_t){BC_MUL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), num_atom_alloc(32));
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 8},
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_DIV},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), num_atom_alloc(2));
}

void test_comparators(){
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_EQ},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), true_atom());
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_NUM, .num = 4},
		(instruction_t){BC_PUSH_NUM, .num = 8},
		(instruction_t){BC_EQ},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL, nil_atom(), false_atom());
}

void test_function_calls(){
	atom_t *ret_first_arg = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .index = 0},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, NULL);
	
	test_sample((instruction_t[]){
		(instruction_t){BC_PUSH_LITERAL, .index = 0},
		(instruction_t){BC_PUSH_NUM, .num = 42},
		(instruction_t){BC_PUSH_NUM, .num = 1},
		(instruction_t){BC_CALL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		ret_first_arg,
		NULL
	}, nil_atom(), num_atom_alloc(42));
	
	// factorial
	/*
		fac(n)
			if (n == 0)
				1
			else
				fac(n - 1) * n
	*/
	atom_t *fac = compiled((instruction_t[]){
		(instruction_t){BC_PUSH_ARG, .index = 0},
		(instruction_t){BC_PUSH_NUM, .num = 1},
		(instruction_t){BC_EQ},  // n == 1
		(instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 2},
			(instruction_t){BC_PUSH_NUM, .num = 1},
		(instruction_t){BC_JUMP, .jump_offset = 8},
			(instruction_t){BC_PUSH_LITERAL, .index = 0},  // fak
				(instruction_t){BC_PUSH_ARG, .index = 0}, // n
				(instruction_t){BC_PUSH_NUM, .num = 1},
				(instruction_t){BC_SUB},  // n - 1
			(instruction_t){BC_PUSH_NUM, .num = 1}, // arg count
			(instruction_t){BC_CALL},
			(instruction_t){BC_PUSH_ARG, .index = 0}, // n for mul
			(instruction_t){BC_MUL},
		(instruction_t){BC_RETURN},
		(instruction_t){BC_NULL}
	}, (atom_t*[]){
		nil_atom(),
		NULL
	});
	// Patch the compiled lambda itself into its own literal table at index 0.
	// This allows the recursive call on itself.
	fak->literal_table.atoms[0] = fac;
	
	test_compiled_sample(fac, pair_atom_alloc(num_atom_alloc(2), nil_atom()), num_atom_alloc(2));
	test_compiled_sample(fac, pair_atom_alloc(num_atom_alloc(4), nil_atom()), num_atom_alloc(24));
	test_compiled_sample(fac, pair_atom_alloc(num_atom_alloc(12), nil_atom()), num_atom_alloc(479001600));
	test_compiled_sample(fac, pair_atom_alloc(num_atom_alloc(16), nil_atom()), num_atom_alloc(20922789888000));
	test_compiled_sample(fac, pair_atom_alloc(num_atom_alloc(20), nil_atom()), num_atom_alloc(2432902008176640000));
	// fac(21) results in an int64 overflow
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
	
	bci_destroy(interpreter);
	return show_test_report();
}