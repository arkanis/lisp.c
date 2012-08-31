#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include <stdio.h>

#include "bytecode_interpreter.h"
#include "logger.h"
#include "eval.h"


void stack_reallocate_if_neccessary(stack_t *stack){
	if ((*stack)->length > (*stack)->allocated){
		(*stack)->allocated = (*stack)->length * 2;
		*stack = gc_realloc(*stack, sizeof(size_t) * 2 + sizeof(atom_t) * (*stack)->allocated);
		assert((*stack)->length <= (*stack)->allocated);
	} else if ((*stack)->length < (*stack)->allocated / 3) {
		(*stack)->allocated /= 2;
		*stack = gc_realloc(*stack, sizeof(size_t) * 2 + sizeof(atom_t) * (*stack)->allocated);
		assert((*stack)->length <= (*stack)->allocated);
	}
}

inline stack_t stack_new(size_t initial_allocated){
	stack_t stack = gc_alloc(sizeof(size_t) * 2 + sizeof(atom_t) * initial_allocated);
	assert(stack != NULL);
	stack->length = 0;
	stack->allocated = initial_allocated;
	return stack;
}

inline void stack_destroy(stack_t *stack){
	assert(stack != NULL && *stack != NULL);
	gc_free(*stack);
	*stack = NULL;
}
	
inline void stack_push(stack_t *stack, atom_t *atom){
	assert(stack != NULL && *stack != NULL);
	assert(atom != NULL);
	(*stack)->length++;
	stack_reallocate_if_neccessary(stack);
	(*stack)->atoms[(*stack)->length-1] = atom;
}

inline void stack_push_n(stack_t *stack, atom_t *atom, size_t n){
	assert(stack != NULL && *stack != NULL);
	assert(atom != NULL);
	(*stack)->length += n;
	stack_reallocate_if_neccessary(stack);
	for (size_t i = 0; i < n; i++)
		(*stack)->atoms[(*stack)->length-1-i] = atom;
}

inline atom_t* stack_pop(stack_t *stack){
	assert(stack != NULL && *stack != NULL);
	atom_t *val = (*stack)->atoms[(*stack)->length-1];
	(*stack)->atoms[(*stack)->length-1] = NULL;
	(*stack)->length--;
	stack_reallocate_if_neccessary(stack);
	return val;
}

inline atom_t* stack_peek(stack_t *stack){
	assert(stack != NULL && *stack != NULL);
	return (*stack)->atoms[(*stack)->length-1];
}

inline void stack_pop_n(stack_t *stack, size_t n){
	assert(stack != NULL && *stack != NULL);
	for(size_t i = 0; i < n; i++)
		(*stack)->atoms[(*stack)->length-1-i] = NULL;
	(*stack)->length -= n;
	stack_reallocate_if_neccessary(stack);
}



bytecode_interpreter_t bci_new(size_t preallocated_stack_size){
	bytecode_interpreter_t interpreter = gc_alloc(sizeof(bytecode_interpreter_s));
	interpreter->stack = stack_new(preallocated_stack_size);
	return interpreter;
}

void bci_destroy(bytecode_interpreter_t interpreter){
	stack_destroy(&interpreter->stack);
	gc_free(interpreter);
}

/**
 * Abbreviations: fp = frame pointer, ip = instruction pointer
 * 
 * Frame layout on stack:
 * 	fp => compiled lambda that is currently executed
 * 	fp + 1 => args
 * 	fp + 1 + arg_count => vars
 * 	fp + 1 + arg_count + var_count => saved fp and ip indices (atom of type T_INTERPRETER_STATE)
 * 
 * fp and ip are saved as indices. fp is an stack index and ip is the index into the bytecode
 * of the previously executed compiled lambda. This allows the stack and previous lambda
 * to be moved in memory. Important for the stack since it can grow (and be reallocated
 * while dooing so). The compiled lambda atom might move due to a future garbage collector.
 */
