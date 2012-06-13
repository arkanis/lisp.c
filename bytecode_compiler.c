#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "logger.h"
#include "bytecode_generator.h"


void compile_statement(atom_t *cl_atom, atom_t *lambda_args, atom_t *ast);
size_t add_atom_to_literal_table(atom_t *cl_atom, atom_t *subject);
ssize_t symbol_in_arg_list(atom_t *lambda_args, atom_t *symbol);
ssize_t symbol_in_arg_list(atom_t *lambda_args, atom_t *symbol);

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
	
	atom_t *args_atom = args->first;
	atom_t *body_atom = args->rest->first;
	atom_t *cl_atom = compiled_lambda_atom_alloc(bcg_init(), (atom_list_t){0, NULL});
	
	compile_statement(cl_atom, args_atom, body_atom);
	bcg_gen(&cl_atom->bytecode, BC_RETURN);
	
	return cl_atom;
}

void compile_statement(atom_t *cl_atom, atom_t *lambda_args, atom_t *ast){
	switch (ast->type) {
		case T_NIL:
			bcg_gen(&cl_atom->bytecode, BC_PUSH_NIL);
			break;
		case T_TRUE:
			bcg_gen(&cl_atom->bytecode, BC_PUSH_TRUE);
			break;
		case T_FALSE:
			bcg_gen(&cl_atom->bytecode, BC_PUSH_FALSE);
			break;
		case T_NUM:
			bcg_gen_with_arg(&cl_atom->bytecode, BC_PUSH_NUM, ast->num);
			break;
		case T_STR: {
			size_t idx = add_atom_to_literal_table(cl_atom, ast);
			bcg_gen_with_arg(&cl_atom->bytecode, BC_PUSH_LITERAL, idx);
			} break;
		case T_SYM: {
			ssize_t idx;
			if ( (idx = symbol_in_arg_list(lambda_args, ast)) != -1 ) {
				// symbol is in argument list, generate a push-arg instruction
				bcg_gen_with_arg(&cl_atom->bytecode, BC_PUSH_ARG, idx);
			} else {
				warn("TODO: handle variable access");
			}
			} break;
		default:
			warn("Don't know how to compile atom type %d", ast->type);
			break;
	}
}

size_t add_atom_to_literal_table(atom_t *cl_atom, atom_t *subject){
	cl_atom->literal_table.length++;
	cl_atom->literal_table.atoms = realloc(cl_atom->literal_table.atoms, cl_atom->literal_table.length * sizeof(atom_t*));
	cl_atom->literal_table.atoms[cl_atom->literal_table.length - 1] = subject;
	return cl_atom->literal_table.length - 1;
}

ssize_t symbol_in_arg_list(atom_t *lambda_args, atom_t *symbol){
	atom_t *atom = lambda_args;
	ssize_t idx = 0;
	while(atom->type == T_PAIR){
		if (atom->first->type == T_SYM && strcmp(atom->first->sym, symbol->sym) == 0)
			return idx;
		atom = atom->rest;
		idx++;
	}
	return -1;
}

void register_compiler_buildins_in(env_t *env){
	env_set(env, "lambda_compile", buildin_atom_alloc(buildin_lambda_compile));
}