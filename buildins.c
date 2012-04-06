#include "buildins.h"
#include "eval.h"
#include "logger.h"

void register_buildins_in(env_t *env){
	env_set(env, "define", alloc_buildin(buildin_define));
	env_set(env, "+", alloc_buildin(buildin_plus));
	env_set(env, "-", alloc_buildin(buildin_minus));
	env_set(env, "if", alloc_buildin(buildin_if));
	env_set(env, "lambda", alloc_buildin(buildin_lambda));
	env_set(env, "*", alloc_buildin(buildin_multiply));
	env_set(env, "/", alloc_buildin(buildin_divide));
	env_set(env, "=", alloc_buildin(buildin_eqal));
}

atom_t* buildin_define(atom_t *args, env_t *env){
	atom_t *name_atom = args->first;
	atom_t *value_atom = args->rest->first;
	
	if (name_atom->type != T_SYM) {
		warn("define expects the first argument to be a symbol");
		return get_nil_atom();
	}
	
	atom_t *evaled_value = eval_atom(value_atom, env);
	env_set(env, name_atom->sym, evaled_value);
	return evaled_value;
}

atom_t* buildin_if(atom_t *args, env_t *env){
	atom_t *cond = eval_atom(args->first, env);
	if (cond->type == T_TRUE)
		return eval_atom(args->rest->first, env);
	else
		return eval_atom(args->rest->rest->first, env);
}

atom_t* buildin_lambda(atom_t *args, env_t *env){
	atom_t *lambda = alloc_lambda();
	lambda->args = args->first;
	lambda->body = args->rest->first;
	lambda->env = env;
	return lambda;
}


//
// Math
//

atom_t* buildin_plus(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	
	if (first_arg->type != T_NUM || second_arg->type != T_NUM){
		warn("plus only works on numbers");
		return get_nil_atom();
	}
	
	atom_t *result = alloc_num();
	result->num = first_arg->num + second_arg->num;
	return result;
}

atom_t* buildin_minus(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	
	if (first_arg->type != T_NUM || second_arg->type != T_NUM){
		warn("minus only works on numbers");
		return get_nil_atom();
	}
	
	atom_t *result = alloc_num();
	result->num = first_arg->num - second_arg->num;
	return result;
}

atom_t* buildin_multiply(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	
	if (first_arg->type != T_NUM || second_arg->type != T_NUM){
		warn("multiply only works on numbers");
		return get_nil_atom();
	}
	
	atom_t *result = alloc_num();
	result->num = first_arg->num * second_arg->num;
	return result;
}

atom_t* buildin_divide(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	
	if (first_arg->type != T_NUM || second_arg->type != T_NUM){
		warn("divide only works on numbers");
		return get_nil_atom();
	}
	
	atom_t *result = alloc_num();
	result->num = first_arg->num / second_arg->num;
	return result;
}


//
// Comperators
//

atom_t* buildin_eqal(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	
	if (first_arg->type != T_NUM || second_arg->type != T_NUM){
		warn("eqal only works on numbers");
		return get_nil_atom();
	}
	
	if (first_arg->num == second_arg->num)
		return get_true_atom();
	else
		return get_false_atom();
}

/*

cons
first
rest

*/