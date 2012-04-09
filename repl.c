#include <stdio.h>
#include <stdbool.h>
#include <errno.h>

#include "memory.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"
#include "buildins.h"


int repl(env_t *env);
int interprete_files(env_t *env, int number_of_files, char **files);

/**
 * Main LispC program. All parameters are interpreted as file names to be executed.
 * Without any arguments an interactive command line is started.
 */
int main(int argc, char **argv){
	memory_init();
	
	env_t *env = env_alloc(NULL);
	register_buildins_in(env);
	
	if (argc == 1)
		return repl(env);
	else
		return interprete_files(env, argc - 1, argv + 1);
}

/**
 * Shows an interactive command line
 */
int repl(env_t *env){
	reader_t reader = {stdin, 1, false};
	
	while ( !reader.eof ) {
		printf("%ld > ", reader.line);
		fflush(stdout);
		atom_t *atom = read_atom(&reader);
		atom = eval_atom(atom, env);
		print_atom(stdout, atom);
		printf("\n");
	}
	
	printf("Encountered EOF. Have a nice day.\n");
	return 0;
}

/**
 * Reads and interprets the specified files statement for statement.
 */
int interprete_files(env_t *env, int number_of_files, char **files){
	for(int i = 0; i < number_of_files; i++){
		FILE *stream = fopen(files[i], "rb");
		if (stream == NULL){
			fprintf(stderr, "Failed to run file %s: %s\n", files[i], sys_errlist[errno]);
			continue;
		}
		
		// Ignore the hash bang line on the files if there is one
		int first_char = fgetc(stream);
		if (first_char == '#')
			fscanf(stream, "%*[^\n] ");
		else
			ungetc(first_char, stream);
		
		reader_t reader = {stream, 1, false};
		while ( !reader.eof ){
			atom_t *atom = read_atom(&reader);
			eval_atom(atom, env);
		}
		
		fclose(stream);
	}
	
	return 0;
}