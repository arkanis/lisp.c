#include <stdio.h>
#include "memory.h"

static atom_t *test(atom_t *args, env_t *env){
	printf("test run\n");
	return nil_atom();
}

int init(env_t *env){
	env_set(env, "test", buildin_atom_alloc(test));
	return 0;
}