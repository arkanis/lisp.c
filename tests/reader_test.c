#include <stdio.h>
#include <string.h>

#include "test_utils.h"
#include "../scanner.h"
#include "../reader.h"
#include "../memory.h"

atom_t* read_test_code(char *code){
	atom_t *atom;
	
	scanner_t scan = scan_open_string(code);
	atom = read_atom(&scan);
	scan_close(&scan);
	
	return atom;
}

void test_reader(){
	atom_t *atom = NULL;
	
	atom = read_test_code("123");
	test(atom->type == T_NUM && atom->num == 123, "got type: %d, num: %ld", atom->type, atom->num);
	
	atom = read_test_code("sym");
	test(atom->type == T_SYM && strcmp(atom->sym, "sym") == 0, "got type: %d, sym: %s", atom->type, atom->sym);
	
	atom = read_test_code("\"str\"");
	test(atom->type == T_STR && strcmp(atom->str, "str") == 0, "got type: %d, str: %s", atom->type, atom->str);
	
	atom = read_test_code("nil");
	test(atom->type == T_NIL && atom == nil_atom(), "got type: %d, atom: %p, nil atom: %p", atom->type, atom, nil_atom());
	
	atom = read_test_code("true");
	test(atom->type == T_TRUE && atom == true_atom(), "got type: %d, atom: %p, true atom: %p", atom->type, atom, true_atom());
	
	atom = read_test_code("false");
	test(atom->type == T_FALSE && atom == false_atom(), "got type: %d, atom: %p, false atom: %p", atom->type, atom, false_atom());
	
	atom = read_test_code("(1)");
	test(atom->type == T_PAIR
		&& atom->first->type == T_NUM && atom->first->num == 1
		&& atom->rest->type == T_NIL
	, "failed to read a list with one entry");
	
	atom = read_test_code("(1 . 2)");
	test(atom->type == T_PAIR
		&& atom->first->type == T_NUM && atom->first->num == 1
		&& atom->rest->type == T_NUM && atom->rest->num == 2
	, "failed to read a not nil terminated list");
	
	atom = read_test_code("(1 2 3)");
	test(atom->type == T_PAIR && atom->first->type == T_NUM && atom->first->num == 1, "failed to read a list with tree elements (1. element)");
	atom = atom->rest;
	test(atom->type == T_PAIR && atom->first->type == T_NUM && atom->first->num == 2, "failed to read a list with tree elements (2. element)");
	atom = atom->rest;
	test(atom->type == T_PAIR && atom->first->type == T_NUM && atom->first->num == 3, "failed to read a list with tree elements (3. element)");
	atom = atom->rest;
	test(atom->type == T_NIL, "failed to read a list with tree elements (nil terminator)");
}


int main(){
	// Important for singleton atoms (nil, true, false). Otherwise we got NULL pointers there...
	memory_init();
	
	test_reader();
	return show_test_report();
}