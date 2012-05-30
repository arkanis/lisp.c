#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "test_utils.h"
#include "../logger.h"

int main(){
	output_stream_t os = os_new_capture(4096);
	//output_stream_t os = os_new(stderr);
	log_setup(LOG_INFO, &os);
	
	info("test info: %d", 123);
	test(strcmp(os.buffer_ptr, "[info in logger_test.c:13 main()]: test info: 123\n") == 0, "expected the first log message but got %s", os.buffer_ptr);
	warn("test warning: %d, %s", 123, "hello");
	test(strcmp(os.buffer_ptr, "[info in logger_test.c:13 main()]: test info: 123\n[warn in logger_test.c:15 main()]: test warning: 123, hello\n") == 0, "expected the first two log messages but got %s", os.buffer_ptr);
	error("test error: %s", "world");
	test(strcmp(os.buffer_ptr, "[info in logger_test.c:13 main()]: test info: 123\n[warn in logger_test.c:15 main()]: test warning: 123, hello\n[ERROR in logger_test.c:17 main()]: test error: world\n") == 0, "expected the first tree log messages but got '%s'", os.buffer_ptr);
	
	os_destroy(&os);
	return show_test_report();
}