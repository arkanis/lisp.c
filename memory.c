#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "memory.h"

// Global singleton atoms
atom_t *allocator_nil_atom, *allocator_true_atom, *allocator_false_atom;

// Private atom allocator function. Every atom comes from here.
atom_t* atom_alloc(uint8_t type){
	atom_t *ptr = gc_alloc(sizeof(atom_t));
	ptr->type = type;
	return ptr;
}


void memory_init(){
	gc_init();
	allocator_nil_atom = atom_alloc(T_NIL);
	allocator_true_atom = atom_alloc(T_TRUE);
	allocator_false_atom = atom_alloc(T_FALSE);
}


atom_t* nil_atom(){
	return allocator_nil_atom;
}

atom_t* true_atom(){
	return allocator_true_atom;
}

atom_t* false_atom(){
	return allocator_false_atom;
}


atom_t* num_atom_alloc(int64_t value){
	atom_t *atom = atom_alloc(T_NUM);
	atom->num = value;
	return atom;
}

atom_t* sym_atom_alloc(char *sym){
	atom_t *atom = atom_alloc(T_SYM);
	atom->sym = sym;
	return atom;
}

atom_t* str_atom_alloc(char *str){
	atom_t *atom = atom_alloc(T_STR);
	atom->str = str;
	return atom;
}

atom_t* pair_atom_alloc(atom_t *first, atom_t *rest){
	if (first == NULL || rest == NULL){
		warn("Tried to allocate a pair with a NULL pointer in it!");
		return nil_atom();
	}
	
	atom_t *atom = atom_alloc(T_PAIR);
	atom->first = first;
	atom->rest = rest;
	return atom;
}

atom_t* buildin_atom_alloc(buildin_func_t func, compile_func_t compile_func){
	atom_t *atom = atom_alloc(T_BUILDIN);
	atom->func = func;
	atom->compile_func = compile_func;
	return atom;
}

atom_t* lambda_atom_alloc(atom_t *args, atom_t *body, env_t *env){
	atom_t *atom = atom_alloc(T_LAMBDA);
	atom->body = body;
	atom->args = args;
	atom->env = env;
	return atom;
}

atom_t* compiled_lambda_atom_alloc(bytecode_t bytecode, atom_list_t literal_table, uint16_t arg_count, uint16_t var_count){
	atom_t *atom = atom_alloc(T_COMPILED_LAMBDA);
	atom->bytecode = bytecode;
	atom->literal_table = literal_table;
	
	atom->comp_data = gc_alloc(sizeof(struct compiler_data));
	atom->comp_data->arg_count = arg_count;
	atom->comp_data->var_count = var_count;
	atom->comp_data->names = NULL;
	atom->comp_data->max_frame_offset = 0;
	
	return atom;
}

atom_t* runtime_lambda_atom_alloc(atom_t *compiled_lambda, scope_p scopes){
	atom_t *atom = atom_alloc(T_RUNTIME_LAMBDA);
	atom->cl = compiled_lambda;
	atom->scopes = scopes;
	return atom;
}

atom_t* env_atom_alloc(env_t *env){
	atom_t *atom = atom_alloc(T_ENV);
	atom->env = env;
	return atom;
}

atom_t* custom_atom_alloc(uint64_t type, void *data, buildin_func_t func){
	atom_t *atom = atom_alloc(T_CUSTOM);
	atom->custom.type = type;
	atom->custom.data = data;
	atom->custom.func = func;
	return atom;
}

atom_t* interpreter_state_atom_alloc(size_t fp_index, size_t ip_index, size_t arg_count, uint8_t scope_escaped, scope_p frame_scope){
	atom_t *atom = atom_alloc(T_INTERPRETER_STATE);
	atom->interpreter_state.fp_index = fp_index;
	atom->interpreter_state.ip_index = ip_index;
	atom->interpreter_state.arg_count = arg_count;
	atom->interpreter_state.padding = 0;
	atom->interpreter_state.scope_escaped = scope_escaped;
	atom->interpreter_state.frame_scope = frame_scope;
	return atom;
}


//
// Scope stuff
//

scope_p scope_stack_alloc(scope_p next, uint16_t arg_count, size_t frame_index){
	scope_p scope = gc_alloc(sizeof(scope_t));
	*scope = (scope_t){
		.next = next,
		.arg_count = arg_count,
		.type = SCOPE_STACK,
		.frame_index = frame_index
	};
	return scope;
}

scope_p scope_heap_alloc(scope_p next, uint16_t arg_count, atom_t **frame){
	scope_p scope = gc_alloc(sizeof(scope_t));
	*scope = (scope_t){
		.next = next,
		.arg_count = arg_count,
		.type = SCOPE_HEAP,
		.atoms = frame
	};
	return scope;
}

scope_p scope_env_alloc(env_t *env){
	scope_p scope = gc_alloc(sizeof(scope_t));
	*scope = (scope_t){
		.next = NULL,
		.arg_count = 0,
		.type = SCOPE_ENV,
		.env = env
	};
	return scope;
}


//
// Environment stuff
//

env_t* env_alloc(env_t *parent){
	env_t *env = gc_alloc(sizeof(env_t));
	env->length = 0;
	env->parent = parent;
	env->bindings = NULL;
	return env;
}

atom_t* env_get(env_t *env, char *key){
	if (env == NULL)
		return NULL;
	
	for(int i = 0; i < env->length; i++){
		if ( strcmp(env->bindings[i].key, key) == 0 )
			return env->bindings[i].value;
	}
	
	return env_get(env->parent, key);
}

void env_set(env_t *env, char *key, atom_t *value){
	if (env == NULL){
		warn("Got NULL pointer as environment");
		return;
	}
	
	for(int i = 0; i < env->length; i++){
		if ( strcmp(env->bindings[i].key, key) == 0 ) {
			env->bindings[i].value = value;
			return;
		}
	}
	
	env->length++;
	env->bindings = gc_realloc(env->bindings, env->length * sizeof(env_binding_t));
	
	env->bindings[env->length-1] = (env_binding_t){
		.key = strdup(key),
		.value = value
	};
	
	return;
}