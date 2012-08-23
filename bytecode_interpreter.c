#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

#include "bytecode_interpreter.h"

static inline void stack_reallocate_if_neccessary(stack_t *stack){
	if ((*stack)->length > (*stack)->allocated){
		(*stack)->allocated = ((*stack)->allocated == 0) ? 2 : (*stack)->allocated * 2;
		*stack = realloc(*stack, sizeof(size_t) * 2 + sizeof(atom_t) * (*stack)->allocated);
		assert((*stack)->length <= (*stack)->allocated);
	}
}

stack_t stack_new(size_t initial_allocated){
	stack_t stack = malloc(sizeof(size_t) * 2 + sizeof(atom_t) * initial_allocated);
	assert(stack != NULL);
	stack->length = 0;
	stack->allocated = initial_allocated;
	return stack;
}

void stack_destroy(stack_t *stack){
	assert(stack != NULL && *stack != NULL);
	free(*stack);
	*stack = NULL;
}
	
void stack_push(stack_t *stack, atom_t *atom){
	assert(stack != NULL && *stack != NULL);
	(*stack)->length++;
	stack_reallocate_if_neccessary(stack);
	(*stack)->atoms[(*stack)->length-1] = atom;
}

atom_t* stack_pop(stack_t *stack){
	assert(stack != NULL && *stack != NULL);
	atom_t *val = (*stack)->atoms[(*stack)->length-1];
	(*stack)->length--;
	stack_reallocate_if_neccessary(stack);
	return val;
}

void stack_pop_n(stack_t *stack, size_t n){
	assert(stack != NULL && *stack != NULL);
	(*stack)->length -= n;
	stack_reallocate_if_neccessary(stack);
}



bytecode_interpreter_t bci_new(size_t preallocated_stack_size){
	bytecode_interpreter_t interpreter = malloc(sizeof(bytecode_interpreter_s));
	interpreter->stack = stack_new(preallocated_stack_size);
	return interpreter;
}

void bci_destroy(bytecode_interpreter_t interpreter){
	stack_destroy(&interpreter->stack);
	free(interpreter);
}

/**
 * Abbreviations: fp = frame pointer, ip = instruction pointer
 * 
 * Frame layout on stack:
 * 	fp -> compiled lambda that is currently executed
 * 	fp + 1 => arg 1
 * 	fp + 2 => arg 2
 * 	fp + (n-1) => arg n-1
 * 	fp + n => saved fp and ip indices (atom of type T_INTERPRETER_STATE)
 * 
 * fp and ip are saved as indices. fp is an stack index and ip is the index into the bytecode
 * of the previously executed compiled lambda. This allows the stack and previous lambda
 * to be moved in memory. Important for the stack since it can grow (and be reallocated
 * while dooing so). The compiled lambda atom might move due to a future garbage collector.
 */
