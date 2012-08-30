#ifndef _TEST_UTILS_H
#define _TEST_UTILS_H

#include <stdio.h>
#include <stdbool.h>

#define test(expr, ...) test_func( (expr), __FILE__, __LINE__, #expr, __VA_ARGS__)
bool test_func(bool expr, char *file, int line, const char *code, const char *message, ...);
extern int tests_passed, tests_failed;
int show_test_report();

#endif