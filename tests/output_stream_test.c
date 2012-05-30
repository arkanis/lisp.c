#include <stdio.h>
#include <string.h>

#include "test_utils.h"
#include "../output_stream.h"


void test_capture_stream(){
	output_stream_t os = os_new_capture(4096);
	
	os_printf(&os, "hello");
	test(strcmp(os.buffer_ptr, "hello") == 0, "buffer contained unexpected content: %s", os.buffer_ptr);
	
	os_printf(&os, " world");
	test(strcmp(os.buffer_ptr, "hello world") == 0, "buffer contained unexpected content: %s", os.buffer_ptr);
	
	os_printf(&os, " from %s", "me");
	test(strcmp(os.buffer_ptr, "hello world from me") == 0, "buffer contained unexpected content: %s", os.buffer_ptr);
	
	os_destroy(&os);
}


int main(){
	test_capture_stream();
	return show_test_report();
}