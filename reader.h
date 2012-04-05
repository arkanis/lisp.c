#ifndef _READER_H
#define _READER_H

#include <stdbool.h>
#include <stdio.h>

typedef struct {
	FILE *stream;
	uint64_t line;
	bool eof;
} reader_t;

atom_t* read_atom(reader_t *reader);

#endif