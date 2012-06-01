#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "scanner.h"


/**
 * Opens a new scanner on the specified file descriptor and allocates some internal resources.
 */
scanner_t scan_open(int fd){
	return (scanner_t){
		.fd = fd, .buffer_ptr = malloc(4096),
		.buffer_size = 4096, .buffer_pos = 0, .buffer_consumed = 0, .buffer_filled = 0,
		.line = 1, .col = 1, .eof = false
	};
}


/**
 * Opens a new scanner with the specified string as buffer content. The scanner will encounter
 * an EOF at the end of the string.
 */
scanner_t scan_open_string(char *code){
	size_t len = strlen(code);
	return (scanner_t){
		.fd = -1, .buffer_ptr = code,
		.buffer_size = len, .buffer_pos = 0, .buffer_consumed = 0, .buffer_filled = len,
		.line = 1, .col = 1, .eof = false
	};
}


/**
 * Closes the scanner and frees all associated resources.
 */
void scan_close(scanner_t *scanner){
	if (scanner->fd != -1)
		free(scanner->buffer_ptr);
	scanner->buffer_ptr = NULL;
	scanner->buffer_size = 0;
	scanner->buffer_pos = 0;
	scanner->buffer_consumed = 0;
	scanner->buffer_filled = 0;
}


/**
 * Fills the buffer with available input from the file descriptor or blocks waiting for it. Returns
 * whatever read() returned and leaves errno unchanged (so you can check yourself). If the
 * scanner has no associated file descriptor 0 is returned (read's code for EOF).
 */
static int read_into_buffer(scanner_t *scanner){
	if (scanner->fd == -1)
		return 0;
	
	// First move all buffer contents to the front of the buffer. We no longer need the consumed part of the buffer.
	if (scanner->buffer_consumed > 0){
		memmove(scanner->buffer_ptr, scanner->buffer_ptr + scanner->buffer_consumed, scanner->buffer_filled - scanner->buffer_consumed);
		scanner->buffer_pos -= scanner->buffer_consumed;
		scanner->buffer_filled -= scanner->buffer_consumed;
		scanner->buffer_consumed = 0;
	}
	
	// Then check if the buffer is still full. If it is expand it.
	if ( scanner->buffer_filled == scanner->buffer_size ){
		scanner->buffer_size += 4096;
		scanner->buffer_ptr = realloc(scanner->buffer_ptr, scanner->buffer_size);
	}
	
	ssize_t bytes_read = read(scanner->fd, scanner->buffer_ptr + scanner->buffer_filled, scanner->buffer_size - scanner->buffer_filled);
	if (bytes_read > 0)
		scanner->buffer_filled += bytes_read;
	
	return bytes_read;
}


/**
 * Returns the next character in the stream without consuming it.
 */
int scan_peek(scanner_t *scanner){
	if (scanner->eof)
		return EOF;
	
	if (scanner->buffer_pos >= scanner->buffer_filled){
		ssize_t bytes_read = read_into_buffer(scanner);
		if (bytes_read == 0){
			scanner->eof = true;
			return EOF;
		}
	}
	
	return scanner->buffer_ptr[scanner->buffer_pos];
}


/**
 * Returns the next character from the scanner buffer or EOF at the end of the file. If we're at the end of the scanner
 * buffer new data is read from the file descriptor.
 * 
 * This function does not mark the character as consumed! You have to increment the buffer_consumed member of the
 * scanner yourself.
 */
static int read_next_char(scanner_t *scanner){
	int c = scan_peek(scanner);
	if (c == EOF)
		return EOF;
	scanner->buffer_pos++;
	
	if (c == '\n') {
		scanner->line++;
		scanner->prev_col = scanner->col;
		scanner->col = 1;
	} else {
		scanner->col++;
	}
	
	return c;
}


/**
 * Moves the buffer position back by 1 and adjusts line and column numbers accordingly.
 */
static void rewind_one_char(scanner_t *scanner){
	if (scanner->buffer_pos < 1)
		return;
	
	if (scanner->buffer_ptr[scanner->buffer_pos - 1] == '\n'){
		scanner->line--;
		scanner->col = scanner->prev_col;
		scanner->prev_col = 1;
	} else {
		scanner->col--;
	}
	
	scanner->buffer_pos--;
}


/**
 * Reads all characters from the input stream until one of the tokens is encountered. The terminating
 * token is consumed by this operation.
 */
int scan_until_with_raw_args(scanner_t *scanner, slice_t* slice, int tokens[]){
	int character;
	while (true) {
		character = read_next_char(scanner);
		/*
		printf("size: %zd, pos: %zd, consumed: %zd, filled: %zd\n",
			scanner->buffer_size, scanner->buffer_pos, scanner->buffer_consumed, scanner->buffer_filled);
		*/
		// Look for each terminator
		for(size_t token_idx = 0; tokens[token_idx] != -2; token_idx++){
			if (character == tokens[token_idx]){
				// We found it, copy the unconsumed buffer into a new memory area and return it.
				// The buffer_pos has already been incremented by read_next_char. Therefore start
				// to copy one char back.
				if (slice != NULL){
					size_t terminator_pos = (character != EOF) ? scanner->buffer_pos - 1 : scanner->buffer_pos;
					size_t content_length = terminator_pos - scanner->buffer_consumed;
					slice->ptr = malloc(content_length + 1);
					memcpy(slice->ptr, scanner->buffer_ptr + scanner->buffer_consumed, content_length);
					slice->ptr[content_length] = '\0';
					slice->length = content_length;
				}
				scanner->buffer_consumed = scanner->buffer_pos;
				return character;
			}
		}
	}
}


