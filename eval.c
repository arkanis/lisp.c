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
		return get_nil_atom();
	} else if (atom->type == T_PAIR) {
		atom_t *function_slot = atom->pair.first;
		atom_t *args = atom->pair.rest;
		atom_t *evaled_function_slot = eval_atom(function_slot, env);
		
		if (evaled_function_slot->type == T_BUILDIN) {
			return evaled_function_slot->func(args, env);
		} else {
			warn("Got unexpected atom in function slot, type: %d", evaled_function_slot->type);
			return get_nil_atom();
		}
	}
	
	warn("Got unknown atom, type: %d", atom->type);
	return get_nil_atom();
}