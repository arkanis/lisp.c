#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "bytecode_compiler.h"
#include "bytecode_generator.h"
#include "logger.h"


void compile_statement(atom_t *cl_atom, atom_t *lambda_args, atom_t *ast, env_t *env);
ssize_t symbol_in_arg_list(atom_t *lambda_args, atom_t *symbol);

/**
 * Compiles an expression into a compiled lamba atom.
 * 
 * TODO:
 * - call bcg_destroy(&bc); when a compiled lambda is freed
 * - handled compilation of nested lambdas
 */
atom_t* bcc_compile_to_lambda(atom_t *arg_names, atom_t *body, env_t *env){
	atom_t *cl_atom = compiled_lambda_atom_alloc(bcg_init(), (atom_list_t){0, NULL});
	
	bcc_compile_expr(cl_atom, arg_names, body, env);
	bcg_gen_op(&cl_atom->bytecode, BC_RETURN);
	
	return cl_atom;
}

void bcc_compile_expr(atom_t *cl_atom, atom_t *arg_names, atom_t *expr, env_t *env){
	switch (expr->type) {
		case T_NIL:
			bcg_gen_op(&cl_atom->bytecode, BC_PUSH_NIL);
			break;
		case T_TRUE:
			bcg_gen_op(&cl_atom->bytecode, BC_PUSH_TRUE);
			break;
		case T_FALSE:
			bcg_gen_op(&cl_atom->bytecode, BC_PUSH_FALSE);
			break;
		case T_NUM:
			if (expr->num > INT32_MAX || expr->num < INT32_MIN) {
				size_t idx = bcc_add_atom_to_literal_table(cl_atom, expr);
				bcg_gen(&cl_atom->bytecode, (instruction_t){BC_PUSH_LITERAL, .index = idx});
			} else {
				bcg_gen(&cl_atom->bytecode, (instruction_t){BC_PUSH_NUM, .num = expr->num});
			}
			break;
		case T_STR: {
			size_t idx = bcc_add_atom_to_literal_table(cl_atom, expr);
			bcg_gen(&cl_atom->bytecode, (instruction_t){BC_PUSH_LITERAL, .index = idx});
			} break;
		case T_SYM: {
			ssize_t idx;
			if ( (idx = symbol_in_arg_list(arg_names, expr)) != -1 ) {
				// symbol is in argument list, generate a push-arg instruction
				bcg_gen(&cl_atom->bytecode, (instruction_t){BC_PUSH_ARG, .index = idx});
			} else {
				warn("TODO: handle variable access");
			}
			} break;
		/*
		case T_PAIR: {
			atom_t *function_slot = ast->first;
			if (function_slot->type == T_SYM) {
				atom_t *looked_up_function_slot = env_get(env, function_slot->sym);
				if (looked_up_function_slot->type == T_BUILDIN && looked_up_function_slot->compile_func != NULL) {
					looked_up_function_slot->compile_func();
					// call buildin compile
					break;
				}
			}
			
			// compile function slot
			// compile args from left to right
			// generate function call
			
			} break;
		*/
		default:
			warn("Don't know how to compile atom type %d", expr->type);
			break;
	}
}


size_t bcc_add_atom_to_literal_table(atom_t *cl_atom, atom_t *subject){
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