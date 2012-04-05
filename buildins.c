#include "buildins.h"
#include "eval.h"

void register_buildins_in(env_t *env){
	env_set(env, "define", alloc_buildin(buildin_define));
	env_set(env, "+", alloc_buildin(buildin_plus));
}

atom_t* buildin_define(atom_t *args, env_t *env){
	char *name = args->first->sym;
	atom_t *first_arg = args->rest->first;
	atom_t *value = eval_atom(first_arg, env);
	env_set(env, name, value);
	return value;
}

atom_t* buildin_plus(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	atom_t *result = alloc_num();
	result->num = first_arg->num + second_arg->num;
	return result;
}