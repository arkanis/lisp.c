#ifndef _BYTECODE_INTERPRETER_H
#define _BYTECODE_INTERPRETER_H

#include "memory.h"

typedef struct {
	size_t length;
	size_t allocated;
	atom_t* atoms[];
} *stack_t, stack_s;

stack_t stack_new(size_t initial_allocated);
void stack_destroy(stack_t *stack);
void stack_push(stack_t *stack, atom_t *atom);
atom_t* stack_pop(stack_t *stack);

typedef struct {
	stack_t stack;
} *bytecode_interpreter_t, bytecode_interpreter_s;

bytecode_interpreter_t bci_new(size_t preallocated_stack_size);
void bci_destroy(bytecode_interpreter_t interpreter);

atom_t* bci_eval(bytecode_interpreter_t interpreter, atom_t* compiled_lambda, atom_t *args, env_t *env);

#endif