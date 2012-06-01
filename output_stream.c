#include <stdarg.h>
#include <stdlib.h>

#include "output_stream.h"

output_stream_t os_new(FILE *stdio_stream){
	return (output_stream_t){
		.stream = stdio_stream,
		.buffer_ptr = NULL, .buffer_size = 0, .buffer_filled = 0
	};
}

output_stream_t os_new_capture(size_t capture_buffer_size){
	return (output_stream_t){
		.stream = NULL,
		.buffer_ptr = malloc(capture_buffer_size), .buffer_size = capture_buffer_size, .buffer_filled = 0
	};
}

void os_destroy(output_stream_t *os){
	if (os->buffer_ptr)
		free(os->buffer_ptr);
	os->buffer_size = 0;
	os->buffer_filled = 0;
}

int os_printf(output_stream_t *os, const char *format, ...){
	int result;
	va_list args;
	
	va_start(args, format);
	result = os_vprintf(os, format, args);
	va_end(args);
	
	return result;
}

int os_vprintf(output_stream_t *os, const char *format, va_list args){
	int result;
	
	if (os->stream) {
		result = vfprintf(os->stream, format, args);
	} else {
		size_t free_buffer_space = os->buffer_size - os->buffer_filled;
		result = vsnprintf(os->buffer_ptr + os->buffer_filled, free_buffer_space, format, args);
		if (result > free_buffer_space)
			result = free_buffer_space;
		os->buffer_filled += result;
	}
	
	return result;
}

void os_clear(output_stream_t *os){
	os->buffer_filled = 0;
	if (os->buffer_ptr && os->buffer_size > 0)
		os->buffer_ptr[0] = '\0';
}