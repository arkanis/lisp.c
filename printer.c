#include <stdint.h>

#include "memory.h"
#include "printer.h"

void print_list(FILE *stream, atom_t *list_atom);

void print_atom(FILE *stream, atom_t *atom){
	switch(atom->type){
		case T_NUM:
			fprintf(stream, "%ld", atom->num);
			break;
		case T_SYM:
			fprintf(stream, "%s", atom->sym);
			break;
		case T_STR:
			fprintf(stream, "\"%s\"", atom->str);
			break;
		case T_NIL:
			fprintf(stream, "nil");
			break;
		case T_TRUE:
			fprintf(stream, "true");
			break;
		case T_FALSE:
			fprintf(stream, "false");
			break;
		case T_PAIR:
			print_list(stream, atom);
			break;
		case T_BUILDIN:
			fprintf(stream, "buildin at %p", atom->func);
			break;
		case T_LAMBDA:
			fprintf(stream, "(lambda ");
			print_atom(stream, atom->args);
			fprintf(stream, " ");
			print_atom(stream, atom->body);
			fprintf(stream, ")");
			break;
		case T_ENV:
			fprintf(stream, "environment %p with %d elements (parent %p)\n", atom->env, atom->env->length, atom->env->parent);
			for(int i = 0; i < atom->env->length; i++){
				fprintf(stream, "  %s: ", atom->env->bindings[i].key);
				print_atom(stream, atom->env->bindings[i].value);
				fprintf(stream, "\n");
			}
			break;
		default:
			fprintf(stream, "unknown atom, type %d", atom->type);
			break;
	}
}

void print_list(FILE *stream, atom_t *list_atom){
	fprintf(stream, "(");
	
	while (list_atom->type == T_PAIR) {
		print_atom(stream, list_atom->first);
		list_atom = list_atom->rest;
		if (list_atom->type == T_PAIR)
			fprintf(stream, " ");
	}
	
	if ( list_atom->type != T_NIL ) {
		fprintf(stream, " . ");
		print_atom(stream, list_atom);
	}
	
	fprintf(stream, ")");
}