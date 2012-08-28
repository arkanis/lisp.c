#include <stdlib.h>

#include "logger.h"
#include "eval.h"

atom_t *eval_atom(atom_t *atom, env_t *env){
	if (atom->type < T_COMPLEX_ATOM) {
		return atom;
	} else if (atom->type == T_SYM) {
		atom_t *result = env_get(env, atom->sym);
		if (result != NULL)
			return result;
		
		warn("Undefined binding for symbol %s in env %p", atom->sym, env);
		return nil_atom();
	} else if (atom->type == T_PAIR) {
		atom_t *function_slot = atom->first;
		atom_t *args = atom->rest;
		atom_t *evaled_function_slot = eval_atom(function_slot, env);
		
		switch(evaled_function_slot->type){
			case T_BUILDIN:
				return evaled_function_slot->func(args, env);
				break;
			case T_LAMBDA:
				{
					env_t *lambda_env = env_alloc(evaled_function_slot->env);
					
					// Eval and bind lambda args
					atom_t *arg_name_pair = evaled_function_slot->args, *arg_value_pair = args;
					while(arg_name_pair->type == T_PAIR && arg_value_pair->type == T_PAIR){
						env_set(lambda_env, arg_name_pair->first->sym, eval_atom(arg_value_pair->first, env));
						arg_name_pair = arg_name_pair->rest;
						arg_value_pair = arg_value_pair->rest;
					}
					
					return eval_atom(evaled_function_slot->body, lambda_env);
				}
				break;
			/*
			case T_RUNTIME_LAMBDA:
				return bci_eval(interp, evaled_function_slot, args, env);
				break;
			*/
			case T_CUSTOM:
				// If a custom atom has a func call it with the atom itself as first argument
				if (evaled_function_slot->custom.func != NULL)
					return evaled_function_slot->custom.func(pair_atom_alloc(evaled_function_slot, args), env);
				// else: fall through
			default:
				warn("Got unexpected atom in function slot, type: %d", evaled_function_slot->type);
				return nil_atom();
		}
	}
	warn("Got unknown atom, type: %d", atom->type);
	return nil_atom();
}