atom_t* bci_eval(bytecode_interpreter_t interp, atom_t* compiled_lambda, atom_t *args, env_t *env){
	// The variables used by the interpreter to refer to the current context
	size_t fp, arg_count;
	instruction_t *ip;
	
	// Build the initial stack frame and context variables
	fp = interp->stack->length;
	stack_push(&interp->stack, compiled_lambda);
	
	arg_count = 0;
	for(atom_t *atom = args; atom->type == T_PAIR; atom = atom->rest){
		stack_push(&interp->stack, atom->first);
		arg_count++;
	}
	
	stack_push(&interp->stack, nil_atom());
	ip = compiled_lambda->bytecode.code;
	
	
	while(true){
		switch(ip->op){
			case BC_PUSH_NIL:
				stack_push(&interp->stack, nil_atom());
				break;
			case BC_PUSH_TRUE:
				stack_push(&interp->stack, true_atom());
				break;
			case BC_PUSH_FALSE:
				stack_push(&interp->stack, false_atom());
				break;
			case BC_PUSH_NUM:
				stack_push(&interp->stack, num_atom_alloc(ip->num));
				break;
			case BC_PUSH_LITERAL:
				assert(ip->index < interp->stack->atoms[fp]->literal_table.length);
				stack_push(&interp->stack, interp->stack->atoms[fp]->literal_table.atoms[ip->index]);
				break;
			case BC_PUSH_ARG:
				assert(ip->index < arg_count);
				stack_push(&interp->stack, interp->stack->atoms[fp + 1 + ip->index]);
				break;
			case BC_PUSH_FROM_ENV:
				assert(ip->index < interp->stack->atoms[fp]->literal_table.length);
				atom_t *key = interp->stack->atoms[fp]->literal_table.atoms[ip->index];
				assert(key->type == T_SYM);
				stack_push(&interp->stack, env_get(env, key->sym));
				break;
				
			case BC_DROP:
				stack_pop(&interp->stack);
				break;
			case BC_JUMP:
				ip += ip->jump_offset;
				break;
			case BC_JUMP_IF_FALSE:
				if (stack_pop(&interp->stack) == false_atom())
					ip += ip->jump_offset;
				break;
				
				
			case BC_CALL: {
					atom_t *new_arg_count_atom = stack_pop(&interp->stack);
					stack_push(&interp->stack, interpreter_state_atom_alloc(fp, ip - interp->stack->atoms[fp]->bytecode.code, arg_count));
					
					assert(new_arg_count_atom->type == T_NUM);
					arg_count = new_arg_count_atom->num;
					fp = interp->stack->length - 1 - 1 - arg_count; // length - 1 => interpreter state, -1 => last arg, - arg_count => func
					ip = interp->stack->atoms[fp]->bytecode.code;
					continue;
				} break;
				
			case BC_RETURN: {
					atom_t *return_value = stack_pop(&interp->stack);
					atom_t *state = stack_pop(&interp->stack);
					// Pop the arguments and the compiled lambda
					stack_pop_n(&interp->stack, arg_count + 1);
					if (state->type == T_INTERPRETER_STATE) {
						arg_count = state->interpreter_state.arg_count;
						fp = state->interpreter_state.fp_index;
						ip = interp->stack->atoms[fp]->bytecode.code + state->interpreter_state.ip_index;
						stack_push(&interp->stack, return_value);
					} else {
						return return_value;
					}
				} break;
				
			
			case BC_ADD: {
				atom_t *b = stack_pop(&interp->stack);
				atom_t *a = stack_pop(&interp->stack);
				assert(a->type == T_NUM && b->type == T_NUM);
				stack_push(&interp->stack, num_atom_alloc(a->num + b->num));
			} break;
			
			case BC_SUB: {
				atom_t *b = stack_pop(&interp->stack);
				atom_t *a = stack_pop(&interp->stack);
				assert(a->type == T_NUM && b->type == T_NUM);
				stack_push(&interp->stack, num_atom_alloc(a->num - b->num));
			} break;
			
			case BC_MUL: {
				atom_t *b = stack_pop(&interp->stack);
				atom_t *a = stack_pop(&interp->stack);
				assert(a->type == T_NUM && b->type == T_NUM);
				stack_push(&interp->stack, num_atom_alloc(a->num * b->num));
			} break;
			
			case BC_DIV: {
				atom_t *b = stack_pop(&interp->stack);
				atom_t *a = stack_pop(&interp->stack);
				assert(a->type == T_NUM && b->type == T_NUM);
				stack_push(&interp->stack, num_atom_alloc(a->num / b->num));
			} break;
			
			case BC_EQ: {
				atom_t *b = stack_pop(&interp->stack);
				atom_t *a = stack_pop(&interp->stack);
				atom_t *result = false_atom();
				
				if (a->type == b->type) {
					switch(a->type){
						case T_NUM:
							if (a->num == b->num)
								result = true_atom();
							break;
						default:
							assert(0);
							break;
					}
				}
				
				stack_push(&interp->stack, result);
			} break;
			
			default:
				// Unknown bytecode instruction
				assert(false);
		}
		ip++;
		assert(ip < interp->stack->atoms[fp]->bytecode.code + interp->stack->atoms[fp]->bytecode.length);
	}
	
	return nil_atom();
}