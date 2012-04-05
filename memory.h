#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdint.h>

//
// Atom structure
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
			struct atom_s *first, *rest;
		} pair;
		struct {
			buildin_func_t func;
		};
	};
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

#define T_BUILDIN 20


void memory_init();

// Getters for the singelton atoms
atom_t* get_nil_atom();
atom_t* get_true_atom();
atom_t* get_false_atom();

// Normal atoms
atom_t* alloc_atom(uint8_t type);
atom_t* alloc_num();
atom_t* alloc_sym();
atom_t* alloc_str();
atom_t* alloc_pair();
atom_t* alloc_buildin(buildin_func_t func);


//
// Environment structures
//

typedef struct {
	char *key;
	atom_t *value;
} env_binding_t;

struct env_s {
	unsigned int length;
	struct env_s *parent;
	env_binding_t *bindings;
};

env_t* alloc_env(env_t *parent);
atom_t* env_get(env_t *env, char *key);
void env_set(env_t *env, char *key, atom_t *value);

#endif