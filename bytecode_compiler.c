#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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
 * - handel compilation of nested lambdas
 */
atom_t* bcc_compile_to_lambda(atom_t *arg_names, atom_t *body, env_t *env){
	atom_t *cl = compiled_lambda_atom_alloc(bcg_init(), (atom_list_t){0, NULL}, 0, 0);
	
	// Put the arg names into the names array
	for(atom_t *atom = arg_names; atom->type == T_PAIR; atom = atom->rest)
		cl->comp_data->arg_count++;
	cl->comp_data->names = malloc(cl->comp_data->arg_count * sizeof(cl->comp_data->names[0]));
	size_t i = 0;
	for(atom_t *atom = arg_names; atom->type == T_PAIR; atom = atom->rest){
		assert(atom->first->type == T_SYM);
		cl->comp_data->names[i] = atom->first->sym;
		i++;
	}
	
	bcc_compile_expr(cl, body, env);
	bcg_gen_op(&cl->bytecode, BC_RETURN);
	
	return cl;
}

void bcc_compile_expr(atom_t *cl_atom, atom_t *expr, env_t *env){
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
				bcg_gen(&cl_atom->bytecode, (instruction_t){BC_PUSH_LITERAL, .index = idx, .offset = 0});
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
			if ( (idx = symbol_in_arg_list(cl_atom, expr)) != -1 ) {
				// symbol is in argument list, generate a push-arg instruction
				bcg_gen(&cl_atom->bytecode, (instruction_t){BC_PUSH_ARG, .index = idx});
			} else {
				warn("TODO: handle variable access");
				assert(0);
			}
			} break;
		case T_PAIR: {
			atom_t *function_slot = expr->first;
			if (function_slot->type == T_SYM) {
				atom_t *looked_up_function_slot = env_get(env, function_slot->sym);
				if (looked_up_function_slot->type == T_BUILDIN && looked_up_function_slot->compile_func != NULL) {
					looked_up_function_slot->compile_func(cl_atom, expr->rest, env);
					break;
				}
			}
			
			// compile function slot
			// compile args from left to right
			// generate function call
			bcc_compile_expr(cl_atom, function_slot, env);
			for(atom_t *atom = expr->rest; atom->type == T_PAIR; atom = atom->rest)
				bcc_compile_expr(cl_atom, atom, env);
			bcg_gen_op(&cl_atom->bytecode, BC_CALL);
			
			} break;
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

ssize_t symbol_in_arg_list(atom_t *cl, atom_t *symbol){
	assert(symbol->type == T_SYM);
	for(size_t i = 0; i < cl->comp_data->arg_count; i++){
		if ( strcmp(cl->comp_data->names[i], symbol->sym) == 0 )
			return i;
	}
	return -1;
}