#include <stdio.h>
#include <stdarg.h>
#include <libgen.h>

#include "logger.h"

int log_level;

void log_setup(int level){
	log_level = level;
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
	
	fprintf(stderr, "[%s in %s:%d %s()]: ", label, basename((char*)file), line, func);
	
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
	
	fprintf(stderr, "\n");
}