/**
 * Same as scan_until_with_raw_args() but uses functions instead of characters for the check. Build in a
 * way that the ctype.h functions like isdigit() can be used.
 */
int scan_until_func_with_raw_args(scanner_t *scanner, slice_t* slice, scanner_check_func_t funcs[]){
	int character;
	while (true) {
		character = read_next_char(scanner);
		// Look for each terminator
		for(size_t func_idx = 0; funcs[func_idx] != NULL; func_idx++){
			if ( funcs[func_idx](character) ){
				// We found it, copy the unconsumed buffer into a new memory area and return it.
				// The buffer_pos has already been incremented by read_next_char. Therefore start
				// to copy one char back.
				if (slice != NULL){
					size_t terminator_pos = (character != EOF) ? scanner->buffer_pos - 1 : scanner->buffer_pos;
					size_t content_length = terminator_pos - scanner->buffer_consumed;
					slice->ptr = malloc(content_length + 1);
					memcpy(slice->ptr, scanner->buffer_ptr + scanner->buffer_consumed, content_length);
					slice->ptr[content_length] = '\0';
					slice->length = content_length;
				}
				scanner->buffer_consumed = scanner->buffer_pos;
				return character;
			}
		}
	}
}


/**
 * Reads all characters from the input stream that are in the token list. The function stops as soon as
 * the first character is encountered that is not part of the input stream. This terminator is not consumed
 * and will be used as input for later scan operations.
 */
int scan_while_with_raw_args(scanner_t *scanner, slice_t* slice, int tokens[]){
	int character;
	bool passed;
	while (true) {
		character = read_next_char(scanner);
		passed = false;
		
		// Look for each terminator
		for(size_t token_idx = 0; tokens[token_idx] != -2; token_idx++){
			if (character == tokens[token_idx]){
				passed = true;
				break;
			}
		}
		
		if (!passed){
			// We found the first character that is not included in the list of tokens. Copy the
			// unconsumed buffer into a new memory area and return it.
			// The buffer_pos has already been incremented by read_next_char. Therefore start
			// to copy one char back. Except we're at an EOF, in this case use all of the buffer
			// since the EOF itself is not in the buffer.
			if (slice != NULL){
				size_t terminator_pos = (character != EOF) ? scanner->buffer_pos - 1 : scanner->buffer_pos;
				size_t content_length = terminator_pos - scanner->buffer_consumed;
				slice->ptr = malloc(content_length + 1);
				memcpy(slice->ptr, scanner->buffer_ptr + scanner->buffer_consumed, content_length);
				slice->ptr[content_length] = '\0';
				slice->length = content_length;
			}
			// Go back one char in the buffer (except we got an EOF). We only want o peek at
			// the terminator not consume it.
			if (character != EOF)
				rewind_one_char(scanner);
			scanner->buffer_consumed = scanner->buffer_pos;
			return character;
		}
	}
}


/**
 * Same as scan_while_with_raw_args() but uses functions instead of characters for the check. Build in a
 * way that the ctype.h functions like isdigit() can be used.
 */
int scan_while_func_with_raw_args(scanner_t *scanner, slice_t* slice, scanner_check_func_t funcs[]){
	int character;
	bool passed;
	while (true) {
		character = read_next_char(scanner);
		passed = false;
		
		// Look for each terminator
		for(size_t func_idx = 0; funcs[func_idx] != NULL; func_idx++){
			if ( funcs[func_idx](character) ){
				passed = true;
				break;
			}
		}
		
		if (!passed){
			// We found the first character that is not included in the list of tokens. Copy the
			// unconsumed buffer into a new memory area and return it.
			// The buffer_pos has already been incremented by read_next_char. Therefore start
			// to copy one char back. Except we're at an EOF, in this case use all of the buffer
			// since the EOF itself is not in the buffer.
			if (slice != NULL){
				size_t terminator_pos = (character != EOF) ? scanner->buffer_pos - 1 : scanner->buffer_pos;
				size_t content_length = terminator_pos - scanner->buffer_consumed;
				slice->ptr = malloc(content_length + 1);
				memcpy(slice->ptr, scanner->buffer_ptr + scanner->buffer_consumed, content_length);
				slice->ptr[content_length] = '\0';
				slice->length = content_length;
			}
			// Go back one char in the buffer (except we got an EOF). We only want o peek at
			// the terminator not consume it.
			if (character != EOF)
				rewind_one_char(scanner);
			scanner->buffer_consumed = scanner->buffer_pos;
			return character;
		}
	}
}


/**
 * Consumes one character and returns it. Only useful right now to consume individual characters.
 * No error handling yet.
 * 
 * The character is only consumed if it matches one of the tokens.
 */
int scan_one_of_with_raw_args(scanner_t *scanner, int tokens[]){
	int c = read_next_char(scanner);
	
	for(size_t token_idx = 0; tokens[token_idx] != -2; token_idx++){
		if (c == tokens[token_idx]){
			scanner->buffer_consumed++;
			return c;
		}
	}
	
	/*
	printf("failed to consume one of the tokens:");
	for(size_t token_idx = 0; tokens[token_idx] != -2; token_idx++)
		printf(" %c", tokens[token_idx]);
	printf("\n");
	*/
	
	return c;
}