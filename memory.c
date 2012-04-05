#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "memory.h"

// Global singleton atoms
atom_t *allocator_nil_atom, *allocator_true_atom, *allocator_false_atom;

void memory_init(){
	allocator_nil_atom = alloc_atom(T_NIL);
	allocator_true_atom = alloc_atom(T_TRUE);
	allocator_false_atom = alloc_atom(T_FALSE);
}

atom_t* get_nil_atom(){
	return allocator_nil_atom;
}

atom_t* get_true_atom(){
	return allocator_true_atom;
}

atom_t* get_false_atom(){
	return allocator_false_atom;
}

atom_t* alloc_atom(uint8_t type){
	atom_t *ptr = malloc(sizeof(atom_t));
	ptr->type = type;
	return ptr;
}

atom_t* alloc_num(){
	return alloc_atom(T_NUM);
}

atom_t* alloc_sym(){
	return alloc_atom(T_SYM);
}

atom_t* alloc_str(){
	return alloc_atom(T_STR);
}

atom_t* alloc_pair(){
	return alloc_atom(T_PAIR);
}

atom_t* alloc_buildin(buildin_func_t func){
	atom_t *atom = alloc_atom(T_BUILDIN);
	atom->func = func;
	return atom;
}


//
// Environment stuff
//

env_t* alloc_env(env_t *parent){
	env_t *env = malloc(sizeof(env_t));
	env->length = 0;
	env->parent = parent;
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
	env->bindings = realloc(env->bindings, env->length * sizeof(env_binding_t));
	
	env->bindings[env->length-1] = (env_binding_t){
		.key = strdup(key),
		.value = value
	};
	
	return;
}
