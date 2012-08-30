#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "bytecode_compiler.h"
#include "bytecode_generator.h"
#include "logger.h"
// DEBUG
#include "printer.h"


void compile_statement(atom_t *cl_atom, atom_t *lambda_args, atom_t *ast, env_t *env);
ssize_t symbol_in_names(atom_t *lambda_args, atom_t *symbol);

/**
 * Compiles an expression into a compiled lamba atom.
 * 
 * TODO:
 * - call bcg_destroy(&bc); when a compiled lambda is freed
 * - handel compilation of nested lambdas
 */
atom_t* bcc_compile_to_lambda(atom_t *arg_names, atom_t *body, env_t *env, atom_t *parent_cl){
	atom_t *cl = compiled_lambda_atom_alloc(bcg_init(), (atom_list_t){0, NULL}, 0, 0);
	cl->comp_data->parent = parent_cl;
	
	// Put the arg names into the names array
	for(atom_t *atom = arg_names; atom->type == T_PAIR; atom = atom->rest)
		cl->comp_data->arg_count++;
	cl->comp_data->names = gc_alloc(cl->comp_data->arg_count * sizeof(cl->comp_data->names[0]));
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
			if (expr->num > INT16_MAX || expr->num < INT16_MIN) {
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
			size_t scope_offset = 0;
			atom_t *current_cl = cl_atom;
			do {
				if ( (idx = symbol_in_names(current_cl, expr)) != -1 ) {
					// symbol is known in current scope (lambda)
					if (idx < current_cl->comp_data->arg_count) {
						// symbol identifies an argument, generate a push-arg instruction
						bcg_gen(&cl_atom->bytecode, (instruction_t){BC_PUSH_ARG, .index = idx, .offset = scope_offset});
					} else {
						// symbol identifies a local variable, generate a push-var instruction
						idx = idx - current_cl->comp_data->arg_count;
						bcg_gen(&cl_atom->bytecode, (instruction_t){BC_PUSH_VAR, .index = idx, .offset = scope_offset});
					}
					break;
				}
				scope_offset++;
				current_cl = current_cl->comp_data->parent;
			} while(current_cl);
			
			if (current_cl == NULL){
				// Symbol not found in any argument or variable lists of all parents, do an env lookup
				size_t literal_idx = bcc_add_atom_to_literal_table(cl_atom, expr);
				bcg_gen(&cl_atom->bytecode, (instruction_t){BC_PUSH_FROM_ENV, .index = literal_idx, .offset = 0});
			}
			
			} break;
		case T_PAIR: {
			atom_t *function_slot = expr->first;
			if (function_slot->type == T_SYM) {
				atom_t *looked_up_function_slot = env_get(env, function_slot->sym);
				if (looked_up_function_slot && looked_up_function_slot->type == T_BUILDIN && looked_up_function_slot->compile_func != NULL) {
					looked_up_function_slot->compile_func(cl_atom, expr->rest, env);
					break;
				}
			}
			
			// compile function slot
			// compile args from left to right
			// generate function call
			bcc_compile_expr(cl_atom, function_slot, env);
			size_t arg_count = 0;
			for(atom_t *atom = expr->rest; atom->type == T_PAIR; atom = atom->rest){
				bcc_compile_expr(cl_atom, atom->first, env);
				arg_count++;
			}
			bcg_gen(&cl_atom->bytecode, (instruction_t){BC_CALL, .num = arg_count});
			
			} break;
		default:
			warn("Don't know how to compile atom type %d", expr->type);
			break;
	}
}


size_t bcc_add_atom_to_literal_table(atom_t *cl_atom, atom_t *subject){
	cl_atom->literal_table.length++;
	cl_atom->literal_table.atoms = gc_realloc(cl_atom->literal_table.atoms, cl_atom->literal_table.length * sizeof(atom_t*));
	cl_atom->literal_table.atoms[cl_atom->literal_table.length - 1] = subject;
	return cl_atom->literal_table.length - 1;
}

ssize_t symbol_in_names(atom_t *cl, atom_t *symbol){
	assert(symbol->type == T_SYM);
	for(size_t i = 0; i < cl->comp_data->arg_count + cl->comp_data->var_count; i++){
		if ( strcmp(cl->comp_data->names[i], symbol->sym) == 0 )
			return i;
	}
	return -1;
}