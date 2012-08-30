#include <string.h>

#include "test_utils.h"
#include "test_bytecode_utils.h"

#include "../memory.h"
#include "../reader.h"
#include "../printer.h"
#include "../eval.h"
#include "../buildins.h"
#include "../bytecode_interpreter.h"
#include "../bytecode_compiler.h"


env_t *env;
bytecode_interpreter_t interpreter;
output_stream_t os;

//
// Helpers
//

static void test_sample(char *lisp_code, const char *expected_lisp_output){
	
	scanner_t scan = scan_open_string(lisp_code);
	atom_t *ast = read_atom(&scan);
	scan_close(&scan);
	
	atom_t *cl = bcc_compile_to_lambda(nil_atom(), ast, env, NULL);
	atom_t *rl = runtime_lambda_atom_alloc(cl, scope_env_alloc(env));
	
	atom_t *result = bci_eval(interpreter, rl, nil_atom(), env);
	
	print_atom(&os, result);
	test(strcmp(os.buffer_ptr, expected_lisp_output) == 0, "unexpected output.\ninput: %s\noutput: %s\nexpected: %s", lisp_code, os.buffer_ptr, expected_lisp_output);
	os_clear(&os);
}


int main(){
	memory_init();
	env = env_alloc(NULL);
	register_buildins_in(env);
	interpreter = bci_new(0);
	os = os_new_capture(4096);
	
	test_sample("42", "42");
	test_sample("(begin \
	(define fac (lambda (n) \
		(if (= n 1) \
			1 \
			(* n (fac (- n 1))) \
		) \
	)) \
	 \
	(fac 7) \
	)", "5040");
	
	os_destroy(&os);
	bci_destroy(interpreter);
	
	return show_test_report();
}