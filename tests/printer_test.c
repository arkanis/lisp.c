#include <stdio.h>
#include <string.h>

#include "test_utils.h"
#include "../printer.h"
#include "../scanner.h"
#include "../reader.h"

void test_printer(){
	atom_t *atom = NULL;
	output_stream_t os = os_new_capture(4096);
	
	char *samples[] = {
		"(1 (2) (3))",
		"(define (plus a b) (+ a b))",
		"(+ 1 (- 1 (* 4 5)))",
		"'quoted",
		NULL
	};
	
	for(size_t i = 0; samples[i] != NULL; i++){
		scanner_t scan = scan_open_string(samples[i]);
		atom = read_atom(&scan);
		scan_close(&scan);
		
		print_atom(&os, atom);
		test(strcmp(os.buffer_ptr, samples[i]) == 0, "reader input and printer output differs.\ninput: %s\noutput: %s", samples[i], os.buffer_ptr);
		os_clear(&os);
	}
	
	os_destroy(&os);
}


int main(){
	// Important for singleton atoms (nil, true, false). Otherwise we got NULL pointers there...
	memory_init();
	
	test_printer();
	return show_test_report();
}