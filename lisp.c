#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "memory-management.c"

//
// Reader
//

typedef struct {
	FILE *stream;
	uint64_t line;
	bool eof;
} reader_t;

int read_whitespaces(reader_t *reader);

atom_t* read_list(reader_t *reader);
atom_t* read_num(reader_t *reader);
atom_t* read_str(reader_t *reader);
atom_t* read_sym(reader_t *reader);

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

int read_scan(reader_t *reader, const char *format, ...){
	va_list args;
	va_start(args, format);
	int ret = vfscanf(reader->stream, format, args);
	va_end(args);
	
	if (ret == EOF)
		reader->eof = true;
	
	return ret;
}

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
	if ( read_scan(reader, "%m[^ \t\n()]", &new_atom->sym) == 1 )
		return new_atom;
	else
		return get_nil_atom();
}


//
// Printer
//

void print_list(FILE *stream, atom_t *list_atom);

void print_atom(FILE *stream, atom_t *atom){
	if (atom->type == T_NUM)
		fprintf(stream, "%ld", atom->num);
	else if (atom->type == T_SYM)
		fprintf(stream, "%s", atom->sym);
	else if (atom->type == T_STR)
		fprintf(stream, "\"%s\"", atom->str);
	else if (atom->type == T_NIL)
		fprintf(stream, "nil");
	else if (atom->type == T_TRUE)
		fprintf(stream, "true");
	else if (atom->type == T_FALSE)
		fprintf(stream, "false");
	else if (atom->type == T_PAIR)
		print_list(stream, atom);
	else
		fprintf(stream, "unknown atom");
}

void print_list(FILE *stream, atom_t *list_atom){
	fprintf(stream, "(");
	
	while (list_atom->type == T_PAIR) {
		print_atom(stream, list_atom->pair.first);
		list_atom = list_atom->pair.rest;
		if (list_atom->type == T_PAIR)
			fprintf(stream, " ");
	}
	
	if ( list_atom->type != T_NIL ) {
		fprintf(stream, " . ");
		print_atom(stream, list_atom);
	}
	
	fprintf(stream, ")");
}


//
// Read (not yet eval) print loop
//

void main(){
	reader_t reader = {stdin, 1, false};
	allocator_init();
	
	while ( !reader.eof ) {
		printf("%ld > ", reader.line);
		fflush(stdout);
		atom_t *atom = read_atom(&reader);
		print_atom(stdout, atom);
		printf("\n");
	}
	
	printf("Encountered EOF. Have a good day.\n");
}