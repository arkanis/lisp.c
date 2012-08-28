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
	int16_t offset;
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

/**
 * This is not an actual bytecode but assumed to never occur in a bytecode array.
 * It is used by the tests to terminate the assumed bytecodes. Just make sure there
 * is no instruction with that code and an argument is unlikely to contain that value.
 */
#define BC_NULL			0

/**
 * These instructions push the corresponding singleton atom on the stack.
 * Instruction properties used: none
 */
#define BC_PUSH_NIL		1
#define BC_PUSH_TRUE		2
#define BC_PUSH_FALSE		3

/**
 * Pushes a numer on top of the stack. The value is stored as part of the instruction.
 * Instruction properties used:
 * 	num
 */
#define BC_PUSH_NUM		4

/**
 * Pushes an atom from the literal table on top of the stack. Used for all kinds of atoms
 * and larger numbers that don't fit into the instructions num property.
 * Instruction properties used:
 * 	frame_offset (number of parents to got up for the literal table)
 * 	index (entry of the literal table to push on the stack)
 */
#define BC_PUSH_LITERAL	5

/**
 * Pushes an argument of a compiled lambda on top of the stack.
 * Instruction properties used:
 * 	frame_offset (number of frames to got up)
 * 	index (index of the argument)
 */
#define BC_PUSH_ARG		6

/**
 * Pushes the value of a local variable on top of the stack.
 * Instruction properties used:
 * 	frame_offset (number of frames to got up)
 * 	index (index of the local variable)
 */
#define BC_PUSH_VAR		7

/**
 * Pops the top of the stack and saves this atom in a local variable.
 * Instruction properties used:
 * 	frame_offset (number of frames to got up)
 * 	index (index of the local variable)
 */
#define BC_SAVE_VAR		9

/**
 * Pops a symbol atom from the top of the stack. Then searches for a matching entry in the
 * outer definition environment and pushes the found atom on the stack.
 * Instruction properties used: none
 * 
 * TODO: actually implement this in the bytecode interpreter
 * TODO: possible optimization: use frame_offset and index to address a literal. should cover
 * 	cases where the symbol name is not calculated at run time (majority of cases?). How to
 * 	encode that the properties should not be used? extra instruction? or negative frame_offset?
 */
#define BC_PUSH_FROM_ENV	8

#define BC_SAVE_ENV		10

#define BC_DROP			11

/**
 * Looks up a compiled lambda in the literal table and creates a runtime lambda for it. This
 * associates a compiled lambda (static compiletime stuff) with a reference to the living stacks
 * that are needed for proper argument and local lookups. The result can be seen as an living
 * instance of the compiled lambda. The runtime lambda is pushed on top of the stack.
 * 
 * Instruction properties used:
 * 	frame_offset (number of parents to got up for the literal table)
 * 	index (entry of the literal table to push on the stack)
 */
#define BC_LAMBDA			30

/**
 * Uses the num property as the number of arguments that are pushed on the stack.
 */
#define BC_CALL			12
#define BC_RETURN			13

#define BC_JUMP			14
// Pops the current value of the stack and checks if it is the false atom
#define BC_JUMP_IF_FALSE	15

#define BC_ADD			16
#define BC_SUB			17
#define BC_MUL			18
#define BC_DIV			19

#define BC_EQ				20

// TODO
#define BC_LT				21
#define BC_GT				22

#define BC_AND			23
#define BC_OR				24
#define BC_NOT			25
#define BC_XOR			26

#endif