atom_t* bci_eval(bytecode_interpreter_t interp, atom_t* rl, atom_t *args, env_t *env){
	// The variables used by the interpreter to refer to the current context
	size_t frame_index, arg_count;
	instruction_t *ip;
	scope_p frame_scope = NULL;  // allocated when the first lambda is built
	uint8_t scope_escaped = false;
	
	// Build the initial stack frame and context variables
	assert(rl->type == T_RUNTIME_LAMBDA);
	frame_index = interp->stack->length;
	stack_push(&interp->stack, rl);
	
	arg_count = 0;
	for(atom_t *atom = args; atom->type == T_PAIR; atom = atom->rest){
		stack_push(&interp->stack, atom->first);
		arg_count++;
	}
	
	if (arg_count != rl->cl->comp_data->arg_count){
		warn("Not enough arguments for function! Got %d, required %d", arg_count, rl->cl->comp_data->arg_count);
		return nil_atom();
	}
	
	stack_push_n(&interp->stack, nil_atom(), rl->cl->comp_data->var_count);
	stack_push(&interp->stack, nil_atom());
	ip = rl->cl->bytecode.code;
	
	
	inline void check_atom_for_escaped_scope(atom_t *subject){
		if (frame_scope == NULL || scope_escaped == true)
			return;
		
		switch(subject->type){
			case T_PAIR:
				check_atom_for_escaped_scope(subject->first);
				check_atom_for_escaped_scope(subject->rest);
				break;
			case T_RUNTIME_LAMBDA:
				for (scope_p s = subject->scopes; s != NULL; s = s->next){
					if (s == frame_scope){
						scope_escaped = true;
						return;
					}
				}
				break;
			default:
				// Other atoms don't need to be checked (can't contain scope references)
				break;
		}
	}
	
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
			case BC_PUSH_LITERAL: case BC_LAMBDA: {
				scope_t this_scope = (scope_t){ .next = rl->scopes, .type = SCOPE_STACK, .arg_count = arg_count, .frame_index = frame_index};
				scope_p target_scope = &this_scope;
				for(uint16_t scope_offset = ip->offset; scope_offset > 0; scope_offset--)
					target_scope = target_scope->next;
				
				assert(target_scope->type != SCOPE_ENV);
				atom_t **frame_pointer = NULL;
				if (target_scope->type == SCOPE_STACK)
					frame_pointer = interp->stack->atoms + target_scope->frame_index;
				else
					frame_pointer = target_scope->atoms;
				
				assert(frame_pointer[0]->type == T_RUNTIME_LAMBDA);
				atom_t *target_cl = frame_pointer[0]->cl;
				assert(ip->index < target_cl->literal_table.length);
				if (ip->op == BC_PUSH_LITERAL) {
					assert(target_cl->literal_table.atoms[ip->index]->type != T_COMPILED_LAMBDA);
					stack_push(&interp->stack, target_cl->literal_table.atoms[ip->index]);
				} else {
					atom_t *compiled_lambda = target_cl->literal_table.atoms[ip->index];
					assert(compiled_lambda->type == T_COMPILED_LAMBDA);
					if (frame_scope == NULL)
						frame_scope = scope_stack_alloc(rl->scopes, arg_count, frame_index);
					atom_t *new_rl = runtime_lambda_atom_alloc(compiled_lambda, frame_scope);
					stack_push(&interp->stack, new_rl);
				}
				} break;
				
			case BC_PUSH_ARG: case BC_PUSH_VAR: case BC_SAVE_VAR: {
				scope_t this_scope = (scope_t){ .next = rl->scopes, .type = SCOPE_STACK, .arg_count = arg_count, .frame_index = frame_index};
				scope_p target_scope = &this_scope;
				for(uint16_t scope_offset = ip->offset; scope_offset > 0; scope_offset--)
					target_scope = target_scope->next;
				
				assert(target_scope->type != SCOPE_ENV);
				atom_t **frame_pointer = NULL;
				if (target_scope->type == SCOPE_STACK)
					frame_pointer = interp->stack->atoms + target_scope->frame_index;
				else
					frame_pointer = target_scope->atoms;
				
				switch(ip->op){
				case BC_PUSH_ARG:
					assert(ip->index < target_scope->arg_count);
					stack_push(&interp->stack, frame_pointer[ip->index+1]);
					break;
				case BC_PUSH_VAR:
					assert(frame_pointer[0]->type == T_RUNTIME_LAMBDA && ip->index < frame_pointer[0]->cl->comp_data->var_count);
					stack_push(&interp->stack, frame_pointer[target_scope->arg_count + ip->index+1]);
					break;
				case BC_SAVE_VAR: {
					assert(frame_pointer[0]->type == T_RUNTIME_LAMBDA && ip->index < frame_pointer[0]->cl->comp_data->var_count);
					atom_t *value = stack_peek(&interp->stack);
					if (ip->offset > 0)  // no need to check if we store the atom in our own stack frame
						check_atom_for_escaped_scope(value);
					frame_pointer[target_scope->arg_count + ip->index+1] = value;
					}break;
				}
				
				} break;
				
			case BC_PUSH_FROM_ENV: case BC_SAVE_ENV: {
				// First loop though the scope chain to get the definition env
				scope_p target_scope = rl->scopes;
				while(target_scope->next != NULL)
					target_scope = target_scope->next;
				assert(target_scope->type == SCOPE_ENV);
				env_t *target_env = target_scope->env;
				
				// Pop the key symbol
				assert(ip->index < rl->cl->literal_table.length);
				atom_t *key = rl->cl->literal_table.atoms[ip->index];
				assert(key->type == T_SYM);
				
				if (ip->op == BC_PUSH_FROM_ENV) {
					atom_t *value = env_get(target_env, key->sym);
					if (value != NULL) {
						stack_push(&interp->stack, value);
					} else {
						warn("BC_PUSH_FROM_ENV: no binding for %s in env %p", key->sym, target_env);
						stack_push(&interp->stack, nil_atom());
					}
				} else {
					atom_t *value = stack_peek(&interp->stack);
					check_atom_for_escaped_scope(value);
					env_set(target_env, key->sym, value);
				}
				} break;
				
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
					uint16_t call_arg_count = ip->num;
					atom_t *func = interp->stack->atoms[interp->stack->length - 1 - call_arg_count]; // length - 1 => last arg, - call_arg_count => func
					
					switch (func->type) {
						case T_RUNTIME_LAMBDA: {
							// Continue to use the stack
							atom_t *saved_state = interpreter_state_atom_alloc(frame_index, ip - rl->cl->bytecode.code, arg_count, scope_escaped, frame_scope);
							
							arg_count = call_arg_count;
							// TODO: check if argument count on stack match the required argument count of the compiled lambda
							frame_index = interp->stack->length - 1 - call_arg_count; // length - 1 => last arg, - call_arg_count => func
							rl = func;
							ip = rl->cl->bytecode.code;
							frame_scope = NULL;
							scope_escaped = false;
							
							stack_push_n(&interp->stack, nil_atom(), rl->cl->comp_data->var_count);
							stack_push(&interp->stack, saved_state);
							
							// Restart the outer while loop because we don't want to increment the instruction pointer (ip). Otherwise we would miss
							// the first instruction of the new compiled lambda.
							continue;
							} break;
							
						case T_BUILDIN: {
							// Build a pair argument list of the args and pop them from the stack while we're at it
							atom_t *arg_atoms = nil_atom();
							for(size_t i = 0; i < call_arg_count; i++)
								arg_atoms = pair_atom_alloc(stack_pop(&interp->stack), arg_atoms);
							
							// Pop the func from the stack
							atom_t *popped_func = stack_pop(&interp->stack);
							assert(func == popped_func);
							
							// Let the buildin create the result. Pass the env of the bytecode interpreter since this is the best aproximation we have right now.
							atom_t *result = func->func(arg_atoms, env);
							stack_push(&interp->stack, result);
							} break;
							
						case T_LAMBDA: {
							// Create a new env with the unevaled args in it (the args on the stack have already been evaled by the compiled bytecode)
							env_t *lambda_env = env_alloc(func->env);
							
							// Build a pair argument list of the args and pop them from the stack while we're at it
							atom_t *arg_atoms = nil_atom();
							for(size_t i = 0; i < call_arg_count; i++)
								arg_atoms = pair_atom_alloc(stack_pop(&interp->stack), arg_atoms);
							
							// Bind lambda args
							atom_t *arg_name_pair = func->args, *arg_value_pair = arg_atoms;
							while(arg_name_pair->type == T_PAIR && arg_value_pair->type == T_PAIR){
								env_def(lambda_env, arg_name_pair->first->sym, arg_value_pair->first);
								arg_name_pair = arg_name_pair->rest;
								arg_value_pair = arg_value_pair->rest;
							}
							
							// Pop the func from the stack
							atom_t *popped_func = stack_pop(&interp->stack);
							assert(func == popped_func);
							
							atom_t *result = eval_atom(func->body, lambda_env);
							stack_push(&interp->stack, result);
							} break;
							
						default:
							// Not sure what to do with T_CUSTOM in general. For the other atoms: they should never arrive here.
							assert(0);
							break;
					}
					
				} break;
				
			case BC_RETURN: {
					atom_t *return_value = stack_pop(&interp->stack);
					// TODO: Make sure to revert to the start frame_index here. Right now we're done for if a function does
					// not pop as many values as it pushes (in all brances).
					atom_t *state = stack_pop(&interp->stack);
					// Search the return value for an escaped scope if necessary
					check_atom_for_escaped_scope(return_value);
					// Capture the scope if it escaped
					if (scope_escaped){
						size_t frame_size = (1 + arg_count + rl->cl->comp_data->var_count) * sizeof(atom_t*);
						frame_scope->type = SCOPE_HEAP;
						// The GC will free the frame when it's no longer needed
						frame_scope->atoms = gc_alloc(frame_size);
						memcpy(frame_scope->atoms, interp->stack->atoms + frame_index, frame_size);
					}
					
					// Pop the arguments, variables and the compiled lambda
					stack_pop_n(&interp->stack, arg_count + rl->cl->comp_data->var_count + 1);
					if (state->type == T_INTERPRETER_STATE) {
						arg_count = state->interpreter_state.arg_count;
						frame_index = state->interpreter_state.fp_index;
						rl = interp->stack->atoms[frame_index];
						ip = rl->cl->bytecode.code + state->interpreter_state.ip_index;
						frame_scope = state->interpreter_state.frame_scope;
						scope_escaped = state->interpreter_state.scope_escaped;
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
			
			case BC_CONS: {
				atom_t *b = stack_pop(&interp->stack);
				atom_t *a = stack_pop(&interp->stack);
				stack_push(&interp->stack, pair_atom_alloc(a, b));
			} break;
			case BC_FIRST: {
				atom_t *pair = stack_pop(&interp->stack);
				assert(pair->type == T_PAIR);
				stack_push(&interp->stack, pair->first);
			} break;
			case BC_REST: {
				atom_t *pair = stack_pop(&interp->stack);
				assert(pair->type == T_PAIR);
				stack_push(&interp->stack, pair->rest);
			} break;
			
			default:
				// Unknown bytecode instruction
				assert(false);
		}
		ip++;
		assert(ip < rl->cl->bytecode.code + rl->cl->bytecode.length);
	}
	
	return nil_atom();
}

