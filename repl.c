#include <stdio.h>
#include <stdbool.h>

#include "memory.h"
#include "reader.h"
#include "printer.h"
#include "eval.h"
#include "buildins.h"

//
// Read (not yet eval) print loop
//

int main(){
	reader_t reader = {stdin, 1, false};
	memory_init();
	env_t *env = alloc_env(NULL);
	
	register_buildins_in(env);
	
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