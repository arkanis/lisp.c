#include <stdarg.h>
#include <libgen.h>

#include "logger.h"

int log_level = 0;
output_stream_t *log_os = NULL;

void log_setup(int level, output_stream_t *os){
	log_level = level;
	log_os = os;
}

void log_printf(int level, const char *file, int line, const char *func, const char *format, ...){
	if (level < log_level)
		return;
	
	char *label = NULL;
	if (level == LOG_INFO)
		label = "info";
	else if (level == LOG_WARN)
		label = "warn";
	else if (level == LOG_ERROR)
		label = "ERROR";
	
	va_list args;
	va_start(args, format);
	
	if (log_os == NULL) {
		fprintf(stderr, "[%s in %s:%d %s()]: ", label, basename((char*)file), line, func);
		vfprintf(stderr, format, args);
		fprintf(stderr, "\n");
	} else {
		os_printf(log_os, "[%s in %s:%d %s()]: ", label, basename((char*)file), line, func);
		os_vprintf(log_os, format, args);
		os_printf(log_os, "\n");
	}
	
	va_end(args);
}