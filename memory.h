#ifndef _MEMORY_H
#define _MEMORY_H

#include <stdint.h>
#include "bytecode.h"

//
// Atom and environment structures
//

typedef struct atom_s atom_t;
typedef struct env_s env_t;
typedef struct compiler_data *compiler_data_t;

typedef struct {
	size_t length;
	atom_t **atoms;
} atom_list_t;


#define SCOPE_STACK	0
#define SCOPE_HEAP	1
#define SCOPE_ENV		2
struct scope {
	struct scope *next;
	uint16_t arg_count;  // remembered per frame because of var args
	uint16_t type;  // if SCOPE_STACK fi is the stack index of the frame, if SCOPE_HEAP atoms is the pointer to the frame atoms
	union {
		size_t frame_index;
		atom_t **atoms;
		env_t *env;
	};
};
typedef struct scope scope_t, *scope_p;


struct compiler_data {
	size_t arg_count, var_count;
	char **names;
	// The number of frames this compiled lambda reaches outwards with its LOAD and STORE instructions
	size_t max_frame_offset;
	// The lambda we were defined in. Can be a normal lambda or a compiled lambda.
	atom_t *parent;
};


typedef atom_t* (*buildin_func_t)(atom_t *args, env_t *env);
typedef void (*compile_func_t)(atom_t *cl, atom_t *args, env_t *env);

struct atom_s {
	int64_t type;
	union {
		int64_t num;
		char *sym;
		char *str;
		struct {
			atom_t *first, *rest;
		};
		struct {
			buildin_func_t func;
			compile_func_t compile_func;
		};
		struct {
			atom_t *body;
			atom_t *args;
			env_t *env;
		};
		struct {
			bytecode_t bytecode;
			atom_list_t literal_table;
			compiler_data_t comp_data;
		};
		struct {
			atom_t *cl;
			scope_p scopes;
		};
		struct {
			uint64_t type;
			void *data;
			buildin_func_t func;
		} custom;
		struct {
			size_t fp_index;
			uint32_t ip_index;
			uint16_t arg_count;
			uint8_t padding;
			uint8_t scope_escaped;
			scope_p frame_scope;
		} interpreter_state;
	};
};

typedef struct {
	char *key;
	atom_t *value;
} env_binding_t;

struct env_s {
	env_t *parent;
	ssize_t length;
	union {
		env_binding_t *bindings;
		size_t fi;
	};
};


//
// Atom type constants
//

// Not meant to be used in actual atoms. It is supposed to be used as a list terminator for atom lists.
#define T_NULL 0

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
#define T_COMPILED_LAMBDA 14
#define T_RUNTIME_LAMBDA 15
#define T_ENV 16
#define T_INTERPRETER_STATE 17

#define T_CUSTOM 20

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
atom_t* buildin_atom_alloc(buildin_func_t func, compile_func_t compile_func);
atom_t* lambda_atom_alloc(atom_t *body, atom_t *args, env_t *env);
atom_t* compiled_lambda_atom_alloc(bytecode_t bytecode, atom_list_t literal_table, uint16_t arg_count, uint16_t var_count);
atom_t* runtime_lambda_atom_alloc(atom_t *compiled_lambda, scope_p scopes);
atom_t* env_atom_alloc(env_t *env);
atom_t* custom_atom_alloc(uint64_t type, void *data, buildin_func_t func);
atom_t* interpreter_state_atom_alloc(size_t fp_index, size_t ip_index, size_t arg_count, uint8_t scope_escaped, scope_p frame_scope);

// Convinience functions to alloc and initialize scope_t structures
scope_p scope_stack_alloc(scope_p next, uint16_t arg_count, size_t frame_index);
scope_p scope_heap_alloc(scope_p next, uint16_t arg_count, atom_t **frame);
scope_p scope_env_alloc(env_t *env);

// Environement functions
env_t* env_alloc(env_t *parent);
atom_t* env_get(env_t *env, char *key);
void env_set(env_t *env, char *key, atom_t *value);

#endif