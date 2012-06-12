#include <stdbool.h>

#include "memory.h"
#include "logger.h"
#include "bytecode_generator.h"


void compile_statement(bytecode_t *bc, atom_t *atom);

/**
 * Offers the same interface as the raw lambda buildin but compiles
 * the AST to bytecode instead.
 * 
 * TODO:
 * - call bcg_destroy(&bc); when a compiled lambda is freed
 */
atom_t* buildin_lambda_compile(atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_NIL)
		return warn("lambda needs exactly two arguments (arg list and body)"), nil_atom();
	
	//atom_t *args_atom = args->first;
	atom_t *body_atom = args->rest->first;
	
	bytecode_t bc = bcg_init();
	compile_statement(&bc, body_atom);
	
	return compiled_lambda_atom_alloc(bc, (atom_list_t){0, NULL});
}

void compile_statement(bytecode_t *bc, atom_t *atom){
	switch (atom->type) {
		case T_NIL:
			bcg_gen(bc, BC_PUSH_NIL);
			break;
		case T_TRUE:
			bcg_gen(bc, BC_PUSH_TRUE);
			break;
		case T_FALSE:
			bcg_gen(bc, BC_PUSH_FALSE);
			break;
		case T_NUM:
			bcg_gen_with_arg(bc, BC_PUSH_NUM, atom->num);
			break;
		default:
			warn("Don't know how to compile atom type %d", atom->type);
			break;
	}
}

void register_compiler_buildins_in(env_t *env){
	env_set(env, "lambda_compile", buildin_atom_alloc(buildin_lambda_compile));
}