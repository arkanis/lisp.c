#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include "test_utils.h"

int tests_passed, tests_failed;
bool last_test_failed = false;

void test_func(bool expr, char *file, int line, const char *code, const char *message, ...){
	if (expr) {
		tests_passed++;
		last_test_failed = false;
		fprintf(stderr, ".");
		fflush(stderr);
	} else {
		tests_failed++;
		if (!last_test_failed)
			fprintf(stderr, "\n");
		fprintf(stderr, "FAIL (%s:%d) %s: ", file, line, code);
		last_test_failed = true;
		
		va_list args;
		va_start(args, message);
		vfprintf(stderr, message, args);
		va_end(args);
		
		fprintf(stderr, "\n");
	}
}

int show_test_report(){
	printf("\n%d tests failed, %d passed\n", tests_failed, tests_passed);
	return tests_failed;
}



capture_stream_t create_capture_stream(){
	int pipe_fds[2];
	pipe(pipe_fds);
	
	fcntl(pipe_fds[0], F_SETFL, fcntl(pipe_fds[0], F_GETFD) | O_NONBLOCK);
	fcntl(pipe_fds[1], F_SETFL, fcntl(pipe_fds[1], F_GETFD) | O_NONBLOCK);
	
	return (capture_stream_t){
		.in = fdopen(pipe_fds[1], "w"),
		.out = fdopen(pipe_fds[0], "r")
	};
}

char* read_capture_stream(capture_stream_t stream){
	fclose(stream.in);
	
	char *buffer = malloc(PIPE_BUF);
	size_t bytes_read = fread(buffer, PIPE_BUF, 1, stream.out);
	buffer[bytes_read] = '\0';
	fclose(stream.out);
	
	return buffer;
}