#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memory.h"
#include "reader.h"
#include "printer.h"
#include "logger.h"
#include "eval.h"
#include "buildins.h"

#define lambda(return_type, body_and_args) \
   ({ \
     return_type __fn__ body_and_args \
     __fn__; \
   })

#define test(expr, ...) test_func( (expr), #expr, __VA_ARGS__)
void test_func(bool expr, const char *code, const char *message, ...);
atom_t* read_code(const char *code);
typedef void (*print_code_handler_t)(char *code);
void print_code(atom_t *atom, print_code_handler_t handler);

int tests_passed = 0;
int tests_failed = 0;

void test_logger(){
	log_setup(LOG_INFO, stderr);
	info("test info: %d", 123);
	warn("test warning: %d, %s", 123, "hello");
	error("test error: %s", "world");
	
	log_setup(LOG_INFO, stderr);
}



void test_printer(){
	atom_t *atom = NULL;
	
	atom = read_code("(1 (2) (3))");
	print_code(atom, lambda(void, (char *code){
		test( strcmp(code, "(1 (2) (3))") == 0, "printer output differs: %s", code);
	}));
	
	atom = read_code("(define (plus a b) (+ a b))");
	print_code(atom, lambda(void, (char *code){
		test( strcmp(code, "(define (plus a b) (+ a b))") == 0, "printer output differs: %s", code);
	}));
}

void test_env(){
	env_t *env = env_alloc(NULL);
	
	test( env_get(env, "does_not_exists") == NULL, "expected a NULL pointer for an undefined binding");
	
	env_set(env, "foo", true_atom());
	test( env_get(env, "foo") == true_atom(), "set and lookup in env failed");
	
	env_set(env, "bar", false_atom());
	env_set(env, "nil_atom", nil_atom());
	
	test( env_get(env, "bar") == false_atom(), "second set and lookup in env failed");
	test( env_get(env, "nil_atom") == nil_atom(), "third set and lookup in env failed");
	
	// Test nested envs
	env_t *nested_env = env_alloc(env);
	test( env_get(nested_env, "foo") == true_atom(), "nested env lookup failed");
	
	env_set(nested_env, "nested", true_atom());
	test( env_get(nested_env, "nested") == true_atom(), "set and lookup in nested env failed");
	test( env_get(env, "nested") == NULL, "nested set leaked into the parent env");
}

bool test_eval_sample_buildin_visited = false;
atom_t* test_eval_sample_buildin(atom_t *args, env_t *env){
	test_eval_sample_buildin_visited = true;
	return args->first;
}

void test_eval(){
	env_t *env = env_alloc(NULL);
	atom_t *atom = NULL;
	
	test( eval_atom(nil_atom(), env) == nil_atom() , "the nil atom should eval to itself");
	test( eval_atom(true_atom(), env) == true_atom() , "the true atom should eval to itself");
	test( eval_atom(false_atom(), env) == false_atom() , "the false atom should eval to itself");
	
	atom = num_atom_alloc(123);
	test( eval_atom(atom, env) == atom , "number atoms should eval to themselfs");
	
	atom = str_atom_alloc("hello world");
	test( eval_atom(atom, env) == atom , "string atoms should eval to themselfs");
	
	env_set(env, "test", buildin_atom_alloc(test_eval_sample_buildin));
	test_eval_sample_buildin_visited = false;
	atom = read_code("(test 1)");
	atom = eval_atom(atom, env);
	test( test_eval_sample_buildin_visited == true && atom->type == T_NUM && atom->num == 1, "failed to execute buildin");
}

void test_buildins(){
	
}


//
// Rest runner
//

int main(){
	memory_init();
	
	test_logger();
	test_reader();
	//test_printer();
	test_env();
	test_eval();
	
	printf("\n%d tests failed, %d passed\n", tests_failed, tests_passed);
	
	return 0;
}


//
// Utility functions
//



atom_t* read_code(const char *code){
	int reader_pipe[2];
	pipe(reader_pipe);
	
	FILE *reader_in = fdopen(reader_pipe[1], "w");
	FILE *reader_out = fdopen(reader_pipe[0], "r");
	
	fprintf(reader_in, "%s", code);
	fclose(reader_in);
	
	reader_t reader = (reader_t){ .stream = reader_out, .line = 0, .eof = false };
	atom_t *result = read_atom(&reader);
	
	fclose(reader_out);
	return result;
}

void print_code(atom_t *atom, print_code_handler_t handler){
	int printer_pipe[2];
	pipe(printer_pipe);
	
	FILE *printer_in = fdopen(printer_pipe[1], "w");
	FILE *printer_out = fdopen(printer_pipe[0], "r");
	
	print_atom(printer_in, atom);
	fprintf(printer_in, "\n");
	fclose(printer_in);
	
	char *printer_output = NULL;
	fscanf(printer_out, " %m[^\n]", &printer_output);
	handler(printer_output);
	free(printer_output);
	
	fclose(printer_out);
}

