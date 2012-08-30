#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include "test_utils.h"

int tests_passed, tests_failed;
bool last_test_failed = false;

bool test_func(bool expr, char *file, int line, const char *code, const char *message, ...){
	if (expr) {
		tests_passed++;
		last_test_failed = false;
		fprintf(stderr, ".");
		fflush(stderr);
		return true;
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
		return false;
	}
}

int show_test_report(){
	printf("\n%d tests failed, %d passed\n", tests_failed, tests_passed);
	return tests_failed;
}