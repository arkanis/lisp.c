#ifndef _BYTECODE_COMPILER_H
#define _BYTECODE_COMPILER_H

#include "memory.h"

/*

Compiled lambda:
| constant table
| bytecode

Frame
| Locals (with args first)
| Stack

*/

typedef struct {
	size_t length;
	char *symbols[];
} symbol_list_t;

atom_t* bcc_compile_to_lambda(atom_t *arg_names, atom_t *body, env_t *env);
void bcc_compile_expr(atom_t *cl_atom, atom_t *arg_names, atom_t *expr, env_t *env);
size_t bcc_add_atom_to_literal_table(atom_t *cl_atom, atom_t *subject);

#endif