#include <stdbool.h>
#include <string.h>

#include "../logger.h"
#include "test_utils.h"
#include "test_bytecode_utils.h"

bool test_instruction(instruction_t subject, instruction_t expected, size_t idx, char *msg){
	bool success;
	success = test(subject.op == expected.op, "%s %zu got wrong op, expected %d, got %d",
			msg, idx, expected.op, subject.op);
	if (!success)
		return false;
	
	if (subject.op == BC_PUSH_NUM)
		return test(subject.num == expected.num, "%s %zu got wrong num, expected %d, got %d",
			msg, idx, expected.num, subject.num);
	else if (subject.op == BC_JUMP || subject.op == BC_JUMP_IF_FALSE)
		return test(subject.jump_offset == expected.jump_offset, "%s %zu got wrong offset, expected %d, got %d",
			msg, idx, expected.jump_offset, subject.jump_offset);
	else if (subject.op == BC_PUSH_LITERAL || subject.op == BC_PUSH_ARG)
		return test(subject.offset == expected.offset && subject.index == expected.index,
			"%s %zu got wrong offset, expected %d, got %d", msg, idx, expected.jump_offset, subject.jump_offset);
	else if (subject.op == BC_CALL)
		return test(subject.num == expected.num,
			"%s %zu got wrong num, expected %d, got %d", msg, idx, expected.num, subject.num);
	
	return true;
}

bool test_atom(atom_t *subject, atom_t expected, size_t idx, char *msg){
	bool success;
	
	success = test(subject->type == expected.type, "%s %zu got wrong type, expected %d, got %d",
		msg, idx, expected.type, subject->type);
	if (!success)
		return false;
	
	switch (subject->type) {
		case T_NIL: case T_TRUE: case T_FALSE:
			return true;
		case T_NUM:
			return test(subject->num == expected.num, "%s %zu got wrong number value, expected %d, got %d",
				msg, idx, expected.num, subject->num);
		case T_STR:
			return test(strcmp(subject->str, expected.str) == 0, "%s %zu got wrong string value, expected %s, got %s",
				msg, idx, expected.str, subject->str);
		case T_SYM:
			return test(strcmp(subject->str, expected.str) == 0, "%s %zu got wrong string value, expected %s, got %s",
				msg, idx, expected.sym, subject->sym);
		case T_PAIR:
			return test_atom(subject->first, *expected.first, idx, msg) && test_atom(subject->rest, *expected.rest, idx, msg);
	};
	
	warn("Don't know how to compare atom type %d", subject->type);
	return true;
}