#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "memory.h"
#include "reader.h"
#include "logger.h"

atom_t* read_sym(scanner_t *scan);
atom_t* read_list(scanner_t *scan);

atom_t* read_atom(scanner_t *scan){
	slice_t slice;
	int c = scan_while_func(scan, NULL, isspace);
	
	while (c == ';'){
		scan_until(scan, NULL, '\n');
		c = scan_peek(scan);
	}
	
	if (c == EOF) {
		return nil_atom();
	} else if (c == '(') {
		return read_list(scan);
	} else if (c == '\'') {
		// Quote
		scan_one_of(scan, '\'');
		atom_t *content = read_atom(scan);
		return pair_atom_alloc(sym_atom_alloc("quote"), pair_atom_alloc(content, nil_atom()));
	} else if (c == '"') {
		// String
		scan_one_of(scan, '"');
		scan_until(scan, &slice, '"');
		return str_atom_alloc(slice.ptr);
	} else if ( isdigit(c) ) {
		// Number
		scan_while_func(scan, &slice, isdigit);
		int64_t value = strtoll(slice.ptr, NULL, 10);
		free(slice.ptr);
		return num_atom_alloc(value);
	} else {
		return read_sym(scan);
	}
}

atom_t* read_list(scanner_t *scan){
	int c;
	atom_t* list_start_atom = pair_atom_alloc( nil_atom(), nil_atom() );
	atom_t* current_atom = list_start_atom;
	
	scan_one_of(scan, '(');
	while (true) {
		current_atom->first = read_atom(scan);
		
		c = scan_while_func(scan, NULL, isspace);
		if (c == ')') {
			scan_one_of(scan, ')');
			current_atom->rest = nil_atom();
			break;
		} else if (c == '.') {
			scan_one_of(scan, '.');
			current_atom->rest = read_atom(scan);
			scan_while_func(scan, NULL, isspace);
			scan_one_of(scan, ')');
			break;
		} else if (c == EOF) {
			return nil_atom();
		}
		
		current_atom->rest = pair_atom_alloc( nil_atom(), nil_atom() );
		current_atom = current_atom->rest;
	}
	
	return list_start_atom;
}

atom_t* read_sym(scanner_t *scan){
	
	int is_extra_sym_char(int c){
		switch(c){
			case '_': case '-':
				return true;
		}
		return false;
	}
	
	slice_t slice;
	scan_while_func(scan, &slice, isalnum, is_extra_sym_char);
	if ( strcmp(slice.ptr, "nil") == 0 ) {
		free(slice.ptr);
		return nil_atom();
	} else if ( strcmp(slice.ptr, "true") == 0 ) {
		free(slice.ptr);
		return true_atom();
	} else if ( strcmp(slice.ptr, "false") == 0 ) {
		free(slice.ptr);
		return false_atom();
	}
	
	return sym_atom_alloc(slice.ptr);
}