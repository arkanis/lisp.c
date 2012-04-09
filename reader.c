#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "memory.h"
#include "reader.h"
#include "logger.h"

/*

TODO:
- comments (";")
- recursive decent parser
- quote

read_while(reader, ...)
read_while_func(reader, func)
read_until(reader, ...)
read_until_func(reader, func)
read_one_of(...)

*/


int read_whitespaces(reader_t *reader);
int read_scan(reader_t *reader, const char *format, ...);

atom_t* read_list(reader_t *reader);
atom_t* read_num(reader_t *reader);
atom_t* read_str(reader_t *reader);
atom_t* read_sym(reader_t *reader);


atom_t* read_atom(reader_t *reader){
	int c = read_whitespaces(reader);
	
	while(c == ';'){
		fscanf(reader->stream, "%*[^\n]");
		c = read_whitespaces(reader);
	}
	
	if ( c == EOF )
		return nil_atom();
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
	atom_t* list_start_atom = pair_atom_alloc( nil_atom(), nil_atom() );
	atom_t* current_atom = list_start_atom;
	
	while (true) {
		current_atom->first = read_atom(reader);
		
		c = read_whitespaces(reader);
		if ( c == EOF ) {
			break;
		} else if ( c == ')' ) {
			current_atom->rest = nil_atom();
			break;
		} else if ( c == '.' ) {
			current_atom->rest = read_atom(reader);
			// Consume trailing whitespaces and the closing list parenthesis
			c = read_whitespaces(reader);
			if ( c == ')' )
				break;
			else
				return nil_atom();
		} else {
			ungetc(c, reader->stream);
		}
		
		current_atom->rest = pair_atom_alloc( nil_atom(), nil_atom() );
		current_atom = current_atom->rest;
	}
	
	if ( reader->eof )
		return nil_atom();
	else
		return list_start_atom;
}

atom_t* read_num(reader_t *reader){
	int64_t value = 0;
	if ( read_scan(reader, "%ld", &value) == 1 )
		return num_atom_alloc(value);
	else
		return nil_atom();
}

atom_t* read_str(reader_t *reader){
	char *str = NULL;
	// Consumes the string until the double quote and the trailing double quote
	if ( read_scan(reader, "%m[^\"]\"", &str) == 1 )
		return str_atom_alloc(str);
	else
		return nil_atom();
}

atom_t* read_sym(reader_t *reader){
	char *sym = NULL;
	// Consume the symbol
	if ( read_scan(reader, "%m[^ \t\n()]", &sym) == 1 ) {
		if ( strcmp(sym, "nil") == 0 )
			return nil_atom();
		else if ( strcmp(sym, "true") == 0 )
			return true_atom();
		else if ( strcmp(sym, "false") == 0 )
			return false_atom();
		
		return sym_atom_alloc(sym);
	}

	return nil_atom();
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