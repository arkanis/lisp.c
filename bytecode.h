#ifndef _BYTECODE_H
#define _BYTECODE_H

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