#ifndef _TEST_UTILS_H
#define _TEST_UTILS_H

#include <stdio.h>
#include <stdbool.h>

#define test(expr, ...) test_func( (expr), __FILE__, __LINE__, #expr, __VA_ARGS__)
void test_func(bool expr, char *file, int line, const char *code, const char *message, ...);
extern int tests_passed, tests_failed;
int show_test_report();

typedef struct {
	FILE *in, *out;
} capture_stream_t;

capture_stream_t create_capture_stream();
char* read_capture_stream(capture_stream_t stream);

#endif