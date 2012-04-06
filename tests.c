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
	info("test info: %d", 123);
	warn("test warning: %d, %s", 123, "hello");
	error("test error: %s", "world");
}

void test_reader(){
	atom_t *atom = NULL;
	
	atom = read_code("123");
	test(atom->type == T_NUM && atom->num == 123, "got type: %d, num: %ld", atom->type, atom->num);
	
	atom = read_code("sym");
	test(atom->type == T_SYM && strcmp(atom->sym, "sym") == 0, "got type: %d, sym: %s", atom->type, atom->sym);
	
	atom = read_code("\"str\"");
	test(atom->type == T_STR && strcmp(atom->str, "str") == 0, "got type: %d, str: %s", atom->type, atom->str);
	
	atom = read_code("nil");
	test(atom->type == T_NIL && atom == get_nil_atom(), "got type: %d, atom: %p, nil atom: %p", atom->type, atom, get_nil_atom());
	
	atom = read_code("true");
	test(atom->type == T_TRUE && atom == get_true_atom(), "got type: %d, atom: %p, true atom: %p", atom->type, atom, get_true_atom());
	
	atom = read_code("false");
	test(atom->type == T_FALSE && atom == get_false_atom(), "got type: %d, atom: %p, false atom: %p", atom->type, atom, get_false_atom());
	
	atom = read_code("(1)");
	test(atom->type == T_PAIR
		&& atom->first->type == T_NUM && atom->first->num == 1
		&& atom->rest->type == T_NIL
	, "failed to read a list with one entry");
	
	atom = read_code("(1 . 2)");
	test(atom->type == T_PAIR
		&& atom->first->type == T_NUM && atom->first->num == 1
		&& atom->rest->type == T_NUM && atom->rest->num == 2
	, "failed to read a not nil terminated list");
	
	atom = read_code("(1 2 3)");
	test(atom->type == T_PAIR && atom->first->type == T_NUM && atom->first->num == 1, "failed to read a list with tree elements (1. element)");
	atom = atom->rest;
	test(atom->type == T_PAIR && atom->first->type == T_NUM && atom->first->num == 2, "failed to read a list with tree elements (2. element)");
	atom = atom->rest;
	test(atom->type == T_PAIR && atom->first->type == T_NUM && atom->first->num == 3, "failed to read a list with tree elements (3. element)");
	atom = atom->rest;
	test(atom->type == T_NIL, "failed to read a list with tree elements (nil terminator)");
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
	env_t *env = alloc_env(NULL);
	
	test( env_get(env, "does_not_exists") == NULL, "expected a NULL pointer for an undefined binding");
	
	env_set(env, "foo", get_true_atom());
	test( env_get(env, "foo") == get_true_atom(), "set and lookup in env failed");
	
	env_set(env, "bar", get_false_atom());
	env_set(env, "nil_atom", get_nil_atom());
	
	test( env_get(env, "bar") == get_false_atom(), "second set and lookup in env failed");
	test( env_get(env, "nil_atom") == get_nil_atom(), "third set and lookup in env failed");
	
	// Test nested envs
	env_t *nested_env = alloc_env(env);
	test( env_get(nested_env, "foo") == get_true_atom(), "nested env lookup failed");
	
	env_set(nested_env, "nested", get_true_atom());
	test( env_get(nested_env, "nested") == get_true_atom(), "set and lookup in nested env failed");
	test( env_get(env, "nested") == NULL, "nested set leaked into the parent env");
}

bool test_eval_sample_buildin_visited = false;
atom_t* test_eval_sample_buildin(atom_t *args, env_t *env){
	test_eval_sample_buildin_visited = true;
	return args->first;
}

void test_eval(){
	env_t *env = alloc_env(NULL);
	atom_t *atom = NULL;
	
	test( eval_atom(get_nil_atom(), env) == get_nil_atom() , "the nil atom should eval to itself");
	test( eval_atom(get_true_atom(), env) == get_true_atom() , "the true atom should eval to itself");
	test( eval_atom(get_false_atom(), env) == get_false_atom() , "the false atom should eval to itself");
	
	atom = alloc_num();
	atom->num = 123;
	test( eval_atom(atom, env) == atom , "number atoms should eval to themselfs");
	
	atom = alloc_str();
	atom->str = "hello world";
	test( eval_atom(atom, env) == atom , "string atoms should eval to themselfs");
	
	env_set(env, "test", alloc_buildin(test_eval_sample_buildin));
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
	test_printer();
	test_env();
	test_eval();
	
	printf("\n%d tests failed, %d passed\n", tests_failed, tests_passed);
	
	return 0;
}


//
// Utility functions
//

void test_func(bool expr, const char *code, const char *message, ...){
	if (expr) {
		tests_passed++;
		fprintf(stderr, ".");
		fflush(stderr);
	} else {
		tests_failed++;
		fprintf(stderr, "FAIL %s: ", code);
		
		va_list args;
		va_start(args, message);
		vfprintf(stderr, message, args);
		va_end(args);
		
		fprintf(stderr, "\n");
	}
}

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