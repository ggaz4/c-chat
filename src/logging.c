#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include "logging.h"


void log_input_error(const char *buffer, int no_conversions) {
	fprintf(stderr, "An error occurred. You entered:\n%s\n", buffer);
	fprintf(stderr, "%d successful conversions", no_conversions);
	exit(EXIT_FAILURE);
}
void log_format(const char *tag, const char *message, va_list args) {

	printf("\n");
}
void log_info(const char *message, ...) {
	va_list args;
	va_start(args, message);
	vprintf(message, args);
	va_end(args);
	fprintf(stdout, "\n");
	fflush(stdout);
}
void log_with_errno(const char *message, ...) {
	va_list args;
	va_start(args, message);
	log_info("[ERROR] %s: %s", message, strerror(errno));
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

