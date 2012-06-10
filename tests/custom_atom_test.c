#include <stdbool.h>

#include "test_utils.h"
#include "../eval.h"
#include "../reader.h"
#include "../printer.h"
#include "../buildins.h"

void test_custom_func(){
	env_t *env = env_alloc(NULL);
	atom_t *custom_atom = NULL;
	bool custom_func_called;
	
	atom_t* custom_func(atom_t *args, env_t *env){
		custom_func_called  = true;
		
		atom_t *arg = args;
		test(arg->type == T_PAIR, "args has to be a list");
		test(arg->first == custom_atom, "first arg is supposed to be the custom atom itself");
		
		arg = arg->rest;
		test(arg->type == T_PAIR, "args has to be a list");
		test(arg->first->type == T_NUM && arg->first->num == 1, "the second arg is supposed to be the number 1");
		
		arg = arg->rest;
		test(arg->type == T_PAIR, "args has to be a list");
		test(arg->first->type == T_NUM && arg->first->num == 2, "the second arg is supposed to be the number 2");
		
		arg = arg->rest;
		test(arg->type == T_NIL, "nil terminator for arg list is missing");
		
		return true_atom();
	}
	
	custom_atom = custom_atom_alloc(1, "some random data", custom_func);
	env_set(env, "custom", custom_atom);
	
	scanner_t scan = scan_open_string("(custom 1 2)");
	atom_t *ast = read_atom(&scan);
	scan_close(&scan);
	
	custom_func_called = false;
	atom_t *result_atom = eval_atom(ast, env);
	test(custom_func_called == true, "the custom atom func was not called");
	test(result_atom == true_atom(), "wrong custom func return value");
}


int main(){
	// Important for singleton atoms (nil, true, false). Otherwise we got NULL pointers there...
	memory_init();
	
	test_custom_func();
	return show_test_report();
}