#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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
	scanner_t scan = scan_open(STDIN_FILENO);
	output_stream_t os = os_new(stdout);
	
	do {
		printf("%zd > ", scan.line);
		fflush(stdout);
		atom_t *atom = read_atom(&scan);
		atom = eval_atom(atom, env);
		print_atom(&os, atom);
		printf("\n");
	} while ( scan_peek(&scan) != EOF );
	
	printf("Encountered EOF. Have a nice day.\n");
	scan_close(&scan);
	
	return 0;
}

/**
 * Reads and interprets the specified files statement for statement.
 */
int interprete_files(env_t *env, int number_of_files, char **files){
	for(int i = 0; i < number_of_files; i++){
		int fd = open(files[i], O_RDONLY);
		if (fd == -1){
			fprintf(stderr, "Failed to run file %s: %s\n", files[i], sys_errlist[errno]);
			continue;
		}
		
		scanner_t scan = scan_open(fd);
		
		// Ignore the hash bang line on the files if there is one.
		if ( scan_peek(&scan) == '#' )
			scan_until(&scan, NULL, '\n');
		
		while ( scan_peek(&scan) != EOF ){
			atom_t *atom = read_atom(&scan);
			eval_atom(atom, env);
		}
		
		scan_close(&scan);
		close(fd);
	}
	
	return 0;
}