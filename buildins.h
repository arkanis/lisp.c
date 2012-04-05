#ifndef _BUILDINS_H
#define _BUILDINS_H

#include "memory.h"

void register_buildins_in(env_t *env);
atom_t* buildin_define(atom_t *args, env_t *env);
atom_t* buildin_plus(atom_t *args, env_t *env);

#endif