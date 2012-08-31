#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "memory.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"
#include "buildins.h"
#include "bytecode_interpreter.h"
#include "bytecode_compiler.h"


typedef struct {
	bool compile;
	char *input_file;
} options_t, *options_p;

void parse_opts(int argc, char **argv, options_p opts);
int repl(env_t *env, options_p opts);
int interprete_files(env_t *env, options_p opts);


/**
 * Main lisp.c program.
 */
int main(int argc, char **argv){
	options_t opts;
	parse_opts(argc, argv, &opts);
	
	memory_init();
	env_t *env = env_alloc(NULL);
	
	register_buildins_in(env);
	env_def(env, "__compile_lambdas", opts.compile ? true_atom() : false_atom());
	
	if (opts.input_file == NULL)
		return repl(env, &opts);
	else
		return interprete_files(env, &opts);
}

/**
 * Runs an interactive command line.
 */
int repl(env_t *env, options_p opts){
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
int interprete_files(env_t *env, options_p opts){
	int fd = open(opts->input_file, O_RDONLY);
	if (fd == -1){
		fprintf(stderr, "Failed to open file %s: %s\n", opts->input_file, strerror(errno));
		return -1;
	}
	
	scanner_t scan = scan_open(fd);
	
	// Ignore the hash bang line on the files if there is one.
	if ( scan_peek(&scan) == '#' )
		scan_until(&scan, NULL, '\n');
	
	if (opts->compile) {
		// Build a large begin statement and compile it
		atom_t *prog = pair_atom_alloc(sym_atom_alloc("begin"), nil_atom());
		atom_t *current_pair = prog;
		while ( scan_peek(&scan) != EOF ){
			atom_t *expr = read_atom(&scan);
			current_pair->rest = pair_atom_alloc(expr, current_pair->rest);
			current_pair = current_pair->rest;
		}
		
		bytecode_interpreter_t interpreter = bci_new(1024);
		atom_t *cl = bcc_compile_to_lambda(nil_atom(), prog, env, NULL);
		atom_t *rl = runtime_lambda_atom_alloc(cl, scope_env_alloc(env));
		
		bci_eval(interpreter, rl, nil_atom(), env);
		bci_destroy(interpreter);
	} else {
		// Do a normal repl but without prompt and printing
		while ( scan_peek(&scan) != EOF ){
			atom_t *atom = read_atom(&scan);
			eval_atom(atom, env);
		}
	}
	
	scan_close(&scan);
	close(fd);
	
	return 0;
}

/**
 * Reads the command line options and fills the supplied options_p struct with values. Also sets the default values
 * for the options.
 */
void parse_opts(int argc, char **argv, options_p opts){
	opts->compile = true;
	opts->input_file = NULL;
	
	int opt;
	bool show_help = false;
	while ( (opt = getopt(argc, argv, "hi")) != -1 ) {
		switch (opt) {
			case 'h':
				show_help = true;
				break;
			case 'i':
				opts->compile = false;
				break;
			default:
				show_help = true;
		}
	}
	
	if (optind < argc)
		opts->input_file = argv[optind];
	
	if (show_help){
		fprintf(stderr, "Usage: %s [-i] [-h] [file]\n", argv[0]);
		fprintf(stderr, "  -i\tinterpret only, disables the bytecode compiler\n");
		fprintf(stderr, "  -h\tshow this help and exit\n");
		fprintf(stderr, "  file\tthe name of a source file to run, if not specified an interactive console starts\n");
		exit(0);
	}
}