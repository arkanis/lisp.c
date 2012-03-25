#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


#include "memory-management.c"

//
// Reader
//

atom_t* read_list(FILE *stream);
atom_t* read_num(FILE *stream);
atom_t* read_str(FILE *stream);
atom_t* read_sym(FILE *stream);

atom_t* read_atom(FILE *stream){
	char c;
	fscanf(stream, " %c", &c);
	
	if ( c == '(' ) {
		return read_list(stream);
	} else if ( c == '"' ) {
		return read_str(stream);
	} else if ( isdigit(c) ) {
		ungetc(c, stream);
		return read_num(stream);
	} else {
		ungetc(c, stream);
		return read_sym(stream);
	}
}


atom_t* read_list(FILE *stream){
	char c;
	atom_t* list_start_atom = alloc_pair();
	atom_t* current_atom = list_start_atom;
	
	while (true) {
		current_atom->pair.first = read_atom(stream);
		
		fscanf(stream, " %c", &c);
		if ( c == ')' ) {
			current_atom->pair.rest = get_nil_atom();
			break;
		} else if ( c == '.' ) {
			current_atom->pair.rest = read_atom(stream);
			// Consume trailing whitespaces and the closing list parenthesis
			fscanf(stream, " )");
			break;
		} else {
			ungetc(c, stream);
		}
		
		current_atom->pair.rest = alloc_pair();
		current_atom = current_atom->pair.rest;
	}
	
	return list_start_atom;
}

atom_t* read_num(FILE *stream){
	atom_t* new_atom = alloc_num();
	fscanf(stdin, "%ld", &new_atom->num);
	return new_atom;
}

atom_t* read_str(FILE *stream){
	atom_t *new_atom = alloc_str();
	// Consumes the string until the double quote and the trailing double quote
	fscanf(stream, "%m[^\"]\"", &new_atom->str);
	return new_atom;
}

atom_t* read_sym(FILE *stream){
	atom_t *new_atom = alloc_sym();
	// Consume the symbol
	scanf("%m[0-9a-zA-Z_]", &new_atom->sym);
	return new_atom;
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
	allocator_init();
	
	while (true) {
		printf("> ");
		fflush(stdout);
		atom_t *atom = read_atom(stdin);
		print_atom(stdout, atom);
		printf("\n");
	}
}