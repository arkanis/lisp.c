#include <stdint.h>
#include <string.h>

#include "printer.h"

void print_list(output_stream_t *stream, atom_t *list_atom);

void print_atom(output_stream_t *stream, atom_t *atom){
	switch(atom->type){
		case T_NUM:
			os_printf(stream, "%ld", atom->num);
			break;
		case T_SYM:
			os_printf(stream, "%s", atom->sym);
			break;
		case T_STR:
			os_printf(stream, "\"%s\"", atom->str);
			break;
		case T_NIL:
			os_printf(stream, "nil");
			break;
		case T_TRUE:
			os_printf(stream, "true");
			break;
		case T_FALSE:
			os_printf(stream, "false");
			break;
		case T_PAIR:
			if ( atom->first->type == T_SYM && atom->rest->type == T_PAIR && strcmp(atom->first->sym, "quote") == 0 ) {
				os_printf(stream, "'");
				print_atom(stream, atom->rest->first);
			} else {
				print_list(stream, atom);
			}
			break;
		case T_BUILDIN:
			os_printf(stream, "buildin at %p", atom->func);
			break;
		case T_LAMBDA:
			os_printf(stream, "(lambda ");
			print_atom(stream, atom->args);
			os_printf(stream, " ");
			print_atom(stream, atom->body);
			os_printf(stream, ")");
			break;
		case T_CUSTOM:
			os_printf(stream, "custom atom, type: %ud, data: %p, func: %p", atom->custom.type, atom->custom.data, atom->custom.func);
			break;
		case T_ENV:
			os_printf(stream, "environment %p with %d elements (parent %p)\n", atom->env, atom->env->length, atom->env->parent);
			for(int i = 0; i < atom->env->length; i++){
				os_printf(stream, "  %s: ", atom->env->bindings[i].key);
				print_atom(stream, atom->env->bindings[i].value);
				os_printf(stream, "\n");
			}
			break;
		default:
			os_printf(stream, "unknown atom, type %d", atom->type);
			break;
	}
}

void print_list(output_stream_t *stream, atom_t *list_atom){
	os_printf(stream, "(");
	
	while (list_atom->type == T_PAIR) {
		print_atom(stream, list_atom->first);
		list_atom = list_atom->rest;
		if (list_atom->type == T_PAIR)
			os_printf(stream, " ");
	}
	
	if ( list_atom->type != T_NIL ) {
		os_printf(stream, " . ");
		print_atom(stream, list_atom);
	}
	
	os_printf(stream, ")");
}