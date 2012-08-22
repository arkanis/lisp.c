#include <string.h>

#include "test_utils.h"
#include "../eval.h"
#include "../reader.h"
#include "../printer.h"
#include "../buildins.h"

void test_env_get_and_set(){
	env_t *env = env_alloc(NULL);
	
	test( env_get(env, "does_not_exists") == NULL, "expected a NULL pointer for an undefined binding");
	
	env_set(env, "foo", true_atom());
	test( env_get(env, "foo") == true_atom(), "set and lookup in env failed");
	
	env_set(env, "bar", false_atom());
	env_set(env, "nil_atom", nil_atom());
	
	test( env_get(env, "bar") == false_atom(), "second set and lookup in env failed");
	test( env_get(env, "nil_atom") == nil_atom(), "third set and lookup in env failed");
}

void test_nested_env(){
	env_t *env = env_alloc(NULL);
	
	env_set(env, "foo", true_atom());
	test( env_get(env, "foo") == true_atom(), "direct lookup in env failed");
	
	env_t *nested_env = env_alloc(env);
	test( env_get(nested_env, "foo") == true_atom(), "nested env lookup failed");
	
	env_set(nested_env, "nested", true_atom());
	test( env_get(nested_env, "nested") == true_atom(), "set and lookup in nested env failed");
	test( env_get(env, "nested") == NULL, "nested set leaked into the parent env");
}

void test_eval_lowlevel(){
	env_t *env = env_alloc(NULL);
	atom_t *atom = NULL;
	
	// Test self evaling atoms
	test( eval_atom(nil_atom(), env) == nil_atom() , "the nil atom should eval to itself");
	test( eval_atom(true_atom(), env) == true_atom() , "the true atom should eval to itself");
	test( eval_atom(false_atom(), env) == false_atom() , "the false atom should eval to itself");
	
	atom = num_atom_alloc(123);
	test( eval_atom(atom, env) == atom , "number atoms should eval to themselfs");
	
	atom = str_atom_alloc("hello world");
	test( eval_atom(atom, env) == atom , "string atoms should eval to themselfs");
	
	// Test symbol evaluation
	env_set(env, "test", true_atom());
	test( eval_atom(sym_atom_alloc("test"), env) == true_atom(), "symbol evaluation failed to look up the bound value");
	
	// Test buildin evaluation
	bool sample_buildin_visited = false;
	atom_t* sample_buildin(atom_t *args, env_t *env){
		sample_buildin_visited = true;
		return args->first;
	}
	
	env_set(env, "test", buildin_atom_alloc(sample_buildin, NULL));
	atom = pair_atom_alloc( sym_atom_alloc("test"), pair_atom_alloc(num_atom_alloc(1), nil_atom()) );
	atom = eval_atom(atom, env);
	test(sample_buildin_visited == true, "failed to execute buildin");
	test(atom->type == T_NUM && atom->num == 1, "buildin return value was screwed up");
	
	// Lambda evaluation not yet tested directly...
	// Not worth the time right now as it is covered by higher level tests
}

char *language_buildin_samples[] = {
	"(define var 1234)", "1234",
	"var", "1234",
	
	"(if true 1 2)", "1",
	"(if false 1 2)", "2",
	
	"(define true_case_evaled false)", "false",
	"(define false_case_evaled false)", "false",
	"(if true (define true_case_evaled true) (define false_case_evaled true))", "true",
	"true_case_evaled", "true",
	"false_case_evaled", "false",
	
	"(define true_case_evaled false)", "false",
	"(define false_case_evaled false)", "false",
	"(if false (define true_case_evaled true) (define false_case_evaled true))", "true",
	"true_case_evaled", "false",
	"false_case_evaled", "true",
	
	"(define foo (lambda (a b) b))", "(lambda (a b) b)",
	"(foo 1 2)", "2",
	"(define implicit_begin_foo (lambda (a b) a a b))", "(lambda (a b) (begin a a b))",
	"(implicit_begin_foo 1 2)", "2",
	
	"(quote (foo 1 2))", "(foo 1 2)",
	"'(foo 1 2)", "(foo 1 2)",
	
	"(define expression_evaled false)", "false",
	"(begin 1 (define expression_evaled true) 3)", "3",
	"expression_evaled", "true",
	
	"(cons 1 2)", "(1 . 2)",
	"(cons 1 (cons 2 nil))", "(1 2)",
	"(first (cons 1 2))", "1",
	"(rest (cons 1 2))", "2",
	"(define pair (cons 1 2))", "(1 . 2)",
	"(first pair)", "1",
	"(rest pair)", "2",
	
	// EOF evals to nil
	"", "nil",
	
	NULL
};

void test_eval_with_buildins(){
	atom_t *atom = NULL;
	output_stream_t os = os_new_capture(4096);
	
	env_t *env = env_alloc(NULL);
	register_buildins_in(env);
	
	char *other_samples[] = {
		"nil", "nil",
		"true", "true",
		"false", "false",
		"123", "123",
		"\"hello\"", "\"hello\"",
		
		"(+ 1 2)", "3",
		
		"(define simple_val 123)", "123",
		"simple_val", "123",
		"(define evaled_val (+ 1 2))", "3",
		"evaled_val", "3",
		
		"(= 1 1)", "true",
		"(= 1 2)", "false",
		
		NULL
	};
	
	char **sample_set_list[] = { language_buildin_samples, other_samples, NULL };
	for(size_t i = 0; sample_set_list[i] != NULL; i++){
		char **samples = sample_set_list[i];
		
		for(size_t j = 0; samples[j] != NULL; j += 2){
			scanner_t scan = scan_open_string(samples[j]);
			atom = read_atom(&scan);
			scan_close(&scan);
			
			atom = eval_atom(atom, env);
			
			print_atom(&os, atom);
			test(strcmp(os.buffer_ptr, samples[j+1]) == 0, "unexpected eval output.\ninput: %s\noutput: %s\nexpected: %s", samples[j], os.buffer_ptr, samples[j+1]);
			os_clear(&os);
		}
		
	}
	
	os_destroy(&os);
}


int main(){
	// Important for singleton atoms (nil, true, false). Otherwise we got NULL pointers there...
	memory_init();
	
	test_env_get_and_set();
	test_nested_env();
	test_eval_lowlevel();
	test_eval_with_buildins();
	return show_test_report();
}