/*

			case BC_PUSH_ARG: case BC_PUSH_VAR: case BC_SAVE_VAR: {
				// remember, what is called fp up above is actually the frame index (fi)
				// Start with the frame pointer on the first arg of the current context
				size_t frame_index = fp;  // stack index of the stack frame we are currently in
				atom_t **frame_ptr = interp->stack->atoms + fp + 1;  // pointer to the first arg in the stack frame or captured frame we're currently in
				size_t frame_arg_count = arg_count;  // the arg count in the stack frame or captured frame we currently are
				
				int16_t fo = ip->offset;  // offset counter, if is reaches zero we're where we want to be
				atom_t *lambda = cl;  // current compiled lambda
				captured_frame_p cf = NULL;  // pointer to the current captured frame. required to advance via the next pointer.
				
				// Repeat the game until we worked through all the required offsets
				while(fo > 0){
					// Take the first captured frame
					if (lambda->comp_data->captured_frames != NULL){
						cf = lambda->comp_data->captured_frames;
						frame_ptr = cf->atoms;
						frame_arg_count = cf->arg_count;
						fo--;
						if (fo == 0)
							break;
						
						// Take the next captured frames attached to this lambda
						while(fo > 0 && cf && cf->next != NULL){
							cf = cf->next;
							frame_ptr = cf->atoms;
							frame_arg_count = cf->arg_count;
							fo--;
						}
					}
					
					// Nothing left in this lambda or its captured frames. This means the frame_offset
					// referes to an outer arg or local that is still on the stack. So lets go and dig up the
					// prev stack frame.
					if (fo > 0){
						atom_t *state = interp->stack->atoms[frame_index + 1 + lambda->comp_data->arg_count + lambda->comp_data->var_count];
						assert(state->type == T_INTERPRETER_STATE);
						lambda = interp->stack->atoms[state->interpreter_state.fp_index];
						assert(lambda->type == T_COMPILED_LAMBDA);
						frame_ptr = interp->stack->atoms + state->interpreter_state.fp_index + 1;
						frame_index = state->interpreter_state.fp_index;
						frame_arg_count = state->interpreter_state.arg_count;
						fo--;
					}
				}
				
				switch(ip->op){
				case BC_PUSH_ARG:
					assert(ip->index < frame_arg_count);
					stack_push(&interp->stack, frame_ptr[ip->index]);
					break;
				case BC_PUSH_VAR:
					assert(ip->index < lambda->comp_data->var_count);
					stack_push(&interp->stack, frame_ptr[frame_arg_count + ip->index]);
					break;
				case BC_SAVE_VAR: {
					assert(ip->index < lambda->comp_data->var_count);
					atom_t *value = stack_pop(&interp->stack);
					frame_ptr[frame_arg_count + ip->index] = value;
					}break;
				}
				
				} break;

*/