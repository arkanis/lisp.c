#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "buildins.h"
#include "eval.h"
#include "bytecode_compiler.h"
#include "bytecode_generator.h"
#include "logger.h"

//
// Language buildins
//

atom_t* buildin_define(atom_t *args, env_t *env){
	if (args->first->type != T_SYM || args->rest->type != T_PAIR || args->rest->rest->type != T_NIL)
		return warn("define requires two arguments and the first one has to be a symbol"), nil_atom();
	
	atom_t *name_atom = args->first;
	atom_t *value_atom = eval_atom(args->rest->first, env);
	env_def(env, name_atom->sym, value_atom);
	return value_atom;
}

void compile_define(atom_t *cl, atom_t *args, env_t *env){
	if (args->first->type != T_SYM || args->rest->type != T_PAIR || args->rest->rest->type != T_NIL){
		warn("define requires two arguments and the first one has to be a symbol");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	// Add the name to the variable name list
	atom_t *name_atom = args->first;
	cl->comp_data->var_count++;
	size_t names_length = cl->comp_data->arg_count + cl->comp_data->var_count;
	cl->comp_data->names = gc_realloc(cl->comp_data->names, names_length * sizeof(cl->comp_data->names[0]));
	cl->comp_data->names[names_length-1] = name_atom->sym;
	
	// Compile value expr and store the initial value afterwards
	bcc_compile_expr(cl, args->rest->first, env);
	bcg_gen(&cl->bytecode, (instruction_t){BC_SAVE_VAR, .index = cl->comp_data->var_count - 1, .offset = 0});
}


atom_t* set_eval(atom_t *args, env_t *env){
	if (args->first->type != T_SYM || args->rest->type != T_PAIR || args->rest->rest->type != T_NIL)
		return warn("set requires two arguments and the first one has to be a symbol"), nil_atom();
	
	atom_t *name_atom = args->first;
	atom_t *value_atom = eval_atom(args->rest->first, env);
	env_set(env, name_atom->sym, value_atom);
	return value_atom;
}

void set_compile(atom_t *cl, atom_t *args, env_t *env){
	if (args->first->type != T_SYM || args->rest->type != T_PAIR || args->rest->rest->type != T_NIL){
		warn("set requires two arguments and the first one has to be a symbol");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	atom_t *name_atom = args->first;
	atom_t *value_expr = args->rest->first;
	
	// Compile the value expr
	bcc_compile_expr(cl, value_expr, env);
	
	// Search for the name in the variable lists
	ssize_t idx;
	size_t scope_offset = 0;
	atom_t *current_cl = cl;
	do {
		if ( (idx = bcc_symbol_in_names(current_cl, name_atom)) != -1 ) {
			// symbol is known in current scope (lambda)
			if (idx < current_cl->comp_data->arg_count) {
				// symbol identifies an argument, can't set args!
				warn("set: can not change the value of arguments!");
			} else {
				// symbol identifies a local variable, generate a save-var instruction
				idx = idx - current_cl->comp_data->arg_count;
				bcg_gen(&cl->bytecode, (instruction_t){BC_SAVE_VAR, .index = idx, .offset = scope_offset});
			}
			break;
		}
		scope_offset++;
		current_cl = current_cl->comp_data->parent;
	} while(current_cl);
	
	if (current_cl == NULL){
		// Symbol not found in any argument or variable lists of all parents
		warn("set: can't find variable %s!", name_atom->sym);
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
	}
}


atom_t* buildin_if(atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_PAIR || args->rest->rest->rest->type != T_NIL)
		return warn("if requires exactly three arguments"), nil_atom();
	
	atom_t *cond = eval_atom(args->first, env);
	if (cond->type == T_TRUE)
		return eval_atom(args->rest->first, env);
	else
		return eval_atom(args->rest->rest->first, env);
}

void compile_if(atom_t *cl, atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_PAIR || args->rest->rest->rest->type != T_NIL){
		warn("if requires exactly three arguments");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	// compile condition
	bcc_compile_expr(cl, args->first, env);
	size_t false_offset = bcg_gen(&cl->bytecode, (instruction_t){BC_JUMP_IF_FALSE, .jump_offset = 0});
	
	// compile true case
	bcc_compile_expr(cl, args->rest->first, env);
	size_t end_offset = bcg_gen(&cl->bytecode, (instruction_t){BC_JUMP, .jump_offset = 0});
	bcg_backpatch_target_in(&cl->bytecode, false_offset);
	
	// compile false case
	bcc_compile_expr(cl, args->rest->rest->first, env);
	bcg_backpatch_target_in(&cl->bytecode, end_offset);
}


atom_t* buildin_quote(atom_t *args, env_t *env){
	if (args->rest->type != T_NIL)
		return warn("quote takes exactly one argument"), nil_atom();
	return args->first;
}

void compile_quote(atom_t *cl, atom_t *args, env_t *env){
	if (args->rest->type != T_NIL){
		warn("quote takes exactly one argument");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	size_t idx = bcc_add_atom_to_literal_table(cl, args->first);
	bcg_gen(&cl->bytecode, (instruction_t){BC_PUSH_LITERAL, .index = idx, .offset = 0});
}


atom_t* buildin_begin(atom_t *args, env_t *env){
	atom_t *result = nil_atom();
	for(atom_t *pair = args; pair->type == T_PAIR; pair = pair->rest)
		result = eval_atom(pair->first, env);
	return result;
}

void compile_begin(atom_t *cl, atom_t *args, env_t *env){
	if (args->type != T_PAIR){
		warn("begin needs at least one expr to compile");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	// Compile first expr
	bcc_compile_expr(cl, args->first, env);
	
	// If there are other ones drop the prev stack value and compile the next expr
	for(atom_t *pair = args->rest; pair->type == T_PAIR; pair = pair->rest){
		bcg_gen_op(&cl->bytecode, BC_DROP);
		bcc_compile_expr(cl, pair->first, env);
	}
}


atom_t* buildin_lambda(atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR)
		return warn("lambda needs at least two arguments (arg list and body)"), nil_atom();
	atom_t *arg_names = args->first;
	atom_t *body = args->rest;
	
	if (body->rest->type == T_NIL) {
		// If we only have one expression in the body discard the trailing nil from the argument list.
		// A lambda can only contain one expression so there is no need for a terminator nil.
		body = body->first;
	} else {
		// If we got multiple expressions wrap them into a begin() call. The terminator nil from the
		// argument list is reused as the terminator nil of the arguments to begin().
		body = pair_atom_alloc(sym_atom_alloc("begin"), body);
	}
	
	// Only try to compile the lambda if `__compile_lambdas` is set to true
	if ( env_get(env, "__compile_lambdas") == true_atom() ){
		// If bcc_compile_to_lambda() returns NULL the compilation failed and we will use a normal AST
		// based lambda instead. As parent compiled lambda we just use NULL since we came from the
		// eval tree interpreter. Therefore there is no outer compiled lambda we know about at compile time.
		atom_t *compiled_lambda = bcc_compile_to_lambda(arg_names, body, env, NULL);
		// Instanciate the compiled lambda and add an env scope to it. This is the root scope for all nested lambdas.
		if (compiled_lambda != NULL)
			return runtime_lambda_atom_alloc(compiled_lambda, scope_env_alloc(env));
	}
	
	return lambda_atom_alloc(arg_names, body, env);
}

void compile_lambda(atom_t *cl, atom_t *args, env_t *env){
	if (args->type != T_PAIR){
		warn("lambda needs at least two arguments (arg list and body)");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	atom_t *arg_names = args->first;
	atom_t *body = args->rest;
	
	if (body->rest->type == T_NIL) {
		// If we only have one expression in the body discard the trailing nil from the argument list.
		// A lambda can only contain one expression so there is no need for a terminator nil.
		body = body->first;
	} else {
		// If we got multiple expressions wrap them into a begin() call. The terminator nil from the
		// argument list is reused as the terminator nil of the arguments to begin().
		body = pair_atom_alloc(sym_atom_alloc("begin"), body);
	}
	
	atom_t *child_cl = bcc_compile_to_lambda(arg_names, body, env, cl);
	size_t literal_idx = bcc_add_atom_to_literal_table(cl, child_cl);
	bcg_gen(&cl->bytecode, (instruction_t){BC_LAMBDA, .index = literal_idx, .offset = 0});
}


//
// Pair handling
//

atom_t* buildin_cons(atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_NIL)
		return warn("cons needs exactly two arguments to build a pair"), nil_atom();
	return pair_atom_alloc(eval_atom(args->first, env), eval_atom(args->rest->first, env));
}

void compile_cons(atom_t *cl, atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_NIL){
		warn("cons needs exactly two arguments to build a pair");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	bcc_compile_expr(cl, args->first, env);
	bcc_compile_expr(cl, args->rest->first, env);
	bcg_gen(&cl->bytecode, (instruction_t){BC_CONS});
}


atom_t* buildin_first(atom_t *args, env_t *env){
	if (args->rest->type != T_NIL)
		return warn("first requires exactly one argument"), nil_atom();	
	
	atom_t *pair_atom = eval_atom(args->first, env);
	if (pair_atom->type != T_PAIR)
		return warn("first: the argument have to eval to a pair"), nil_atom();
	
	return pair_atom->first;
}

void compile_first(atom_t *cl, atom_t *args, env_t *env){
	if (args->rest->type != T_NIL){
		warn("first requires exactly one argument");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	bcc_compile_expr(cl, args->first, env);
	bcg_gen(&cl->bytecode, (instruction_t){BC_FIRST});
}


atom_t* buildin_rest(atom_t *args, env_t *env){
	if (args->rest->type != T_NIL)
		return warn("rest requires exactly one argument"), nil_atom();
	
	atom_t *pair_atom = eval_atom(args->first, env);
	if (pair_atom->type != T_PAIR)
		return warn("rest: the argument have to eval to a pair"), nil_atom();
	
	return pair_atom->rest;
}

void compile_rest(atom_t *cl, atom_t *args, env_t *env){
	if (args->rest->type != T_NIL){
		warn("rest requires exactly one argument");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	bcc_compile_expr(cl, args->first, env);
	bcg_gen(&cl->bytecode, (instruction_t){BC_REST});
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

void compile_plus(atom_t *cl, atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_NIL){
		warn("plus requires two arguments");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	bcc_compile_expr(cl, args->first, env);
	bcc_compile_expr(cl, args->rest->first, env);
	bcg_gen_op(&cl->bytecode, BC_ADD);
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

void compile_minus(atom_t *cl, atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_NIL){
		warn("minus requires two arguments");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	bcc_compile_expr(cl, args->first, env);
	bcc_compile_expr(cl, args->rest->first, env);
	bcg_gen_op(&cl->bytecode, BC_SUB);
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

void compile_multiply(atom_t *cl, atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_NIL){
		warn("multiply requires two arguments");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	bcc_compile_expr(cl, args->first, env);
	bcc_compile_expr(cl, args->rest->first, env);
	bcg_gen_op(&cl->bytecode, BC_MUL);
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

void compile_divide(atom_t *cl, atom_t *args, env_t *env){
	if (args->rest->type != T_PAIR || args->rest->rest->type != T_NIL){
		warn("divide requires two arguments");
		bcg_gen_op(&cl->bytecode, BC_PUSH_NIL);
		return;
	}
	
	bcc_compile_expr(cl, args->first, env);
	bcc_compile_expr(cl, args->rest->first, env);
	bcg_gen_op(&cl->bytecode, BC_DIV);
}


//
// Comperators
//

atom_t* buildin_equal(atom_t *args, env_t *env){
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

void compile_equal(atom_t *cl, atom_t *args, env_t *env){
	bcc_compile_expr(cl, args->first, env);
	bcc_compile_expr(cl, args->rest->first, env);
	bcg_gen_op(&cl->bytecode, BC_EQ);
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

atom_t* gc_heap_size_eval(atom_t *args, env_t *env){
	return num_atom_alloc(gc_heap_size());
}


void register_buildins_in(env_t *env){
	env_def(env, "define", buildin_atom_alloc(buildin_define, compile_define));
	env_def(env, "set!", buildin_atom_alloc(set_eval, set_compile));
	
	env_def(env, "if", buildin_atom_alloc(buildin_if, compile_if));
	env_def(env, "quote", buildin_atom_alloc(buildin_quote, compile_quote));
	env_def(env, "begin", buildin_atom_alloc(buildin_begin, compile_begin));
	env_def(env, "lambda", buildin_atom_alloc(buildin_lambda, compile_lambda));
	
	env_def(env, "cons", buildin_atom_alloc(buildin_cons, compile_cons));
	env_def(env, "first", buildin_atom_alloc(buildin_first, compile_first));
	env_def(env, "rest", buildin_atom_alloc(buildin_rest, compile_rest));
	
	env_def(env, "+", buildin_atom_alloc(buildin_plus, compile_plus));
	env_def(env, "-", buildin_atom_alloc(buildin_minus, compile_minus));
	env_def(env, "*", buildin_atom_alloc(buildin_multiply, compile_multiply));
	env_def(env, "/", buildin_atom_alloc(buildin_divide, compile_divide));
	
	env_def(env, "=", buildin_atom_alloc(buildin_equal, compile_equal));
	
	env_def(env, "mod_load", buildin_atom_alloc(buildin_mod_load, NULL));
	env_def(env, "env_self", buildin_atom_alloc(buildin_env_self, NULL));
	env_def(env, "env_new", buildin_atom_alloc(buildin_env_new, NULL));
	env_def(env, "env_get", buildin_atom_alloc(buildin_env_get, NULL));
	env_def(env, "env_set", buildin_atom_alloc(buildin_env_set, NULL));
	
	env_def(env, "print", buildin_atom_alloc(buildin_print, NULL));
	env_def(env, "gc_heap_size", buildin_atom_alloc(gc_heap_size_eval, NULL));
}
