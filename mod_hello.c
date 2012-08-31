#include <stdio.h>
#include "memory.h"

static atom_t* test(atom_t *args, env_t *env){
	printf("hello from shared object!\n");
	return nil_atom();
}

int init(env_t *env){
	env_def(env, "test", buildin_atom_alloc(test, NULL));
	return 0;
}