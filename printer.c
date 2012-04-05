#include <stdint.h>

#include "memory.h"
#include "printer.h"

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