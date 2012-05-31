#ifndef _SCANNER_H
#define _SCANNER_H

typedef struct {
	int fd;
	char *buffer_ptr;
	size_t buffer_size, buffer_pos, buffer_consumed, buffer_filled;
	size_t line, col, prev_col;
} scanner_t;

typedef struct {
	char *ptr;
	size_t length;
} slice_t;

typedef int (*scanner_check_func_t)(int);

scanner_t scan_open(int fd);
scanner_t scan_open_string(char *code);
void scan_close(scanner_t *scanner);

#define scan_until(scanner, slice, ...) scan_until_with_raw_args(scanner, slice, (int[]){ __VA_ARGS__, -2 })
int scan_until_with_raw_args(scanner_t *scanner, slice_t* slice, int tokens[]);
#define scan_while(scanner, slice, ...) scan_while_with_raw_args(scanner, slice, (int[]){ __VA_ARGS__, -2 })
int scan_while_with_raw_args(scanner_t *scanner, slice_t* slice, int tokens[]);

#define scan_until_func(scanner, slice, ...) scan_until_func_with_raw_args(scanner, slice, (scanner_check_func_t[]){ __VA_ARGS__, NULL })
int scan_until_func_with_raw_args(scanner_t *scanner, slice_t* slice, scanner_check_func_t funcs[]);
#define scan_while_func(scanner, slice, ...) scan_while_func_with_raw_args(scanner, slice, (scanner_check_func_t[]){ __VA_ARGS__, NULL })
int scan_while_func_with_raw_args(scanner_t *scanner, slice_t* slice, scanner_check_func_t funcs[]);

#define scan_one_of(scanner, ...) scan_one_of_with_raw_args(scanner, (int[]){ __VA_ARGS__, -2 })
int scan_one_of_with_raw_args(scanner_t *scanner, int tokens[]);

int scan_peek(scanner_t *scanner);

#endif