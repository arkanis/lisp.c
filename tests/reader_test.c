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
	
	atom = read_test_code("+");
	test(atom->type == T_SYM && strcmp(atom->sym, "+") == 0, "got type: %d, sym: %s", atom->type, atom->sym);
	
	atom = read_test_code("\"str\"");
	test(atom->type == T_STR && strcmp(atom->str, "str") == 0, "got type: %d, str: %s", atom->type, atom->str);
	
	atom = read_test_code("nil");
	test(atom->type == T_NIL && atom == nil_atom(), "got type: %d, atom: %p, nil atom: %p", atom->type, atom, nil_atom());
	
	atom = read_test_code("true");
	test(atom->type == T_TRUE && atom == true_atom(), "got type: %d, atom: %p, true atom: %p", atom->type, atom, true_atom());
	
	atom = read_test_code("false");
	test(atom->type == T_FALSE && atom == false_atom(), "got type: %d, atom: %p, false atom: %p", atom->type, atom, false_atom());
	
	atom = read_test_code("()");
	test(atom->type == T_NIL, "failed to read an empty list, got type %d", atom->type);
	
	atom = read_test_code("(() ())");
	test(atom->type == T_PAIR && atom->first->type == T_NIL
		&& atom->rest->type == T_PAIR && atom->rest->first->type == T_NIL
		&& atom->rest->rest->type == T_NIL
	, "failed to read two empty lists");
	
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
	
	atom = read_test_code("(symbol)");
	test(atom->type == T_PAIR, "expected a pair, got type %d", atom->type);
	test(atom->first->type == T_SYM, "expected a symbol as first element, got type %d", atom->first->type);
	test(strcmp(atom->first->sym, "symbol") == 0, "unexpected symbol value: %s", atom->first->sym);
	test(atom->rest->type == T_NIL, "expected a nil terminator in the rest, got type %d", atom->rest->type);
}

void test_quoting(){
	atom_t *atom = read_test_code("'foo");
	test(atom->type == T_PAIR, "expected a pair with quote in it, got type: %d", atom->type);
	test(atom->first->type == T_SYM, "expected the quote symbol, got type: %d", atom->first->type);
	test(strcmp(atom->first->sym, "quote") == 0, "expected the quote symbol, got symbol %s", atom->first->sym);
	test(atom->rest->type == T_PAIR, "expected the argument list pair for the quote, got type: %d", atom->rest->type);
	test(atom->rest->first->type == T_SYM, "expected the foo symbol as quote argument, got type: %d", atom->rest->first->type);
	test(strcmp(atom->rest->first->sym, "foo") == 0, "expected the foo symbol as quote argument, got symbol: %s", atom->rest->first->sym);
	test(atom->rest->rest->type == T_NIL, "expected the nil terminator, got type: %d", atom->rest->rest->type);
}


int main(){
	// Important for singleton atoms (nil, true, false). Otherwise we got NULL pointers there...
	memory_init();
	
	test_reader();
	test_quoting();
	return show_test_report();
}