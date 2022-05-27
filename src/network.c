#include <stdlib.h>
#include <string.h>

#include "network.h"
#include "logging.h"



int extract_status_code(char *plaintext) {
	int status_code;
	char buffer[3];
	memset(&buffer, 0, sizeof(buffer));
	strncpy(buffer, plaintext, 3);
	status_code = (int) strtol(buffer, NULL, 10);
	return status_code;
}

void received_bytes_increase_and_report(const size_t *rxb, size_t *t_rxb, const char *tag, int flag) {
	*t_rxb += *rxb;
	if (flag == 0) {
		log_info("[%s] Received bytes: %zu", tag, *rxb);
		log_info("[%s] Total received bytes: %zu", tag, *t_rxb);
	}
}

void transmitted_bytes_increase_and_report(const size_t *txb, size_t *t_txb, const char *tag, int flag) {
	*t_txb += *txb;
	if (flag == 0) {
		log_info("[%s] Transmitted bytes: %zu", tag, *txb);
		log_info("[%s] Total transmitted bytes: %zu", tag, *t_txb);
	}
}

