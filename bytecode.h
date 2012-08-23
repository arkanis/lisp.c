#ifndef _BYTECODE_H
#define _BYTECODE_H


#include <stdint.h>
#include <stddef.h>

typedef struct {
	uint8_t op;
	// Don't know how to do an easy 24bit int for the offset. We would use a union over
	// the op and a bitshift each time to access the offset.
	uint8_t padding;
	// Used for the number of frames we have to go up to find a literal
	int16_t frame_offset;
	union {
		// Used as index to retrieve atoms from the literal table, index of local variables
		// and arguments, etc.
		uint32_t index;
		// Used to store 32bit numbers directly in the instruction
		int32_t num;
		// Jump offset of jump instructions. This is signed in case we want to implement
		// something like loops that need to jump backwards in the insturction stream
		// (negative offset).
		int32_t jump_offset;
	};
} instruction_t;

typedef struct {
	size_t length;
	instruction_t *code;
} bytecode_t;

// This is not an actual bytecode but assumed to never occur in a bytecode array.
// It is used by the tests to terminate the assumed bytecodes. Just make sure there
// is no instruction with that code and an argument is unlikely to contain that value.
#define BC_NULL			0

#define BC_PUSH_NIL		1
#define BC_PUSH_TRUE		2
#define BC_PUSH_FALSE		3

// Only for numbers that fit into an int32_t type, the rest is done via
// BC_PUSH_LITERAL and an entry in the literal table
#define BC_PUSH_NUM		4
#define BC_PUSH_LITERAL	5
#define BC_PUSH_ARG		6
#define BC_PUSH_FROM_ENV	7

#define BC_DROP			8

#define BC_CALL			9
#define BC_RETURN			10

#define BC_JUMP			11
// Pops the current value of the stack and checks if it is the false atom
#define BC_JUMP_IF_FALSE	12

#define BC_ADD			13
#define BC_SUB			14
#define BC_MUL			15
#define BC_DIV			16

#define BC_EQ				17

#endif