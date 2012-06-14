#ifndef _BYTECODE_H
#define _BYTECODE_H


#include <stdint.h>
#include <sys/types.h>

typedef struct {
	uint8_t op;
	uint8_t padding;
	uint16_t offset;
	uint32_t index;
} instruction_t;

typedef struct {
	size_t length;
	instruction_t *code;
} bytecode_t;

// This is not an actual bytecode but assumed to never occur in a bytecode array.
// It is used by the tests to terminate the assumed bytecodes. Just make sure there
// is no instruction with that code and an argument is unlikely to contain that value.
#define BC_NULL			INT64_MIN

#define BC_PUSH_NIL		1
#define BC_PUSH_TRUE		2
#define BC_PUSH_FALSE		3

#define BC_PUSH_NUM		4
#define BC_PUSH_LITERAL	5
#define BC_PUSH_ARG		6
#define BC_PUSH_FROM_ENV	7

#define BC_DROP			8

#define BC_CALL			9
#define BC_RETURN			10

#define BC_JUMP			11
#define BC_JUMP_IF_FALSE	12

#endif