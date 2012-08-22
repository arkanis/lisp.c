#ifndef _OUTPUT_STREAM_H
#define _OUTPUT_STREAM_H

/**
 * The purpose of the output stream is to make output testable. An output stream is usually
 * just a wrapper for an stdio stream. But it can also be configured to capture the output
 * into a buffer instead of writing it to an stdio stream. The contents of the buffer can then
 * be compared to check if the output matches the expectations.
 */

#include <stdio.h>

typedef struct {
	FILE *stream;
	char *buffer_ptr;
	size_t buffer_size, buffer_filled;
} output_stream_t;

output_stream_t os_new(FILE *stdio_stream);
output_stream_t os_new_capture(size_t capture_buffer_size);
void os_destroy(output_stream_t *os);
int os_printf(output_stream_t *os, const char *format, ...);
int os_vprintf(output_stream_t *os, const char *format, va_list args);
void os_clear(output_stream_t *os);

#endif