#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "memory.h"
#include "reader.h"


int read_whitespaces(reader_t *reader);
int read_scan(reader_t *reader, const char *format, ...);

atom_t* read_list(reader_t *reader);
atom_t* read_num(reader_t *reader);
atom_t* read_str(reader_t *reader);
atom_t* read_sym(reader_t *reader);


atom_t* read_atom(reader_t *reader){
	int c = read_whitespaces(reader);
	
	if ( c == EOF )
		return get_nil_atom();
	else if ( c == '(' ) {
		return read_list(reader);
	} else if ( c == '"' ) {
		return read_str(reader);
	} else if ( isdigit(c) ) {
		ungetc(c, reader->stream);
		return read_num(reader);
	} else {
		ungetc(c, reader->stream);
		return read_sym(reader);
	}
}


/**
 * Reads all whitespaces from the reader stream. Increments the reader
 * line number for each encountered line break. The first non whitespace
 * character is returned (as `int` to preserve EOF).
 */
int read_whitespaces(reader_t *reader){
	char c;
	do {
		c = fgetc(reader->stream);
		if (c == '\n')
			reader->line++;
		else if (c == EOF)
			reader->eof = true;
	} while ( isspace(c) );
	return c;
}

atom_t* read_list(reader_t *reader){
	int c;
	atom_t* list_start_atom = alloc_pair();
	atom_t* current_atom = list_start_atom;
	
	while (true) {
		current_atom->pair.first = read_atom(reader);
		
		c = read_whitespaces(reader);
		if ( c == EOF ) {
			break;
		} else if ( c == ')' ) {
			current_atom->pair.rest = get_nil_atom();
			break;
		} else if ( c == '.' ) {
			current_atom->pair.rest = read_atom(reader);
			// Consume trailing whitespaces and the closing list parenthesis
			c = read_whitespaces(reader);
			if ( c == ')' )
				break;
			else
				return get_nil_atom();
		} else {
			ungetc(c, reader->stream);
		}
		
		current_atom->pair.rest = alloc_pair();
		current_atom = current_atom->pair.rest;
	}
	
	if ( reader->eof )
		return get_nil_atom();
	else
		return list_start_atom;
}

atom_t* read_num(reader_t *reader){
	atom_t* new_atom = alloc_num();
	if ( read_scan(reader, "%ld", &new_atom->num) == 1 )
		return new_atom;
	else
		return get_nil_atom();
}

atom_t* read_str(reader_t *reader){
	atom_t *new_atom = alloc_str();
	// Consumes the string until the double quote and the trailing double quote
	if ( read_scan(reader, "%m[^\"]\"", &new_atom->str) == 1 )
		return new_atom;
	else
		return get_nil_atom();
}

atom_t* read_sym(reader_t *reader){
	atom_t *new_atom = alloc_sym();
	// Consume the symbol
	if ( read_scan(reader, "%m[^ \t\n()]", &new_atom->sym) == 1 ) {
		if ( strcmp(new_atom->sym, "nil") == 0 )
			return get_nil_atom();
		else if ( strcmp(new_atom->sym, "true") == 0 )
			return get_true_atom();
		else if ( strcmp(new_atom->sym, "false") == 0 )
			return get_false_atom();
		
		return new_atom;
	}

	return get_nil_atom();
}

int read_scan(reader_t *reader, const char *format, ...){
	va_list args;
	va_start(args, format);
	int ret = vfscanf(reader->stream, format, args);
	va_end(args);
	
	if (ret == EOF)
		reader->eof = true;
	
	return ret;
}