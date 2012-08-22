#ifndef _TEST_BYTECODE_UTILS_H
#define _TEST_BYTECODE_UTILS_H

#include "../memory.h"
#include "../bytecode.h"

bool test_instruction(instruction_t subject, instruction_t expected, size_t idx, char *msg);
bool test_atom(atom_t *subject, atom_t expected, size_t idx, char *msg);

#endif