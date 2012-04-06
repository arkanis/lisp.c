#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdint.h>

//
// Atom and environment structures
//

typedef struct atom_s atom_t;
typedef struct env_s env_t;

typedef atom_t* (*buildin_func_t)(atom_t *args, env_t *env);

struct atom_s {
	uint8_t type;
	union {
		int64_t num;
		char *sym;
		char *str;
		struct {
			atom_t *first, *rest;
		};
		struct {
			buildin_func_t func;
		};
		struct {
			atom_t *body;
			atom_t *args;
			env_t *env;
		};
	};
};

typedef struct {
	char *key;
	atom_t *value;
} env_binding_t;

struct env_s {
	unsigned int length;
	env_t *parent;
	env_binding_t *bindings;
};


//
// Atom type constants
//

// Meant for atoms that are not really ready yet (not used right now)
#define T_UNINITIALIZED 0

// Atoms that eval to themselfs. Albeit some of these are singletons the type should be used
// for comparisons.
#define T_NUM 1
#define T_STR 2
#define T_NIL 3
#define T_TRUE 4
#define T_FALSE 5

// Atoms with complex eval behaviour. T_COMPLEX_ATOM is used to distinguish simple from
// complex atoms, it isn't a type in itself.
#define T_COMPLEX_ATOM 10
#define T_SYM 10
#define T_PAIR 11
#define T_BUILDIN 12
#define T_LAMBDA 13


//
// Functions
//

void memory_init();

// Functions that return singleton atoms
atom_t* nil_atom();
atom_t* true_atom();
atom_t* false_atom();

// Atom allocator values that already get the content
atom_t* num_atom_alloc(int64_t value);
atom_t* sym_atom_alloc(char *sym);
atom_t* str_atom_alloc(char *str);
atom_t* pair_atom_alloc(atom_t *first, atom_t *rest);
atom_t* buildin_atom_alloc(buildin_func_t func);
atom_t* lambda_atom_alloc(atom_t *body, atom_t *args, env_t *env);

// Environement functions
env_t* env_alloc(env_t *parent);
atom_t* env_get(env_t *env, char *key);
void env_set(env_t *env, char *key, atom_t *value);

#endif