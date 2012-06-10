#include <dlfcn.h>
#include <stdio.h>

#include "buildins.h"
#include "eval.h"
#include "logger.h"

//
// Language buildins
//

atom_t* buildin_define(atom_t *args, env_t *env){
	if (args->first->type != T_SYM || args->rest->type != T_PAIR || args->rest->rest->type != T_NIL)
		return warn("define requires two arguments and the first one has to be a symbol"), nil_atom();
	
	atom_t *name_atom = args->first;
	atom_t *value_atom = eval_atom(args->rest->first, env);
	env_set(env, name_atom->sym, value_atom);
	return value_atom;
}

atom_t* buildin_if(atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_PAIR || args->rest->rest->rest->type != T_NIL)
		return warn("if requires exactly tree arguments"), nil_atom();
	
	atom_t *cond = eval_atom(args->first, env);
	if (cond->type == T_TRUE)
		return eval_atom(args->rest->first, env);
	else
		return eval_atom(args->rest->rest->first, env);
}

atom_t* buildin_quote(atom_t *args, env_t *env){
	if (args->rest->type != T_NIL)
		return warn("quote takes exactly one argument"), nil_atom();
	return args->first;
}

atom_t* buildin_lambda(atom_t *args, env_t *env){
	if (args->first->type != T_PAIR || args->rest->type != T_PAIR || args->rest->rest->type != T_NIL)
		return warn("lambda needs exactly two arguments (arg list and body)"), nil_atom();
	return lambda_atom_alloc(args->first, args->rest->first, env);
}

atom_t* buildin_begin(atom_t *args, env_t *env){
	atom_t *result = nil_atom();
	for(atom_t *pair = args; pair->type == T_PAIR; pair = pair->rest)
		result = eval_atom(pair->first, env);
	return result;
}

atom_t* buildin_cons(atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_NIL)
		return warn("cons needs exactly two arguments to build a pair"), nil_atom();
	return pair_atom_alloc(eval_atom(args->first, env), eval_atom(args->rest->first, env));
}

atom_t* buildin_first(atom_t *args, env_t *env){
	if (args->rest->type != T_NIL)
		return warn("first requires exactly one argument"), nil_atom();	
	
	atom_t *pair_atom = eval_atom(args->first, env);
	if (pair_atom->type != T_PAIR)
		return warn("first: the argument have to eval to a pair"), nil_atom();
	
	return pair_atom->first;
}

atom_t* buildin_rest(atom_t *args, env_t *env){
	if (args->rest->type != T_NIL)
		return warn("rest requires exactly one argument"), nil_atom();	
	
	atom_t *pair_atom = eval_atom(args->first, env);
	if (pair_atom->type != T_PAIR)
		return warn("rest: the argument have to eval to a pair"), nil_atom();
	
	return pair_atom->rest;
}


//
// Math
//

atom_t* buildin_plus(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	
	if (first_arg->type != T_NUM || second_arg->type != T_NUM){
		warn("plus only works on numbers");
		return nil_atom();
	}
	
	return num_atom_alloc(first_arg->num + second_arg->num);
}

atom_t* buildin_minus(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	
	if (first_arg->type != T_NUM || second_arg->type != T_NUM){
		warn("minus only works on numbers");
		return nil_atom();
	}
	
	return num_atom_alloc(first_arg->num - second_arg->num);
}

atom_t* buildin_multiply(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	
	if (first_arg->type != T_NUM || second_arg->type != T_NUM){
		warn("multiply only works on numbers");
		return nil_atom();
	}
	
	return num_atom_alloc(first_arg->num * second_arg->num);
}

atom_t* buildin_divide(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	
	if (first_arg->type != T_NUM || second_arg->type != T_NUM){
		warn("divide only works on numbers");
		return nil_atom();
	}
	
	return num_atom_alloc(first_arg->num / second_arg->num);
}


//
// Comperators
//

atom_t* buildin_eqal(atom_t *args, env_t *env){
	atom_t *first_arg = eval_atom(args->first, env);
	atom_t *second_arg = eval_atom(args->rest->first, env);
	
	if (first_arg->type != T_NUM || second_arg->type != T_NUM){
		warn("eqal only works on numbers");
		return nil_atom();
	}
	
	if (first_arg->num == second_arg->num)
		return true_atom();
	else
		return false_atom();
}


//
// Module loading
//

typedef atom_t* (*mod_init_func_t)(env_t *env);

