#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>

#include "logging.h"


void log_format(const char *level, const char *message, va_list args) {
	char *buffer = malloc(512 * sizeof(char));
	char time_str[256];
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	sprintf(
			time_str,
			"%d-%d-%d %d:%d:%d",
			timeinfo->tm_mday,
			timeinfo->tm_mon + 1,
			timeinfo->tm_year + 1900,
			timeinfo->tm_hour,
			timeinfo->tm_min,
			timeinfo->tm_sec
	);

	sprintf(buffer, "[%s] [%s] %s", time_str, level, message);


	int level_no;
	if (strcmp(level, "DEBUG") == 0) {
		level_no = DEBUG_LEVEL;
	} else if (strcmp(level, "INFO") == 0) {
		level_no = INFO_LEVEL;
	} else if (strcmp(level, "ERROR") == 0) {
		level_no = ERROR_LEVEL;
	} else {
		level_no = -1;
	}

	if (level_no >= LOG_LEVEL) {
		vprintf(buffer, args);
		fprintf(stdout, "\n");
		fflush(stdout);
	}

	free(buffer);
}

void log_debug(const char *message, ...) {
	va_list args;
	va_start(args, message);
	log_format("DEBUG", message, args);
	va_end(args);
}

void log_info(const char *message, ...) {
	va_list args;
	va_start(args, message);
	log_format("INFO", message, args);
	va_end(args);
}

void log_error(const char *message, ...) {
	va_list args;
	va_start(args, message);
	log_format("ERROR", message, args);
	va_end(args);
}

void log_with_errno(char *message, ...) {
	va_list args;
	va_start(args, message);
	log_error("%s - %s",message, strerror(errno));
	va_end(args);
}

void log_usage(const char *message, const char *options) {
	fprintf(stdout, "\n");
	fprintf(stdout, "Usage: \n");
	fprintf(stdout, "%s\n", message);
	fprintf(stdout, "\n");
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "%s\n", options);
	fflush(stdout);
}

