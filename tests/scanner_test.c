#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "test_utils.h"
#include "../scanner.h"


void test_scan_until(){
	int fd = open("scanner_test_code", O_RDONLY);
	test(fd != -1, "failed to open the scanner_test_code file");
	scanner_t scan = scan_open(fd);
	slice_t slice;
	
	int c = scan_until(&scan, &slice, '"');
	test(c == '"', "expected terminator \" but got %c (%d)", c, c);
	test(slice.length == 0, "expected length of 0 but got %d", slice.length);
	test(strcmp(slice.ptr, "") == 0, "expected an empty string as slice string but got %s", slice.ptr);
	free(slice.ptr);
	
	c = scan_until(&scan, &slice, '"');
	test(c == '"', "expected terminator \" but got %c (%d)", c, c);
	test(slice.length == 11, "expected length of 11 but got %d", slice.length);
	test(strcmp(slice.ptr, "hello world") == 0, "expected the hello world string as slice string but got %s", slice.ptr);
	free(slice.ptr);
	
	c = scan_until(&scan, &slice, EOF);
	test(c == EOF, "expected terminator EOF but got %d", c);
	test(strcmp(slice.ptr, "\n\t \n1234567890\n----") == 0, "expected the rest of the sample code as slice string but got %s", slice.ptr);
	free(slice.ptr);
	
	scan_close(&scan);
	close(fd);
}

void test_scan_while(){
	int fd = open("scanner_test_code", O_RDONLY);
	test(fd != -1, "failed to open the scanner_test_code file");
	scanner_t scan = scan_open(fd);
	slice_t slice;
	
	// Throw the first two lines away
	scan_until(&scan, &slice, '\n');
	test(strcmp(slice.ptr, "\"hello world\"") == 0, "expected the first line but gut %s", slice.ptr);
	free(slice.ptr);
	scan_until(&scan, &slice, '\n');
	test(strcmp(slice.ptr, "\t ") == 0, "expected the second line but gut %s", slice.ptr);
	free(slice.ptr);
	
	int c = scan_while(&scan, &slice, '0', '1', '2', '3', '4', '5', '6', '7', '8', '9');
	test(c == '\n', "expected newline as terminator but got %d", c);
	test(slice.length == 10, "expected length of 10 but got %d", slice.length);
	test(strcmp(slice.ptr, "1234567890") == 0, "expected a string with digits from the source code but got %s", slice.ptr);
	free(slice.ptr);
	
	c = scan_while(&scan, &slice, '\n', '-');
	test(c == EOF, "expected EOF as terminator but got %d", c);
	test(slice.length == 5, "expected length of 10 but got %d", slice.length);
	test(strcmp(slice.ptr, "\n----") == 0, "expected the last line from the source code but got %s", slice.ptr);
	free(slice.ptr);
	
	scan_close(&scan);
	close(fd);
}

void test_scan_one_of(){
	int fd = open("scanner_test_code", O_RDONLY);
	test(fd != -1, "failed to open the scanner_test_code file");
	scanner_t scan = scan_open(fd);
	
	int c = scan_one_of(&scan, '"');
	test(c == '"', "expected an \", got %c", c);
	c = scan_one_of(&scan, 'h');
	test(c == 'h', "expected h, got %c", c);
	c = scan_one_of(&scan, 'x', 'y', 'e');
	test(c == 'e', "expected a, got %c", c);
	
	scan_close(&scan);
	close(fd);
}

void test_scan_until_func(){
	int fd = open("scanner_test_code", O_RDONLY);
	test(fd != -1, "failed to open the scanner_test_code file");
	scanner_t scan = scan_open(fd);
	slice_t slice;
	
	int c = scan_until_func(&scan, &slice, isspace);
	test(c == ' ', "expected a space as terminator but got %d", c);
	test(slice.length == 6, "expected length of 6 but got %d", slice.length);
	test(strcmp(slice.ptr, "\"hello") == 0, "expected the first line from the source code but got %s", slice.ptr);
	free(slice.ptr);
	
	scan_close(&scan);
	close(fd);
}

void test_scan_while_func(){
	int fd = open("scanner_test_code", O_RDONLY);
	test(fd != -1, "failed to open the scanner_test_code file");
	scanner_t scan = scan_open(fd);
	slice_t slice;
	
	int c = scan_while_func(&scan, &slice, ispunct, islower, isblank);
	test(c == '\n', "expected newline as terminator but got %c", c);
	test(slice.length == 13, "expected length of 13 but got %d", slice.length);
	test(strcmp(slice.ptr, "\"hello world\"") == 0, "expected the first line from the source code but got %s", slice.ptr);
	free(slice.ptr);
	
	scan_close(&scan);
	close(fd);
}

void test_line_and_col_numbers(){
	int fd = open("scanner_test_code", O_RDONLY);
	test(fd != -1, "failed to open the scanner_test_code file");
	scanner_t scan = scan_open(fd);
	
	test(scan.line == 1, "the line number should start with 1 but got %d", scan.line);
	test(scan.col == 1, "the column number should start with 1 but got %d", scan.col);
	
	int c = scan_until(&scan, NULL, '"');
	test(scan.line == 1, "the line number should not change but it did: %d", scan.line);
	test(scan.col == 2, "one char consumed, col should be 2 but is %d", scan.col);
	
	c = scan_until(&scan, NULL, '"');
	test(scan.line == 1, "the line number should not change but it did: %d", scan.line);
	test(scan.col == 14, "we're at the end of the first line, col is supposed to be 14 but is %d", scan.col);
	
	c = scan_while_func(&scan, NULL, isspace);
	test(c == '1', "expected 1 as terminator but got %c", c);
	test(scan.line == 3, "the line number should not change but it did: %d", scan.line);
	test(scan.col == 1, "we're at the start of the third line, col is supposed to be 1 but is %d", scan.col);
	
	scan_close(&scan);
	close(fd);
}

void test_string_scanner(){
	scanner_t scan = scan_open_string("say(hello)");
	slice_t slice;
	
	int c = scan_while_func(&scan, &slice, isalpha);
	test(c == '(', "expected ( as terminator but got %c", c);
	test(slice.length == 3, "expected 3 characters for the first word but got length %d", slice.length);
	test(strcmp(slice.ptr, "say") == 0, "got unexpected first word: \"%s\"", slice.ptr);
	free(slice.ptr);
	
	c = scan_one_of(&scan, '(');
	test(c == '(', "somehow reading the opening braces failed, got %c", c);
	
	c = scan_until(&scan, &slice, ')');
	test(c == ')', "expected ) as terminator but got %c", c);
	test(strcmp(slice.ptr, "hello"), "got unexpected argument word: %s", slice.ptr);
	free(slice.ptr);
	
	c = scan_until(&scan, &slice, EOF);
	test(c == EOF, "expected EOF as terminator but got %d", c);
	test(slice.length == 0, "expected an empty string until EOF but got length %d", slice.length);
	test(strcmp(slice.ptr, "") == 0, "expected only an empty string until EOF but got: \"%s\"", slice.ptr);
	free(slice.ptr);
	
	scan_close(&scan);
}


int main(){
	test_scan_until();
	test_scan_while();
	test_scan_one_of();
	test_scan_until_func();
	test_scan_while_func();
	test_line_and_col_numbers();
	test_string_scanner();
	
	return show_test_report();
}