atom_t* buildin_mod_load(atom_t *args, env_t *env){
	atom_t *name_atom = eval_atom(args->first, env);
	void *shared_obj = dlopen(name_atom->str, RTLD_LAZY);
	
	if (shared_obj == NULL){
		warn("Failed to load module %s: %s", name_atom->str, dlerror());
		return nil_atom();
	}
	
	mod_init_func_t init = dlsym(shared_obj, "init");
	if (init == NULL){
		warn("Module %s does not have an init function, aborting load", name_atom->str);
		dlclose(shared_obj);
		return nil_atom();
	}
	
	init(env);
	return nil_atom();
}

//
// Environments
//

atom_t* buildin_env_self(atom_t *args, env_t *env){
	return env_atom_alloc(env);
}

atom_t* buildin_env_new(atom_t *args, env_t *env){
	atom_t *parent_env_atom = eval_atom(args->first, env);
	env_t *parent_env;
	
	if (parent_env_atom->type == T_ENV)
		parent_env = parent_env_atom->env;
	else if (parent_env_atom->type != T_NIL)
		warn("The parent environment has to be an environment atom");
	
	return env_atom_alloc( env_alloc(parent_env) );
}

atom_t* buildin_env_get(atom_t *args, env_t *env){
	atom_t *env_atom = eval_atom(args->first, env);
	atom_t *key_atom = eval_atom(args->rest->first, env);
	
	if (env_atom->type != T_ENV){
		warn("env_get only works with environments");
		return nil_atom();
	}
	
	atom_t* value_atom = NULL;
	if (key_atom->type == T_SYM)
		value_atom = env_get(env_atom->env, key_atom->sym);
	else if (key_atom->type == T_STR)
		value_atom = env_get(env_atom->env, key_atom->str);
	else
		warn("env_get needs a symbol or string as key");
	
	return (value_atom != NULL) ? value_atom : nil_atom();
}

atom_t* buildin_env_set(atom_t *args, env_t *env){
	atom_t *env_atom = eval_atom(args->first, env);
	atom_t *key_atom = eval_atom(args->rest->first, env);
	atom_t *value_atom = eval_atom(args->rest->rest->first, env);
	
	if (env_atom->type != T_ENV){
		warn("env_set only works with environments");
		return nil_atom();
	}
	
	if (key_atom->type == T_SYM)
		env_set(env_atom->env, key_atom->sym, value_atom);
	else if (key_atom->type == T_STR)
		env_set(env_atom->env, key_atom->str, value_atom);
	else
		warn("env_set needs a symbol or string as key");
	
	return env_atom;
}




atom_t* buildin_print(atom_t *args, env_t *env){
	atom_t *atom = eval_atom(args->first, env);
	switch(atom->type){
		case T_STR:
			printf("%s\n", atom->str);
			break;
		case T_NUM:
			printf("%ld\n", atom->num);
			break;
		case T_NIL:
			printf("nil\n");
			break;
		case T_TRUE:
			printf("true\n");
			break;
		case T_FALSE:
			printf("false\n");
			break;
		default:
			warn("unsuppored atom type for print: %d", atom->type);
			break;
	}
	
	return nil_atom();
}

void register_buildins_in(env_t *env){
	env_set(env, "define", buildin_atom_alloc(buildin_define));
	env_set(env, "if", buildin_atom_alloc(buildin_if));
	env_set(env, "quote", buildin_atom_alloc(buildin_quote));
	env_set(env, "lambda", buildin_atom_alloc(buildin_lambda));
	env_set(env, "begin", buildin_atom_alloc(buildin_begin));
	env_set(env, "cons", buildin_atom_alloc(buildin_cons));
	env_set(env, "first", buildin_atom_alloc(buildin_first));
	env_set(env, "rest", buildin_atom_alloc(buildin_rest));
	
	env_set(env, "+", buildin_atom_alloc(buildin_plus));
	env_set(env, "-", buildin_atom_alloc(buildin_minus));
	env_set(env, "*", buildin_atom_alloc(buildin_multiply));
	env_set(env, "/", buildin_atom_alloc(buildin_divide));
	env_set(env, "=", buildin_atom_alloc(buildin_eqal));
	
	env_set(env, "mod_load", buildin_atom_alloc(buildin_mod_load));
	env_set(env, "env_self", buildin_atom_alloc(buildin_env_self));
	env_set(env, "env_new", buildin_atom_alloc(buildin_env_new));
	env_set(env, "env_get", buildin_atom_alloc(buildin_env_get));
	env_set(env, "env_set", buildin_atom_alloc(buildin_env_set));
	env_set(env, "print", buildin_atom_alloc(buildin_print));
}
